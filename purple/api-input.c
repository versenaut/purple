/*
 * 
*/

#include <stdio.h>

#include "log.h"
#include "purple.h"
#include "plugins.h"

/* ----------------------------------------------------------------------------------------- */

/* These are the plug-in-visible actual Purple API functions. */

real32 p_input_real32(PPInput input)
{
	const PInputValue	*v = input;

	if(input == NULL)
		return 0.0f;
	switch(v->type)
	{
	case P_INPUT_BOOLEAN:
		return v->v.vboolean;
	case P_INPUT_INT32:
		return v->v.vint32;
	case P_INPUT_UINT32:
		return v->v.vuint32;
	case P_INPUT_REAL32:
		return v->v.vreal32;
	case P_INPUT_REAL64:
		return v->v.vreal64;
	case P_INPUT_STRING:
		return (real32) strtod(v->v.vstring, NULL);
	default:
		LOG_WARN(("Can't convert value of type %d to real32", v->type));
	}
	return 0.0f;
}
