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
#include "iter.h"
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

void p_node_tag_group_iter(PINode *node, PIter *iter)
{
	iter_init_dynarr_string(iter, ((Node *) node)->tag_groups, offsetof(NdbTagGroup, name));
}

const char * p_node_tag_group_get_name(const PNTagGroup *group)
{
	if(group != NULL)
		return ((NdbTagGroup *) group)->name;
	return NULL;
}

PNTagGroup * p_node_tag_group_create(PONode *node, const char *name)
{
	return nodedb_tag_group_create(node, ~0, name);
}

unsigned int p_node_tag_group_tag_num(const PNTagGroup *group)
{
	return nodedb_tag_group_tag_num((NdbTagGroup *) group);
}

PNTag * p_node_tag_group_tag_nth(const PNTagGroup *group, unsigned int n)
{
	return nodedb_tag_group_tag_nth((NdbTagGroup *) group, n);
}

void p_node_tag_group_tag_iter(const PNTagGroup *group, PIter *iter)
{
	if(group == NULL)
		return;
	iter_init_dynarr_string(iter, ((NdbTagGroup *) group)->tags, offsetof(NdbTag, name));
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
		if((tag = nodedb_tag_group_tag_find(tg, path)) != NULL)
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
			nodedb_tag_destroy(g, nodedb_tag_group_tag_find(g, path));
	}
}

const char * p_node_tag_get_name(const PNTag *tag)
{
	return nodedb_tag_get_name(tag);
}

