/*
 * A module for managing plug-in loading. 
*/

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "purple.h"

#include "dynarr.h"
#include "dynlib.h"
#include "dynstr.h"
#include "filelist.h"
#include "hash.h"
#include "list.h"
#include "log.h"
#include "mem.h"
#include "memchunk.h"
#include "strutil.h"

#include "plugins.h"

#include "api-init.h"

/* ----------------------------------------------------------------------------------------- */

/* Libraries are shared objects/DLLs, that hold plug-in code. There can be
 * more plug-ins than there are libraries, since a library can define multiple
 * plug-ins in its init() function.
*/
struct Library
{
	char	name[PATH_MAX];
	DynLib	*lib;
	void	(*init)(void);
	int	initialized;
};

#define	META_CATEGORY_LIMIT	32
#define	META_TEXT_LIMIT		1024

typedef struct
{
	char	category[META_CATEGORY_LIMIT];
	char	*text;
} MetaEntry;

typedef union
{
	boolean	vboolean;
	int32	vint32;
	uint32	vuint32;
	real32	vreal32;
	real32	vreal32_vec2[2];
	real32	vreal32_vec3[3];
	real32	vreal32_vec4[4];
	real32	vreal32_mat16[16];

	real64	vreal64;
	real64	vreal64_vec2[2];
	real64	vreal64_vec3[3];
	real64	vreal64_vec4[4];
	real64	vreal64_mat16[16];

	char	*vstring;
} InputValue;

typedef struct
{
	unsigned int	req : 1;
	unsigned int	def : 1;
	unsigned int	min : 1;
	unsigned int	max : 1;
	InputValue	def_val, min_val, max_val;
} InputSpec;

typedef struct
{
	char		name[16];
	PInputType	type;
	InputSpec	spec;
} Input;

struct Plugin
{
	uint32	id;
	char	name[64];
	Library	*library;

	DynArr	*input;

	Hash	*meta;
	void	(*compute)(PPInput *input, PPOutput output, void *user);
	void	*compute_user;
};

static struct
{
	char		**paths;
	List		*libraries;

	MemChunk	*chunk_meta;

	uint32		next_id;
	Hash		*plugins;		/* All currently loaded, available plugins. */
} plugins_info = { NULL };

/* ----------------------------------------------------------------------------------------- */

static void plugins_id_reset(void)
{
	plugins_info.next_id = 1;
}

static uint32 plugins_id_get(void)
{
	return plugins_info.next_id++;
}

Plugin * plugin_new(const char *name)
{
	Plugin	*p;

	if(name == NULL)
		return NULL;

	if((p = mem_alloc(sizeof *p)) != NULL)
	{
		p->id = plugins_id_get();
		stu_strncpy(p->name, sizeof p->name, name);
		p->library = NULL;
		p->meta = NULL;
		p->compute = NULL;
		p->compute_user = NULL;
	}
	return p;
}

static int set_value(PInputType type, InputValue *value, va_list *taglist)
{
	switch(type)
	{
	case P_INPUT_BOOLEAN:
		return 0;
	case P_INPUT_INT32:
		value->vint32 = (int32) va_arg(*taglist, double);
		return 1;
	case P_INPUT_UINT32:
		value->vuint32 = (uint32) va_arg(*taglist, double);
		return 1;
	case P_INPUT_REAL32:
		value->vreal32 = (real32) va_arg(*taglist, double);
		return 1;
	case P_INPUT_REAL64:
		value->vreal64 = (real64) va_arg(*taglist, double);
		return 1;
	case P_INPUT_STRING:
		if(value->vstring != NULL)
			mem_free(value->vstring);
		value->vstring = stu_strdup((const char *) va_arg(*taglist, const char *));
		return 1;
	}
	LOG_WARN(("Unhandled type %d", type));
	return 0;
}

