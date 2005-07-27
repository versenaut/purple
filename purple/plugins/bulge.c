/*
 * This plug-in tries to achieve some kind of "warp" or twist effect.
*/

#include <math.h>

#include "purple.h"

/* This gets called whenever input changes. Traverse vertices and warp them. */
static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	PINode		*in, *ingeo;
	PONode		*obj, *geo;
	size_t		i, j, k, size;
	const real64	*pos, *vec;
	real64		radius, point[3], tmp[3], dist;

	pos    = p_input_real64_vec3(input[1]);
	vec    = p_input_real64_vec3(input[2]);
	radius = p_input_real64(input[3]);

	printf("bulge: pos=(%g,%g,%g), vec=(%g,%g,%g), radius=%g\n", pos[0], pos[1], pos[2], vec[0], vec[1], vec[2], radius);

	for(i = 0; (in = p_input_node_nth(input[0], i)) != NULL; i++)
	{
		PNGLayer *inlayer, *outlayer;

		if(p_node_get_type(in) != V_NT_OBJECT)
			continue;
		ingeo = p_node_o_link_get(in, "geometry", 0);
		if(p_node_get_type(ingeo) != V_NT_GEOMETRY)
			continue;
		obj = p_output_node_copy(output, in, 0);
		geo = p_output_node_copy(output, ingeo, 1);
		p_node_o_link_set(obj, geo, "geometry", 0);

		inlayer  = p_node_g_layer_find(ingeo, "vertex");
		outlayer = p_node_g_layer_find(geo, "vertex");
		size  = p_node_g_layer_get_size(inlayer);		/* Safely handles NULL layer. */
		for(j = 0; j < size; j++)
		{
			p_node_g_vertex_get_xyz(inlayer, j, point, point + 1, point + 2);
			tmp[0] = (point[0] - pos[0]) / radius;
			tmp[1] = (point[1] - pos[1]) / radius;
			tmp[2] = (point[2] - pos[2]) / radius;
			dist = tmp[0] * tmp[0] + tmp[1] * tmp[1] + tmp[2] * tmp[2];
			dist = 1.0 / (1.0 + dist);
			point[0] += vec[0] * dist;
			point[1] += vec[1] * dist;
			point[2] += vec[2] * dist;
			p_node_g_vertex_set_xyz(outlayer, j, point[0], point[1], point[2]);								    
		}
		break;
	}
	return P_COMPUTE_DONE;
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("bulge");
	p_init_input(0, P_VALUE_MODULE,      "data",     P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(1, P_VALUE_REAL64_VEC3, "position", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(2, P_VALUE_REAL64_VEC3, "vector",   P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(3, P_VALUE_REAL64,      "radius",   P_INPUT_REQUIRED, P_INPUT_DEFAULT(1.0), P_INPUT_DONE);
	p_init_compute(compute);
}
