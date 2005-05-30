/*
 * This plug-in computes the bounding box of input geometry.
*/

#include "purple.h"

static int vertex_deleted(const real64 *vtx)
{
	return vtx != NULL && fabs(*vtx) > 1e300;
}

/* This gets called whenever the input changes. Traverse vertices and compute bbox. */
static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	PINode		*in;
	size_t		i, j, k, size;
	real64		min[4] = { 1E20, 1E20, 1E20, 0.0 }, max[3] = { -1E20, -1E20, -1E20 }, point[3];

	printf("computing bbox\n");
	for(i = 0; (in = p_input_node_nth(input[0], i)) != NULL; i++)
	{
		const PNGLayer *layer;

		/* If object, check its 'geometry' link. */
		if(p_node_get_type(in) == V_NT_OBJECT)
		{
			in = p_node_o_link_get(in, "geometry", 0);
			printf("geometry link followed, got %p\n", in);
		}
		if(p_node_get_type(in) != V_NT_GEOMETRY)
			continue;
		layer = p_node_g_layer_find(in, "vertex");
		size = p_node_g_layer_get_size(layer);		/* Safely handles NULL layer. */
		for(j = 0; j < size; j++)
		{
			p_node_g_vertex_get_xyz(layer, j, point, point + 1, point + 2);
			if(vertex_deleted(point))
				continue;
			for(k = 0; k < sizeof point / sizeof *point; k++)
			{
				if(point[k] > max[k])
					max[k] = point[k];
				else if(point[k] < min[k])
					min[k] = point[k];
			}
		}
		p_output_real64_vec3(output, max);
		p_output_real64_vec4(output, min);	/* Slightly hackish. */
		printf("bbox computed: (%g,%g,%g)-(%g,%g,%g)\n",
		       min[0], min[1], min[2],
		       max[0], max[1], max[2]);
		break;		/* Only process first geometry node. */
	}
	return P_COMPUTE_DONE;
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("bbox");
	p_init_input(0, P_VALUE_MODULE, "data", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_compute(compute);
}
