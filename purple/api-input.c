/*
 * api-input.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * 
*/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define PURPLE_INTERNAL

#include "purple.h"

#include "dynstr.h"
#include "log.h"
#include "value.h"
#include "nodeset.h"
#include "strutil.h"
#include "port.h"
#include "vecutil.h"

#include "plugins.h"

/* ----------------------------------------------------------------------------------------- */

#if 0

/* Helper function: return input value as a 64-bit floating point number. This is handy,
 * since all scalar numerical types supported are representable as such. Cuts down on
 * code in type-specific input readers. Returns 1 if <value> was set, 0 if not.
*/
static int input_real64(const PValue *v, real64 *value)
{
	if(value == NULL)
		return 0;
	if(v == NULL)		/* Non-connected input simply reads as zero. Might be good. */
	{
		*value = 0;
		return 1;
	}
	switch(v->type)
	{
	case P_INPUT_BOOLEAN:
		*value = v->v.vboolean;
		break;
	case P_INPUT_INT32:
		*value = v->v.vint32;
		break;
	case P_INPUT_UINT32:
		*value = v->v.vuint32;
		break;
	case P_INPUT_REAL32:
		*value = v->v.vreal32;
		break;
	case P_INPUT_REAL32_VEC2:
		*value = vec_real32_vec2_magnitude(v->v.vreal32_vec2);
		break;
	case P_INPUT_REAL32_VEC3:
		*value = vec_real32_vec3_magnitude(v->v.vreal32_vec2);
		break;
	case P_INPUT_REAL32_VEC4:
		*value = vec_real32_vec4_magnitude(v->v.vreal32_vec2);
		break;
	case P_INPUT_REAL32_MAT16:
		*value = vec_real32_mat16_determinant(v->v.vreal32_mat16);
		break;
	case P_INPUT_REAL64:
		*value = v->v.vreal64;
		break;
	case P_INPUT_REAL64_VEC2:
		*value = vec_real64_vec2_magnitude(v->v.vreal64_vec2);
		break;
	case P_INPUT_REAL64_VEC3:
		*value = vec_real64_vec3_magnitude(v->v.vreal64_vec2);
		break;
	case P_INPUT_REAL64_VEC4:
		*value = vec_real64_vec4_magnitude(v->v.vreal64_vec2);
		break;
	case P_INPUT_REAL64_MAT16:
		*value = vec_real64_mat16_determinant(v->v.vreal64_mat16);
		break;
	case P_INPUT_STRING:
		*value = strtod(v->v.vstring, NULL);
		break;
	default:
		return 0;
	}
	return 1;
}

/* These are the plug-in-visible actual Purple API functions. */

boolean p_input_boolean(PPInput input)
{
	const PInputValue	*in = input;
	real64			value;

	if(input_real64(in, &value))
		return value != 0.0;
	LOG_WARN(("Can't convert value of type %d to boolean", in->type));
	return 0;
}

int32 p_input_int32(PPInput input)
{
	const PInputValue	*in = input;
	real64			value;

	if(input_real64(in, &value))
		return value;
	LOG_WARN(("Can't convert value of type %d to int32", in->type));
	return 0;
}

uint32 p_input_uint32(PPInput input)
{
	const PInputValue	*in = input;
	real64			value;

	if(input_real64(in, &value))
		return value;
	LOG_WARN(("Can't convert value of type %d to uint32", in->type));
	return 0u;
}

real32 p_input_real32(PPInput input)
{
	const PInputValue	*in = input;
	real64			value;

	if(input_real64(in, &value))
		return value;
	LOG_WARN(("Can't convert value of type %d to real32", in->type));
	return 0.0f;
}

/* Spread the value for the first component into the other three.
 * Handy when making a 4-dimensional vector of a scalar.
*/
static const real32 * spread_real32_vec4(real32 *buffer)
{
	buffer[1] = buffer[2] = buffer[3] = buffer[0];
	return buffer;
}

