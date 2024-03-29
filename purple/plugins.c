/*
 * plugins.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * A module for managing plug-in loading. 
*/

#include <limits.h>
#include <stdarg.h>
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
#include "value.h"
#include "nodeset.h"
#include "strutil.h"
#include "plugin-clock.h"
#include "plugin-input.h"
#include "plugin-output.h"
#include "port.h"
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
	char	name[128];
	DynLib	*lib;
	void	(*init)(void);
	int	initialized;
};

#define	META_CATEGORY_LIMIT	64
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
	PValue		def_val, min_val, max_val;
	DynStr		*enums;
} InputSpec;

typedef struct
{
	char		name[16];
	PValueType	type;
	InputSpec	spec;
	const char	*desc;
} Input;

struct Plugin
{
	unsigned int	id;
	char		name[64];
	Library		*library;

	DynArr		*input;

	Hash		*meta;
	void		(*ctor)(void *state);
	void		(*dtor)(void *state);
	PComputeStatus	(*compute)(PPInput *input, PPOutput output, void *state);
	MemChunk	*state;		/* Instance state blocks allocated from here. */
};

struct PPortSet
{
	size_t		size;		/* Number of inputs in set. */
	uint32		*use;		/* Bitmask telling if a value is present. */
	PPort		*input;		/* Array of storage for inputs. */
	PPInput		*port;		/* Array of pointers to storage, link-resolved, presented to plug-ins. */
	const Plugin	*plugin;	/* Plug-in for which this portset was created. */
};

static struct
{
	char		**paths;
	List		*libraries;

	MemChunk	*chunk_meta;

	IdSet		*plugins;
	Hash		*plugins_name;		/* All currently loaded, available plugins. */

	uint32		sequence_next;		/* Index of next instance. Helps UI-clients de-alias across create-delete-create. */
} plugins_info;

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
		p->input = NULL;
		p->meta = NULL;
		p->compute = NULL;
		p->state = NULL;
	}
	return p;
}

void plugin_set_input(Plugin *p, int index, PValueType type, const char *name, va_list taglist)
{
	if(p == NULL)
		return;
	if(index < 0)
	{
		LOG_ERR(("Plug-in \"%s\" attempted to set input with negative index--ignored", p->name));
		return;
	}
	if((p->input == NULL && index != 0) || (p->input != NULL && (size_t) index != dynarr_size(p->input)))
	{
		LOG_ERR(("Plug-in \"%s\" attempted to set input \"%s\" with bad index %d--ignored", p->name, name, index));
		return;
	}
	if(type < P_VALUE_BOOLEAN || type > P_VALUE_MODULE)
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
		i.spec.enums = NULL;
		i.desc = NULL;
		value_init(&i.spec.min_val);
		value_init(&i.spec.max_val);
		value_init(&i.spec.def_val);
		
		for(;;)
		{
			int	tag = va_arg(taglist, int);
	
			if(tag == P_INPUT_TAG_DONE)
				break;
			else if(tag < P_INPUT_TAG_DONE || tag > P_INPUT_TAG_DESC)	/* Generous. */
			{
				LOG_WARN(("Aborting on bad tag value for %s.%s: %d", p->name, i.name, tag));
				break;
			}
			else if(tag == P_INPUT_TAG_REQUIRED)
				i.spec.req = 1;
			else if(tag == P_INPUT_TAG_MIN)
				i.spec.min = value_set_defminmax_va(&i.spec.min_val, i.type, &taglist);
			else if(tag == P_INPUT_TAG_MAX)
				i.spec.max = value_set_defminmax_va(&i.spec.max_val, i.type, &taglist);
			else if(tag == P_INPUT_TAG_DEFAULT)
				i.spec.def = value_set_defminmax_va(&i.spec.def_val, i.type, &taglist);
			else if(tag == P_INPUT_TAG_ENUM)
			{
				const char	*st;

				if(i.spec.enums == NULL)
					i.spec.enums = dynstr_new_sized(64);
				st = dynstr_string(i.spec.enums);
				if(st != NULL && *st != '\0' && st[strlen(st) - 1] != '|')	/* Make sure there are separators. */
					dynstr_append_c(i.spec.enums, '|');
				dynstr_append(i.spec.enums, va_arg(taglist, char *));
			}
			else if(tag == P_INPUT_TAG_DESC)
				i.desc = va_arg(taglist, char *);
		}
		dynarr_append(p->input, &i, NULL);
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
			hash_insert(p->meta, cat, m);
		}
	}
}

void plugin_set_state(Plugin *p, size_t size, void (*ctor)(void *state), void (*dtor)(void *state))
{
	if(p == NULL)
		return;
	if(p->state != NULL)
	{
		memchunk_destroy(p->state);
		p->state = NULL;
	}
	if(size > 0)
		p->state = memchunk_new(p->name, size, 4);
	p->ctor = ctor;
	p->dtor = dtor;
}

