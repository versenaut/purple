/*
 * 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "verse.h"

#include "dynarr.h"
#include "dynstr.h"
#include "hash.h"
#include "idset.h"
#include "list.h"
#include "log.h"
#include "mem.h"
#include "memchunk.h"
#include "plugins.h"
#include "strutil.h"
#include "textbuf.h"

#include "nodedb.h"

#include "client.h"

#include "graph.h"

/* ----------------------------------------------------------------------------------------- */

typedef struct
{
	uint32		id;
	const Plugin	*plugin;	/* FIXME: Should be properly typed, of course. Placeholder. */
	PInputSet	*inputs;

	uint32		start, length;	/* Region in graph XML buffer used for this module. */
} Module;

typedef struct
{
	char	name[32];

	uint32	index_start, index_length;

	VNodeID	node;
	uint16	buffer;
	uint32	desc_start;	/* Base location in graph XML buffer, first module starts here. */
	IdSet	*modules;
} Graph;

typedef struct
{
	const char	   *name;
	size_t		   param_count;
	const VNOParamType param_type[4];	/* Wastes a bit, but simplifies code. */
	const char	   *param_name[4];
	const NdbOMethod   *method;		/* Filled-in once created. */ 
} MethodInfo;

enum { CREATE, DESTROY, MOD_CREATE, MOD_INPUT_CLEAR, MOD_INPUT_SET_REAL32 };

#define	MI_INPUT(lct, uct)	\
	{ "m_i_set_" #lct, 4, { VN_O_METHOD_PTYPE_UINT32, VN_O_METHOD_PTYPE_UINT32, VN_O_METHOD_PTYPE_UINT8, \
		VN_O_METHOD_PTYPE_ ##uct }, { "graph_id", "plugin_id", "input", "value" }, NULL \
	}

static MethodInfo method_info[] = {
	{ "create",  3, { VN_O_METHOD_PTYPE_NODE, VN_O_METHOD_PTYPE_LAYER },
			{ "node_id", "buffer_id", "name" }, NULL},
	{ "destroy",	1, { VN_O_METHOD_PTYPE_UINT32 }, { "graph_id" }, NULL },
	
	{ "mod_create",  2, { VN_O_METHOD_PTYPE_UINT32, VN_O_METHOD_PTYPE_UINT32 }, { "graph_id", "plugin_id" } },
	{ "mod_input_clear", 3, { VN_O_METHOD_PTYPE_UINT32, VN_O_METHOD_PTYPE_UINT32, VN_O_METHOD_PTYPE_UINT8 },
			{ "graph_id", "plugin_id", "input" }, NULL },
	MI_INPUT(real32, REAL32),
/*	{ 0, "mod_destroy", 2, { VN_O_METHOD_PTYPE_UINT32, VN_O_METHOD_PTYPE_UINT32 }, { "graph_id", "module_id" } }
*/};

static struct
{
	size_t		to_register;
	IdSet		*graphs;
	Hash		*graphs_name;	/* Graphs hashed on name. */

	MemChunk	*chunk_module;
} graph_info = { sizeof method_info / sizeof *method_info };

/* ----------------------------------------------------------------------------------------- */

void graph_init(void)
{
	graph_info.graphs = idset_new(1);
	graph_info.graphs_name = hash_new_string();
	graph_info.chunk_module = memchunk_new("graph/module", sizeof (Module), 8);
}

void graph_method_send_creates(uint32 avatar, uint8 group_id)
{
	int	i;

	for(i = 0; i < sizeof method_info / sizeof *method_info; i++)
	{
		verse_send_o_method_create(avatar, group_id, -(1 + i), method_info[i].name,
					   method_info[i].param_count,
					   method_info[i].param_type,
					   method_info[i].param_name);
	}
}

/* ----------------------------------------------------------------------------------------- */

void graph_method_receive_create(NodeObject *obj)
{
	NdbOMethodGroup	*g;
	unsigned int	i;

	if(graph_info.to_register == 0)
		return;
	if((g = nodedb_o_method_group_lookup(obj, "PurpleGraph")) == NULL)
		return;

	for(i = 0; i < sizeof method_info / sizeof *method_info; i++)
	{
		const NdbOMethod	*m;

		if(method_info[i].method == NULL && (m = nodedb_o_method_lookup(g, method_info[i].name)) != NULL)
		{
			method_info[i].method = m;
			graph_info.to_register--;
			if(graph_info.to_register == 0)
				LOG_MSG(("Caught all %u methods, ready to send calls", sizeof method_info / sizeof *method_info));
			return;
		}
	}
	LOG_WARN(("Received unknown (non graph-related?) method creation"));
}

/* ----------------------------------------------------------------------------------------- */

/* Build the XML description of a graph, for the index. */
static void graph_index_build(uint32 id, const Graph *g, char *buf, size_t bufsize)
{
	snprintf(buf, bufsize,	" <graph id=\"%u\" name=\"%s\">\n"
				"  <at>\n"
				"   <node>%u</node>\n"
				"   <buffer>%u</buffer>\n"
				"  </at>\n"
				" </graph>\n", id, g->name, g->node, g->buffer);
}

