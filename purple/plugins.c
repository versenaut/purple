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
#include "idset.h"
#include "list.h"
#include "log.h"
#include "mem.h"
#include "memchunk.h"
#include "strutil.h"
#include "xmlutil.h"

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

typedef struct
{
	unsigned int	req : 1;
	unsigned int	def : 1;
	unsigned int	min : 1;
	unsigned int	max : 1;
	PInputValue	def_val, min_val, max_val;
} InputSpec;

typedef struct
{
	char		name[16];
	PInputType	type;
	InputSpec	spec;
} Input;

struct Plugin
{
	unsigned int	id;
	char		name[64];
	Library		*library;

	DynArr		*input;

	Hash		*meta;
	void		(*compute)(PPInput *input, PPOutput output, void *user);
	void		*compute_user;
};

struct PInputSet
{
	size_t		size;
	uint32		*use;
	PInputValue	*value;
};

static struct
{
	char		**paths;
	List		*libraries;

	MemChunk	*chunk_meta;

	IdSet		*plugins;
	Hash		*plugins_name;		/* All currently loaded, available plugins. */
} plugins_info = { NULL };

static struct TypeName
{
	PInputType	type;
	const char	*name;
} type_map_by_value[] = {
	{ P_INPUT_BOOLEAN,	"boolean" },
	{ P_INPUT_INT32,	"int32" },
	{ P_INPUT_UINT32,	"uint32" },
	{ P_INPUT_REAL32,	"real32" },
	{ P_INPUT_REAL32_VEC2,	"real32_vec2" },
	{ P_INPUT_REAL32_VEC3,	"real32_vec3" },
	{ P_INPUT_REAL32_VEC4,	"real32_vec4" },
	{ P_INPUT_REAL32_MAT16,	"real32_mat16" },
	{ P_INPUT_REAL64,	"real64" },
	{ P_INPUT_REAL64_VEC2,	"real64_vec2" },
	{ P_INPUT_REAL64_VEC3,	"real64_vec3" },
	{ P_INPUT_REAL64_VEC4,	"real64_vec4" },
	{ P_INPUT_REAL64_MAT16,	"real64_mat16" },
	{ P_INPUT_MODULE,	"module" },
	{ P_INPUT_STRING,	"string" },
	}, type_map_by_name[sizeof type_map_by_value / sizeof *type_map_by_value];

/* ----------------------------------------------------------------------------------------- */

const char * plugin_input_type_to_name(PInputType t)
{
	return type_map_by_value[t].name;
}

/* bsearch() callback for comparing a string, <key>, with a struct TypeName. */
static int srch_type_name(const void *key, const void *t)
{
	return strcmp(key, ((const struct TypeName *) t)->name);
}

PInputType plugin_input_type_from_name(const char *name)
{
	struct TypeName	*m = bsearch(name, type_map_by_name,
			     sizeof type_map_by_name / sizeof *type_map_by_name,
			     sizeof type_map_by_name, srch_type_name);
	if(m != NULL)
		return m->type;
	return -1;
}

/* qsort() callback for sorting the by-name mapping. */
static int cmp_type_name(const void *a, const void *b)
{
	const struct TypeName	*na = a, *nb = b;

	return strcmp(na->name, nb->name);
}

/* ----------------------------------------------------------------------------------------- */

Plugin * plugin_new(const char *name)
{
	Plugin	*p;

	if(name == NULL)
		return NULL;

	if((p = mem_alloc(sizeof *p)) != NULL)
	{
		p->id = 0;
		stu_strncpy(p->name, sizeof p->name, name);
		p->library = NULL;
		p->meta = NULL;
		p->compute = NULL;
		p->compute_user = NULL;
	}
	return p;
}