const real32 * p_input_real32_vec4(PPInput input, real32 *buffer)
{
	const PInputValue	*in = input;

	if(in == NULL || buffer == NULL)
		return NULL;
	switch(in->type)
	{
	case P_INPUT_BOOLEAN:
		buffer[0] = in->v.vboolean;
		return spread_real32_vec4(buffer);
	case P_INPUT_INT32:
		buffer[0] = in->v.vint32;
		return spread_real32_vec4(buffer);
	case P_INPUT_UINT32:
		buffer[0] = in->v.vuint32;
		return spread_real32_vec4(buffer);
	case P_INPUT_REAL32:
		buffer[0] = in->v.vreal32;
		return spread_real32_vec4(buffer);
	case P_INPUT_REAL32_VEC2:
		buffer[0] = in->v.vreal32_vec2[0];
		buffer[1] = in->v.vreal32_vec2[1];
		buffer[2] = buffer[3] = 0.0f;
		return buffer;
	case P_INPUT_REAL32_VEC3:
		buffer[0] = in->v.vreal32_vec2[0];
		buffer[1] = in->v.vreal32_vec2[1];
		buffer[2] = in->v.vreal32_vec2[2];
		buffer[3] = 0.0f;
		return buffer;
	case P_INPUT_REAL32_VEC4:
		memcpy(buffer, in->v.vreal32_vec4, 4 * sizeof *buffer);
		return buffer;
	case P_INPUT_REAL64:
		buffer[0] = in->v.vreal64;
		return spread_real32_vec4(buffer);
	case P_INPUT_STRING:
		/* Complexity called for. */
		break;
	}
	return NULL;
}

real64 p_input_real64(PPInput input)
{
	const PInputValue	*in = input;
	real64			value;

	if(input_real64(in, &value))
		return value;
	LOG_WARN(("Can't convert value of type %d to real64", in->type));
	return 0.0;
}

/* Input a string. Always copies into user-supplied buffer, even for string inputs. */
const char * p_input_string(PPInput input, char *buffer, size_t buf_max)
{
	const PInputValue	*in = input;
	int			put;

	if(in == NULL || buffer == NULL || buf_max < 2)
		return NULL;
	switch(in->type)
	{
	case P_INPUT_BOOLEAN:
		put = snprintf(buffer, buf_max, "%s", in->v.vboolean ? "true" : "false");
		break;
	case P_INPUT_INT32:
		put = snprintf(buffer, buf_max, "%d", in->v.vint32);
		break;
	case P_INPUT_UINT32:
		put = snprintf(buffer, buf_max, "%u", in->v.vuint32);
		break;
	case P_INPUT_REAL32:
		put = snprintf(buffer, buf_max, "%g", in->v.vreal32);
		break;
	case P_INPUT_REAL32_VEC2:
		put = snprintf(buffer, buf_max, "[%g %g]", in->v.vreal32_vec2[0], in->v.vreal32_vec2[1]);
		break;
	case P_INPUT_REAL32_VEC3:
		put = snprintf(buffer, buf_max, "[%g %g %g]", in->v.vreal32_vec3[0], in->v.vreal32_vec3[1], in->v.vreal32_vec3[2]);
		break;
	case P_INPUT_REAL32_VEC4:
		put = snprintf(buffer, buf_max, "[%g %g %g %g]",
			 in->v.vreal32_vec4[0], in->v.vreal32_vec4[1], in->v.vreal32_vec4[2], in->v.vreal32_vec4[3]);
		break;
	case P_INPUT_REAL32_MAT16:
		put = snprintf(buffer, buf_max, "[[%g %g %g %g][%g %g %g %g][%g %g %g %g][%g %g %g %g]]",
			 in->v.vreal32_mat16[0], in->v.vreal32_mat16[1], in->v.vreal32_mat16[2], in->v.vreal32_mat16[3],
			 in->v.vreal32_mat16[4], in->v.vreal32_mat16[5], in->v.vreal32_mat16[6], in->v.vreal32_mat16[7],
			 in->v.vreal32_mat16[8], in->v.vreal32_mat16[9], in->v.vreal32_mat16[10], in->v.vreal32_mat16[11],
			 in->v.vreal32_mat16[12], in->v.vreal32_mat16[13], in->v.vreal32_mat16[14], in->v.vreal32_mat16[15]);
		break;
	case P_INPUT_REAL64:
		put = snprintf(buffer, buf_max, "%.10g", in->v.vreal64);
		break;
	case P_INPUT_REAL64_VEC2:
		put = snprintf(buffer, buf_max, "[%.10g %.10g]", in->v.vreal64_vec2[0], in->v.vreal64_vec2[1]);
		break;
	case P_INPUT_REAL64_VEC3:
		put = snprintf(buffer, buf_max, "[%.10g %.10g %.10g]", in->v.vreal64_vec3[0], in->v.vreal64_vec3[1], in->v.vreal64_vec3[2]);
		break;
	case P_INPUT_REAL64_VEC4:
		put = snprintf(buffer, buf_max, "[%.10g %.10g %.10g %.10g]",
			 in->v.vreal64_vec4[0], in->v.vreal64_vec4[1], in->v.vreal64_vec4[2], in->v.vreal64_vec4[3]);
		break;
	case P_INPUT_REAL64_MAT16:
		put = snprintf(buffer, buf_max, "[[%.10g %.10g %.10g %.10g][%.10g %.10g %.10g %.10g]"
			 "[%.10g %.10g %.10g %.10g][%.10g %.10g %.10g %.10g]]",
			 in->v.vreal64_mat16[0], in->v.vreal64_mat16[1], in->v.vreal64_mat16[2], in->v.vreal64_mat16[3],
			 in->v.vreal64_mat16[4], in->v.vreal64_mat16[5], in->v.vreal64_mat16[6], in->v.vreal64_mat16[7],
			 in->v.vreal64_mat16[8], in->v.vreal64_mat16[9], in->v.vreal64_mat16[10], in->v.vreal64_mat16[11],
			 in->v.vreal64_mat16[12], in->v.vreal64_mat16[13], in->v.vreal64_mat16[14], in->v.vreal64_mat16[15]);
		break;
	case P_INPUT_STRING:
		return stu_strncpy(buffer, buf_max, in->v.vstring);	/* Possibly quicker than snprintf(). */
	}
	if(put > 0 && put < buf_max)
		return buffer;
	return NULL;
}

