/*
 * purple.h
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Header file for the "Purple API", i.e. the API to use when writing
 * plug-ins for the Purple system.
*/

#if !defined PURPLE_H
#define	PURPLE_H

#include "verse.h"

typedef struct Node	Node;
typedef const Node	PINode;
typedef Node		PONode;

typedef enum
{
	P_VALUE_NONE = -1,
	P_VALUE_BOOLEAN = 0,
	P_VALUE_INT32,
	P_VALUE_UINT32,
	P_VALUE_REAL32,
	P_VALUE_REAL32_VEC2,
	P_VALUE_REAL32_VEC3,
	P_VALUE_REAL32_VEC4,
	P_VALUE_REAL32_MAT16,
	P_VALUE_REAL64,
	P_VALUE_REAL64_VEC2,
	P_VALUE_REAL64_VEC3,
	P_VALUE_REAL64_VEC4,
	P_VALUE_REAL64_MAT16,
	P_VALUE_STRING,
	P_VALUE_MODULE
} PValueType;

/* These are Purple-internal, you will know them only as pointers. */
typedef struct PPort	*PPInput;	/* An input is a pointer to a port. */
typedef struct PPort	*PPOutput;	/* So is an output. Simplicity. */

typedef enum
{
	P_INPUT_TAG_DONE = 0,
	P_INPUT_TAG_REQUIRED,
	P_INPUT_TAG_MIN = 16,
	P_INPUT_TAG_MAX,
	P_INPUT_TAG_DEFAULT,
} PInputTag;

/* Idea: use macros to make tag lists a bit safer, and pass all min/max/default values as doubles. */
#define	P_INPUT_DONE		P_INPUT_TAG_DONE
#define	P_INPUT_MIN(v)		P_INPUT_TAG_MIN, (double) v
#define	P_INPUT_MAX(v) 		P_INPUT_TAG_MAX, (double) v
#define	P_INPUT_DEFAULT(v)	P_INPUT_TAG_DEFAULT, (double) v
#define	P_INPUT_DEFAULT_STR(v)	P_INPUT_TAG_DEFAULT, (const char *) v
#define	P_INPUT_REQUIRED	P_INPUT_TAG_REQUIRED

void		p_init_create(const char *name);

void		p_init_input(int index, PValueType type, const char *name, ...);

void		p_init_meta(const char *category, const char *text);

/* Each instance can have its own unique state, passed to compute(). Opaque, specify size in bytes.
 * The constructor() and destructor() functions will be called when plug-in are instantiated/de-inst:ed.
 * If the constructor is NULL, the state will be initialized by Purple to "all bits zero".
 */
void		p_init_state(size_t size,
			     void (*constructor)(void *state),
			     void (*destructor)(void *state));

typedef enum
{
	P_COMPUTE_DONE = 0,
	P_COMPUTE_AGAIN
} PComputeStatus;

void		p_init_compute(PComputeStatus (*compute)(PPInput *input, PPOutput output, void *state));

/* Read out inputs, registered earlier. One for each type. :/ If this wasn't in C, we could use meta
 * information to just say p_input(input) and have it return a value of the proper registered type.
*/
boolean		p_input_boolean(PPInput input);
int32		p_input_int32(PPInput input);
uint32		p_input_uint32(PPInput input);
real32		p_input_real32(PPInput input);
const real32 *	p_input_real32_vec2(PPInput input);
const real32 *	p_input_real32_vec3(PPInput input);
const real32 *	p_input_real32_vec4(PPInput input);
const real32 *	p_input_real32_mat16(PPInput input);
real64		p_input_real64(PPInput input);
const real64 *	p_input_real64_vec2(PPInput input);
const real64 *	p_input_real64_vec3(PPInput input);
const real64 *	p_input_real64_vec4(PPInput input);
const real64 *	p_input_real64_mat16(PPInput input);
const char *	p_input_string(PPInput input);
PINode *	p_input_node(PPInput input);			/* Inputs the "first" node, somehow. */
PINode *	p_input_node_nth(PPInput input, int index);	/* Input n:th node, or NULL. */

/* General-purpose iterator structure. All fields private, this is
 * public only to support automatic (on-stack) allocations.
 * Use like this:
 * 
 * PIter iter;
 *
 * for(p_whatever_iter(whatever, &iter); (el = p_iter_get(&iter)) != NULL; p_iter_next(&iter))
 * 	{ process element <el> here }
*/
typedef struct
{
	unsigned short	flags;
	unsigned short	offset;
	unsigned int	index;
	union
	{
	struct
	{
	void		*arr;
	unsigned int	index;
	}		dynarr;
	void		*list;
	}		data;
} PIter;

/* Dereference iterator, returns next element or NULL if end of sequence. */
void *		p_iter_data(PIter *iter);
/* Returns index of current element, counting from zero and up. */
unsigned int	p_iter_index(const PIter *iter);
/* Advance the iterator to point at the next element (if there is one). */
void		p_iter_next(PIter *iter);


