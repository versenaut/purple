/*
 * Graph editing module.
*/

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "verse.h"

#include "purple.h"

#include "dynarr.h"
#include "dynstr.h"
#include "hash.h"
#include "idlist.h"
#include "idset.h"
#include "list.h"
#include "log.h"
#include "mem.h"
#include "memchunk.h"
#include "value.h"
#include "nodeset.h"
#include "plugins.h"
#include "scheduler.h"
#include "strutil.h"
#include "textbuf.h"
#include "port.h"

#include "nodedb.h"

#include "client.h"

#include "graph.h"

/* ----------------------------------------------------------------------------------------- */

typedef struct
{
	DynArr	*node;
	uint32	next;		/* Next expected label. These must be "tight", enforced by create(). */
} ONodes;

typedef struct
{
	PPort	port;		/* Result is stored here. */
	IdList	dependants;	/* Tracks dependants, to notify when output changes. */
	boolean	changed;	/* Output changed recently? Don't notify all the time... */
	ONodes	nodes;		/* Output nodes. Caches between invocations, using 'label' on create(). */
} Output;

typedef struct
{
	char	name[32];

	uint32	index_start, index_length;

	VNodeID	node;
	uint16	buffer;
	uint32	desc_start;		/* Base location in graph XML buffer, first module starts here. */
	IdSet	*modules;
} Graph;

/* A module is an instance of a plug-in, i.e. a node in a Graph. Can't call it "node", collides w/ Verse. */
typedef struct
{
	uint32		id;
	Graph		*graph;		/* Which graph does this module belong to? */
	Plugin		*plugin;
	PInstance	instance;	/* Input values, state data. */
	Output		out;		/* Things having to do with output/result, see above. */

	uint32		start, length;	/* Region in graph XML buffer used for this module. */
} Module;

/* Bookkeeping structure used to keep track of the various methods used to control the Purple engine. */
typedef struct
{
	const char	   *name;
	size_t		   param_count;
	const VNOParamType param_type[4];	/* Wastes a bit, but simplifies code. */
	const char	   *param_name[4];
	uint8		   id;			/* Filled-in once created. */ 
} MethodInfo;

/* Enumeration to allow simple IDs for methods. */
enum
{
	CREATE, DESTROY, MOD_CREATE, MOD_DESTROY, MOD_INPUT_CLEAR,
	MOD_INPUT_SET_BOOLEAN, MOD_INPUT_SET_INT32, MOD_INPUT_SET_UINT32,
	MOD_INPUT_SET_REAL32, MOD_INPUT_SET_REAL32_VEC2, MOD_INPUT_SET_REAL32_VEC3, MOD_INPUT_SET_REAL32_VEC4, MOD_INPUT_SET_REAL32_MAT16,
	MOD_INPUT_SET_REAL64, MOD_INPUT_SET_REAL64_VEC2, MOD_INPUT_SET_REAL64_VEC3, MOD_INPUT_SET_REAL64_VEC4, MOD_INPUT_SET_REAL64_MAT16,
	MOD_INPUT_SET_STRING,
	MOD_INPUT_SET_MODULE,
};

/* Create name of input-setting method. */
#define	MI_INPUT_NAME(lct)	"mod_set_" #lct

