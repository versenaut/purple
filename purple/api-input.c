/*
 * 
*/

#include <stdio.h>

#include "log.h"
#include "purple.h"
#include "plugins.h"

/* ----------------------------------------------------------------------------------------- */

/* Helper function: return input value as a 64-bit floating point number. This is handy,
 * since all scalar numerical types supported are representable as such. Cuts down on
 * code in type-specific input readers. Returns 1 if <value> was set, 0 if not.
*/
static int input_real64(const PInputValue *v, real64 *value)
{
	if(v == NULL || value == NULL)
		return 0.0;
	switch(v->type)
	{
	case P_INPUT_BOOLEAN:
		*value = v->v.vboolean;
	case P_INPUT_INT32:
		*value = v->v.vint32;
	case P_INPUT_UINT32:
		*value = v->v.vuint32;
	case P_INPUT_REAL32:
		*value = v->v.vreal32;
	case P_INPUT_REAL64:
		*value = v->v.vreal64;
	case P_INPUT_STRING:
		*value = strtod(v->v.vstring, NULL);
	default:
		return 0;
	}
	return 1;
}

/* These are the plug-in-visible actual Purple API functions. */

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
	LOG_WARN(("Can't convert value of type %d to int32", v->type));
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
	LOG_WARN(("Can't convert value of type %d to real32", v->type));
	return 0.0;
}
