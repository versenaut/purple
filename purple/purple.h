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

/* Magic to get symbols to export properly. Never define PURPLE_INTERNAL
 * outside of the Purple engine (for instance, when building a plug-in).
*/
#if defined __win32
# if defined PURPLE_INTERNAL
#  define PURPLEAPI	__declspec(dllexport)
# else
#  define PURPLEAPI	extern __declspec(dllimport)
# endif
#else
# if defined PURPLE_INTERNAL
#  define PURPLEAPI
# else
#  define PURPLEAPI	extern
# endif
#endif		/* __win32 */

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

PURPLEAPI void		p_init_create(const char *name);

PURPLEAPI void		p_init_input(int index, PValueType type, const char *name, ...);

PURPLEAPI void		p_init_meta(const char *category, const char *text);

/* Each instance can have its own unique state, passed to compute(). Opaque, specify size in bytes.
 * The constructor() and destructor() functions will be called when plug-in are instantiated/de-inst:ed.
 * If the constructor is NULL, the state will be initialized by Purple to "all bits zero".
 */
PURPLEAPI void		p_init_state(size_t size,
			     void (*constructor)(void *state),
			     void (*destructor)(void *state));

typedef enum
{
	P_COMPUTE_DONE = 0,
	P_COMPUTE_AGAIN
} PComputeStatus;

PURPLEAPI void		p_init_compute(PComputeStatus (*compute)(PPInput *input, PPOutput output, void *state));

/* Read out inputs, registered earlier. One for each type. :/ If this wasn't in C, we could use meta
 * information to just say p_input(input) and have it return a value of the proper registered type.
*/
PURPLEAPI boolean		p_input_boolean(PPInput input);
PURPLEAPI int32			p_input_int32(PPInput input);
PURPLEAPI uint32		p_input_uint32(PPInput input);
PURPLEAPI real32		p_input_real32(PPInput input);
PURPLEAPI const real32 *	p_input_real32_vec2(PPInput input);
PURPLEAPI const real32 *	p_input_real32_vec3(PPInput input);
PURPLEAPI const real32 *	p_input_real32_vec4(PPInput input);
PURPLEAPI const real32 *	p_input_real32_mat16(PPInput input);
PURPLEAPI real64		p_input_real64(PPInput input);
PURPLEAPI const real64 *	p_input_real64_vec2(PPInput input);
PURPLEAPI const real64 *	p_input_real64_vec3(PPInput input);
PURPLEAPI const real64 *	p_input_real64_vec4(PPInput input);
PURPLEAPI const real64 *	p_input_real64_mat16(PPInput input);
PURPLEAPI const char *		p_input_string(PPInput input);
PURPLEAPI PINode *		p_input_node(PPInput input);			/* Inputs the "first" node, somehow. */
PURPLEAPI PINode *		p_input_node_nth(PPInput input, int index);	/* Input n:th node, or NULL. */

/* General-purpose iterator structure. All fields private, this is
 * public only to support automatic (on-stack) allocations.
 * Use like this:
 * 
 * PIter iter;
 *
 * for(p_whatever_iter(whatever, &iter); (el = p_iter_get(&iter)) != NULL; p_iter_next(&iter))
 * { process element <el> here }
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
	const void	*arr;
	unsigned int	index;
	}		dynarr;
	const void	*list;
	}		data;
} PIter;

/* Dereference iterator, returns next element or NULL if end of sequence. */
PURPLEAPI void *		p_iter_data(PIter *iter);
/* Returns index of current element, counting from zero and up. */
PURPLEAPI unsigned int		p_iter_index(const PIter *iter);
/* Advance the iterator to point at the next element (if there is one). */
PURPLEAPI void			p_iter_next(PIter *iter);


/* Node manipulation functions. Getters work on both input and output
 * nodes, setting requires output.
*/
PURPLEAPI VNodeType		p_node_get_type(const Node *node);

PURPLEAPI const char *		p_node_get_name(const Node *node);
PURPLEAPI void			p_node_set_name(PONode *node, const char *name);

/* Tag functions. */
typedef void	PNTagGroup, PNTag;

