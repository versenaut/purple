/*
 * 
*/

#if !defined PURPLE_H
#define	PURPLE_H

#include "verse.h"

typedef struct PINode PINode;	/* Input node, read-only. */
typedef struct PONode PONode;	/* Output node, read-write. */

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

#if 0
typedef enum
{
	P_INPUT_BOOLEAN = 0,
	P_INPUT_INT32,
	P_INPUT_UINT32,
	P_INPUT_REAL32,
	P_INPUT_REAL32_VEC2,
	P_INPUT_REAL32_VEC3,
	P_INPUT_REAL32_VEC4,
	P_INPUT_REAL32_MAT16,
	P_INPUT_REAL64,
	P_INPUT_REAL64_VEC2,
	P_INPUT_REAL64_VEC3,
	P_INPUT_REAL64_VEC4,
	P_INPUT_REAL64_MAT16,
	P_INPUT_STRING,
	P_INPUT_MODULE,
} PInputType;

typedef struct
{
	PInputType	type;		/* The type of value currently stored. */
	union {
	boolean	vboolean;
	int32	vint32;
	uint32	vuint32;
	real32	vreal32;
	real32	vreal32_vec2[2];
	real32	vreal32_vec3[3];
	real32	vreal32_vec4[4];
	real32	vreal32_mat16[16];

	real64	vreal64;
	real64	vreal64_vec2[2];
	real64	vreal64_vec3[3];
	real64	vreal64_vec4[4];
	real64	vreal64_mat16[16];

	char	*vstring;
	uint32	vmodule;
	}	v;
} PInputValue;

/* "Ports" are used to model inputs and outputs. */
typedef void 		PPort;
typedef const PPort *	PPInput;
typedef PPort *		PPOutput;
#endif

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
const PINode *	p_input_node(PPInput input);

/* Node manipulation functions. Getters work on both input and output
 * nodes, setting requires output.
*/

const char *	p_node_name_get(const void *node);
void		p_node_name_set(PONode *node, const char *name);

/* Tag functions. */
typedef struct PNTagGroup	PNTagGroup;

PNTagGroup *	p_node_tag_group_create(PONode *node, const char *name);
PNTagGroup *	p_node_tag_group_find(const PONode *node, const char *name);
void		p_node_tag_set(PNTagGroup *group, const char *name, VNTagType type, const VNTag *value);
void		p_node_tag_destroy(PNTagGroup *group, const char *name);

/* Geometry-node manipulation functions. */
typedef struct PNGLayer	PNGLayer;

PNGLayer *	p_node_g_layer_create(PONode *node, const char *name, VNGLayerType type,
					     uint32 def_int, real32 def_real);
void		p_node_g_layer_vertex_set_real32_xyz(PNGLayer *layer, uint32 id, real32 x, real32 y, real32 z);
void		p_node_g_layer_vertex_set_real64_xyz(PNGLayer *layer, uint32 id, real64 x, real64 y, real64 z);
void		p_node_g_layer_delete(PONode *node, const char *name);


/* Duplicates an input node, and returns something you can actually edit. */
PONode *	p_output_node(PPOutput out, const PINode *node);

/* Creates a new empty output node for editing/data creation. */
PONode * 	p_output_node_create(VNodeType type, const char *name);

/* Fills in the various single-value slots in the output. */
void		p_output_int32(PPOutput out,  int32 value);
void		p_output_uint32(PPOutput out, uint32 value);
void		p_output_real32(PPOutput out, real32 value);
void		p_output_real32_vec2(PPOutput out, const real32 *value);
void		p_output_real32_vec3(PPOutput out, const real32 *value);
void		p_output_real32_vec4(PPOutput out, const real32 *value);
void		p_output_real32_mat16(PPOutput out, const real32 *value);
void		p_output_real64(PPOutput out, real64 value);
void		p_output_string(PPOutput out, const char *value);

#endif		/* PURPLE_H */
