/*
 *
*/

#include <math.h>

#include "verse.h"

real32 vec_real32_vec2_magnitude(const real32 *vec)
{
	return sqrtf(vec[0] * vec[0] + vec[1] * vec[1]);
}

real32 vec_real32_vec3_magnitude(const real32 *vec)
{
	return sqrtf(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
}

real32 vec_real32_vec4_magnitude(const real32 *vec)
{
	return sqrtf(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2] + vec[3] * vec[3]);
}

/* ----------------------------------------------------------------------------------------- */

real64 vec_real64_vec2_magnitude(const real64 *vec)
{
	return sqrt(vec[0] * vec[0] + vec[1] * vec[1]);
}

real64 vec_real64_vec3_magnitude(const real64 *vec)
{
	return sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
}

real64 vec_real64_vec4_magnitude(const real64 *vec)
{
	return sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2] + vec[3] * vec[3]);
}
