/*
 * A simple accessor plug-in, that outputs the position of a single vertex
 * from the first geometry node found in the input.
*/

#include "purple.h"

static PComputeStatus compute(PPInput *input, PPOutput output, void *user)
{
	PINode		*geo;
	const PNGLayer	*vtx;
	uint32		index;
	real64		v[3];

	if((geo = p_input_node_first_type(input[0], V_NT_GEOMETRY)) == NULL)
		return P_COMPUTE_DONE;
	if((vtx = p_node_g_layer_find(geo, "vertex")) == NULL)
		return P_COMPUTE_DONE;
	index = p_input_uint32(input[1]);
	p_node_g_vertex_get_xyz(vtx, index, v + 0, v + 1, v + 2);
	p_output_real64_vec3(output, v);
	return P_COMPUTE_DONE;
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("getvertex");
	p_init_input(0, P_VALUE_MODULE, "node", P_INPUT_REQUIRED, P_INPUT_DESC("The first geometry node found in the input will have a vertex extracted."), P_INPUT_DONE);
	p_init_input(1, P_VALUE_UINT32, "index", P_INPUT_REQUIRED, P_INPUT_DEFAULT(0), P_INPUT_DESC("The integer index of the vertex to output, counting from 0."), P_INPUT_DONE);
	p_init_compute(compute);
}
