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
	if(input == NULL)
		return;
	return 0.0f;
}
