/*
 * 
*/

#include <stdio.h>
#include <string.h>

#include "verse.h"

#include "hash.h"
#include "list.h"
#include "log.h"
#include "mem.h"
#include "strutil.h"
#include "textbuf.h"

#include "client.h"

#include "graph.h"

/* ----------------------------------------------------------------------------------------- */

typedef struct
{
	uint32	id;
	char	name[32];

	VNodeID	node;
	uint16	buffer;

	uint	xml_start, xml_length;
} Graph;

static const VNOParamType	create_type[]   = { VN_O_METHOD_PTYPE_NODE, VN_O_METHOD_PTYPE_LAYER,
							VN_O_METHOD_PTYPE_STRING },
				rename_type[]   = { VN_O_METHOD_PTYPE_UINT32 },
				destroy_type[]  = { VN_O_METHOD_PTYPE_UINT32 };
static char			*create_name[]  = { "node", "buffer", "name" },
				*rename_name[]  = { "graph" },
				*destroy_name[] = { "graph" };

static struct
{
	uint8	mid_create, mid_rename, mid_destroy;
	uint32	next_id;
	List	*free_ids;
	Hash	*graphs_id, *graphs_name;	/* Graphs hashed on ID and name, respectively. */
} graph_info = { 0 };

/* ----------------------------------------------------------------------------------------- */

void graph_init(void)
{
	graph_info.next_id  = 0;
	graph_info.free_ids = NULL;
}

void graph_method_send_creates(uint32 avatar, uint8 group_id)
{
	verse_send_o_method_create(avatar, group_id, -1, "graph_create",
				   sizeof create_type / sizeof *create_type, create_type, create_name);
	verse_send_o_method_create(avatar, group_id, -2, "graph_rename", 
				   sizeof rename_type / sizeof *rename_type, rename_type, rename_name);
	verse_send_o_method_create(avatar, group_id, -3, "graph_destroy", 
				   sizeof destroy_type / sizeof *destroy_type, destroy_type, destroy_name);
}

/* ----------------------------------------------------------------------------------------- */

void graph_method_receive_create(uint8 id, const char *name)
{
	if(strcmp(name, "graph_create") == 0)
		graph_info.mid_create = id;
	else if(strcmp(name, "graph_rename") == 0)
		graph_info.mid_rename = id;
	else if(strcmp(name, "graph_destroy") == 0)
		graph_info.mid_destroy = id;
	else
		LOG_WARN(("Received unknown (non graph-related?) method creation, %s()", name));
}

/* ----------------------------------------------------------------------------------------- */

static unsigned int graph_direct_hash(const void *a)
{
	return ((const Graph *) a)->id;
}

static int graph_direct_compare(const void *a, const void *b)
{
	return strcmp(((const Graph *) a)->name, ((const Graph *) b)->name) == 0;
}

static Graph * graph_create(VNodeID node_id, uint16 buffer_id, const char *name)
{
	Graph	*g;

	printf("Create graph named '%s' in node %u, buffer %u\n", name, node_id, buffer_id);
	/* Make sure name is unique. */
	if(hash_lookup(graph_info.graphs_name, name) != NULL)
		return NULL;	/* It wasn't. */

	if(graph_info.graphs_id == NULL)
		graph_info.graphs_id = hash_new(graph_direct_hash, graph_direct_compare);
	if(graph_info.graphs_name == NULL)
		graph_info.graphs_name = hash_new_string();

	g = mem_alloc(sizeof *g);
	if(graph_info.free_ids != NULL)
	{
		g->id = (uint32) list_data(graph_info.free_ids);
		graph_info.free_ids = list_tail(graph_info.free_ids);
	}
	else
		g->id = graph_info.next_id++;
	stu_strncpy(g->name, sizeof g->name, name);
	g->node   = node_id;
	g->buffer = buffer_id;

	printf("<graph name=\"%s\" id=\"%u\">\n"
	       " <at>\n"
	       "  <node>%u</node>\n"
	       "  <buffer>%u</buffer>\n"
	       " </at>\n"
	       "</graph>\n", g->name, g->id, node_id, buffer_id);

	hash_insert(graph_info.graphs_id, (void *) g->id, g);
	hash_insert(graph_info.graphs_name, g->name, g);

	return g;
}

static void graph_destroy(uint32 id)
{
	Graph	*g;

	if((g = hash_lookup(graph_info.graphs_id, id)) == NULL)
	{
		LOG_WARN(("Couldn't destroy graph %u, not found", id));
		return;
	}
	graph_info.free_ids = list_prepend(graph_info.free_ids, list_new((void *) g->id));

	hash_remove(graph_info.graphs_id, g->id);
	hash_remove(graph_info.graphs_id, g->name);
	mem_free(g);
}

/* ----------------------------------------------------------------------------------------- */

void graph_method_send_call_create(VNodeID node, VLayerID buffer, const char *name)
{
	VNOParam	param[sizeof create_type / sizeof *create_type];
	void		*pack;

	param[0].vnode   = node;
	param[1].vlayer  = buffer;
	param[2].vstring = name;
	if((pack = verse_method_call_pack(sizeof create_type / sizeof *create_type, create_type, param)) != NULL)
	{
		verse_send_o_method_call(client_info.avatar, client_info.gid_control, graph_info.mid_create, 0, pack);
	}
}

void graph_method_send_call_destroy(uint32 id)
{
	VNOParam	param[sizeof destroy_type / sizeof *destroy_type];
	void		*pack;

	param[0].vuint32 = id;
	if((pack = verse_method_call_pack(sizeof param / sizeof *param, destroy_type, param)) != NULL)
	{
		verse_send_o_method_call(client_info.avatar, client_info.gid_control, graph_info.mid_destroy, 0, pack);
	}
}

void graph_method_receive_call(uint8 id, const void *param)
{
	VNOParam	arg[sizeof create_type / sizeof *create_type];

	if(id == graph_info.mid_create)
	{
		if(verse_method_call_unpack(param, sizeof create_type / sizeof *create_type, create_type, arg))
			graph_create(arg[0].vnode, arg[1].vlayer, arg[2].vstring);
	}
	else if(id == graph_info.mid_rename)
		;
	else if(id == graph_info.mid_destroy)
		;
	else
		LOG_WARN(("Received call to unknown graph method %u", id));
}
