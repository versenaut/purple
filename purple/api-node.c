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
#include "strutil.h"
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

unsigned int p_node_tag_group_num(PINode *node)
{
	return nodedb_tag_group_num(node);
}

PNTagGroup * p_node_tag_group_nth(PINode *node, unsigned int n)
{
	return nodedb_tag_group_nth(node, n);
}

PNTagGroup * p_node_tag_group_find(PINode *node, const char *name)
{
	return nodedb_tag_group_find(node, name);
}

PNTagGroup * p_node_tag_group_create(PONode *node, const char *name)
{
	return nodedb_tag_group_create(node, ~0, name);
}

void p_node_tag_create(PNTagGroup *group, const char *name, VNTagType type, const VNTag *value)
{
	nodedb_tag_create(group, ~0, name, type, value);
}

/* Convenience routine to lookup a group, and set a tag all in one call. Uses group/tag syntax. */
void p_node_tag_create_path(PONode *node, const char *path, VNTagType type, ...)
{
	char	group[32], *put;

	if(node == NULL || path == NULL || *path == '\0')
		return;
	for(put = group; *path != '/' && (put - group) < (sizeof group - 1);)
		*put++ = *path++;
	*put = '\0';
	if(*path == '/')
	{
		NdbTagGroup	*tg;
		NdbTag		*tag;
		VNTag		value;
		va_list		arg;

		if((tg = nodedb_tag_group_find(node, group)) == NULL)
			tg = nodedb_tag_group_create(node, ~0, group);
		path++;
		va_start(arg, type);
		switch(type)
		{
		case VN_TAG_BOOLEAN:
			value.vboolean = va_arg(arg, int);
			break;
		case VN_TAG_UINT32:
			value.vuint32 = va_arg(arg, uint32);
			break;
		case VN_TAG_REAL64:
			value.vreal64 = va_arg(arg, real64);
			break;
		case VN_TAG_STRING:
			value.vstring = va_arg(arg, char *);	/* Duplicated by nodedb. */
			break;
		case VN_TAG_REAL64_VEC3:
			value.vreal64_vec3[0] = va_arg(arg, real64);
			value.vreal64_vec3[1] = va_arg(arg, real64);
			value.vreal64_vec3[2] = va_arg(arg, real64);
			break;
		case VN_TAG_LINK:
			value.vlink = va_arg(arg, VNodeID);
			break;
		case VN_TAG_ANIMATION:
			value.vanimation.curve = va_arg(arg, VNodeID);
			value.vanimation.start = va_arg(arg, uint32);
			value.vanimation.end   = va_arg(arg, uint32);
			break;
		case VN_TAG_BLOB:
			value.vblob.size = va_arg(arg, unsigned int);
			value.vblob.blob = va_arg(arg, void *);
			break;
		case VN_TAG_TYPE_COUNT:
			;
		}
		va_end(arg);
		/* If tag exists, just (re)set the value, else create it. */
		if((tag = nodedb_tag_lookup(tg, path)) != NULL)
			nodedb_tag_value_set(tag, type, &value);
		else
			nodedb_tag_create(tg, ~0, path, type, &value);
	}
}

void p_node_tag_destroy_path(PONode *node, const char *path)
{
	char		group[32], *put;
	NdbTagGroup	*g;

	if(node == NULL || path == NULL || *path == '\0')
		return;
	for(put = group; *path != '\0' && *path != '/' && (put - group) < (sizeof group - 1);)
		*put++ = *path++;
	*put = '\0';
	if(*path == '/')
		path++;
	if((g = nodedb_tag_group_find(node, group)) != NULL)
	{
		if(*path == '\0')		/* If no second part, destroy group. */
			nodedb_tag_group_destroy(g);
		else if(*path == '*')		/* If asterisk, destroy all tags but leave group. */
			nodedb_tag_destroy_all(g);
		else				/* Else destroy named tag only. */
			nodedb_tag_destroy(g, nodedb_tag_lookup(g, path));
	}
}

/* ----------------------------------------------------------------------------------------- */

void p_node_o_light_set(PONode *node, real64 red, real64 green, real64 blue)
{
	nodedb_o_light_set((NodeObject *) node, red, green, blue);
}

void p_node_o_light_get(PINode *node, real64 *red, real64 *green, real64 *blue)
{
	nodedb_o_light_get((NodeObject *) node, red, green, blue);
}

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
	return nodedb_g_layer_find((NodeGeometry *) node, name);
}

size_t p_node_g_layer_get_size(const PNGLayer *layer)
{
	return nodedb_g_layer_get_size(layer);
}

const char * p_node_g_layer_get_name(const PNGLayer *layer)
{
	return nodedb_g_layer_get_name(layer);
}

void p_node_g_vertex_set_xyz(PNGLayer *layer, uint32 id, real64 x, real64 y, real64 z)
{
	nodedb_g_vertex_set_xyz(layer, id, x, y, z);
}

void p_node_g_vertex_get_xyz(const PNGLayer *layer, uint32 id, real64 *x, real64 *y, real64 *z)
{
	nodedb_g_vertex_get_xyz(layer, id, x, y, z);
}

void p_node_g_polygon_set_corner_uint32(PNGLayer *layer, uint32 index,  uint32 v0, uint32 v1, uint32 v2, uint32 v3)
{
	nodedb_g_polygon_set_corner_uint32(layer, index, v0, v1, v2, v3);
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

unsigned int p_node_b_layer_num(PINode *node)
{
	return nodedb_b_layer_num((NodeBitmap *) node);
}

PNBLayer * p_node_b_layer_nth(PINode *node, unsigned int n)
{
	return nodedb_b_layer_nth((NodeBitmap *) node, n);
}

PNBLayer * p_node_b_layer_find(PINode *node, const char *name)
{
	return nodedb_b_layer_find((NodeBitmap *) node, name);
}

PNBLayer * p_node_b_layer_create(PONode *node, const char *name, VNBLayerType type)
{
	PNBLayer	*l;

	if(node == NULL)
		return NULL;
	if((l = nodedb_b_layer_find((NodeBitmap *) node, name)) != NULL)
		return l;
	return nodedb_b_layer_create((NodeBitmap *) node, ~0, name, type);
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

void p_node_t_langauge_set(PONode *node, const char *language)
{
	nodedb_t_language_set((NodeText *) node, language);
}

size_t p_node_t_buffer_get_count(PINode *node)
{
	return nodedb_t_buffer_get_count((NodeText *) node);
}

PNTBuffer * p_node_t_buffer_create(PONode *node, const char *name)
{
	PNTBuffer	*b;

	if((b = nodedb_t_buffer_get_named((NodeText *) node, name)) != NULL)
	{
		nodedb_t_buffer_clear(b);
		return b;
	}
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

char * p_node_t_buffer_read_line(PNTBuffer *buffer, unsigned int line, char *put, size_t putmax)
{
	return nodedb_t_buffer_read_line(buffer, line, put, putmax);
}
