/*
 * This plug-in tries to achieve some kind of "warp" or twist effect.
*/

#include <math.h>

#include "purple.h"
#include "purple-plugin.h"

static void rot_matrix_build_y(real64 *mtx, real64 angle)
{
	mtx[0] = cos(angle);
	mtx[1] = 0.0;
	mtx[2] = -sin(angle);
	mtx[3] = 0.0;
	mtx[4] = 1.0;
	mtx[5] = 0.0;
	mtx[6] = -mtx[2];
	mtx[7] = 0.0;
	mtx[8] = mtx[0];
}

static void point_rotate(real64 *xyz, const real64 *mtx)
{
	real64	tmp[3];

	tmp[0] = xyz[0] * mtx[0] + xyz[1] * mtx[1] + xyz[2] * mtx[2];
	tmp[1] = xyz[0] * mtx[3] + xyz[1] * mtx[4] + xyz[2] * mtx[5];
	tmp[2] = xyz[0] * mtx[6] + xyz[1] * mtx[7] + xyz[2] * mtx[8];

	memcpy(xyz, tmp, sizeof tmp);
}

/* This gets called whenever input changes. Traverse vertices and warp them. */
static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	PINode		*in, *ingeo;
	PONode		*obj, *geo;
	size_t		i, j, k, size;
	const real64	*min, *max;
	real64		point[3], ytot, yrel, yrelprev, matrix[9], twist;

	max   = p_input_real64_vec3(input[1]);
	min   = p_input_real64_vec4(input[1]);
	twist = p_input_real64(input[2]);
	ytot  = max[1] - min[1];
	printf("warp: max=(%g,%g,%g), min=(%g,%g,%g), twist=%g\n", max[0], max[1], max[2], min[0], min[1], min[2], twist);

	for(i = 0; (in = p_input_node_nth(input[0], i)) != NULL; i++)
	{
		PNGLayer *inlayer, *outlayer;

		if(p_node_type_get(in) != V_NT_OBJECT)
			continue;
		if((ingeo = p_node_o_link_get(in, "geometry", 0)) == NULL)
			continue;
		if(p_node_type_get(ingeo) != V_NT_GEOMETRY)
			continue;
		obj = p_output_node_copy(output, in, 0);
		geo = p_output_node_copy(output, ingeo, 1);
		p_node_o_link_set(obj, geo, "geometry", 0);

		inlayer  = p_node_g_layer_find(ingeo, "vertex");
		printf(" input layer at %p\n", inlayer);
		outlayer = p_node_g_layer_find(geo, "vertex");
		size  = p_node_g_layer_size(inlayer);		/* Safely handles NULL layer. */
		for(j = 0, yrelprev = -1E20; j < size; j++, yrelprev = yrel)
		{
			p_node_g_vertex_get_xyz(inlayer, j, point, point + 1, point + 2);
			yrel = (point[1] - min[1]) / ytot;
			if(yrel != yrelprev)
				rot_matrix_build_y(matrix, yrel * twist);
			printf(" point (%g,%g,%g) gets yrel=%g -> ", point[0], point[1], point[2], yrel);
			point_rotate(point, matrix);
			printf("warped into (%g,%g,%g)\n", point[0], point[1], point[2]);
			p_node_g_vertex_set_xyz(outlayer, j, point[0], point[1], point[2]);								    
		}
		break;
	}
	return P_COMPUTE_DONE;
}

void init(void)
{
	p_init_create("warp");
	p_init_input(0, P_VALUE_MODULE, "data",  P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(1, P_VALUE_MODULE, "bbox",  P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(2, P_VALUE_REAL64, "twist", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_compute(compute);
}
