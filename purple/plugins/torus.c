/*
 * A torus primitive. Far simpler than I had thought.
*/

#include <math.h>
#include "purple.h"

/* Rotate a vertex around the Y axis. */
static void rotate_around_y(real64 *vtx, real64 angle)
{
	real64	tmp[3], ca = cos(angle), sa = sin(angle);

	tmp[0] =  ca * vtx[0] + sa * vtx[2];
	tmp[2] = -sa * vtx[0] + ca * vtx[2];

	vtx[0] = tmp[0];
	vtx[2] = tmp[2];
}

static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	real32		r1 = p_input_real32(input[0]), r2 = p_input_real32(input[1]), r;
	uint32		s1 = p_input_uint32(input[2]), s2 = p_input_uint32(input[3]);
	PONode		*obj, *geo;
	PNGLayer	*lay;
	uint32		i, j, next;

	if(r2 < r1)
		return P_COMPUTE_DONE;
	r = (r2 - r1) / 2.0;	/* Compute radius of actual tube. */

	obj = p_output_node_create(output, V_NT_OBJECT, 0);
	p_node_set_name(obj, "torus");
	geo = p_output_node_create(output, V_NT_GEOMETRY, 1);
	p_node_set_name(geo, "torus-geo");
	p_node_o_link_set(obj, geo, "geometry", 0u);

	lay = p_node_g_layer_find(geo, "vertex");
	for(i = 0; i < s1; i++)
	{
		real64	oa = i * (2 * M_PI / s1);
		for(j = 0; j < s2; j++)
		{
			real64	ia = j * (2 * M_PI) / s2, vtx[3];

			vtx[0] = r2 + r * cos(ia);
			vtx[1] = r * sin(ia);
			vtx[2] = 0.0;
			rotate_around_y(vtx, oa);
			p_node_g_vertex_set_xyz(lay, i * s2 + j, vtx[0], vtx[1], vtx[2]);
		}
	}

	lay = p_node_g_layer_find(geo, "polygon");
	for(i = 0; i < s1; i++)
	{
		next = (i + 1) % s1;
		for(j = 0; j < s2 - 1; j++)
		{
			p_node_g_polygon_set_corner_uint32(lay, i * s2 + j,
							   i * s2 + j,
							   i * s2 + j + 1,
							   next * s2 + j + 1,
							   next * s2 + j);
		}
		p_node_g_polygon_set_corner_uint32(lay, i * s2 + s2 - 1,
						   i * s2,
						   next * s2,
						   next * s2 + s2 - 1,
						   i * s2 + s2 - 1);
	}
	return P_COMPUTE_DONE;
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("torus");
	p_init_input(0, P_VALUE_REAL32, "radius, inner", P_INPUT_REQUIRED, P_INPUT_MIN(0.1), P_INPUT_DEFAULT(0.5), P_INPUT_DONE);
	p_init_input(1, P_VALUE_REAL32, "radius, outer", P_INPUT_REQUIRED, P_INPUT_MIN(0.2), P_INPUT_DEFAULT(1.0), P_INPUT_DONE);
	p_init_input(2, P_VALUE_UINT32, "tube splits",   P_INPUT_REQUIRED, P_INPUT_MIN(3), P_INPUT_MAX(128), P_INPUT_DEFAULT(8), P_INPUT_DONE);
	p_init_input(3, P_VALUE_UINT32, "ring splits",   P_INPUT_REQUIRED, P_INPUT_MIN(3), P_INPUT_MAX(128), P_INPUT_DEFAULT(5), P_INPUT_DONE);
	p_init_compute(compute);
}
