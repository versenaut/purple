/*
 * api-node.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * 
*/

#include <stdarg.h>

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

VNodeType p_node_type_get(PINode *node)
{
	return nodedb_type_get(node);
}

/* ----------------------------------------------------------------------------------------- */

void p_node_o_link_set(PONode *node, const PONode *link, const char *label, uint32 target_id)
{
	if(node != NULL && link != NULL && label != NULL)
		nodedb_o_link_set_local((NodeObject *) node, link, label, target_id);
}

PINode * p_node_o_link_get(const PONode *node, const char *label, uint32 target_id)
{
	if(node != NULL && label != NULL)
	{
		PINode	*n = nodedb_o_link_get((NodeObject *) node, label, target_id);

		if(n == NULL)
			return (PINode *) nodedb_o_link_get_local((NodeObject *) node, label, target_id);
	}
	return NULL;
}

/* ----------------------------------------------------------------------------------------- */

PNGLayer * p_node_g_layer_lookup(PINode *node, const char *name)
{
	if(node == NULL || name == NULL)
		return NULL;
	if(node->type != V_NT_GEOMETRY)
		return NULL;
	return nodedb_g_layer_lookup((NodeGeometry *) node, name);
}

size_t p_node_g_layer_size(PINode *node, const PNGLayer *layer)
{
	if(node == NULL || layer == NULL)
		return 0;
	return nodedb_g_layer_size((const NodeGeometry *) node, layer);
}

void p_node_g_vertex_set_xyz(PONode *node, PNGLayer *layer, uint32 id, real64 x, real64 y, real64 z)
{
	if(node == NULL || layer == NULL || node->type != V_NT_GEOMETRY)
		return;
	nodedb_g_vertex_set_xyz((NodeGeometry *) node, layer, id, x, y, z);
}

void p_node_g_vertex_get_xyz(const PONode *node, const PNGLayer *layer, uint32 id, real64 *x, real64 *y, real64 *z)
{
	if(node == NULL || layer == NULL || node->type != V_NT_GEOMETRY)
		return;
	nodedb_g_vertex_get_xyz((const NodeGeometry *) node, layer, id, x, y, z);
}

void p_node_g_polygon_set_corner_uint32(PONode *node, PNGLayer *layer, uint32 index,  uint32 v0, uint32 v1, uint32 v2, uint32 v3)
{
	if(node == NULL || layer == NULL)
		return;
	nodedb_g_polygon_set_corner_uint32((NodeGeometry *) node, layer, index, v0, v1, v2, v3);
}

void p_node_g_crease_set_vertex(PONode *node, const char *layer, uint32 def)
{
	if(node == NULL || layer == NULL)
		return;
	nodedb_g_crease_set_vertex((NodeGeometry *) node, layer, def);
}

void p_node_g_crease_set_edge(PONode *node, const char *layer, uint32 def)
{
	if(node == NULL || layer == NULL)
		return;
	nodedb_g_crease_set_edge((NodeGeometry *) node, layer, def);
}

/* ----------------------------------------------------------------------------------------- */

void p_node_b_dimensions_set(PONode *node, uint16 width, uint16 height, uint16 depth)
{
	if(node == NULL)
		return;
	nodedb_b_dimensions_set((NodeBitmap *) node, width, height, depth);
}

void p_node_b_dimensions_get(PINode *node, uint16 *width, uint16 *height, uint16 *depth)
{
	nodedb_b_dimensions_get((NodeBitmap *) node, width, height, depth);
}

PNBLayer * p_node_b_layer_create(PONode *node, const char *name, VNBLayerType type)
{
	PNBLayer	*l;

	if(node == NULL)
		return NULL;
	if((l = nodedb_b_layer_lookup((NodeBitmap *) node, name)) != NULL)
		return l;
	return nodedb_b_layer_create((NodeBitmap *) node, ~0, name, type);
}

PNBLayer * p_node_b_layer_lookup(PINode *node, const char *name)
{
	return nodedb_b_layer_lookup((NodeBitmap *) node, name);
}

void * p_node_b_layer_access_begin(PONode *node, PNBLayer *layer)
{
	return nodedb_b_layer_access_begin((NodeBitmap *) node, layer);
}

void p_node_b_layer_access_end(PONode *node, PNBLayer *layer, void *framebuffer)
{
	nodedb_b_layer_access_end((NodeBitmap *) node, layer, framebuffer);
}

void p_node_b_layer_foreach_set(PONode *node, PNBLayer *layer,
				real64 (*pixel)(uint32 x, uint32 y, uint32 z, void *user), void *user)
{
	nodedb_b_layer_foreach_set((NodeBitmap *) node, layer, pixel, user);
}

void * p_node_b_layer_access_multi_begin(PONode *node, VNBLayerType format, ...)
{
	va_list	layers;
	void	*fb;

	va_start(layers, format);
	fb = nodedb_b_layer_access_multi_begin((NodeBitmap *) node, format, layers);
	va_end(layers);
	return fb;
}

void p_node_b_layer_access_multi_end(PONode *node, void *framebuffer)
{
	nodedb_b_layer_access_multi_end((NodeBitmap *) node, framebuffer);
}

/* ----------------------------------------------------------------------------------------- */

PNCCurve * p_node_c_curve_create(PONode *node, const char *name, uint8 dimensions)
{
	return nodedb_c_curve_create((NodeCurve *) node, ~0, name, dimensions);
}

PNCCurve * p_node_c_curve_lookup(PINode *node, const char *name)
{
	return nodedb_c_curve_lookup((NodeCurve *) node, name);
}

uint8 p_node_c_curve_dimensions_get(const PNCCurve *curve)
{
	return nodedb_c_curve_dimensions_get(curve);
}

size_t p_node_c_curve_key_get_count(const PNCCurve *curve)
{
	return nodedb_c_curve_key_get_count(curve);
}

PNCKey * p_node_c_curve_key_get_nth(const PNCCurve *curve, unsigned int n)
{
	return nodedb_c_curve_key_get_nth(curve, n);
}

/* ----------------------------------------------------------------------------------------- */

const char * p_node_t_language_get(PINode *node)
{
	return nodedb_t_language_get((NodeText *) node);
}

size_t p_node_t_buffer_get_count(PINode *node)
{
	return nodedb_t_buffer_get_count((NodeText *) node);
}

PNTBuffer * p_node_t_buffer_create(PONode *node, const char *name)
{
	return nodedb_t_buffer_create((NodeText *) node, ~0, name);
}

void p_node_t_buffer_insert(PNTBuffer *buffer, size_t pos, const char *text)
{
	return nodedb_t_buffer_insert(buffer, pos, text);
}

void p_node_t_buffer_delete(PNTBuffer *buffer, size_t pos, size_t length)
{
	return nodedb_t_buffer_delete(buffer, pos, length);
}

void p_node_t_buffer_append(PNTBuffer *buffer, const char *text)
{
	return nodedb_t_buffer_append(buffer, text);
}