PURPLEAPI unsigned int		p_node_tag_group_num(PINode *node);
PURPLEAPI PNTagGroup *		p_node_tag_group_nth(PINode *node, unsigned int n);
PURPLEAPI PNTagGroup *		p_node_tag_group_find(PINode *node, const char *name);
PURPLEAPI void			p_node_tag_group_iter(PINode *node, PIter *iter);
PURPLEAPI const char *		p_node_tag_group_get_name(const PNTagGroup *group);

PURPLEAPI PNTagGroup *		p_node_tag_group_create(PONode *node, const char *name);
PURPLEAPI void			p_node_tag_group_destroy(PONode *node, PNTagGroup *group);
PURPLEAPI unsigned int		p_node_tag_group_tag_num(const PNTagGroup *group);
PURPLEAPI PNTag *		p_node_tag_group_tag_nth(const PNTagGroup *group, unsigned int n);
PURPLEAPI PNTag *		p_node_tag_group_tag_find(const PNTagGroup *group, const char *name);
PURPLEAPI void			p_node_tag_group_tag_iter(const PNTagGroup *group, PIter *iter);
PURPLEAPI void			p_node_tag_group_tag_create(PNTagGroup *group, const char *name, VNTagType type, const VNTag *value);
PURPLEAPI void			p_node_tag_group_tag_destroy(PNTagGroup *group, const char *name);
PURPLEAPI const char *		p_node_tag_get_name(const PNTag *tag);
PURPLEAPI VNTagType		p_node_tag_get_type(const PNTag *tag);

/* Set from a "group/tag"-style path, with vararg for scalar values. */
PURPLEAPI void			p_node_tag_create_path(PONode *node, const char *path, VNTagType type, ...);
PURPLEAPI void			p_node_tag_destroy_path(PONode *node, const char *path);

PURPLEAPI void			p_node_o_light_set(PONode *node, real64 red, real64 green, real64 blue);
PURPLEAPI void			p_node_o_light_get(PINode *node, real64 *red, real64 *green, real64 *blue);

PURPLEAPI void			p_node_o_link_set(PONode *node, const PONode *link, const char *label, uint32 target_id);
PURPLEAPI PINode *		p_node_o_link_get(const PONode *node, const char *label, uint32 target_id);

/* Geometry-node manipulation functions. */
typedef void	PNGLayer;
typedef void	PNGBone;

PURPLEAPI unsigned int		p_node_g_layer_num(PINode *node);
PURPLEAPI PNGLayer *		p_node_g_layer_nth(PINode *node, unsigned int n);
PURPLEAPI PNGLayer *		p_node_g_layer_find(PINode *node, const char *name);
PURPLEAPI size_t		p_node_g_layer_get_size(const PNGLayer *layer);
PURPLEAPI const char *		p_node_g_layer_get_name(const PNGLayer *layer);
PURPLEAPI VNGLayerType		p_node_g_layer_get_type(const PNGLayer *layer);

PURPLEAPI PNGLayer *		p_node_g_layer_create(PONode *node, const char *name, VNGLayerType type,
						      uint32 def_int, real32 def_real);
PURPLEAPI void			p_node_g_layer_destroy(PNGLayer *node, const char *name);
PURPLEAPI void			p_node_g_vertex_set_xyz(PNGLayer *layer, uint32 id, real64 x, real64 y, real64 z);
PURPLEAPI void			p_node_g_vertex_get_xyz(const PNGLayer *layer, uint32 id, real64 *x, real64 *y, real64 *z);
PURPLEAPI void			p_node_g_polygon_set_corner_uint32(PNGLayer *layer, uint32 id, uint32 v0, uint32 v1, uint32 v2, uint32 v3);
PURPLEAPI void			p_node_g_polygon_get_corner_uint32(const PNGLayer *layer, uint32 id, uint32 *v0, uint32 *v1, uint32 *v2, uint32 *v3);
PURPLEAPI void			p_node_g_polygon_set_corner_real32(PNGLayer *layer, uint32 id, real32 v0, real32 v1, real32 v2, real32 v3);
PURPLEAPI void			p_node_g_polygon_get_corner_real32(const PNGLayer *layer, uint32 id, real32 *v0, real32 *v1, real32 *v2, real32 *v3);
PURPLEAPI void			p_node_g_polygon_set_corner_real64(PNGLayer *layer, uint32 id, real64 v0, real64 v1, real64 v2, real64 v3);
PURPLEAPI void			p_node_g_polygon_get_corner_real64(const PNGLayer *layer, uint32 id, real64 *v0, real64 *v1, real64 *v2, real64 *v3);

