/*
 * 
*/

#include <stdio.h>
#include <string.h>

#include "verse.h"

#include "dynarr.h"
#include "dynstr.h"
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
	char	name[32];

	VNodeID	node;
	uint16	buffer;

	uint	xml_start, xml_length;
} Graph;

static const VNOParamType	create_type[]   = { VN_O_METHOD_PTYPE_NODE, VN_O_METHOD_PTYPE_LAYER,
							VN_O_METHOD_PTYPE_STRING },
				rename_type[]   = { VN_O_METHOD_PTYPE_UINT32, VN_O_METHOD_PTYPE_STRING },
				destroy_type[]  = { VN_O_METHOD_PTYPE_UINT32 };
static char			*create_name[]  = { "node", "buffer", "name" },
				*rename_name[]  = { "graph", "name" },
				*destroy_name[] = { "graph" };

static struct
{
	uint8	mid_create, mid_rename, mid_destroy;
	DynArr	*graphs;	/* Array holding graphs indexed on ID. */
	List	*free_ids;
	Hash	*graphs_name;	/* Graphs hashed on name. */
} graph_info = { 0 };

/* ----------------------------------------------------------------------------------------- */

void graph_init(void)
{
	graph_info.graphs = dynarr_new(sizeof (Graph), 8);
	graph_info.free_ids = NULL;
	graph_info.graphs_name = hash_new_string();
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

/* Build the XML description of a graph, for the index. */
static void graph_build_xml(uint32 id, const Graph *g, char *buf, size_t bufsize)
{
	snprintf(buf, bufsize,	" <graph id=\"%u\" name=\"%s\">\n"
				"  <at>\n"
				"   <node>%u</node>\n"
				"   <buffer>%u</buffer>\n"
				"  </at>\n"
				" </graph>\n", id, g->name, g->node, g->buffer);
}

static void graph_create(VNodeID node_id, uint16 buffer_id, const char *name)
{
	unsigned int	id, i, pos;
	Graph		g, *gg;
	char		xml[256];

	/* Make sure name is unique. */
	if(hash_lookup(graph_info.graphs_name, name) != NULL)
		return;	/* It wasn't. */

	stu_strncpy(g.name, sizeof g.name, name);
	g.node   = node_id;
	g.buffer = buffer_id;
	if(graph_info.free_ids != NULL)
	{
		id = (uint32) list_data(graph_info.free_ids);
		graph_info.free_ids = list_tail(graph_info.free_ids);
		dynarr_set(graph_info.graphs, id, &g);
	}
	else
		id = dynarr_append(graph_info.graphs, &g);

	for(i = 0, pos = client_info.graphs.start; i < id; i++)
	{
		gg = dynarr_index(graph_info.graphs, i);
		if(gg == NULL || gg->name[0] == '\0')
			continue;
		pos += gg->xml_length;
	}
	gg = dynarr_index(graph_info.graphs, id);
	gg->xml_start = pos;
	graph_build_xml(id, gg, xml, sizeof xml);
	gg->xml_length = strlen(xml);
	verse_send_t_text_set(client_info.meta, client_info.graphs.buffer, gg->xml_start, 0, xml);
	hash_insert(graph_info.graphs_name, gg->name, gg);
}

static void graph_rename(uint32 id, const char *name)
{
	Graph		*g;
	unsigned int	i, pos;
	char		xml[256];

	if((g = dynarr_index(graph_info.graphs, id)) == NULL || g->name[0] == '\0')
	{
		LOG_WARN(("Couldn't rename graph %u, not found", id));
		return;
	}
	if(strcmp(g->name, name) == 0)
		return;
	if(hash_lookup(graph_info.graphs_name, name) != NULL)
	{
		LOG_WARN(("Couldn't rename graph %u, \"%s\", into \"%s\"--name collision", id, g->name, name));
		return;
	}
	hash_remove(graph_info.graphs_name, g->name);
	stu_strncpy(g->name, sizeof g->name, name);
	hash_insert(graph_info.graphs_name, g->name, g);
	graph_build_xml(id, g, xml, sizeof xml);
	verse_send_t_text_set(client_info.meta, client_info.graphs.buffer, g->xml_start, g->xml_length, xml);
	g->xml_length = strlen(xml);

	for(i = 0, pos = client_info.graphs.start; i < dynarr_size(graph_info.graphs); i++)
	{
		if((g = dynarr_index(graph_info.graphs, i)) == NULL || g->name[0] == '\0')
			continue;
		g->xml_start = pos;
		pos += g->xml_length;
	}
}

static void graph_destroy(uint32 id)
{
	Graph		*g;
	unsigned int	i, pos;

	if((g = dynarr_index(graph_info.graphs, id)) == NULL || g->name[0] == '\0')
	{
		LOG_WARN(("Couldn't destroy graph %u, not found", id));
		return;
	}
	g->name[0] = '\0';
	graph_info.free_ids = list_prepend(graph_info.free_ids, (void *) id);
	hash_remove(graph_info.graphs_name, g->name);
	verse_send_t_text_set(client_info.meta, client_info.graphs.buffer, g->xml_start, g->xml_length, NULL);

	for(i = 0, pos = client_info.graphs.start; i < dynarr_size(graph_info.graphs); i++)
	{
		if((g = dynarr_index(graph_info.graphs, i)) == NULL || g->name[0] == '\0')
			continue;
		g->xml_start = pos;
		pos += g->xml_length;
	}
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
		verse_send_o_method_call(client_info.avatar, client_info.gid_control, graph_info.mid_create, 0, pack);
}

void graph_method_send_call_rename(uint32 id, const char *name)
{
	VNOParam	param[sizeof rename_type / sizeof *rename_type];
	void		*pack;

	param[0].vuint32 = id;
	param[1].vstring = name;
	if((pack = verse_method_call_pack(sizeof rename_type / sizeof *rename_type, rename_type, param)) != NULL)
		verse_send_o_method_call(client_info.avatar, client_info.gid_control, graph_info.mid_rename, 0, pack);
}

void graph_method_send_call_destroy(uint32 id)
{
	VNOParam	param[sizeof destroy_type / sizeof *destroy_type];
	void		*pack;

	param[0].vuint32 = id;
	if((pack = verse_method_call_pack(sizeof param / sizeof *param, destroy_type, param)) != NULL)
		verse_send_o_method_call(client_info.avatar, client_info.gid_control, graph_info.mid_destroy, 0, pack);
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
	{
		if(verse_method_call_unpack(param, sizeof rename_type / sizeof *rename_type, rename_type, arg))
			graph_rename(arg[0].vuint32, arg[1].vstring);
	}
	else if(id == graph_info.mid_destroy)
	{
		if(verse_method_call_unpack(param, sizeof destroy_type / sizeof *destroy_type, destroy_type, arg))
			graph_destroy(arg[0].vuint32);
	}
	else
		LOG_WARN(("Received call to unknown graph method %u", id));
}