#endif

/** \defgroup api_input Input Functions
 * 
 * Input functions are used to read out the values connected to a plug-in instance's inputs. Such values might come
 * either directly due to a user manipulating some UI to input a constant, or indirectly thanks to the input being
 * connected to some other plug-in instance's output. From the point of view of the plug-in code, there is no
 * difference between the two.
 * 
 * The functions are all named after which type of value they return. Because the API is in C, there is no
 * language support for polymorphism, so it has been made explicit like this.
 * 
 * There is no requirement that the type of value retreived from an input matches the type that input was
 * created with. It is a recommended and natural way to operate, to keep the two matched, but Purple will
 * do conversions where possible if they do not. Since most of the functions do not have a way of communicating
 * failure, they will instead return some default value if an input is not available, or if a conversion
 * cannot be performed.
 * 
 * @{
*/

/**
 * Read the value of an input, and convert value to boolean (0 or 1).
*/
PURPLEAPI boolean p_input_boolean(PPInput input	/** The input to read from. */)
{
	return port_input_boolean(input);
}

/**
 * Read the value of an input, and convert it to \c int32, i.e. 32-bit signed integer.
*/
PURPLEAPI int32 p_input_int32(PPInput input	/** The input to read from. */)
{
	return port_input_int32(input);
}

/**
 * Read the value of an input, and convert it to \c uint32, i.e. 32-bit unsigned integer.
*/
PURPLEAPI uint32 p_input_uint32(PPInput input	/** The input to read from. */)
{
	return port_input_uint32(input);
}

/**
 * Read the value of an input, and convert it to \c real32, i.e. 32-bit floating point.
*/
PURPLEAPI real32 p_input_real32(PPInput input	/** The input to read from. */)
{
	return port_input_real32(input);
}

/**
 * Read the value of an input, and convert it to 2D vector of 32-bit floating point numbers.
*/
PURPLEAPI const real32 * p_input_real32_vec2(PPInput input	/** The input to read from. */)
{
	return port_input_real32_vec2(input);
}

/**
 * Read the value of an input, and convert it to 3D vector of 32-bit floating point numbers.
*/
PURPLEAPI const real32 * p_input_real32_vec3(PPInput input	/** The input to read from. */)
{
	return port_input_real32_vec3(input);
}

/**
 * Read the value of an input, and convert it to 4D vector of 32-bit floating point numbers.
*/
PURPLEAPI const real32 * p_input_real32_vec4(PPInput input	/** The input to read from. */)
{
	return port_input_real32_vec4(input);
}

