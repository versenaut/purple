/*
 * This plug-in rescales input geometry node.
*/

#include "purple.h"
#include "purple-plugin.h"

/* This gets called whenever the input, the scale, changes. Traverse vertices and scale them. */
static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	PINode		*in;
	PONode		*out;
	real32		scale = p_input_real32(input[1]);	/* Read out the size. */;
	size_t		i, j, size;

	for(i = 0; (in = p_input_node_nth(input[0], i)) != NULL; i++)
	{
		PONode	*out;
		PNGLayer *layer;

		if(p_node_type_get(in) != V_NT_GEOMETRY)
			continue;
		out   = p_output_node(output, in);
		layer = p_node_g_layer_find(out, "vertex");
		size  = p_node_g_layer_size(layer);	/* Safely handles NULL layer. */
		for(j = 0; j < size; j++)
		{
			real64	x, y, z;
			p_node_g_vertex_get_xyz(layer, j, &x, &y, &z);
			p_node_g_vertex_set_xyz(layer, j, scale * x, scale * y, scale * z);
		}
	}
	return P_COMPUTE_DONE;	/* Sleep until scale changes. */
}

void init(void)
{
	p_init_create("scale");
	p_init_input(0, P_VALUE_MODULE, "data",  P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(1, P_VALUE_REAL32, "scale", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_compute(compute);
}