static int set_value(PInputValue *value, PInputType new_type, va_list *taglist)
{
	if(value->type == P_INPUT_STRING)
	{
		mem_free(value->v.vstring);
		value->v.vstring = NULL;
	}
	value->type = new_type;
	switch(value->type)
	{
	case P_INPUT_BOOLEAN:
		value->v.vboolean = (boolean) va_arg(*taglist, int);
		return 0;
	case P_INPUT_INT32:
		value->v.vint32 = (int32) va_arg(*taglist, int32);
		return 1;
	case P_INPUT_UINT32:
		value->v.vuint32 = (uint32) va_arg(*taglist, uint32);
		return 1;
	case P_INPUT_REAL32:
		value->v.vreal32 = (real32) va_arg(*taglist, double);
		return 1;
	case P_INPUT_REAL32_VEC2:
		{
			const real32 *data = (const real32 *) va_arg(*taglist, const real32 *);
			value->v.vreal32_vec2[0] = data[0];
			value->v.vreal32_vec2[1] = data[1];
		}
		return 1;
	case P_INPUT_REAL32_VEC3:
		{
			const real32 *data = (const real32 *) va_arg(*taglist, const real32 *);
			value->v.vreal32_vec3[0] = data[0];
			value->v.vreal32_vec3[1] = data[1];
			value->v.vreal32_vec3[2] = data[2];
		}
		return 1;
	case P_INPUT_REAL32_VEC4:
		{
			const real32 *data = (const real32 *) va_arg(*taglist, const real32 *);
			value->v.vreal32_vec4[0] = data[0];
			value->v.vreal32_vec4[1] = data[1];
			value->v.vreal32_vec4[2] = data[2];
			value->v.vreal32_vec4[3] = data[3];
		}
		return 1;
	case P_INPUT_REAL32_MAT16:
		{
			const real32	*data = (const real32 *) va_arg(*taglist, const real32 *);
			int		i;

			for(i = 0; i < 16; i++)
				value->v.vreal32_mat16[i] = data[i];
		}
		return 1;
	case P_INPUT_REAL64:
		value->v.vreal64 = (real64) va_arg(*taglist, double);
		return 1;
	case P_INPUT_REAL64_VEC2:
		{
			const real64 *data = (const real64 *) va_arg(*taglist, const real64 *);
			value->v.vreal64_vec2[0] = data[0];
			value->v.vreal64_vec2[1] = data[1];
		}
		return 1;
	case P_INPUT_REAL64_VEC3:
		{
			const real64 *data = (const real64 *) va_arg(*taglist, const real64 *);
			value->v.vreal64_vec3[0] = data[0];
			value->v.vreal64_vec3[1] = data[1];
			value->v.vreal64_vec3[2] = data[2];
		}
		return 1;
	case P_INPUT_REAL64_VEC4:
		{
			const real64 *data = (const real64 *) va_arg(*taglist, const real64 *);
			value->v.vreal64_vec4[0] = data[0];
			value->v.vreal64_vec4[1] = data[1];
			value->v.vreal64_vec4[2] = data[2];
			value->v.vreal64_vec4[3] = data[3];
		}
		return 1;
	case P_INPUT_REAL64_MAT16:
		{
			const real64	*data = (const real64 *) va_arg(*taglist, const real64 *);
			int		i;

			for(i = 0; i < 16; i++)
				value->v.vreal64_mat16[i] = data[i];
		}
		return 1;
	case P_INPUT_MODULE:
		value->v.vmodule = (uint32) va_arg(*taglist, uint32);
		return 1;
	case P_INPUT_STRING:
		value->v.vstring = stu_strdup((const char *) va_arg(*taglist, const char *));
		return 1;
	default:
		LOG_WARN(("Unhandled type %d", new_type));
	}
	value->type = P_INPUT_BOOLEAN;
	value->v.vboolean = 0;
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
	if(type < P_INPUT_BOOLEAN || type > P_INPUT_STRING)
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
		i.spec.def_val.v.vstring = NULL;
		for(;;)
		{
			int	tag = va_arg(taglist, int);

			if(tag <= P_INPUT_TAG_DONE || tag > P_INPUT_TAG_DEFAULT)	/* Generous. */
				break;
			else if(tag == P_INPUT_TAG_REQUIRED)
				i.spec.req = 1;
			else if(tag == P_INPUT_TAG_MIN)
				i.spec.min = set_value(&i.spec.min_val, i.type, &taglist);
			else if(tag == P_INPUT_TAG_MAX)
				i.spec.max = set_value(&i.spec.max_val, i.type, &taglist);
			else if(tag == P_INPUT_TAG_DEFAULT)
				i.spec.def = set_value(&i.spec.def_val, i.type, &taglist);
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

static void append_value(DynStr *d, const PInputValue *v)
{
	switch(v->type)
	{
	case P_INPUT_BOOLEAN:
		dynstr_append(d, v->v.vboolean ? "true" : "false");
		break;
	case P_INPUT_INT32:
		dynstr_append_printf(d, "%d", v->v.vint32);
		break;
	case P_INPUT_UINT32:
		dynstr_append_printf(d, "%u", v->v.vuint32);
		break;
	case P_INPUT_REAL32:
		dynstr_append_printf(d, "%g", v->v.vreal32);
		break;
	case P_INPUT_REAL32_VEC2:
		dynstr_append_printf(d, "[%g %g]", v->v.vreal32_vec2[0], v->v.vreal32_vec2[1]);
		break;
	case P_INPUT_REAL32_VEC3:
		dynstr_append_printf(d, "[%g %g %g]", v->v.vreal32_vec3[0], v->v.vreal32_vec3[1], v->v.vreal32_vec3[2]);
		break;
	case P_INPUT_REAL32_VEC4:
		dynstr_append_printf(d, "[%g %g %g %g]", v->v.vreal32_vec4[0], v->v.vreal32_vec4[1], v->v.vreal32_vec4[2], v->v.vreal32_vec4[3]);
		break;
	case P_INPUT_REAL32_MAT16:
		{
			int	i, j;

			dynstr_append_printf(d, "[");
			for(i = 0; i < 4; i++)
			{
				dynstr_append_printf(d, "[");
				for(j = 0; j < 4; j++)
					dynstr_append_printf(d, "%s%g", j > 0 ? " " : "", v->v.vreal32_mat16[4 * i + j]);
				dynstr_append_printf(d, "]");
			}
			dynstr_append_printf(d, "]");
		}
		break;
	case P_INPUT_REAL64:
		dynstr_append_printf(d, "%.10g", v->v.vreal64);
		break;
	case P_INPUT_REAL64_VEC2:
		dynstr_append_printf(d, "[%.10g %.10g]", v->v.vreal64_vec2[0], v->v.vreal64_vec2[1]);
		break;
	case P_INPUT_REAL64_VEC3:
		dynstr_append_printf(d, "[%.10g %.10g %.10g]", v->v.vreal64_vec3[0], v->v.vreal64_vec3[1], v->v.vreal64_vec3[2]);
		break;
	case P_INPUT_REAL64_VEC4:
		dynstr_append_printf(d, "[%.10g %.10g %.10g %.10g]", v->v.vreal64_vec4[0], v->v.vreal64_vec4[1], v->v.vreal64_vec4[2], v->v.vreal64_vec4[3]);
		break;
	case P_INPUT_REAL64_MAT16:
		{
			int	i, j;

			dynstr_append_printf(d, "[");
			for(i = 0; i < 4; i++)
			{
				dynstr_append_printf(d, "[");
				for(j = 0; j < 4; j++)
					dynstr_append_printf(d, "%s%.10g", j > 0 ? " " : "", v->v.vreal64_mat16[4 * i + j]);
				dynstr_append_printf(d, "]");
			}
			dynstr_append_printf(d, "]");
		}
		break;
	case P_INPUT_MODULE:
		dynstr_append_printf(d, "%u", v->v.vmodule);
		break;
	case P_INPUT_STRING:
		xml_dynstr_append(d, v->v.vstring);
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
	dynstr_append_printf(d, "<plug-in id=\"%u\" name=\"%s\">\n", p->id, p->name);

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
					append_value(d, &in->spec.def_val);
					dynstr_append(d, "</def>\n");
				}
				if(in->spec.min)
				{
					dynstr_append(d, "    <min>");
					append_value(d, &in->spec.min_val);
					dynstr_append(d, "</min>\n");
				}
				if(in->spec.max)
				{
					dynstr_append(d, "    <max>");
					append_value(d, &in->spec.max_val);
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

const Plugin * plugin_lookup(unsigned int id)
{
	return idset_lookup(plugins_info.plugins, id);
}

unsigned int plugin_id(const Plugin *p)
{
	return p != NULL ? p->id : 0;
}

const char * plugin_name(const Plugin *p)
{
	return p != NULL ? p->name : NULL;
}

void plugin_destroy(Plugin *p)
{
	if(p != NULL)
	{
		idset_remove(plugins_info.plugins, p->id);
		hash_remove(plugins_info.plugins_name, p);
		dynarr_destroy(p->input);
		hash_destroy(p->meta);
		mem_free(p);
	}
}

/* ----------------------------------------------------------------------------------------- */

PInputSet * plugin_inputset_new(const Plugin *p)
{
	PInputSet	*is;
	size_t		size, num;

	if(p == NULL)
		return NULL;
	size = dynarr_size(p->input);
	if(size == 0)
		return NULL;
	num = (size + 31) / 32;
	is = mem_alloc(sizeof *is + num * sizeof *is->use + size * sizeof *is->value);
	is->size  = size;
	is->use   = (uint32 *) (is + 1);
	is->value = (PInputValue *) (is->use + num);
	memset(is->use, 0, num * sizeof *is->use);
	return is;
}

void plugin_inputset_set_va(PInputSet *is, unsigned int index, PInputType type, va_list arg)
{
	if(is == NULL || index >= is->size)
		return;
	is->use[index / 32] |= 1 << (index % 32);
	if(set_value(is->value + index, type, &arg) == 0)
		LOG_WARN(("Input setting failed"));
}

void plugin_inputset_clear(PInputSet *is, unsigned int index)
{
	if(is == NULL || index >= is->size)
		return;
	is->use[index / 32] &= ~(1 << (index % 32));
}

boolean plugin_inputset_is_set(const PInputSet *is, unsigned int index)
{
	if(is == NULL || index >= is->size)
		return 0;
	return (is->use[index / 32] & (1 << (index % 32))) != 0;
}

void plugin_inputset_describe(const PInputSet *is, DynStr *d)
{
	int	i;

	if(is == NULL || d == NULL)
		return;

	for(i = 0; i < is->size; i++)
	{
		if(is->use[i / 32] & (1 << (i % 32)))
		{
			dynstr_append_printf(d, "  <set input=\"%u\" type=\"", i);
			dynstr_append(d, plugin_input_type_to_name(is->value[i].type));
			dynstr_append_printf(d, "\">");
			append_value(d, &is->value[i]);
			dynstr_append(d, "</set>\n");
		}
	}
}

void plugin_inputset_destroy(PInputSet *is)
{
	if(is != NULL)
		mem_free(is);
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
	plugins_info.plugins = idset_new(1);
	plugins_info.plugins_name = hash_new_string();
	plugins_info.chunk_meta  = memchunk_new("Meta", sizeof (MetaEntry), 4);

	memcpy(type_map_by_name, type_map_by_value, sizeof type_map_by_name);
	qsort(type_map_by_name, sizeof type_map_by_name / sizeof *type_map_by_name, sizeof *type_map_by_name, cmp_type_name);
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
		size_t	num = plugins_amount();
		LOG_MSG(("Initializing library \"%s\"", lib->name));
		api_init_begin(lib);
		lib->init();
		api_init_end();
		LOG_MSG(("Done, that added %u plug-ins", plugins_amount() - num));
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
	if(hash_lookup(plugins_info.plugins_name, p->name) != NULL)
	{
		LOG_WARN(("Library \"%s\" attempted to register plugin \"%s\", which is already registered--ignored",
			  owner->name, p->name));
		plugin_destroy(p);
		return;
	}
	p->library = owner;
	p->id = idset_insert(plugins_info.plugins, p);
	hash_insert(plugins_info.plugins_name, p->name, p);
	LOG_MSG(("Registered plug-in \"%s\" with %u inputs", p->name, dynarr_size(p->input)));
}

size_t plugins_amount(void)
{
	return idset_size(plugins_info.plugins);
}

char * plugins_build_xml(void)
{
	unsigned int	id;
	Plugin		*p;
	DynStr		*d;

	d = dynstr_new("<?xml version=\"1.0\" standalone=\"yes\"?>\n\n"
			  "<purple-plugins>");
	
	for(id = idset_foreach_first(plugins_info.plugins); (p = idset_lookup(plugins_info.plugins, id)) != NULL;
	    id = idset_foreach_next(plugins_info.plugins, id))
	{
		dynstr_append_c(d, '\n');
		plugin_describe_append(p, d);
	}
	dynstr_append(d, "</purple-plugins>\n");

	return dynstr_destroy(d, 0);
}