void plugin_set_input(Plugin *p, int index, PInputType type, const char *name, va_list taglist)
{
	if(p == NULL)
		return;
	if(index < 0)
	{
		LOG_ERR(("Plug-in \"%s\" attempted to set input with negative index--ignored", p->name));
		return;
	}
	if((p->input == NULL && index != 0) || (p->input != NULL && index != dynarr_size(p->input)))
	{
		LOG_ERR(("Plug-in \"%s\" attempted to set input \"%s\" with bad index %d--ignored", p->name, name, index));
		return;
	}
	if(type < P_INPUT_BOOLEAN || type > P_INPUT_NODE)
	{
		LOG_ERR(("Plug-in \"%s\" attempted to set input %d with bad type %d--ignored", p->name, index, type));
		return;
	}
	if(p->input == NULL)
		p->input = dynarr_new(sizeof (Input), 2);
	if(p->input != NULL)
	{
		Input	i;

		stu_strncpy(i.name, sizeof i.name, name);
		i.type = type;
		i.spec.req = i.spec.def = i.spec.min = i.spec.max = 0;
		i.spec.def_val.vstring = NULL;
		for(;;)
		{
			int	tag = va_arg(taglist, int);

			if(tag <= P_INPUT_TAG_DONE || tag > P_INPUT_TAG_DEFAULT)	/* Generous. */
				break;
			else if(tag == P_INPUT_TAG_REQUIRED)
				i.spec.req = 1;
			else if(tag == P_INPUT_TAG_MIN)
				i.spec.min = set_value(i.type, &i.spec.min_val, &taglist);
			else if(tag == P_INPUT_TAG_MAX)
				i.spec.max = set_value(i.type, &i.spec.max_val, &taglist);
			else if(tag == P_INPUT_TAG_DEFAULT)
				i.spec.def = set_value(i.type, &i.spec.def_val, &taglist);
		}
		dynarr_append(p->input, &i);
	}
}

void plugin_set_meta(Plugin *p, const char *category, const char *text)
{
	char		cat[META_CATEGORY_LIMIT];
	MetaEntry	*m;

	if(category == NULL || *category == '\0')
	{
		LOG_WARN(("Plug-in \"%s\" attempted to set meta with empty category--ignored", p->name));
		return;
	}
	if(text == NULL || *text == '\0')
	{
		LOG_WARN(("Plug-in \"%s\" attempted to set meta with empty text--ignored", p->name));
		return;
	}

	stu_strncpy(cat, sizeof cat, category);
	if(strcmp(cat, category) != 0)
		LOG_WARN(("Plug-in \"%s\" attempted to set meta with too long category--truncated", p->name));
	if((p->meta != NULL) && ((m = hash_lookup(p->meta, cat)) != NULL))
	{
		LOG_MSG(("Replacing meta text for category '%s' in \"%s\"", cat, p->name));
		mem_free(m->text);
		m->text = stu_strdup_maxlen(text, META_TEXT_LIMIT);
		return;
	}
	if(p->meta == NULL)
		p->meta = hash_new_string();
	if(p->meta != NULL)
	{
		m = memchunk_alloc(plugins_info.chunk_meta);
		if(m != NULL)
		{
			strcpy(m->category, cat);
			m->text = stu_strdup_maxlen(text, META_TEXT_LIMIT);
			printf("meta string: '%s'\n", m->text);
			hash_insert(p->meta, cat, m);
		}
	}
}

void plugin_set_compute(Plugin *p, void (*compute)(PPInput *input, PPOutput output, void *user), void *user)
{
	if(p != NULL && compute != NULL)
	{
		p->compute = compute;
		p->compute_user = user;
	}
}

static void append_value(DynStr *d, PInputType type, const InputValue *v)
{
	switch(type)
	{
	case P_INPUT_BOOLEAN:
		dynstr_append(d, v->vboolean ? "true" : "false");
		break;
	case P_INPUT_INT32:
		dynstr_append_printf(d, "%d", v->vint32);
		break;
	case P_INPUT_UINT32:
		dynstr_append_printf(d, "%u", v->vuint32);
		break;
	case P_INPUT_REAL32:
		dynstr_append_printf(d, "%f", v->vreal32);
		break;
	case P_INPUT_REAL64:
		dynstr_append_printf(d, "%g", v->vreal64);
		break;
	case P_INPUT_STRING:
		xml_dynstr_append(d, v->vstring);
		break;
	}
}

