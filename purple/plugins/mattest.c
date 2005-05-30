/*
 * 
*/

#include "purple.h"

static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	PINode		*inbit;
	PONode		*outbit, *mat;
	PNMFragment	*out, *blender = NULL, *light, *texture, *geometry;
	PNMFragment	*color1, *color2, *matrix;
	const real64	mtx[] = { -1.0, 0.0, 0.0, 0.0,  0.0, 1.0, 0.0, 0.0,  0.0, 0.0, 1.0, 0.0,  0.0, 0.0, 0.0, 1.0 };
	uint16		bw, bh, bd;

	inbit = p_input_node(input[0]);
	if(p_node_get_type(inbit) != V_NT_BITMAP)
		return P_COMPUTE_DONE;
	outbit = p_output_node(output, inbit);
	p_node_b_get_dimensions(outbit, &bw, &bh, &bd);

	mat = p_output_node_create(output, V_NT_MATERIAL, 0);
	p_node_set_name(mat, "mattest");
	geometry = p_node_m_fragment_create_geometry(mat, "map_u", "map_v", NULL);
	texture  = p_node_m_fragment_create_texture(mat, outbit, "col_r", "col_g", "col_b", geometry);
	if(bw > 16)
	{
		printf("bitmap is big, better light it\n");
		light    = p_node_m_fragment_create_light(mat, VN_M_LIGHT_DIRECT_AND_AMBIENT, 0.0, NULL, NULL, NULL, NULL);
		blender  = p_node_m_fragment_create_blender(mat, VN_M_BLEND_MULTIPLY, light, texture, NULL);
	}
	output   = p_node_m_fragment_create_output(mat, "color", bw > 16 ? blender : texture, NULL);

/*	output   = p_node_m_fragment_create_output(mat, "color", NULL, NULL);

	color1   = p_node_m_fragment_create_color(mat, 0.6, 0.6, 0.8);
	color2   = p_node_m_fragment_create_color(mat, 0.8, 0.6, 0.6);
	blender  = p_node_m_fragment_create_blender(mat, VN_M_BLEND_MULTIPLY, color1, color2, color1);
	matrix   = p_node_m_fragment_create_matrix(mat, mtx, blender);
	output   = p_node_m_fragment_create_output(mat, "color", matrix, blender);
*/
	return P_COMPUTE_DONE;
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("mattest");
	p_init_input(0, P_VALUE_MODULE, "texture", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_compute(compute);
}