/* Go through all graphs, and recompute XML starting points. Handy when a graph has been
 * deleted, or when the length of one or more graph's XML representation has changed.
*/
static void graph_index_renumber(void)
{
	unsigned int	id, pos;
	Graph		*g;

	pos = client_info.graphs.start;
	for(id = idset_foreach_first(graph_info.graphs); (g = idset_lookup(graph_info.graphs, id)) != NULL; id = idset_foreach_next(graph_info.graphs, id))
	{
		g->index_start = pos;
		pos += g->index_length;
	}
}

static void graph_create(VNodeID node_id, uint16 buffer_id, const char *name)
{
	unsigned int	id, i, pos;
	Graph		*g, *me;
	char		xml[256];

	/* Make sure name is unique. */
	if(hash_lookup(graph_info.graphs_name, name) != NULL)
		return;	/* It wasn't. */

	me = g = mem_alloc(sizeof *g);
	stu_strncpy(g->name, sizeof g->name, name);
	g->node   = node_id;
	g->buffer = buffer_id;
	id = idset_insert(graph_info.graphs, g);
	g->desc_start = 0;
	g->modules = NULL;		/* Be a bit lazy. */

	for(i = 0, pos = client_info.graphs.start; i < id; i++)
	{
		g = idset_lookup(graph_info.graphs, i);
		if(g == NULL)
			continue;
		pos += g->index_length;
	}
	me->index_start = pos;
	graph_index_build(id, me, xml, sizeof xml);
	me->index_length = strlen(xml);
	verse_send_t_text_set(client_info.meta, client_info.graphs.buffer, me->index_start, 0, xml);
	hash_insert(graph_info.graphs_name, me->name, me);
	snprintf(xml, sizeof xml, "<graph>\n</graph>\n");
	verse_send_t_text_set(me->node, me->buffer, 0, ~0, xml);
	me->desc_start = strchr(xml, '/') - xml - 1;
}

static void graph_destroy(uint32 id)
{
	Graph	*g;

	if((g = idset_lookup(graph_info.graphs, id)) == NULL)
	{
		LOG_WARN(("Couldn't destroy graph %u, not found", id));
		return;
	}
	g->name[0] = '\0';
	hash_remove(graph_info.graphs_name, g->name);
	idset_remove(graph_info.graphs, id);
	mem_free(g);
	verse_send_t_text_set(client_info.meta, client_info.graphs.buffer, g->index_start, g->index_length, NULL);
	graph_index_renumber();
}

/* ----------------------------------------------------------------------------------------- */

static DynStr * module_build_desc(const Module *m)
{
	DynStr	*d;

	d = dynstr_new_sized(256);
	dynstr_append_printf(d, " <module id=\"%u\" plug-in=\"%u\">\n", m->id, plugin_id(m->plugin));
	plugin_inputset_describe(m->inputs, d);
	dynstr_append(d, " </module>\n");

	return d;
}

static void modules_desc_start_update(Graph *g)
{
	unsigned int	id;
	Module		*m;
	size_t		pos;

	for(id = idset_foreach_first(g->modules), pos = g->desc_start;
	    (m = idset_lookup(g->modules, id)) != NULL;
	    id = idset_foreach_next(g->modules, id))
	{
		m->start = pos;
		pos += m->length;
	}
}

static void module_create(uint32 graph_id, uint32 plugin_id)
{
	const Plugin	*p;
	Graph		*g;
	Module		*m;
	DynStr		*desc;

	if((p = plugin_lookup(plugin_id)) == NULL)
	{
		LOG_WARN(("Attempted to instantiate plug-in %u; not found", plugin_id));
		return;
	}
	if((g = idset_lookup(graph_info.graphs, graph_id)) == NULL)
	{
		LOG_WARN(("Attempted to instantiate plug-in %u in unknown graph %u, aborting", plugin_id, graph_id));
		return;
	}
	m = memchunk_alloc(graph_info.chunk_module);
	if(m == NULL)
	{
		LOG_WARN(("Module allocation failed in graph %u, plug-in %u", graph_id, plugin_id));
		return;
	}
	m->plugin = p;
	m->inputs = plugin_inputset_new(m->plugin);
	m->start = m->length = 0;
	if(g->modules == NULL)
		g->modules = idset_new(0);
	m->id = idset_insert(g->modules, m);
	LOG_MSG(("Instantiated plugin %u (%s) as module %u in graph %u (%s)", plugin_id, plugin_name(p), m->id, graph_id, g->name));
	desc = module_build_desc(m);
	m->length = dynstr_length(desc);
	modules_desc_start_update(g);
	verse_send_t_text_set(g->node, g->buffer, m->start, 0, dynstr_string(desc));
	dynstr_destroy(desc, 1);
}