static int cb_describe_meta(const void *data, void *user)
{
	const MetaEntry	*m = data;

	dynstr_append_printf(user,
		     "  <entry>\n"
		     "   <category>%s</category>\n"
		     "   <value>", m->category);
	xml_dynstr_append(user, m->text);
	dynstr_append(user,
		     "</value>\n"
		     "  </entry>\n");
	return 1;	           
}

char * plugin_describe(const Plugin *p)
{
	DynStr	*d;

	d = dynstr_new("<plug-in");

	plugin_describe_append(p, d);

	return dynstr_destroy(d, 0);	/* Destroys dynstr, but keeps and returns buffer. */
}

void plugin_describe_append(const Plugin *p, DynStr *d)
{
	size_t	num;

	if(p == NULL || d == NULL)
		return;

	/* Head. */
	dynstr_append_printf(d, "<plug-in id=\"%u\" name=\"%s\"", p->id, p->name);
	dynstr_append_printf(d, ">\n");

	/* Any inputs? */
	if((num = dynarr_size(p->input)) > 0)
	{
		const Input	*in;
		size_t		i;

		dynstr_append(d, " <inputs>\n");
		for(i = 0; i < num; i++)
		{
			dynstr_append_printf(d, "  <input type=\"real32\">\n");
			in = dynarr_index(p->input, i);
			if(in->name[0] != '\0')
				dynstr_append_printf(d, "   <name>%s</name>\n", in->name);
			if(in->spec.req)
				dynstr_append(d, "   <flag name=\"required\" value=\"true\"/>\n");
			if(in->spec.def || in->spec.min || in->spec.max)
			{
				dynstr_append(d, "   <range>\n");
				if(in->spec.def)
				{
					dynstr_append(d, "    <def>");
					append_value(d, in->type, &in->spec.def_val);
					dynstr_append(d, "</def>\n");
				}
				if(in->spec.min)
				{
					dynstr_append(d, "    <min>");
					append_value(d, in->type, &in->spec.min_val);
					dynstr_append(d, "</min>\n");
				}
				if(in->spec.max)
				{
					dynstr_append(d, "    <max>");
					append_value(d, in->type, &in->spec.max_val);
					dynstr_append(d, "</max>\n");
				}
				dynstr_append(d, "   </range>\n");
			}
			dynstr_append_printf(d, "  </input>\n");
		}
		dynstr_append(d, " </inputs>\n");
	}

	/* Meta info present? */
	if(hash_size(p->meta) > 0)
	{
		dynstr_append(d, " <meta>\n");
		hash_foreach(p->meta, cb_describe_meta, d);
		dynstr_append(d, " </meta>\n");
	}
	
	/* Done. */
	dynstr_append(d, "</plug-in>\n");
}

void plugin_destroy(Plugin *p)
{
	if(p != NULL)
	{
		hash_remove(plugins_info.plugins, p);
		dynarr_destroy(p->input);
		hash_destroy(p->meta);
		mem_free(p);
	}
}

/* ----------------------------------------------------------------------------------------- */

static void paths_set(const char *paths)
{
	if(paths == NULL)
		return;
	if(plugins_info.paths != NULL)
		mem_free(plugins_info.paths);
	plugins_info.paths = stu_split(paths, '|');
}

void plugins_init(const char *paths)
{
	paths_set(paths);
	plugins_info.libraries = NULL;
	plugins_info.plugins = hash_new_string();
	plugins_info.chunk_meta  = memchunk_new("Meta", sizeof (MetaEntry), 4);
}

static int library_known(const char *name)
{
	const List	*iter;

	for(iter = plugins_info.libraries; iter != NULL; iter = list_next(iter))
	{
		if(strcmp(((Library *) list_data(iter))->name, name) == 0)
			return 1;
	}
	return 0;
}