/* Node manipulation functions. Getters work on both input and output
 * nodes, setting requires output.
*/
VNodeType	p_node_type_get(const Node *node);

const char *	p_node_name_get(const Node *node);
void		p_node_name_set(PONode *node, const char *name);

/* Tag functions. */
typedef void	PNTagGroup, PNTag;

unsigned int	p_node_tag_group_num(PINode *node);
PNTagGroup *	p_node_tag_group_nth(PINode *node, unsigned int n);
PNTagGroup *	p_node_tag_group_find(PINode *node, const char *name);
void		p_node_tag_group_iter(PINode *node, PIter *iter);
const char *	p_node_tag_group_get_name(const PNTagGroup *group);

PNTagGroup *	p_node_tag_group_create(PONode *node, const char *name);
void		p_node_tag_group_destroy(PONode *node, PNTagGroup *group);
unsigned int	p_node_tag_group_tag_num(const PNTagGroup *group);
PNTag *		p_node_tag_group_tag_nth(const PNTagGroup *group, unsigned int n);
PNTag *		p_node_tag_group_tag_find(const PNTagGroup *group, const char *name);
void		p_node_tag_group_tag_iter(const PNTagGroup *group, PIter *iter);
void		p_node_tag_group_tag_create(PNTagGroup *group, const char *name, VNTagType type, const VNTag *value);
void		p_node_tag_group_tag_destroy(PNTagGroup *group, const char *name);
const char *	p_node_tag_get_name(const PNTag *tag);
VNTagType	p_node_tag_get_type(const PNTag *tag);

/* Set from a "group/tag"-style path, with vararg for scalar values. */
void		p_node_tag_create_path(PONode *node, const char *path, VNTagType type, ...);
void		p_node_tag_destroy_path(PONode *node, const char *path);

void		p_node_o_light_set(PONode *node, real64 red, real64 green, real64 blue);
void		p_node_o_light_get(PINode *node, real64 *red, real64 *green, real64 *blue);

void		p_node_o_link_set(PONode *node, const PONode *link, const char *label, uint32 target_id);
PINode *	p_node_o_link_get(const PONode *node, const char *label, uint32 target_id);

/* Geometry-node manipulation functions. */
typedef void	PNGLayer;
typedef void	PNGBone;

unsigned int	p_node_g_layer_num(PINode *node);
PNGLayer *	p_node_g_layer_nth(PINode *node, unsigned int n);
PNGLayer *	p_node_g_layer_find(PINode *node, const char *name);
size_t		p_node_g_layer_get_size(const PNGLayer *layer);
const char *	p_node_g_layer_get_name(const PNGLayer *layer);
VNGLayerType	p_node_g_layer_get_type(const PNGLayer *layer);

PNGLayer *	p_node_g_layer_create(PONode *node, const char *name, VNGLayerType type,
					     uint32 def_int, real32 def_real);
void		p_node_g_layer_destroy(PNGLayer *node, const char *name);
void		p_node_g_vertex_set_xyz(PNGLayer *layer, uint32 id, real64 x, real64 y, real64 z);
void		p_node_g_vertex_get_xyz(const PNGLayer *layer, uint32 id, real64 *x, real64 *y, real64 *z);
void		p_node_g_polygon_set_corner_uint32(PNGLayer *layer, uint32 id, uint32 v0, uint32 v1, uint32 v2, uint32 v3);
void		p_node_g_polygon_get_corner_uint32(const PNGLayer *layer, uint32 id, uint32 *v0, uint32 *v1, uint32 *v2, uint32 *v3);
void		p_node_g_polygon_set_corner_real32(PNGLayer *layer, uint32 id, real32 v0, real32 v1, real32 v2, real32 v3);
void		p_node_g_polygon_get_corner_real32(const PNGLayer *layer, uint32 id, real32 *v0, real32 *v1, real32 *v2, real32 *v3);
void		p_node_g_polygon_set_corner_real64(PNGLayer *layer, uint32 id, real64 v0, real64 v1, real64 v2, real64 v3);
void		p_node_g_polygon_get_corner_real64(const PNGLayer *layer, uint32 id, real64 *v0, real64 *v1, real64 *v2, real64 *v3);

void		p_node_g_polygon_set_face_uint8(PNGLayer *layer, uint32 id, uint8 value);
uint8		p_node_g_polygon_get_face_uint8(const PNGLayer *layer, uint32 id);
void		p_node_g_polygon_set_face_uint32(PNGLayer *layer, uint32 id, uint32 value);
uint32		p_node_g_polygon_get_face_uint32(const PNGLayer *layer, uint32 id);
void		p_node_g_polygon_set_face_real64(PNGLayer *layer, uint32 id, real64 value);
real64		p_node_g_polygon_get_face_real64(const PNGLayer *layer, uint32 id);

void		p_node_g_crease_set_vertex(PONode *node, const char *layer, uint32 def);
void		p_node_g_crease_set_edge(PONode *node, const char *layer, uint32 def);


/* Bitmap-node manipulation functions. */
typedef void	PNBLayer;

