/*
 * vecutil.h
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Some very basic vector math utilities. Needed in input processing,
 * but might also be useful to export to plug-in code at some point.
*/

extern real32	vec_real32_vec2_magnitude(const real32 *vec);
extern real32	vec_real32_vec3_magnitude(const real32 *vec);
extern real32	vec_real32_vec4_magnitude(const real32 *vec);

extern real32	vec_real32_mat16_determinant(const real32 *mat);

extern real64	vec_real64_vec2_magnitude(const real64 *vec);
extern real64	vec_real64_vec3_magnitude(const real64 *vec);
extern real64	vec_real64_vec4_magnitude(const real64 *vec);

extern real64	vec_real64_mat16_determinant(const real64 *mat);
