/*
 * Simplistic displacement mapping.
*/

#define _USE_MATH_DEFINES
#include <math.h>

#include "purple.h"

/* --------------------------------------------------------------------------------------------- */

static void project_2d(real64 *flat, real64 *point)
{
	flat[0] = point[0];
	flat[1] = point[2];	/* Make Y = Z. */
}

static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	PINode		*in = NULL, *inobj = NULL, *ingeo = NULL, *inbm = NULL;
	PONode		*obj, *geo;
	size_t		i, size;
	real64		min[2], max[2], point[3], flat[2], v, scale;
	PNGLayer	*inlayer, *outlayer;
	const uint8	*pixel;
	uint16		dim[3], x, y;

	for(i = 0; (in = p_input_node_nth(input[0], i)) != NULL; i++)
	{
		PNGLayer *inlayer, *outlayer;

		if(p_node_get_type(in) != V_NT_OBJECT)
			continue;
		inobj = in;
		ingeo = p_node_o_link_get(in, "geometry", 0);
		if(p_node_get_type(ingeo) != V_NT_GEOMETRY)
			continue;
		break;
	}
	for(i = 0; (in = p_input_node_nth(input[1], i)) != NULL; i++)
	{
		if(p_node_get_type(in) == V_NT_BITMAP)
		{
			inbm = in;
			break;
		}
	}
	if(in == NULL || inbm == NULL || ingeo == NULL)
		return P_COMPUTE_DONE;

	scale = p_input_real64(input[2]);

	obj = p_output_node_copy(output, inobj, 0);
	geo = p_output_node_copy(output, ingeo, 1);
	p_node_o_link_set(obj, geo, "geometry", 0);

	inlayer  = p_node_g_layer_find(ingeo, "vertex");
	outlayer = p_node_g_layer_find(geo, "vertex");
	size     = p_node_g_layer_get_size(inlayer);		/* Safely handles NULL layer. */

	/* Compute projected bounding box. */
	min[0] = min[1] = 1E300;
	max[0] = max[1] = -1E300;
	for(i = 0; i < size; i++)
	{
		p_node_g_vertex_get_xyz(inlayer, i, point, point + 1, point + 2);
		project_2d(flat, point);
		if(flat[0] < min[0])
			min[0] = flat[0];
		if(flat[0] > max[0])
			max[0] = flat[0];
		if(flat[1] < min[1])
			min[1] = flat[1];
		if(flat[1] > max[1])
			max[1] = flat[1];
	}

	if((pixel = p_node_b_layer_read_multi_begin(inbm, VN_B_LAYER_UINT8, "col_r", NULL)) != NULL)
	{
		p_node_b_get_dimensions(inbm, dim, dim + 1, dim + 2);
		for(i = 0; i < size; i++)
		{
			p_node_g_vertex_get_xyz(inlayer, i, point, point + 1, point + 2);
			project_2d(flat, point);
			flat[0] = (flat[0] - min[0]) / (max[0] - min[0]);	/* Convert to UV space. */
			flat[1] = (flat[1] - min[1]) / (max[1] - min[1]);
			x = flat[0] * (dim[0] - 1);
			y = flat[1] * (dim[1] - 1);
			v = pixel[y * dim[0] + x] / 255.0;
			p_node_g_vertex_set_xyz(outlayer, i, point[0], point[1] + scale * v, point[2]);
		}
		p_node_b_layer_read_multi_end(inbm, pixel);
	}
	return P_COMPUTE_DONE;
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("displace");
	p_init_input(0, P_VALUE_MODULE, "geometry", P_INPUT_REQUIRED,
		     P_INPUT_DESC("The first object with a geometry link will have its geometry displaced."), P_INPUT_DONE);
	p_init_input(1, P_VALUE_MODULE, "map", P_INPUT_REQUIRED,
		     P_INPUT_DESC("The first bitmap node will be used as the displacement map."), P_INPUT_DONE);
	p_init_input(2, P_VALUE_REAL64, "scale", P_INPUT_DEFAULT(1.0), P_INPUT_MIN(0.01),
		     P_INPUT_DESC("A displacement scale factor."));
	p_init_meta("authors", "Emil Brink");
	p_init_meta("copyright", "2005 PDC, KTH");
	p_init_meta("desc/purpose", "Displaces geometry, using a bitmap as a displacement map.");
	p_init_compute(compute);
}