/* Create initializer for ordinary scalar input setting method. */
#define	MI_INPUT(lct, uct)	\
	{ MI_INPUT_NAME(lct), 4, { VN_O_METHOD_PTYPE_UINT32, VN_O_METHOD_PTYPE_UINT32, VN_O_METHOD_PTYPE_UINT8, \
		VN_O_METHOD_PTYPE_ ##uct }, { "graph_id", "module_id", "input", "value" } }

/* Create initializer for vector input setting method. Use shorter type name. */
#define MI_INPUT_VEC(lct, uct, len)	\
	{ MI_INPUT_NAME(lct) "v" #len, 4, { VN_O_METHOD_PTYPE_UINT32, VN_O_METHOD_PTYPE_UINT32, VN_O_METHOD_PTYPE_UINT8, \
	VN_O_METHOD_PTYPE_ ## uct ##_VEC ## len }, { "graph_id", "module_id", "input", "value" } }

/* Initialize the method bookkeeping information. This data is duplicated and sorted to create a by-name version; this
 * literal one is for lookup by the enumeration above so the order of initializers *must* match.
*/
static MethodInfo method_info[] = {
	{ "create",  3, { VN_O_METHOD_PTYPE_NODE, VN_O_METHOD_PTYPE_LAYER, VN_O_METHOD_PTYPE_STRING },
			{ "node_id", "buffer_id", "name" } },
	{ "destroy",	1, { VN_O_METHOD_PTYPE_UINT32 }, { "graph_id" } },
	
	{ "mod_create",  2, { VN_O_METHOD_PTYPE_UINT32, VN_O_METHOD_PTYPE_UINT32 }, { "graph_id", "plugin_id" } },
	{ "mod_destroy", 2, { VN_O_METHOD_PTYPE_UINT32, VN_O_METHOD_PTYPE_UINT32 }, { "graph_id", "module_id" } },
	{ "mod_input_clear", 3, { VN_O_METHOD_PTYPE_UINT32, VN_O_METHOD_PTYPE_UINT32, VN_O_METHOD_PTYPE_UINT8 },
			{ "graph_id", "module_id", "input" } },
	MI_INPUT(boolean, UINT8),
	MI_INPUT(int32, INT32),
	MI_INPUT(uint32, UINT32),
	MI_INPUT(real32, REAL32),
	MI_INPUT_VEC(r32, REAL32, 2),
	MI_INPUT_VEC(r32, REAL32, 3),
	MI_INPUT_VEC(r32, REAL32, 4),
	MI_INPUT(r32_m16, REAL32_MAT16),
	MI_INPUT(real64, REAL64),
	MI_INPUT_VEC(r64, REAL64, 2),
	MI_INPUT_VEC(r64, REAL64, 3),
	MI_INPUT_VEC(r64, REAL64, 4),
	MI_INPUT(r64_m16, REAL64_MAT16),
	MI_INPUT(string, STRING),
	MI_INPUT(module, UINT32)
};

static struct
{
	size_t		to_register;
	IdSet		*graphs;
	Hash		*graphs_name;	/* Graphs hashed on name. */

	MemChunk	*chunk_module;
} graph_info = { sizeof method_info / sizeof *method_info };

/* ----------------------------------------------------------------------------------------- */

static void	module_input_clear_links_to(Graph *g, uint32 module_id, uint32 rm);

/* ----------------------------------------------------------------------------------------- */

void graph_init(void)
{
	unsigned int	i;

	graph_info.graphs = idset_new(1);
	graph_info.graphs_name = hash_new_string();
	graph_info.chunk_module = memchunk_new("graph/module", sizeof (Module), 8);

	for(i = 0; i < sizeof method_info / sizeof *method_info; i++)
		method_info[i].id = ~0;
}

void graph_method_send_creates(uint32 avatar, uint8 group_id)
{
	int	i;

	for(i = 0; i < sizeof method_info / sizeof *method_info; i++)
	{
		verse_send_o_method_create(avatar, group_id, ~0, method_info[i].name,
					   method_info[i].param_count,
					   (VNOParamType *) method_info[i].param_type,
					   (char **) method_info[i].param_name);
	}
}

/* ----------------------------------------------------------------------------------------- */

void graph_method_check_created(NodeObject *obj)
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

		if(method_info[i].id == (uint8) ~0 && (m = nodedb_o_method_lookup(g, method_info[i].name)) != NULL)
		{
			method_info[i].id = m->id;
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
	const Node	*node;

	if((node = nodedb_lookup(g->node)) == NULL)
	{
		snprintf(buf, bufsize, " <graph id=\"%u\" name=\"%s\"/>\n", id, g->name);
		return;
	}
	snprintf(buf, bufsize,	" <graph id=\"%u\" name=\"%s\">\n"
				"  <at>\n"
				"   <node>%s</node>\n"
				"   <buffer>%u</buffer>\n"
				"  </at>\n"
				" </graph>\n", id, g->name, node->name, g->buffer);
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

/* Create a new graph. */
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

/* Destroy a graph. */
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

static void module_dep_add(const Graph *g, uint32 module_id, uint32 dep_new)
{
	Module	*m;

	if((m = idset_lookup(g->modules, module_id)) == NULL)
		return;
	idlist_insert(&m->out.dependants, dep_new);
	printf("dep %u added\n", dep_new);
}

static void module_dep_remove(const Graph *g, uint32 module_id, uint32 dep_old)
{
	Module	*m;

	if((m = idset_lookup(g->modules, module_id)) == NULL)
		return;
	idlist_remove(&m->out.dependants, dep_old);
	printf("dep %u removed\n", dep_old);
}

/* Module <m> is about to be destroyed. Notify all dependants, in both directions. Not too expensive. */
static void module_dep_destroy_warning(Module *m)
{
	IdListIter	iter;
	size_t		num, i;
	uint32		lt;

	/* First, remove input links to this module from others, the dependants. */
	for(idlist_foreach_init(&m->out.dependants, &iter); idlist_foreach_step(&m->out.dependants, &iter); )
		module_input_clear_links_to(m->graph, iter.id, m->id);
	/* Second, tell sources this module no longer depends on them. */
	num = plugin_portset_size(m->instance.inputs);
	for(i = 0; i < num; i++)
	{
		if(plugin_portset_get_module(m->instance.inputs, i, &lt))
			module_dep_remove(m->graph, lt, m->id);
	}
}

/* Mildly hackish perhaps, but simplifies interfaces. We know the port is part of a Module, so use that fact. */
#define	MODULE_FROM_PORT(p)	(Module *) ((char *) (p) - offsetof(Module, out.port))

void graph_port_output_begin(PPOutput port)
{
	Module	*m = MODULE_FROM_PORT(port);

	port_clear(port);
	m->out.changed = FALSE;
}

void graph_port_output_set(PPOutput port, PValueType type, ...)
{
	Module		*m = MODULE_FROM_PORT(port);
	va_list		arg;

	va_start(arg, type);
	port_set_va(port, type, arg);
	m->out.changed = TRUE;
	va_end(arg);
}

void graph_port_output_set_node(PPOutput port, PONode *node)
{
	Module		*m = MODULE_FROM_PORT(port);

	port_set_node(port, node);
	m->out.changed = TRUE;
}

PONode * graph_port_output_node_create(PPOutput port, VNodeType type, uint32 label)
{
	Module	*m = MODULE_FROM_PORT(port);
	Node	**node = NULL;

	if(label < m->out.nodes.next)	/* Lookup of existing? */
	{
		if(m->out.nodes.node == NULL)
			return NULL;
		if((node = dynarr_index(m->out.nodes.node, label)) != NULL)
		{
			printf("Returning previously created node with label %u\n", label);
			graph_port_output_set_node(port, *node);
			return *node;
		}
		return NULL;
	}
	else if(label == m->out.nodes.next)
	{
		printf("This would be a good time to create a new node and label it %u\n", label);
		if(m->out.nodes.node == NULL)
			m->out.nodes.node = dynarr_new(sizeof *node, 1);
		if(m->out.nodes.node != NULL)
		{
			if((node = dynarr_set(m->out.nodes.node, m->out.nodes.next, NULL)) != NULL)
			{
				if((*node = nodedb_new(type)) != NULL)
				{
					m->out.nodes.next++;
					nodedb_ref(*node);
				}
			}
		}
		if(node != NULL)
			graph_port_output_set_node(port, *node);
		return node != NULL ? *node : NULL;
	}
	else
		LOG_WARN(("Mismatched node label %u, expected %u", label, m->out.nodes.next));
	return NULL;
}

void graph_port_output_end(PPOutput port)
{
	Module		*m = MODULE_FROM_PORT(port), *dep;
	IdListIter	iter;

	if(m->out.changed)
	{
		/* Our output changed, so ask scheduler to compute() any dependant modules. */
		for(idlist_foreach_init(&m->out.dependants, &iter); idlist_foreach_step(&m->out.dependants, &iter); )
		{
			if((dep = idset_lookup(m->graph->modules, iter.id)) != NULL)
				sched_add(&dep->instance);
			else
				printf("Couldn't find module %u in graph %s, a dependant of module %u\n", iter.id, m->graph->name, m->id);
		}
	}
}

/* ----------------------------------------------------------------------------------------- */

/* Information used while traversing modules checking for cycles, helps keep argument count down. */
struct traverse_info
{
	uint32	module_id;
	uint8	input;
	uint32	source;
};

/* Traverse module links from <module_id>, looking for <source_id>. Returns FALSE if cyclic link found. */
static boolean traverse_module(const Graph *g, uint32 module_id, uint32 source_id, const struct traverse_info *ti)
{
	const Module	*m;

	if((m = idset_lookup(g->modules, module_id)) != NULL)
	{
		unsigned int	i;

		for(i = plugin_portset_size(m->instance.inputs); i-- > 0;)
		{
			uint32	link;

			if(module_id == ti->module_id && i == ti->input)
				link = ti->source;
			else if(plugin_portset_get_module(m->instance.inputs, i, &link) && idset_lookup(g->modules, link))
				;
			else
				continue;
			if(link == source_id)
				return FALSE;
			if(!traverse_module(g, link, source_id, ti))
				return FALSE;
		}
	}
	return TRUE;
}

/* Answer the fairly specific question: does the graph <graph_id> become cyclic if module
 * <module_id>'s <input>:th input is set to be <source>? Used to disallow such setting.
 * This implementation is totally naive, and requires O(n^2) for a graph with n nodes.
*/
static boolean graph_cyclic_after(uint32 graph_id, uint32 module_id, uint8 input, uint32 source)
{
	const Graph	*g;
	unsigned int	i;
	size_t		num;
	struct traverse_info	ti;

	if((g = idset_lookup(graph_info.graphs, graph_id)) == NULL)
		return FALSE;

	num = idset_size(g->modules);

	ti.module_id = module_id;
	ti.input     = input;
	ti.source    = source;

	for(i = 0; i < num; i++)
	{
		if(!traverse_module(g, i, i, &ti))
			return TRUE;
	}
	return FALSE;
}

/* ----------------------------------------------------------------------------------------- */

/* Build description of a single module, as a freshly created dynamic string. */
static DynStr * module_build_desc(const Module *m)
{
	DynStr	*d;

	d = dynstr_new_sized(256);
	dynstr_append_printf(d, " <module id=\"%u\" plug-in=\"%u\">\n", m->id, plugin_id(m->plugin));
	plugin_portset_describe(m->instance.inputs, d);
	dynstr_append(d, " </module>\n");

	return d;
}

/* Update start positions of module descriptions. Handy when one changes/appears. */
static void graph_modules_desc_start_update(Graph *g)
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

static PPOutput cb_module_lookup(uint32 module_id, void *data)
{
	Module	*m;

	if((m = idset_lookup(((Graph *) data)->modules, module_id)) != NULL)
		return &m->out.port;
	return NULL;
}

/* Create a new module, i.e. a plug-in instance, in a graph. */
static void module_create(uint32 graph_id, uint32 plugin_id)
{
	Plugin	*p;
	Graph	*g;
	Module	*m;
	DynStr	*desc;

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
	m->graph = g;
	m->plugin = p;
	port_init(&m->out.port);
	plugin_instance_init(m->plugin, &m->instance);
	plugin_instance_set_output(&m->instance, &m->out.port);
	plugin_instance_set_link_resolver(&m->instance, cb_module_lookup, g);
	idlist_construct(&m->out.dependants);
	m->out.nodes.node = NULL;
	m->out.nodes.next = 0;
	m->start = m->length = 0;
	if(g->modules == NULL)
		g->modules = idset_new(0);
	m->id = idset_insert(g->modules, m);
	LOG_MSG(("Module %u.%u is plugin %u (%s)", graph_id, m->id, plugin_id, plugin_name(p)));
	desc = module_build_desc(m);
	m->length = dynstr_length(desc);
	graph_modules_desc_start_update(g);
	verse_send_t_text_set(g->node, g->buffer, m->start, 0, dynstr_string(desc));
	dynstr_destroy(desc, 1);
}

/* Release all labeled nodes created by an instance. */
static void output_nodes_clear(Output *o)
{
	if(o->nodes.node != NULL)
	{
		unsigned int	i;
		Node		**n;

		for(i = 0; i < o->nodes.next; i++)
		{
			if((n = dynarr_index(o->nodes.node, i)) != NULL)
				nodedb_unref(*n);
		}
		dynarr_destroy(o->nodes.node);
	}
}

/* Destroy a module. */
static void module_destroy(uint32 graph_id, uint32 module_id)
{
	Graph	*g;
	Module	*m;

	if((g = idset_lookup(graph_info.graphs, graph_id)) == NULL)
	{
		LOG_WARN(("Attempted to destroy module %u in unknown graph %u, aborting", module_id, graph_id));
		return;
	}
	if((m = idset_lookup(g->modules, module_id)) == NULL)
	{
		LOG_WARN(("Attempted to destroy unknown module %u in graph %u, aborting", module_id, graph_id));
		return;
	}
	module_dep_destroy_warning(m);
	idset_remove(g->modules, module_id);
	verse_send_t_text_set(g->node, g->buffer, m->start, m->length, NULL);
	idlist_destruct(&m->out.dependants);
	plugin_instance_free(&m->instance);
	port_clear(&m->out.port);
	output_nodes_clear(&m->out);
	memchunk_free(graph_info.chunk_module, m);
	graph_modules_desc_start_update(g);
}

/* Set a module input to a value. The value might be either a literal, or a reference to another module's output. */
static void module_input_set(uint32 graph_id, uint32 module_id, uint8 input_index, PValueType type, ...)
{
	va_list	arg;
	Graph	*g;
	Module	*m;
	DynStr	*desc;
	uint32	old_link;

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

	/* Remove any existing dependency by this input. */
	if(plugin_portset_get_module(m->instance.inputs, input_index, &old_link))
		module_dep_remove(g, old_link, m->id);

	va_start(arg, type);
	plugin_portset_set_va(m->instance.inputs, input_index, type, arg);
	va_end(arg);
	desc = module_build_desc(m);
	verse_send_t_text_set(g->node, g->buffer, m->start, m->length, dynstr_string(desc));
	m->length = dynstr_length(desc);
	dynstr_destroy(desc, 1);
	graph_modules_desc_start_update(g);

	/* Did we just set a link to someone? Then notify that someone about new dependant. */
	if(type == P_VALUE_MODULE)
	{
		uint32	other;

		plugin_portset_get_module(m->instance.inputs, input_index, &other);
		module_dep_add(g, other, module_id);
	}
	sched_add(&m->instance);
}

/* Clear a module input, i.e. remove any assigned value and leave it "floating" as after module creation. */
static void module_input_clear(uint32 graph_id, uint32 module_id, uint8 input_index)
{
	Graph	*g;
	Module	*m;
	DynStr	*desc;
	boolean	was_link;
	uint32	old_link;

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
	was_link = plugin_portset_get_module(m->instance.inputs, input_index, &old_link);
	plugin_portset_clear(m->instance.inputs, input_index);
	desc = module_build_desc(m);
	verse_send_t_text_set(g->node, g->buffer, m->start, m->length, dynstr_string(desc));
	m->length = dynstr_length(desc);
	dynstr_destroy(desc, 1);
	graph_modules_desc_start_update(g);

	/* If a link was cleared, notify other end it has one less dependants. */
	if(was_link)
		module_dep_remove(g, old_link, m->id);
}

/* Go through module's inputs, and clear any inputs that refer to module <rm>. This typically happens
 * because it (rm) is about to be deleted, and we don't want any dangling links afterwards.
*/
static void module_input_clear_links_to(Graph *g, uint32 module_id, uint32 rm)
{
	Module	*m;
	size_t	ni, i;
	boolean	refresh = FALSE;
	uint32	lt;

	if((m = idset_lookup(g->modules, module_id)) == NULL)
		return;
	ni = plugin_portset_size(m->instance.inputs);
	for(i = 0; i < ni; i++)
	{
		if(plugin_portset_get_module(m->instance.inputs, i, &lt) && lt == rm)
		{
			plugin_portset_clear(m->instance.inputs, i);
			refresh = TRUE;
		}
	}
	if(refresh)
	{
		DynStr	*desc;

		desc = module_build_desc(m);
		verse_send_t_text_set(g->node, g->buffer, m->start, m->length, dynstr_string(desc));
		m->length = dynstr_length(desc);
		dynstr_destroy(desc, 1);
		graph_modules_desc_start_update(g);
	}
}

/* ----------------------------------------------------------------------------------------- */

void send_method_call(int method, const VNOParam *param)
{
	const MethodInfo *mi;
	void *pack;

	if(method < 0 || method >= sizeof method_info / sizeof *method_info)
		return;
	mi = method_info + method;
	if(mi->id == (uint8) ~0)
	{
		LOG_WARN(("Can't send call to method %s(), not created yet", mi->name));
		return;
	}
	if((pack = verse_method_call_pack(mi->param_count, mi->param_type, param)) != NULL)
		verse_send_o_method_call(client_info.avatar, client_info.gid_control, mi->id, 0, pack);
}

void graph_method_send_call_create(VNodeID node, VLayerID buffer, const char *name)
{
	VNOParam	param[3];

	param[0].vnode   = node;
	param[1].vlayer  = buffer;
	param[2].vstring = (char *) name;
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

void graph_method_send_call_mod_destroy(uint32 graph_id, uint32 module_id)
{
	VNOParam	param[2];

	param[0].vuint32 = graph_id;
	param[1].vuint32 = module_id;
	send_method_call(MOD_DESTROY, param);
}

void graph_method_send_call_mod_input_set(uint32 graph_id, uint32 mod_id, uint32 index, PValueType vtype, const PValue *value)
{
	VNOParam	param[4];
	VNOParamType	type[4] = { VN_O_METHOD_PTYPE_UINT32, VN_O_METHOD_PTYPE_UINT32, VN_O_METHOD_PTYPE_UINT8 };
	void		*pack;
	int		method = 0, i;

	param[0].vuint32 = graph_id;
	param[1].vuint32 = mod_id;
	param[2].vuint8  = index;
	/* Map module input type "down" to Verse method call parameter type. */
	switch(vtype)
	{
	case P_VALUE_BOOLEAN:
		type[3] = VN_O_METHOD_PTYPE_UINT8;
		param[3].vuint8 = value->v.vboolean;
		method = MOD_INPUT_SET_BOOLEAN;
		break;
	case P_VALUE_INT32:
		type[3] = VN_O_METHOD_PTYPE_INT32;
		param[3].vint32 = value->v.vint32;
		method = MOD_INPUT_SET_INT32;
		break;
	case P_VALUE_UINT32:
		type[3] = VN_O_METHOD_PTYPE_UINT32;
		param[3].vuint32 = value->v.vuint32;
		method = MOD_INPUT_SET_UINT32;
		break;
	case P_VALUE_REAL32:
		type[3] = VN_O_METHOD_PTYPE_REAL32;
		param[3].vreal32 = value->v.vreal32;
		method = MOD_INPUT_SET_REAL32;
		break;
	case P_VALUE_REAL32_VEC2:
		type[3] = VN_O_METHOD_PTYPE_REAL32_VEC2;
		param[3].vreal32_vec[0] = value->v.vreal32_vec2[0];
		param[3].vreal32_vec[1] = value->v.vreal32_vec2[1];
		method = MOD_INPUT_SET_REAL32_VEC2;
		break;
	case P_VALUE_REAL32_VEC3:
		type[3] = VN_O_METHOD_PTYPE_REAL32_VEC3;
		param[3].vreal32_vec[0] = value->v.vreal32_vec3[0];
		param[3].vreal32_vec[1] = value->v.vreal32_vec3[1];
		param[3].vreal32_vec[2] = value->v.vreal32_vec3[2];
		method = MOD_INPUT_SET_REAL32_VEC3;
		break;
	case P_VALUE_REAL32_VEC4:
		type[3] = VN_O_METHOD_PTYPE_REAL32_VEC4;
		param[3].vreal32_vec[0] = value->v.vreal32_vec4[0];
		param[3].vreal32_vec[1] = value->v.vreal32_vec4[1];
		param[3].vreal32_vec[2] = value->v.vreal32_vec4[2];
		param[3].vreal32_vec[3] = value->v.vreal32_vec4[3];
		method = MOD_INPUT_SET_REAL32_VEC4;
		break;
	case P_VALUE_REAL32_MAT16:
		type[3] = VN_O_METHOD_PTYPE_REAL32_MAT16;
		for(i = 0; i < 16; i++)
			param[3].vreal32_mat[i] = value->v.vreal32_mat16[i];
		method = MOD_INPUT_SET_REAL32_MAT16;
		break;
	case P_VALUE_REAL64:
		type[3] = VN_O_METHOD_PTYPE_REAL64;
		param[3].vreal64 = value->v.vreal64;
		method = MOD_INPUT_SET_REAL64;
		break;
	case P_VALUE_REAL64_VEC2:
		type[3] = VN_O_METHOD_PTYPE_REAL64_VEC2;
		param[3].vreal64_vec[0] = value->v.vreal64_vec2[0];
		param[3].vreal64_vec[1] = value->v.vreal64_vec2[1];
		method = MOD_INPUT_SET_REAL64_VEC2;
		break;
	case P_VALUE_REAL64_VEC3:
		type[3] = VN_O_METHOD_PTYPE_REAL64_VEC3;
		param[3].vreal64_vec[0] = value->v.vreal64_vec3[0];
		param[3].vreal64_vec[1] = value->v.vreal64_vec3[1];
		param[3].vreal64_vec[2] = value->v.vreal64_vec3[2];
		method = MOD_INPUT_SET_REAL64_VEC3;
		break;
	case P_VALUE_REAL64_VEC4:
		type[3] = VN_O_METHOD_PTYPE_REAL64_VEC4;
		param[3].vreal64_vec[0] = value->v.vreal64_vec4[0];
		param[3].vreal64_vec[1] = value->v.vreal64_vec4[1];
		param[3].vreal64_vec[2] = value->v.vreal64_vec4[2];
		param[3].vreal64_vec[3] = value->v.vreal64_vec4[3];
		method = MOD_INPUT_SET_REAL64_VEC4;
		break;
	case P_VALUE_REAL64_MAT16:
		type[3] = VN_O_METHOD_PTYPE_REAL64_MAT16;
		for(i = 0; i < 16; i++)
			param[3].vreal64_mat[i] = value->v.vreal64_mat16[i];
		method = MOD_INPUT_SET_REAL64_MAT16;
		break;
	case P_VALUE_MODULE:
		type[3] = VN_O_METHOD_PTYPE_UINT32;
		param[3].vuint32 = value->v.vmodule;
		method = MOD_INPUT_SET_MODULE;
		break;
	case P_VALUE_STRING:
		type[3] = VN_O_METHOD_PTYPE_STRING;
		param[3].vstring = value->v.vstring;
		method = MOD_INPUT_SET_STRING;
		break;
	default:
		LOG_WARN(("Can't prepare input setting, type %d", vtype));
		return;
	}
	if((pack = verse_method_call_pack(4, type, param)) != NULL)
		verse_send_o_method_call(client_info.avatar, client_info.gid_control,
					 method_info[method].id, 0, pack);
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

		if(id != mi->id)
			continue;
		verse_method_call_unpack(param, mi->param_count, mi->param_type, arg);
		switch(i)
		{
		case CREATE:	graph_create(arg[0].vnode, arg[1].vlayer, arg[2].vstring);	break;
		case DESTROY:	graph_destroy(arg[0].vuint32);					break;
		case MOD_CREATE: module_create(arg[0].vuint32, arg[1].vuint32);			break;
		case MOD_DESTROY: module_destroy(arg[0].vuint32, arg[1].vuint32);		break;
		case MOD_INPUT_CLEAR:
			module_input_clear(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8);
			break;
		case MOD_INPUT_SET_BOOLEAN:
			module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_BOOLEAN, arg[3].vuint8);
			break;
		case MOD_INPUT_SET_INT32:
			module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_INT32, arg[3].vint32);
			break;
		case MOD_INPUT_SET_UINT32:
			module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_UINT32, arg[3].vuint32);
			break;
		case MOD_INPUT_SET_REAL32:
			module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_REAL32, arg[3].vreal32);
			break;
		case MOD_INPUT_SET_REAL32_VEC2:
			module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_REAL32_VEC2, &arg[3].vreal32_vec);
			break;
		case MOD_INPUT_SET_REAL32_VEC3:
			module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_REAL32_VEC3, &arg[3].vreal32_vec);
			break;
		case MOD_INPUT_SET_REAL32_VEC4:
			module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_REAL32_VEC4, &arg[3].vreal32_vec);
			break;
		case MOD_INPUT_SET_REAL32_MAT16:
			module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_REAL32_MAT16, &arg[3].vreal32_mat);
			break;
		case MOD_INPUT_SET_REAL64:
			module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_REAL64, arg[3].vreal64);
			break;
		case MOD_INPUT_SET_REAL64_VEC2:
			module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_REAL64_VEC2, &arg[3].vreal64_vec);
			break;
		case MOD_INPUT_SET_REAL64_VEC3:
			module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_REAL64_VEC3, &arg[3].vreal64_vec);
			break;
		case MOD_INPUT_SET_REAL64_VEC4:
			module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_REAL64_VEC4, &arg[3].vreal64_vec);
			break;
		case MOD_INPUT_SET_REAL64_MAT16:
			module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_REAL64_MAT16, &arg[3].vreal64_mat);
			break;
		case MOD_INPUT_SET_MODULE:
			if(!graph_cyclic_after(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, arg[3].vuint32))
				module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_MODULE, arg[3].vuint32);
			break;
		case MOD_INPUT_SET_STRING:
			module_input_set(arg[0].vuint32, arg[1].vuint32, arg[2].vuint8, P_VALUE_STRING, arg[3].vstring);
			break;
		}
		return;
	}
	LOG_WARN(("Received call to unknown graph method %u", id));
}