/**
 * Read the value of an input, and convert it to 4x4 matrix of 32-bit floating point numbers.
*/
PURPLEAPI const real32 * p_input_real32_mat16(PPInput input	/** The input to read from. */)
{
	return port_input_real32_mat16(input);
}

/**
 * Read the value of an input, and convert it to \c real64, i.e. 64-bit floating point.
*/
PURPLEAPI real64 p_input_real64(PPInput input	/** The input to read from. */)
{
	return port_input_real64(input);
}

/**
 * Read the value of an input, and convert it to 2D vector of 64-bit floating point numbers.
*/
PURPLEAPI const real64 * p_input_real64_vec2(PPInput input	/** The input to read from. */)
{
	return port_input_real64_vec2(input);
}

/**
 * Read the value of an input, and convert it to 3D vector of 64-bit floating point numbers.
*/
PURPLEAPI const real64 * p_input_real64_vec3(PPInput input	/** The input to read from. */)
{
	return port_input_real64_vec3(input);
}

/**
 * Read the value of an input, and convert it to 4D vector of 64-bit floating point numbers.
*/
PURPLEAPI const real64 * p_input_real64_vec4(PPInput input	/** The input to read from. */)
{
	return port_input_real64_vec4(input);
}

/**
 * Read the value of an input, and convert it to 4x4 matrix of 64-bit floating point numbers.
*/
PURPLEAPI const real64 * p_input_real64_mat16(PPInput input	/** The input to read from. */)
{
	return port_input_real64_mat16(input);
}

/**
 * Read the value of an input, and convert the value to string.
 * 
 * Because of the way strings work in C, and since the function does not require (or allow) the
 * calling plug-in code to provide buffer space, it is generally wise to copy the value to some
 * other buffer space shortly after retreiving it. The Purple engine does manage a buffer
 * associated with the input for holding a "cached" string version of whatever is there, but
 * since this buffer space might be re-used further on, a plug-in cannot retain the buffer pointer
 * indefinitely with well-defined results.
*/
PURPLEAPI const char * p_input_string(PPInput input	/** The input to read from. */)
{
	return port_input_string(input);
}

/**
 * Read the value of an input, and return the first node reference found.
 * 
 * Unlike the "simple" value types above, Purple cannot automatically convert any value into
 * the desired type. If there are no nodes present on the specified input, this function will
 * return \c NULL.
*/
PURPLEAPI PINode * p_input_node(PPInput input	/** The input to read from. */)
{
	return port_input_node(input);
}

/**
 * Read the value of an input, and return the \a n:th node reference found.
 * 
 * If \a index is larger than the number of node references present, \c NULL is returned.
 * Thus, this function can be used to iterate over all node references present on a given
 * input, like so:
 * 
 * \code
 * int		i;
 * PINode	*node;
 * 
 * for(i = 0; (node = p_input_node_nth(input, i)) != NULL; i++)
 * {
 * 	// ... process the node here ...
 * }
 * \endcode
 * 
 * As with \c p_input_node() above, this function cannot convert non-node data into node
 * references. If no nodes are present on the specified input, NULL will be returned for
 * all values of \a index.
 * 
 * No assumptions can be made about the order in which nodes appear.
*/
PURPLEAPI PINode * p_input_node_nth(PPInput input	/** The input to read from. */,
				    int index		/** Index (from 0) of the node reference to retreive. */)
{
	return port_input_node_nth(input, index);
}

/** \brief Input the first node of a given type.
 * 
 * This function returns the first node available at the indicated input, that has the
 * indicated type.
 * 
 * Internally, this is just a loop using \c p_input_node_nth(), but it saves some typing
 * and makes plug-in code look cleaner, and might be implemented otherwise in the future.
*/
PURPLEAPI PINode * p_input_node_first_type(PPInput input,
					   VNodeType type)
{
	int	i;
	PINode	*node;

	for(i = 0; (node = p_input_node_nth(input, i)) != NULL; i++)
	{
		if(p_node_get_type(node) == type)
			return node;
	}
	return NULL;
}

PURPLEAPI PValueType p_input_get_type(PPInput input)
{
	return port_get_type(input);
}

/** @} */