VNTagType p_node_tag_get_type(const PNTag *tag)
{
	if(tag != NULL)
		return ((NdbTag *) tag)->type;
	return -1;
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

unsigned int p_node_g_layer_num(PINode *node)
{
	return nodedb_g_layer_num((NodeGeometry *) node);
}

PNGLayer * p_node_g_layer_nth(PINode *node, unsigned int n)
{
	return nodedb_g_layer_nth((NodeGeometry *) node, n);
}

PNGLayer * p_node_g_layer_find(PINode *node, const char *name)
{
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

VNGLayerType p_node_g_layer_get_type(const PNGLayer *layer)
{
	if(layer != NULL)
		return ((NdbGLayer *) layer)->type;
	return -1;
}

void p_node_g_vertex_set_xyz(PNGLayer *layer, uint32 id, real64 x, real64 y, real64 z)
{
	nodedb_g_vertex_set_xyz(layer, id, x, y, z);
}

void p_node_g_vertex_get_xyz(const PNGLayer *layer, uint32 id, real64 *x, real64 *y, real64 *z)
{
	nodedb_g_vertex_get_xyz(layer, id, x, y, z);
}

void p_node_g_polygon_set_corner_uint32(PNGLayer *layer, uint32 id,  uint32 v0, uint32 v1, uint32 v2, uint32 v3)
{
	nodedb_g_polygon_set_corner_uint32(layer, id, v0, v1, v2, v3);
}

void p_node_g_polygon_get_corner_uint32(const PNGLayer *layer, uint32 id, uint32 *v0, uint32 *v1, uint32 *v2, uint32 *v3)
{
	nodedb_g_polygon_get_corner_uint32(layer, id, v0, v1, v2, v3);
}

void p_node_g_polygon_set_corner_real32(PNGLayer *layer, uint32 id, real32 v0, real32 v1, real32 v2, real32 v3)
{
	nodedb_g_polygon_set_corner_real32(layer, id, v0, v1, v2, v3);
}

void p_node_g_polygon_get_corner_real32(const PNGLayer *layer, uint32 id, real32 *v0, real32 *v1, real32 *v2, real32 *v3)
{
	nodedb_g_polygon_get_corner_real32(layer, id, v0, v1, v2, v3);
}

void p_node_g_polygon_set_corner_real64(PNGLayer *layer, uint32 id, real64 v0, real64 v1, real64 v2, real64 v3)
{
	nodedb_g_polygon_set_corner_real64(layer, id, v0, v1, v2, v3);
}

void p_node_g_polygon_get_corner_real64(const PNGLayer *layer, uint32 id, real64 *v0, real64 *v1, real64 *v2, real64 *v3)
{
	nodedb_g_polygon_get_corner_real64(layer, id, v0, v1, v2, v3);
}

void p_node_g_polygon_set_face_uint8(PNGLayer *layer, uint32 id, uint8 value)
{
	nodedb_g_polygon_set_face_uint8(layer, id, value);
}

uint8 p_node_g_polygon_get_face_uint8(const PNGLayer *layer, uint32 id)
{
	return nodedb_g_polygon_get_face_uint8(layer, id);
}

void p_node_g_polygon_set_face_uint32(PNGLayer *layer, uint32 id, uint32 value)
{
	nodedb_g_polygon_set_face_uint32(layer, id, value);
}

uint32 p_node_g_polygon_get_face_uint32(const PNGLayer *layer, uint32 id)
{
	return nodedb_g_polygon_get_face_uint32(layer, id);
}

void p_node_g_polygon_set_face_real64(PNGLayer *layer, uint32 id, real64 value)
{
	nodedb_g_polygon_set_face_real64(layer, id, value);
}

real64 p_node_g_polygon_get_face_real64(const PNGLayer *layer, uint32 id)
{
	return nodedb_g_polygon_get_face_real64(layer, id);
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

const char * p_node_b_layer_get_name(const PNBLayer *layer)
{
	if(layer != NULL)
		return ((NdbBLayer *) layer)->name;
	return NULL;
}

VNBLayerType p_node_b_layer_get_type(const PNBLayer *layer)
{
	if(layer != NULL)
		return ((NdbBLayer *) layer)->type;
	return -1;
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

unsigned int p_node_c_curve_num(PINode *node)
{
	return nodedb_c_curve_num((NodeCurve *) node);
}

PNCCurve * p_node_c_curve_nth(PINode *node, unsigned int n)
{
	return nodedb_c_curve_nth((NodeCurve *) node, n);
}

PNCCurve * p_node_c_curve_find(PINode *node, const char *name)
{
	return nodedb_c_curve_find((NodeCurve *) node, name);
}

const char * p_node_c_curve_get_name(const PNCCurve *curve)
{
	if(curve != NULL)
		return ((NdbCCurve *) curve)->name;
	return NULL;
}

uint8 p_node_c_curve_get_dimensions(const PNCCurve *curve)
{
	if(curve != NULL)
		return ((NdbCCurve *) curve)->dimensions;
	return 0;
}

PNCCurve * p_node_c_curve_create(PONode *node, const char *name, uint8 dimensions)
{
	return nodedb_c_curve_create((NodeCurve *) node, ~0, name, dimensions);
}

size_t p_node_c_curve_key_num(const PNCCurve *curve)
{
	return nodedb_c_curve_key_num(curve);
}

PNCKey * p_node_c_curve_key_nth(const PNCCurve *curve, unsigned int n)
{
	return nodedb_c_curve_key_nth(curve, n);
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

unsigned int p_node_t_buffer_get_num(PINode *node)
{
	return nodedb_t_buffer_num((NodeText *) node);
}

PNTBuffer * p_node_t_buffer_nth(PINode *node, unsigned int n)
{
	return nodedb_t_buffer_nth((NodeText *) node, n);
}

PNTBuffer * p_node_t_buffer_find(PINode *node, const char *name)
{
	return nodedb_t_buffer_find((NodeText *) node, name);
}

const char * p_node_t_buffer_get_name(const PNTBuffer *buffer)
{
	if(buffer != NULL)
		return ((NdbTBuffer *) buffer)->name;
	return NULL;
}

PNTBuffer * p_node_t_buffer_create(PONode *node, const char *name)
{
	PNTBuffer	*b;

	if((b = nodedb_t_buffer_find((NodeText *) node, name)) != NULL)
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
