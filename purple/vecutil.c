/*
 * vecutil.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Vector (and matrix) math routines.
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

/* A semi-horrible macro to compute determinant of 4x4 matrix 'm', assumed to be simply a linear
 * array of floating point numbers. Done as macro to allow m to be either real32 or real64 easily.
*/
#define	MAT16_DET(m)	\
          m[(1-1)*4+1-1] * m[(2-1)*4+2-1] * m[(3-1)*4+3-1] * m[(4-1)*4+4-1] \
        - m[(1-1)*4+1-1] * m[(2-1)*4+2-1] * m[(3-1)*4+4-1] * m[(4-1)*4+3-1] \
        + m[(1-1)*4+1-1] * m[(2-1)*4+3-1] * m[(3-1)*4+4-1] * m[(4-1)*4+2-1] \
        - m[(1-1)*4+1-1] * m[(2-1)*4+3-1] * m[(3-1)*4+2-1] * m[(4-1)*4+4-1] \
        + m[(1-1)*4+1-1] * m[(2-1)*4+4-1] * m[(3-1)*4+2-1] * m[(4-1)*4+3-1] \
        - m[(1-1)*4+1-1] * m[(2-1)*4+4-1] * m[(3-1)*4+3-1] * m[(4-1)*4+2-1] \
        - m[(1-1)*4+2-1] * m[(2-1)*4+3-1] * m[(3-1)*4+4-1] * m[(4-1)*4+1-1] \
        + m[(1-1)*4+2-1] * m[(2-1)*4+3-1] * m[(3-1)*4+1-1] * m[(4-1)*4+4-1] \
        - m[(1-1)*4+2-1] * m[(2-1)*4+4-1] * m[(3-1)*4+1-1] * m[(4-1)*4+3-1] \
        + m[(1-1)*4+2-1] * m[(2-1)*4+4-1] * m[(3-1)*4+3-1] * m[(4-1)*4+1-1] \
        - m[(1-1)*4+2-1] * m[(2-1)*4+1-1] * m[(3-1)*4+3-1] * m[(4-1)*4+4-1] \
        + m[(1-1)*4+2-1] * m[(2-1)*4+1-1] * m[(3-1)*4+4-1] * m[(4-1)*4+3-1] \
        + m[(1-1)*4+3-1] * m[(2-1)*4+4-1] * m[(3-1)*4+1-1] * m[(4-1)*4+2-1] \
        - m[(1-1)*4+3-1] * m[(2-1)*4+4-1] * m[(3-1)*4+2-1] * m[(4-1)*4+1-1] \
        + m[(1-1)*4+3-1] * m[(2-1)*4+1-1] * m[(3-1)*4+2-1] * m[(4-1)*4+4-1] \
        - m[(1-1)*4+3-1] * m[(2-1)*4+1-1] * m[(3-1)*4+4-1] * m[(4-1)*4+2-1] \
        + m[(1-1)*4+3-1] * m[(2-1)*4+2-1] * m[(3-1)*4+4-1] * m[(4-1)*4+1-1] \
        - m[(1-1)*4+3-1] * m[(2-1)*4+2-1] * m[(3-1)*4+1-1] * m[(4-1)*4+4-1] \
        - m[(1-1)*4+4-1] * m[(2-1)*4+1-1] * m[(3-1)*4+2-1] * m[(4-1)*4+3-1] \
        + m[(1-1)*4+4-1] * m[(2-1)*4+1-1] * m[(3-1)*4+3-1] * m[(4-1)*4+2-1] \
        - m[(1-1)*4+4-1] * m[(2-1)*4+2-1] * m[(3-1)*4+3-1] * m[(4-1)*4+1-1] \
        + m[(1-1)*4+4-1] * m[(2-1)*4+2-1] * m[(3-1)*4+1-1] * m[(4-1)*4+3-1] \
        - m[(1-1)*4+4-1] * m[(2-1)*4+3-1] * m[(3-1)*4+1-1] * m[(4-1)*4+2-1] \
        + m[(1-1)*4+4-1] * m[(2-1)*4+3-1] * m[(3-1)*4+2-1] * m[(4-1)*4+1-1]

real32 vec_real32_mat16_determinant(const real32 *mat)
{
	return MAT16_DET(mat);
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

real64 vec_real64_mat16_determinant(const real64 *mat)
{
	return MAT16_DET(mat);
}
