/*
 * 
*/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "purple.h"

#include "dynstr.h"
#include "log.h"
#include "strutil.h"
#include "value.h"
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

real32 p_input_real32(PPInput input)
{
	return port_input_real32(input);
}

const char * p_input_string(PPInput input)
{
	return port_input_string(input);
}