static void module_input_set(uint32 graph_id, uint32 module_id, uint8 input_index, PInputType type, ...)
{
	va_list	arg;
	Graph	*g;
	Module	*m;
	DynStr	*desc;

	if((g = idset_lookup(graph_info.graphs, graph_id)) == NULL)
	{
		LOG_WARN(("Attempted to set module input in non-existant graph %u", graph_id));
		return;
	}
	if((m = idset_lookup(g->modules, module_id)) == NULL)
	{
		LOG_WARN(("Attempted to set module input in non-existant module %u.%u", graph_id, module_id));
		return;
	}
	va_start(arg, type);
	plugin_inputset_set_va(m->inputs, input_index, type, arg);
	va_end(arg);
	desc = module_build_desc(m);
	verse_send_t_text_set(g->node, g->buffer, m->start, m->length, dynstr_string(desc));
	m->length = dynstr_length(desc);
	dynstr_destroy(desc, 1);
	modules_desc_start_update(g);
}

static void module_input_clear(uint32 graph_id, uint32 module_id, uint8 input_index)
{
	Graph	*g;
	Module	*m;
	DynStr	*desc;

	if((g = idset_lookup(graph_info.graphs, graph_id)) == NULL)
	{
		LOG_WARN(("Attempted to clear module input in non-existant graph %u", graph_id));
		return;
	}
	if((m = idset_lookup(g->modules, module_id)) == NULL)
	{
		LOG_WARN(("Attempted to clear module input in non-existant module %u.%u", graph_id, module_id));
		return;
	}

	plugin_inputset_clear(m->inputs, input_index);
	desc = module_build_desc(m);
	verse_send_t_text_set(g->node, g->buffer, m->start, m->length, dynstr_string(desc));
	m->length = dynstr_length(desc);
	dynstr_destroy(desc, 1);
	modules_desc_start_update(g);
}

/* ----------------------------------------------------------------------------------------- */

void send_method_call(int method, const VNOParam *param)
{
	const MethodInfo *mi;
	void *pack;

	if(method < 0 || method >= sizeof method_info / sizeof *method_info)
		return;
	mi = method_info + method;
	if((pack = verse_method_call_pack(mi->param_count, mi->param_type, param)) != NULL)
		verse_send_o_method_call(client_info.avatar, client_info.gid_control, mi->method->id, 0, pack);
}

void graph_method_send_call_create(VNodeID node, VLayerID buffer, const char *name)
{
	VNOParam	param[3];

	param[0].vnode   = node;
	param[1].vlayer  = buffer;
	param[2].vstring = name;
	send_method_call(CREATE, param);
}

void graph_method_send_call_destroy(uint32 id)
{
	VNOParam	param[1];

	param[0].vuint32 = id;
	send_method_call(DESTROY, param);
}

void graph_method_send_call_mod_create(uint32 graph_id, uint32 plugin_id)
{
	VNOParam	param[2];

	param[0].vuint32 = graph_id;
	param[1].vuint32 = plugin_id;
	send_method_call(MOD_CREATE, param);
}

void graph_method_send_call_mod_input_set(uint32 graph_id, uint32 mod_id, uint32 index, PInputType vtype, ...)
{
	VNOParam	param[4];
	VNOParamType	type[4] = { VN_O_METHOD_PTYPE_UINT32, VN_O_METHOD_PTYPE_UINT32, VN_O_METHOD_PTYPE_UINT8 };
	va_list		arg;
	void		*pack;

	param[0].vuint32 = graph_id;
	param[1].vuint32 = mod_id;
	param[2].vuint8  = index;
	va_start(arg, vtype);
	switch(vtype)
	{
	case P_INPUT_REAL32:
		param[3].vreal32 = (real32) va_arg(arg, double);
		type[3] = VN_O_METHOD_PTYPE_REAL32;
		break;
	default:
		LOG_WARN(("Input-setting type %d not implemented", vtype));
		break;
	}
	va_end(arg);
/*	if((pack = verse_method_call_pack(4, type, param)) != NULL)
		verse_send_o_method_call(client_info.avatar, client_info.gid_control->id, method_info[MOD_INPUT_SET_REAL32].id, 0, pack);
*/
}

void graph_method_send_call_mod_input_clear(uint32 graph_id, uint32 module_id, uint32 input)
{
	VNOParam	param[3];

	param[0].vuint32 = graph_id;
	param[1].vuint32 = module_id;
	param[2].vuint8  = input;
	send_method_call(MOD_INPUT_CLEAR, param);
}

void graph_method_receive_call(uint8 id, const void *param)
{
	VNOParam	arg[8];
	int		i;

	for(i = 0; i < sizeof method_info / sizeof *method_info; i++)
	{
		const MethodInfo	*mi = method_info + i;

		if(id == mi->method->id)
		{
			verse_method_call_unpack(param, mi->param_count, mi->param_type, arg);
			switch(i)
			{
			case CREATE:	graph_create(arg[0].vnode, arg[1].vlayer, arg[2].vstring);	break;
			case DESTROY:	graph_destroy(arg[0].vuint32);					break;
			case MOD_CREATE: module_create(arg[0].vuint32, arg[1].vuint32);			break;
			case MOD_INPUT_CLEAR:
				module_input_clear(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8);
				break;
			case MOD_INPUT_SET_REAL32:
				module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_INPUT_REAL32, arg[3].vreal32);
				break;
			}
			return;
		}
	}
	LOG_WARN(("Received call to unknown graph method %u", id));
}