PURPLEAPI void			p_node_g_polygon_set_face_uint8(PNGLayer *layer, uint32 id, uint8 value);
PURPLEAPI uint8			p_node_g_polygon_get_face_uint8(const PNGLayer *layer, uint32 id);
PURPLEAPI void			p_node_g_polygon_set_face_uint32(PNGLayer *layer, uint32 id, uint32 value);
PURPLEAPI uint32		p_node_g_polygon_get_face_uint32(const PNGLayer *layer, uint32 id);
PURPLEAPI void			p_node_g_polygon_set_face_real64(PNGLayer *layer, uint32 id, real64 value);
PURPLEAPI real64		p_node_g_polygon_get_face_real64(const PNGLayer *layer, uint32 id);

PURPLEAPI void			p_node_g_crease_set_vertex(PONode *node, const char *layer, uint32 def);
PURPLEAPI void			p_node_g_crease_set_edge(PONode *node, const char *layer, uint32 def);

/* Material-node manipulation functions. */

typedef void	PNMFragment;

PURPLEAPI unsigned int		p_node_m_fragment_num(PINode *node);
PURPLEAPI PNMFragment *		p_node_m_fragment_nth(PINode *node, unsigned int n);
PURPLEAPI void			p_node_m_fragment_iter(PINode *node, PIter *iter);
PURPLEAPI VNMFragmentType	p_node_m_fragment_get_type(const PNMFragment *f);
PURPLEAPI PNMFragment *		p_node_m_fragment_get_link(const PNMFragment *f, unsigned char *label);
PURPLEAPI PNMFragment *		p_node_m_fragment_create_color(PONode *node, real64 red, real64 green, real64 blue);
PURPLEAPI PNMFragment *		p_node_m_fragment_create_light(PONode *node, VNMLightType type, real64 normal_falloff,
							       PINode *brdf,
							       const char *brdf_red, const char *brdf_green, const char *brdf_blue);
PURPLEAPI PNMFragment *		p_node_m_fragment_create_reflection(PONode *node, real64 normal_falloff);
PURPLEAPI PNMFragment *		p_node_m_fragment_create_transparency(PONode *node, real64 normal_fallof, real64 refract);
PURPLEAPI PNMFragment *		p_node_m_fragment_create_volume(PONode *node, real64 diffusion, real64 col_r, real64 col_g, real64 col_b,
								const PNMFragment *color);
PURPLEAPI PNMFragment *		p_node_m_fragment_create_geometry(PONode *node,
								  const char *layer_r, const char *layer_g, const char *layer_b);
PURPLEAPI PNMFragment *		p_node_m_fragment_create_texture(PONode *node, PINode *bitmap,
								 const char *layer_r, const char *layer_g, const char *layer_b,
								 const PNMFragment *mapping);
PURPLEAPI PNMFragment *		p_node_m_fragment_create_noise(PONode *node, VNMNoiseType type, const PNMFragment *mapping);
PURPLEAPI PNMFragment *		p_node_m_fragment_create_blender(PONode *node, VNMBlendType type,
								 const PNMFragment *data_a, const PNMFragment *data_b, const PNMFragment *ctrl);
PURPLEAPI PNMFragment *		p_node_m_fragment_create_matrix(PONode *node, const real64 *matrix, const PNMFragment *data);
PURPLEAPI PNMFragment *		p_node_m_fragment_create_ramp(PONode *node, VNMRampType type, uint8 channel,
							      const PNMFragment *mapping, uint8 point_count,
							      const VNMRampPoint *ramp);
PURPLEAPI PNMFragment *		p_node_m_fragment_create_animation(PONode *node, const char *label);
PURPLEAPI PNMFragment *		p_node_m_fragment_create_alternative(PONode *node, const PNMFragment *alt_a, const PNMFragment *alt_b);
PURPLEAPI PNMFragment *		p_node_m_fragment_create_output(PONode *node, const char *label,
								const PNMFragment *front, const PNMFragment *back);


