/*
 * A simple accessor plug-in that reads out the specified coordinate value from a bitmap.
 * The result is output as a real64 vector of length three, with the element values
 * corresponding to the value of the pixel at that point in the red, green and blue layers
 * of the bitmap, respectively.
*/

#include "purple.h"

static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	PINode		*bitmap;
	const char	*ln[] = { "col_r", "col_g", "col_b" };
	const real64	*xyz;
	real64		out[3];
	int		i;

	if((bitmap = p_input_node_first_type(input[0], V_NT_BITMAP)) == NULL)
		return P_COMPUTE_DONE;
	xyz = p_input_real64_vec3(input[1]);
	for(i = 0; i < 3; i++)
	{
		const PNBLayer	*l = p_node_b_layer_find(bitmap, ln[i]);

		if(l != NULL)
			out[i] = p_node_b_layer_pixel_read(bitmap, l, xyz[0], xyz[1], xyz[2]);
		else
			out[i] = 0.0;
	}
	p_output_real64_vec3(output, out);
	return P_COMPUTE_DONE;
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("getpixel");
	p_init_input(0, P_VALUE_MODULE, "bitmap", "The first bitmap found in the input set of nodes will be accessed.", P_INPUT_DONE);
	p_init_input(1, P_VALUE_REAL32_VEC3, "position", "The (x,y,z) position in the bitmap to access. Coordinates are rounded to integer before the access.", P_INPUT_DONE);
	p_init_compute(compute);
}