void		p_node_b_dimensions_set(PONode *node, uint16 width, uint16 height, uint16 depth);
void		p_node_b_dimensions_get(PINode *node, uint16 *width, uint16 *height, uint16 *depth);
unsigned int	p_node_b_layer_num(PINode *node);
PNBLayer *	p_node_b_layer_nth(PINode *node, unsigned int n);
PNBLayer *	p_node_b_layer_find(PINode *node, const char *name);
const char *	p_node_b_layer_get_name(const PNBLayer *layer);
VNBLayerType	p_node_b_layer_get_type(const PNBLayer *layer);

PNBLayer *	p_node_b_layer_create(PONode *node, const char *name, VNBLayerType type);
void *		p_node_b_layer_access_begin(PONode *node, PNBLayer *layer);
void		p_node_b_layer_access_end(PONode *node, PNBLayer *layer, void *framebuffer);
void *		p_node_b_layer_access_multi_begin(PONode *node, VNBLayerType format, ... /* Layer names ending with NULL. */);
void		p_node_b_layer_access_multi_end(PONode *node, void *framebuffer);
/* Simple write-only set-function, that should return pixel for (x,y,z). Can't read; not suitable for filtering. */
void		p_node_b_layer_foreach_set(PONode *node, PNBLayer *layer,
					   real64 (*pixel)(uint32 x, uint32 y, uint32 z, void *user), void *user);
void		p_node_b_layer_destroy(PONode *node, PNBLayer *layer);


/* Curve-node manipulation functions. */
typedef void	PNCCurve, PNCKey;

unsigned int	p_node_c_curve_num(PINode *node);
PNCCurve *	p_node_c_curve_nth(PINode *node, unsigned int n);
PNCCurve *	p_node_c_curve_find(PINode *node, const char *name);
const char *	p_node_c_curve_get_name(const PNCCurve *curve);
uint8		p_node_c_curve_get_dimensions(const PNCCurve *curve);

PNCCurve *	p_node_c_curve_create(PONode *node, const char *name, uint8 dimensions);
unsigned int	p_node_c_curve_key_num(const PNCCurve *curve);
PNCKey *	p_node_c_curve_key_nth(const PNCCurve *curve, unsigned int n);


/* Text-node manipulation functions. */
typedef void	PNTBuffer;

const char *	p_node_t_language_get(PINode *node);
void		p_node_t_language_set(PONode *node, const char *language);

unsigned int	p_node_t_buffer_num(PINode *node);
PNTBuffer *	p_node_t_buffer_nth(PINode *node, unsigned int n);
PNTBuffer *	p_node_t_buffer_find(PINode *node, const char *name);
const char *	p_node_t_buffer_get_name(const PNTBuffer *buffer);

PNTBuffer *	p_node_t_buffer_create(PONode *node, const char *name);
const char *	p_node_t_buffer_read_begin(PNTBuffer *buffer);
void		p_node_t_buffer_read_end(PNTBuffer *buffer, const char *text);
char *		p_node_t_buffer_read_line(PNTBuffer *buffer, unsigned int line, char *put, size_t putmax);
void		p_node_t_buffer_insert(PNTBuffer *buffer, size_t pos, const char *text);
void		p_node_t_buffer_delete(PNTBuffer *buffer, size_t pos, size_t length);
void		p_node_t_buffer_append(PNTBuffer *buffer, const char *text);

/* Duplicates an input node, and returns something you can actually edit. */
PONode *	p_output_node(PPOutput out, PINode *node);

/* Create an output node from scratch. */
PONode *	p_output_node_create(PPOutput out, VNodeType type, uint32 label);

/* This should probably never be used. FIXME: Well? */
PONode *	p_output_node_copy(PPOutput out, PINode *node, uint32 label);

/* Retreive object link target, copy it, and update link to point to new copy. */
PONode *	p_output_node_o_link(PPOutput out, PONode *node, const char *label);

/* Pass on an input node *directly*, does not (as other create calls do) copy it. Dangerous. */
PONode *	p_output_node_pass(PPOutput out, PINode *node);

/* Fills in the various single-value slots in the output. */
void		p_output_int32(PPOutput out,  int32 value);
void		p_output_uint32(PPOutput out, uint32 value);
void		p_output_real32(PPOutput out, real32 value);
void		p_output_real32_vec2(PPOutput out, const real32 *value);
void		p_output_real32_vec3(PPOutput out, const real32 *value);
void		p_output_real32_vec4(PPOutput out, const real32 *value);
void		p_output_real32_mat16(PPOutput out, const real32 *value);
void		p_output_real64(PPOutput out, real64 value);
void		p_output_real64_vec2(PPOutput out, const real64 *v);
void		p_output_real64_vec3(PPOutput out, const real64 *v);
void		p_output_real64_vec4(PPOutput out, const real64 *v);
void		p_output_real64_mat16(PPOutput out, const real64 *v);
void		p_output_string(PPOutput out, const char *value);

#endif		/* PURPLE_H */