/* Bitmap-node manipulation functions. */
typedef void	PNBLayer;

PURPLEAPI void			p_node_b_set_dimensions(PONode *node, uint16 width, uint16 height, uint16 depth);
PURPLEAPI void			p_node_b_get_dimensions(PINode *node, uint16 *width, uint16 *height, uint16 *depth);
PURPLEAPI unsigned int		p_node_b_layer_num(PINode *node);
PURPLEAPI PNBLayer *		p_node_b_layer_nth(PINode *node, unsigned int n);
PURPLEAPI PNBLayer *		p_node_b_layer_find(PINode *node, const char *name);
PURPLEAPI const char *		p_node_b_layer_get_name(const PNBLayer *layer);
PURPLEAPI VNBLayerType		p_node_b_layer_get_type(const PNBLayer *layer);

PURPLEAPI PNBLayer *		p_node_b_layer_create(PONode *node, const char *name, VNBLayerType type);
PURPLEAPI void *		p_node_b_layer_access_begin(PONode *node, PNBLayer *layer);
PURPLEAPI void			p_node_b_layer_access_end(PONode *node, PNBLayer *layer, void *framebuffer);
PURPLEAPI void *		p_node_b_layer_access_multi_begin(PONode *node, VNBLayerType format, ... /* Layer names ending with NULL. */);
PURPLEAPI void			p_node_b_layer_access_multi_end(PONode *node, void *framebuffer);
/* Simple write-only set-function, that should return pixel for (x,y,z). Can't read; not suitable for filtering. */
PURPLEAPI void			p_node_b_layer_foreach_set(PONode *node, PNBLayer *layer,
							   real64 (*pixel)(uint32 x, uint32 y, uint32 z, void *user), void *user);
PURPLEAPI void			p_node_b_layer_destroy(PONode *node, PNBLayer *layer);


/* Curve-node manipulation functions. */
typedef void	PNCCurve, PNCKey;

PURPLEAPI unsigned int		p_node_c_curve_num(PINode *node);
PURPLEAPI PNCCurve *		p_node_c_curve_nth(PINode *node, unsigned int n);
PURPLEAPI PNCCurve *		p_node_c_curve_find(PINode *node, const char *name);
PURPLEAPI void			p_node_c_curve_iter(PINode *node, PIter *iter);
PURPLEAPI const char *		p_node_c_curve_get_name(const PNCCurve *curve);
PURPLEAPI uint8			p_node_c_curve_get_dimensions(const PNCCurve *curve);

PURPLEAPI PNCCurve *		p_node_c_curve_create(PONode *node, const char *name, uint8 dimensions);
PURPLEAPI void			p_node_c_curve_destroy(PONode *node, PNCCurve *curve);
PURPLEAPI unsigned int		p_node_c_curve_key_num(const PNCCurve *curve);
PURPLEAPI PNCKey *		p_node_c_curve_key_nth(const PNCCurve *curve, unsigned int n);
PURPLEAPI real64		p_node_c_curve_key_get_pos(const PNCKey *key);
PURPLEAPI real64		p_node_c_curve_key_get_value(const PNCKey *key, uint8 dimension);
PURPLEAPI uint32		p_node_c_curve_key_get_pre(const PNCKey *key, uint8 dimension, real64 *value);
PURPLEAPI uint32		p_node_c_curve_key_get_post(const PNCKey *key, uint8 dimension, real64 *value);
PURPLEAPI PNCKey *		p_node_c_curve_key_create(PNCCurve *curve, real64 pos, const real64 *value,
							  const uint32 *pre_pos, const real64 *pre_value,
							  const uint32 *post_pos, const real64 *post_value);
PURPLEAPI void			p_node_c_curve_key_destroy(PNCCurve *curve, PNCKey *key);

/* Text-node manipulation functions. */
typedef void	PNTBuffer;

PURPLEAPI const char *		p_node_t_language_get(PINode *node);
PURPLEAPI void			p_node_t_language_set(PONode *node, const char *language);