static void library_new(const char *name)
{
	DynLib	dl;

	if(library_known(name))
	{
		LOG_WARN(("Not loading \"%s\", library already registered", name));
		return;
	}

	if(DYNLIB_VALID(dl = dynlib_load(name)))
	{
		void	(*init)(void);
		Library	*lib;

		if((init = dynlib_resolve(dl, "init")) == NULL)
		{
			LOG_WARN(("No init() entrypoint found in \"%s\"", name));
			dynlib_unload(dl);
			return;
		}

		lib = mem_alloc(sizeof *lib);
		stu_strncpy(lib->name, sizeof lib->name, name);
		lib->lib = dl;
		lib->init = init;
		lib->initialized = 0;

		plugins_info.libraries = list_append(plugins_info.libraries, lib);
		LOG_MSG(("Loaded and stored library from \"%s\"", name));
		printf("there are now %u libs loaded\n", list_length(plugins_info.libraries));
	}
}

void plugins_libraries_load(void)
{
	int	i;

	if(plugins_info.paths == NULL)
	{
		LOG_WARN(("Can't load plug-ins, no paths set"));
		return;
	}
	for(i = 0; plugins_info.paths[i] != NULL; i++)
	{
		FileList	*fl;
#if defined __win32
		const char	*suffix = ".dll";
#else
		const char	*suffix = ".so";
#endif
		plugins_id_reset();
		if((fl = filelist_new(plugins_info.paths[i], suffix)) != NULL)
		{
			int	j;

			for(j = 0; j < filelist_size(fl); j++)
				library_new(filelist_filename_full(fl, j));
		}
		else
			LOG_MSG(("Couldn't load file list from \"%s\"", plugins_info.paths[i]));
	}
}

void plugins_libraries_init(void)
{
	const List	*iter;

	for(iter = plugins_info.libraries; iter != NULL; iter = list_next(iter))
	{
		Library	*lib = list_data(iter);
		size_t	num = hash_size(plugins_info.plugins);
		LOG_MSG(("Initializing library \"%s\"", lib->name));
		api_init_begin(lib);
		lib->init();
		api_init_end();
		LOG_MSG(("Done, that added %u plug-ins", hash_size(plugins_info.plugins) - num));
	}
}

/* Register a new plugin. This is rather picky. */
void plugins_register(Library *owner, Plugin *p)
{
	if(p == NULL)
		return;
	if(p->name[0] == '\0')
	{
		LOG_WARN(("Library \"%s\" attempted to register non-named plugin--ignored", owner->name));
		plugin_destroy(p);
		return;
	}
	if(p->compute == NULL)
	{
		LOG_WARN(("Library \"%s\" attempted to register plugin \"%s\" with no compute()--ignored",
			  owner->name, p->name));
		plugin_destroy(p);
		return;
	}
	if(hash_lookup(plugins_info.plugins, p->name) != NULL)
	{
		LOG_WARN(("Library \"%s\" attempted to register plugin \"%s\", which is already registered--ignored",
			  owner->name, p->name));
		plugin_destroy(p);
		return;
	}
	p->library = owner;
	hash_insert(plugins_info.plugins, p->name, p);
	LOG_MSG(("Registered plug-in \"%s\" with %u inputs", p->name, dynarr_size(p->input)));
}

size_t plugins_amount(void)
{
	return hash_size(plugins_info.plugins);
}

static int cb_build_xml(const void *data, void *user)
{
	dynstr_append_c(user, '\n');
	plugin_describe_append(data, user);

	return 1;
}

char * plugins_build_xml(void)
{
	DynStr	*d;

	d = dynstr_new("<?xml version=\"1.0\" standalone=\"yes\"?>\n\n"
			  "<purple-plugins>");
	hash_foreach(plugins_info.plugins, cb_build_xml, d);
	dynstr_append(d, "</purple-plugins>\n");

	return dynstr_destroy(d, 0);
}
