/*
 * 
*/

#include "verse.h"

#include "purple.h"

#include "dynarr.h"
#include "list.h"
#include "textbuf.h"

#include "nodedb.h"

/* ----------------------------------------------------------------------------------------- */

const char * p_node_name_get(const Node *node)
{
	return node != NULL ? node->name : NULL;
}

void p_node_name_set(PONode *node, const char *name)
{
	nodedb_rename(node, name);
}

/* ----------------------------------------------------------------------------------------- */

void p_node_o_link_set(PONode *node, const PONode *link, const char *label, uint32 target_id)
{
	if(node != NULL && link != NULL && label != NULL)
		nodedb_o_link_set((NodeObject *) node, ~0, link->id, label, target_id);
}

/* ----------------------------------------------------------------------------------------- */

PNGLayer * p_node_g_layer_lookup(PONode *node, const char *name)
{
	if(node == NULL || name == NULL)
		return NULL;
	if(node->type != V_NT_GEOMETRY)
		return NULL;
	return nodedb_g_layer_lookup((NodeGeometry *) node, name);
}

void p_node_g_vertex_set_xyz(PONode *node, PNGLayer *layer, uint32 index, real64 x, real64 y, real64 z)
{
	if(node == NULL || layer == NULL || node->type != V_NT_GEOMETRY)
		return;
	nodedb_g_vertex_set_xyz(node, layer, index, x, y, z);
}

void p_node_g_polygon_set_corner_uint32(PONode *node, PNGLayer *layer, uint32 index,  uint32 v0, uint32 v1, uint32 v2, uint32 v3)
{
	if(node == NULL || layer == NULL)
		return;
	nodedb_g_polygon_set_corner_uint32(node, layer, index, v0, v1, v2, v3);
}
