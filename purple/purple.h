/*
 * 
*/

#if !defined PURPLE_H
#define	PURPLE_H

#include "verse.h"

typedef struct Node	Node;
typedef const Node	PINode;
typedef Node		PONode;

#if 0
typedef struct PINode PINode;	/* Input node, read-only. */
typedef struct PONode PONode;	/* Output node, read-write. */
#endif

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
 * The constructor() and destructor() functions will be called when plug-in is instantiated/de-inst:ed.
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

/* Node manipulation functions. Getters work on both input and output
 * nodes, setting requires output.
*/

VNodeType	p_node_type_get(const Node *node);

const char *	p_node_name_get(const Node *node);
void		p_node_name_set(PONode *node, const char *name);

/* Tag functions. */
typedef struct PNTagGroup	PNTagGroup;

PNTagGroup *	p_node_tag_group_create(PONode *node, const char *name);
PNTagGroup *	p_node_tag_group_find(const PONode *node, const char *name);
void		p_node_tag_set(PNTagGroup *group, const char *name, VNTagType type, const VNTag *value);
void		p_node_tag_destroy(PNTagGroup *group, const char *name);

void		p_node_o_link_set(PONode *node, const PONode *link, const char *label, uint32 target_id);
PINode *	p_node_o_link_get(const PONode *node, const char *label, uint32 target_id);

/* Geometry-node manipulation functions. */
typedef void	PNGLayer;
typedef void	PNGBone;

PNGLayer *	p_node_g_layer_lookup(PINode *node, const char *name);
size_t		p_node_g_layer_size(PINode *node, const PNGLayer *layer);
PNGLayer *	p_node_g_layer_create(PONode *node, const char *name, VNGLayerType type,
					     uint32 def_int, real32 def_real);
void		p_node_g_layer_delete(PNGLayer *node, const char *name);
void		p_node_g_vertex_set_xyz(PONode *node, PNGLayer *layer, uint32 id, real64 x, real64 y, real64 z);
void		p_node_g_vertex_get_xyz(const PONode *node, const PNGLayer *layer, uint32 id, real64 *x, real64 *y, real64 *z);
void		p_node_g_polygon_set_corner_uint32(PONode *node, PNGLayer *layer, uint32 id, uint32 v0, uint32 v1, uint32 v2, uint32 v3);

void		p_node_g_crease_set_vertex(PONode *node, const char *layer, uint32 def);
void		p_node_g_crease_set_edge(PONode *node, const char *layer, uint32 def);


/* Bitmap-node manipulation functions. */
void		p_node_b_dimensions_set(PONode *node, uint16 width, uint16 height, uint16 depth);
uint16		p_node_b_layer_create(PONode *node, const char *name, VNBLayerType type);
void *		p_node_b_layer_access_begin(PONode *node, uint16 layer_id);
void		p_node_b_layer_access_end(PONode *node, uint16 layer_id, void *framebuffer);
void		p_node_b_layer_destroy(PONode *node, uint16 layer_id);


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
