/*
 * This plug-in measures size of first input geometry node.
*/

#include "purple.h"
#include "purple-plugin.h"

/* This gets called whenever the input changes. Traverse vertices and measure. */
static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	PINode		*in;
	size_t		i, j, k, size;
	real64		min[3] = { 1E20, 1E20, 1E20 }, max[3] = { -1E20, -1E20, -1E20 }, point[3];

	for(i = 0; (in = p_input_node_nth(input[0], i)) != NULL; i++)
	{
		const PNGLayer *layer;

		if(p_node_type_get(in) != V_NT_GEOMETRY)
			continue;
		layer = p_node_g_layer_find(in, "vertex");
		size = p_node_g_layer_get_size(layer);		/* Safely handles NULL layer. */
		for(j = 0; j < size; j++)
		{
			p_node_g_vertex_get_xyz(layer, j, point, point + 1, point + 2);
			for(k = 0; k < sizeof point / sizeof *point; k++)
			{
				if(point[k] > max[k])
					max[k] = point[k];
				else if(point[k] < min[k])
					min[k] = point[k];
			}
		}
		for(k = 0; k < sizeof max / sizeof *max; k++)
			max[k] -= min[k];
		p_output_real64_vec3(output, max);
		break;		/* Only process first geometry node. */
	}
	return P_COMPUTE_DONE;
}

void init(void)
{
	p_init_create("measure");
	p_init_input(0, P_VALUE_MODULE, "data", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_compute(compute);
}
