/*
 * 
*/

#include <stdio.h>

#include "purple.h"

#include "log.h"
#include "strutil.h"
#include "vecutil.h"

#include "plugins.h"

/* ----------------------------------------------------------------------------------------- */

/* Helper function: return input value as a 64-bit floating point number. This is handy,
 * since all scalar numerical types supported are representable as such. Cuts down on
 * code in type-specific input readers. Returns 1 if <value> was set, 0 if not.
*/
static int input_real64(const PInputValue *v, real64 *value)
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
	const PInputValue	*v = input;
	real64			value;

	if(input_real64(v, &value))
		return value != 0.0;
	LOG_WARN(("Can't convert value of type %d to boolean", v->type));
	return 0;
}

int32 p_input_int32(PPInput input)
{
	const PInputValue	*v = input;
	real64			value;

	if(input_real64(v, &value))
		return value;
	LOG_WARN(("Can't convert value of type %d to int32", v->type));
	return 0;
}

uint32 p_input_uint32(PPInput input)
{
	const PInputValue	*v = input;
	real64			value;

	if(input_real64(v, &value))
		return value;
	LOG_WARN(("Can't convert value of type %d to uint32", v->type));
	return 0u;
}

real32 p_input_real32(PPInput input)
{
	const PInputValue	*v = input;
	real64			value;

	if(input_real64(v, &value))
		return value;
	LOG_WARN(("Can't convert value of type %d to real32", v->type));
	return 0.0f;
}

real64 p_input_real64(PPInput input)
{
	const PInputValue	*v = input;
	real64			value;

	if(input_real64(v, &value))
		return value;
	LOG_WARN(("Can't convert value of type %d to real64", v->type));
	return 0.0;
}
