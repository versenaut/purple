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

VNodeType p_node_get_type(PINode *node)
{
	return nodedb_type_get(node);
}

const char * p_node_get_name(const Node *node)
{
	return node != NULL ? node->name : NULL;
}

void p_node_set_name(PONode *node, const char *name)
{
	nodedb_rename(node, name);
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
	for(put = group; *path != '/' && (size_t) (put - group) < (sizeof group - 1);)
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
	for(put = group; *path != '\0' && *path != '/' && (size_t) (put - group) < (sizeof group - 1);)
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
		return n;
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

unsigned int p_node_m_fragment_num(PINode *node)
{
	return nodedb_m_fragment_num((NodeMaterial *) node);
}

PNMFragment * p_node_fragment_nth(PINode *node, unsigned int n)
{
	return nodedb_m_fragment_nth((NodeMaterial *) node, n);
}

void p_node_m_fragment_iter(PINode *node, PIter *iter)
{
	iter_init_dynarr_enum_negative(iter, ((NodeMaterial *) node)->fragments, offsetof(NdbMFragment, type));
}

VNMFragmentType p_node_m_fragment_get_type(const PNMFragment *fragment)
{
	return fragment != NULL ? ((NdbMFragment *) fragment)->type : (VNMFragmentType) -1;
}

PNMFragment * p_node_m_fragment_create_color(PONode *node, real64 red, real64 green, real64 blue)
{
	return nodedb_m_fragment_create_color((NodeMaterial *) node, red, green, blue);
}

PNMFragment * p_node_m_fragment_create_light(PONode *node, VNMLightType type, real64 normal_falloff,
					     PINode *brdf, const char *brdf_red, const char *brdf_green, const char *brdf_blue)
{
	return nodedb_m_fragment_create_light((NodeMaterial *) node, type, normal_falloff, (Node *) brdf,
					      brdf_red, brdf_green, brdf_blue);
}

PNMFragment * p_node_m_fragment_create_reflection(PONode *node, real64 normal_falloff)
{
	return nodedb_m_fragment_create_reflection((NodeMaterial *) node, normal_falloff);
}

PNMFragment * p_node_m_fragment_create_transparency(PONode *node, real64 normal_falloff, real64 refraction_index)
{
	return nodedb_m_fragment_create_transparency((NodeMaterial *) node, normal_falloff, refraction_index);
}

PNMFragment * p_node_m_fragment_create_volume(PONode *node,  real64 diffusion, real64 col_r, real64 col_g, real64 col_b,
							const PNMFragment *color)
{
	return nodedb_m_fragment_create_volume((NodeMaterial *) node, diffusion, col_r, col_g, col_b, color);
}

PNMFragment * p_node_m_fragment_create_geometry(PONode *node,
						const char *layer_r, const char *layer_g, const char *layer_b)
{
	return nodedb_m_fragment_create_geometry((NodeMaterial *) node, layer_r, layer_g, layer_b);
}

PNMFragment * p_node_m_fragment_create_texture(PONode *node, PINode *bitmap,
					    const char *layer_r, const char *layer_g, const char *layer_b,
					    const PNMFragment *mapping)
{
	return nodedb_m_fragment_create_texture((NodeMaterial *) node, bitmap, layer_r, layer_g, layer_b, mapping);
}

PNMFragment * p_node_m_fragment_create_noise(PONode *node, VNMNoiseType type, const PNMFragment *mapping)
{
	return nodedb_m_fragment_create_noise((NodeMaterial *) node, type, mapping);
}

PNMFragment * p_node_m_fragment_create_blender(PONode *node, VNMBlendType type,
					       const PNMFragment *data_a, const PNMFragment *data_b, const PNMFragment *ctrl)
{
	return nodedb_m_fragment_create_blender((NodeMaterial *) node, type, data_a, data_b, ctrl);
}

PNMFragment * p_node_m_fragment_create_matrix(PONode *node, const real64 *matrix, const PNMFragment *data)
{
	return nodedb_m_fragment_create_matrix((NodeMaterial *) node, matrix, data);
}

PNMFragment * p_node_m_fragment_create_ramp(PONode *node, VNMRampType type, uint8 channel,
						      const PNMFragment *mapping, uint8 point_count,
						      const VNMRampPoint *ramp)
{
	return nodedb_m_fragment_create_ramp((NodeMaterial *) node, type, channel, mapping, point_count, ramp);
}

PNMFragment * p_node_m_fragment_create_animation(PONode *node, const char *label)
{
	return nodedb_m_fragment_create_animation((NodeMaterial *) node, label);
}

PNMFragment * p_node_m_fragment_create_alternative(PONode *node, const PNMFragment *alt_a, const PNMFragment *alt_b)
{
	return nodedb_m_fragment_create_alternative((NodeMaterial *) node, alt_a, alt_b);
}

PNMFragment * p_node_m_fragment_create_output(PONode *node, const char *label, const PNMFragment *front, const PNMFragment *back)
{
	return nodedb_m_fragment_create_output((NodeMaterial *) node, label, front, back);
}

/* ----------------------------------------------------------------------------------------- */

void p_node_b_set_dimensions(PONode *node, uint16 width, uint16 height, uint16 depth)
{
	if(node == NULL)
		return;
	nodedb_b_set_dimensions((NodeBitmap *) node, width, height, depth);
}

void p_node_b_get_dimensions(PINode *node, uint16 *width, uint16 *height, uint16 *depth)
{
	nodedb_b_get_dimensions((NodeBitmap *) node, width, height, depth);
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

void p_node_c_curve_iter(PINode *node, PIter *iter)
{
	if(node != NULL)
		iter_init_dynarr_string(iter, ((NodeCurve *) node)->curves, offsetof(NodeCurve, curves));
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
	PNCCurve	*c;

	if((c = nodedb_c_curve_find((NodeCurve *) node, name)) != NULL)
		return c;
	return nodedb_c_curve_create((NodeCurve *) node, ~0, name, dimensions);
}

void p_node_c_curve_destroy(PONode *node, PNCCurve *curve)
{
	nodedb_c_curve_destroy((NodeCurve *) node, curve);
}

size_t p_node_c_curve_key_num(const PNCCurve *curve)
{
	return nodedb_c_curve_key_num(curve);
}

PNCKey * p_node_c_curve_key_nth(const PNCCurve *curve, unsigned int n)
{
	return nodedb_c_curve_key_nth(curve, n);
}

real64 p_node_c_curve_key_get_pos(const PNCKey *key)
{
	return key != NULL ? ((NdbCKey *) key)->pos : 0.0;
}

real64 p_node_c_curve_key_get_value(const PNCKey *key, uint8 dimension)
{
	return key != NULL ? ((NdbCKey *) key)->value[dimension] : 0.0;
}

uint32 p_node_c_curve_key_get_pre(const PNCKey *key, uint8 dimension, real64 *value)
{
	if(key != NULL)
	{
		if(value != NULL)
			*value = ((NdbCKey *) key)->pre.value[dimension];
		return ((NdbCKey *) key)->pre.pos[dimension];
	}
	return 0;
}

uint32 p_node_c_curve_key_get_post(const PNCKey *key, uint8 dimension, real64 *value)
{
	if(key != NULL)
	{
		if(value != NULL)
			*value = ((NdbCKey *) key)->post.value[dimension];
		return ((NdbCKey *) key)->post.pos[dimension];
	}
	return 0;
}

PNCKey * p_node_c_curve_key_create(PNCCurve *curve,
				   real64 pos, const real64 *value,
				   const uint32 *pre_pos, const real64 *pre_value,
				   const uint32 *post_pos, const real64 *post_value)
{
	return (PNCKey *) nodedb_c_key_create(curve, ~0, pos, value, pre_pos, pre_value, post_pos, post_value);
}

void p_node_curve_key_destroy(PNCCurve *curve, PNCKey *key)
{
	nodedb_c_key_destroy((NdbCCurve *) curve, (NdbCKey *) key);
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

/* ----------------------------------------------------------------------------------------- */

unsigned int p_node_a_buffer_get_num(PINode *node)
{
	return nodedb_a_buffer_num((NodeAudio *) node);
}

PNABuffer * p_node_a_buffer_nth(PINode *node, unsigned int n)
{
	return nodedb_a_buffer_nth((NodeAudio *) node, n);
}

PNABuffer * p_node_a_buffer_find(PINode *node, const char *name)
{
	return nodedb_a_buffer_find((NodeAudio *) node, name);
}

const char * p_node_a_buffer_get_name(const PNABuffer *layer)
{
	if(layer != NULL)
		return ((NdbABuffer *) layer)->name;
	return NULL;
}

real64 p_node_a_buffer_get_frequency(const PNABuffer *layer)
{
	return layer != NULL ? ((NdbABuffer *) layer)->frequency : 0.0;
}

PNABuffer * p_node_a_buffer_create(PONode *node, const char *name, VNABlockType type, real64 frequency)
{
	PNABuffer	*l;

	if(node == NULL)
		return NULL;
	if((l = nodedb_a_buffer_find((NodeAudio *) node, name)) != NULL)
		return l;
	return nodedb_a_buffer_create((NodeAudio *) node, ~0, name, type, frequency);
}

unsigned int p_node_a_buffer_read_samples(const PNABuffer *buffer, unsigned int start, real64 *samples, unsigned int length)
{
	return nodedb_a_buffer_read_samples((NdbABuffer *) buffer, start, samples, length);
}

void p_node_a_buffer_write_samples(PNABuffer *buffer, unsigned int start, const real64 *samples, unsigned int length)
{
	nodedb_a_buffer_write_samples((NdbABuffer *) buffer, start, samples, length);
}