void plugin_set_compute(Plugin *p, PComputeStatus (*compute)(PPInput *input, PPOutput output, void *state))
{
	if(p != NULL && compute != NULL)
	{
		p->compute = compute;
	}
}

/* Check if a plug-in has at least on default value on one of its inputs.
 * FIXME: This could easily be buffered from creation-time.
*/
int plugin_has_default_inputs(const Plugin *p)
{
	size_t	num, i;

	if(p == NULL)
		return 0;
	if((num = dynarr_size(p->input)) > 0)
	{
		const Input	*in;

		for(i = 0; i < num; i++)
		{
			in = dynarr_index(p->input, i);
			if(in->spec.def)
				return 1;	/* As soon as we know the answer, deliver it. */
		}
	}
	return 0;
}

static int cb_describe_meta(void *data, void *user)
{
	const MetaEntry	*m = data;

	dynstr_append(user,"  <entry category=\"");
	xml_dynstr_append(user, m->category);
	dynstr_append(user, "\">");
	xml_dynstr_append(user, m->text);
	dynstr_append(user, "</entry>\n");

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
			in = dynarr_index(p->input, i);
			dynstr_append_printf(d, "  <input type=\"%s\">\n", value_type_to_name(in->type));
			if(in->name[0] != '\0')
				dynstr_append_printf(d, "   <name>%s</name>\n", in->name);
			if(in->spec.req)
				dynstr_append(d, "   <flag name=\"required\" value=\"true\"/>\n");
			if(in->spec.def || in->spec.min || in->spec.max || in->spec.enums != NULL)
			{
				char		buf[1024];
				const char	*ptr;

				dynstr_append(d, "   <range>\n");
				if(in->spec.def)
				{
					dynstr_append(d, "    <def>");
					if((ptr = value_as_string(&in->spec.def_val, buf, sizeof buf, NULL)) != NULL)
						dynstr_append(d, ptr);
					dynstr_append(d, "</def>\n");
				}
				if(in->spec.min)
				{
					dynstr_append(d, "    <min>");
					if((ptr = value_as_string(&in->spec.min_val, buf, sizeof buf, NULL)) != NULL)
						dynstr_append(d, ptr);
					dynstr_append(d, "</min>\n");
				}
				if(in->spec.max)
				{
					dynstr_append(d, "    <max>");
					if((ptr = value_as_string(&in->spec.max_val, buf, sizeof buf, NULL)) != NULL)
						dynstr_append(d, ptr);
					dynstr_append(d, "</max>\n");
				}
				dynstr_append(d, "   </range>\n");
				if(in->spec.enums != NULL)
				{
					const char	*def;

					dynstr_append(d, "   <enums>\n");
					/* Split single string into multiple nice XML value/name pairs. */
					for(def = dynstr_string(in->spec.enums); *def != '\0';)
					{
						char	temp[64], *put, *name;

						for(put = temp; *def != '\0' && *def != '|' && put < (temp + sizeof temp - 1);)
						{
							if(*def == ':')
							{
								def++;
								*put++ = '\0';		/* Terminate value part. */
								name = put;
							}
							else
								*put++ = *def++;
						}
						*put = '\0';
						dynstr_append_printf(d, "    <enum value=\"%s\">%s</enum>\n", temp, name);
						if(*def == '|')
							def++;
					}
					dynstr_append(d, "   </enums>\n");
				}
			}
			if(in->desc != NULL)
			{
				dynstr_append(d, "   <desc>");
				xml_dynstr_append(d, in->desc);
				dynstr_append(d, "</desc>\n");
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

Plugin * plugin_lookup(unsigned int id)
{
	return idset_lookup(plugins_info.plugins, id);
}

Plugin * plugin_lookup_by_name(const char *name)
{
	unsigned int	id;
	Plugin		*p;

	for(id = idset_foreach_first(plugins_info.plugins); (p = idset_lookup(plugins_info.plugins, id)) != NULL;
	    id = idset_foreach_next(plugins_info.plugins, id))
	{
		if(strcmp(p->name, name) == 0)
			return p;
	}
	return NULL;
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

PPortSet * plugin_portset_new(const Plugin *p)
{
	PPortSet	*ps;
	size_t		size, num, i;

	if(p == NULL)
		return NULL;
	size = dynarr_size(p->input);
	if(size == 0)
		return NULL;
	num = (size + 31) / 32;
	ps = mem_alloc(sizeof *ps + num * sizeof *ps->use + size * (sizeof *ps->input + sizeof *ps->port));
	ps->size  = size;
	ps->use   = (uint32 *) (ps + 1);
	ps->input = (PPort *) (ps->use + num);
	ps->port  = (PPInput *) (ps->input + size);
	memset(ps->use, 0, num * sizeof *ps->use);
	ps->plugin = p;
	/* Iterate over all ports, clearing them and setting to any default value. */
	for(i = 0;i < ps->size; i++)
	{
		port_init(&ps->input[i]);
		ps->port[i] = NULL;
		plugin_portset_set_default(ps, i);
	}
	return ps;
}

PPInput * plugin_portset_ports(PPortSet *ps)
{
	size_t	i;

	if(ps == NULL)
		return NULL;
	for(i = 0; i < ps->size; i++)
	{
		if(ps->use[i / 32] & (1 << (i % 32)))
		{
/*			if(port_peek_module(ps->input + i, NULL))
				printf("foo!\n");
*/			ps->port[i] = (PPInput) (ps->input + i);
		}
		else
			ps->port[i] = NULL;
	}
	return ps->port;
}

void plugin_portset_set_default(PPortSet *ps, unsigned int index)
{
	Input	*in;

	if(ps == NULL || index >= ps->size)
		return;
	in = dynarr_index(ps->plugin->input, index);
	if(in->spec.def)
	{
		port_set_from_default(ps->input + index, &in->spec.def_val);
		ps->use[index / 32] |= 1 << (index % 32);
	}
}

void plugin_portset_set_va(PPortSet *ps, unsigned int index, PValueType type, va_list arg)
{
	if(ps == NULL || index >= ps->size)
		return;
	port_clear(ps->input + index);
	if(port_set_va(ps->input + index, type, arg) == 0)
		LOG_WARN(("Input setting failed"));
	else
		ps->use[index / 32] |= 1 << (index % 32);
}

/* Set value from string representation. */
void plugin_portset_set_from_string(PPortSet *ps, unsigned int index, PValueType type, const char *string)
{
	if(ps == NULL || index >= ps->size || string == NULL)
		return;
	port_clear(ps->input + index);
	if(port_set_from_string(ps->input + index, type, string) == 0)
		LOG_WARN(("Input setting from string failed"));
	else
		ps->use[index / 32] |= 1 << (index % 32);
}

void plugin_portset_clear(PPortSet *ps, unsigned int index)
{
	uint32	pos, mask;

	if(ps == NULL || index >= ps->size)
		return;
	pos  = index / 32;
	mask = 1 << (index % 32);
	if(ps->use[pos] & mask)
	{
		port_clear(ps->input + index);
		ps->use[pos] &= ~mask;
	}
	plugin_portset_set_default(ps, index);
}

boolean plugin_portset_is_set(const PPortSet *ps, unsigned int index)
{
	if(ps == NULL || index >= ps->size)
		return 0;
	return (ps->use[index / 32] & (1 << (index % 32))) != 0;
}

size_t plugin_portset_size(const PPortSet *ps)
{
	return ps != NULL ? ps->size : 0;
}

boolean plugin_portset_get_module(const PPortSet *ps, unsigned int index, uint32 *module_id)
{
	if(ps == NULL || index >= ps->size)
		return FALSE;
	if(ps->use[index / 32] & (1 << (index % 32)))
	{
		if(port_peek_module(ps->input + index, module_id))
			return TRUE;
	}
	return FALSE;
}

void plugin_portset_describe(const PPortSet *ps, DynStr *d)
{
	unsigned int	i;

	if(ps == NULL || d == NULL)
		return;

	for(i = 0; i < ps->size; i++)
	{
		if(ps->use[i / 32] & (1 << (i % 32)))
		{
			dynstr_append_printf(d, "  <set input=\"%u\" type=\"", i);
			dynstr_append(d, port_get_type_name(&ps->input[i]));
			dynstr_append_printf(d, "\">");
			port_append_value(&ps->input[i], d);
			dynstr_append(d, "</set>\n");
		}
	}
}

void plugin_portset_destroy(PPortSet *ps)
{
	size_t	i;

	if(ps == NULL)
		return;
	for(i = 0; i < ps->size; i++)
		plugin_portset_clear(ps, i);	/* A bit costly, but destroy is rather infrequent, so... */
	mem_free(ps);
}

/* ----------------------------------------------------------------------------------------- */

int plugin_instance_init(Plugin *p, PInstance *inst)
{
	if(p == NULL || inst == NULL)
		return 0;
	inst->plugin = p;
	if((inst->inputs = plugin_portset_new(p)) == NULL)
	{
		LOG_WARN(("Couldn't get portset for instance of %s", plugin_name(p)));
		return 0;
	}
	inst->sequence = plugins_info.sequence_next++;
	printf("instance has sequence ID %u\n", inst->sequence);
	if(p->state != NULL)
	{
		if((inst->state = memchunk_alloc(p->state)) != NULL)
		{
			if(p->ctor != NULL)
				p->ctor(inst->state);
			else
				memset(inst->state, 0, memchunk_chunk_size(p->state));
			return 1;
		}
		plugin_portset_destroy(inst->inputs);
		inst->inputs = NULL;
		return 0;
	}
	inst->state = NULL;
	return 1;
}

void plugin_instance_set_output(PInstance *inst, PPOutput output)
{
	if(inst != NULL)
		inst->output = output;
}

void plugin_instance_set_link_resolver(PInstance *inst, PPOutput (*get_module)(uint32 module_id, void *data), void *data)
{
	if(inst != NULL && get_module != NULL)
	{
		inst->resolver = get_module;
		inst->resolver_data = data;
	}
}

/* Check if all inputs that require values have them. */
boolean plugin_instance_inputs_ready(const PInstance *inst)
{
	size_t	i;

	if(inst == NULL)
		return 0;
	for(i = 0; i < inst->inputs->size; i++)
	{
		Input	*in;

		in = dynarr_index(inst->plugin->input, i);
		if(in->spec.req)	/* Found a required input. Now check if there is anything there. */
		{
			if(!plugin_portset_is_set(inst->inputs, i))	/* No value there. */
				return 0;
		}
	}
	return 1;
}

PluginStatus plugin_instance_compute(PInstance *inst)
{
	PPortSet	*ps = inst->inputs;
	Plugin		*p;
	PPInput		*port;

	if(ps == NULL)
		return PLUGIN_STOP_FAILURE;
	p = inst->plugin;
	if((port = plugin_portset_ports(ps)) != NULL)
	{
		size_t		i;
		const Input	*in;

		/* Check that all required inputs have values, and re-link ports to outputs for module-inputs. */
		for(i = 0; i < dynarr_size(p->input); i++)
		{
			uint32	module;

			in = dynarr_index(p->input, i);
			if(in->spec.req && port_is_unset(ps->input + i))
				return PLUGIN_STOP_INPUT_MISSING;
			if(plugin_portset_get_module(ps, i, &module))
			{
				PPOutput	o;

				if((o = inst->resolver(module, inst->resolver_data)) != NULL)
					ps->port[i] = o;
				else
					printf("Couldn't resolve port %u\n", (unsigned int) i);
			}
			else
				ps->port[i] = &ps->input[i];
		}
		if(p->compute(port, inst->output, inst->state) == P_COMPUTE_DONE)
			return PLUGIN_STOP_COMPLETE;
		return PLUGIN_RETRY_INCOMPLETE;
	}
	return PLUGIN_STOP_FAILURE;
}

void plugin_instance_free(PInstance *inst)
{
	if(inst == NULL)
		return;
	if(inst->inputs != NULL)
	{
		plugin_portset_destroy(inst->inputs);
		inst->inputs = NULL;
	}
	if(inst->state != NULL)
	{
		if(inst->plugin->dtor != NULL)
			inst->plugin->dtor(inst->state);
		memchunk_free(inst->plugin->state, inst->state);
		inst->state = NULL;
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
	plugins_info.plugins = idset_new(1);
	plugins_info.plugins_name = hash_new_string();
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
/*		LOG_MSG(("Loaded and stored library from \"%s\"", name));*/
	}
}

#if defined _WIN32
 #define SUFFIX	".dll"
#else
 #define SUFFIX	".so"
#endif	/* _WIN32 */

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

		if((fl = filelist_new(plugins_info.paths[i], SUFFIX)) != NULL)
		{
			unsigned int	j;

			for(j = 0; j < filelist_size(fl); j++)
				library_new(filelist_filename_full(fl, j));
			filelist_destroy(fl);
		}
		else
			LOG_MSG(("Couldn't load file list from \"%s\"", plugins_info.paths[i]));
	}
}

void plugins_libraries_init(void)
{
	const List	*iter;

	/* Initialize "built-in" plug-ins. */
	plugin_input_init();
	plugin_output_init();
	plugin_clock_init();
	/* Then the ones read from disk. */
	for(iter = plugins_info.libraries; iter != NULL; iter = list_next(iter))
	{
		Library	*lib = list_data(iter);
/*		size_t	num = plugins_amount();*/
/*		LOG_MSG(("Initializing library \"%s\"", lib->name));*/
		api_init_begin(lib);
		lib->init();
		api_init_end();
/*		LOG_MSG(("Done, that added %u plug-ins", plugins_amount() - num));*/
	}
	LOG_MSG(("Initialized %u plug-ins, from %u on-disk libraries (%s-files)", idset_size(plugins_info.plugins), list_length(plugins_info.libraries), SUFFIX));
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
/*	LOG_MSG(("Registered plug-in \"%s\" with %u inputs", p->name, dynarr_size(p->input)));*/
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

	d = dynstr_new("<?xml version=\"1.0\" encoding=\"ISO-8859-1\" standalone=\"yes\"?>\n\n"
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