PURPLEAPI unsigned int		p_node_t_buffer_num(PINode *node);
PURPLEAPI PNTBuffer *		p_node_t_buffer_nth(PINode *node, unsigned int n);
PURPLEAPI PNTBuffer *		p_node_t_buffer_find(PINode *node, const char *name);
PURPLEAPI const char *		p_node_t_buffer_get_name(const PNTBuffer *buffer);

PURPLEAPI PNTBuffer *		p_node_t_buffer_create(PONode *node, const char *name);
PURPLEAPI const char *		p_node_t_buffer_read_begin(PNTBuffer *buffer);
PURPLEAPI void			p_node_t_buffer_read_end(PNTBuffer *buffer, const char *text);
PURPLEAPI char *		p_node_t_buffer_read_line(PNTBuffer *buffer, unsigned int line, char *put, size_t putmax);
PURPLEAPI void			p_node_t_buffer_insert(PNTBuffer *buffer, size_t pos, const char *text);
PURPLEAPI void			p_node_t_buffer_delete(PNTBuffer *buffer, size_t pos, size_t length);
PURPLEAPI void			p_node_t_buffer_append(PNTBuffer *buffer, const char *text);

/* Audio-node manipulation functions. */
typedef void	PNABuffer;

PURPLEAPI unsigned int		p_node_a_buffer_num(PINode *node);
PURPLEAPI PNABuffer *		p_node_a_buffer_nth(PINode *node, unsigned int n);
PURPLEAPI PNABuffer *		p_node_a_buffer_find(PINode *node, const char *name);
PURPLEAPI const char *		p_node_a_buffer_get_name(const PNABuffer *layer);
PURPLEAPI real64		p_node_a_buffer_get_frequency(const PNABuffer *layer);
PURPLEAPI unsigned int		p_node_a_buffer_get_length(const PNABuffer *layer);

PURPLEAPI PNABuffer *		p_node_a_buffer_create(PONode *node, const char *name, VNABlockType type, real64 frequency);

PURPLEAPI unsigned int		p_node_a_buffer_read_samples(const PNABuffer *buffer, unsigned int start, real64 *samples, unsigned int len);
PURPLEAPI void			p_node_a_buffer_write_samples(PNABuffer *buffer, unsigned int start, const real64 *samples, unsigned int len);

/* Duplicates an input node, and returns something you can actually edit. */
PURPLEAPI PONode *		p_output_node(PPOutput out, PINode *node);

/* Create an output node from scratch. */
PURPLEAPI PONode *		p_output_node_create(PPOutput out, VNodeType type, uint32 label);

/* This should probably never be used. FIXME: Well? */
PURPLEAPI PONode *		p_output_node_copy(PPOutput out, PINode *node, uint32 label);

/* Retreive object link target, copy it, and update link to point to new copy. */
PURPLEAPI PONode *		p_output_node_o_link(PPOutput out, PONode *node, const char *label);

/* Pass on an input node *directly*, does not (as other create calls do) copy it. Dangerous. */
PURPLEAPI PONode *		p_output_node_pass(PPOutput out, PINode *node);

/* Fills in the various single-value slots in the output. */
PURPLEAPI void			p_output_int32(PPOutput out,  int32 value);
PURPLEAPI void			p_output_uint32(PPOutput out, uint32 value);
PURPLEAPI void			p_output_real32(PPOutput out, real32 value);
PURPLEAPI void			p_output_real32_vec2(PPOutput out, const real32 *value);
PURPLEAPI void			p_output_real32_vec3(PPOutput out, const real32 *value);
PURPLEAPI void			p_output_real32_vec4(PPOutput out, const real32 *value);
PURPLEAPI void			p_output_real32_mat16(PPOutput out, const real32 *value);
PURPLEAPI void			p_output_real64(PPOutput out, real64 value);
PURPLEAPI void			p_output_real64_vec2(PPOutput out, const real64 *v);
PURPLEAPI void			p_output_real64_vec3(PPOutput out, const real64 *v);
PURPLEAPI void			p_output_real64_vec4(PPOutput out, const real64 *v);
PURPLEAPI void			p_output_real64_mat16(PPOutput out, const real64 *v);
PURPLEAPI void			p_output_string(PPOutput out, const char *value);

#endif		/* PURPLE_H */
