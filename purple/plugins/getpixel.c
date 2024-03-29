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
	const char	*ln[] = { "color_r", "color_g", "color_b" };
	const real64	*xyz;
	real64		out[3];
	int		i;

	if((bitmap = p_input_node_first_type(input[0], V_NT_BITMAP)) == NULL)
		return P_COMPUTE_DONE;
	xyz = p_input_real64_vec3(input[1]);
	for(i = 0; i < 3; i++)
	{
		const PNBLayer	*lay;

		if(p_input_boolean(input[2 + i]))	/* Skip disabled layers. */
		{
			lay = p_node_b_layer_find(bitmap, ln[i]);
			if(lay != NULL)
				out[i] = p_node_b_layer_pixel_read(bitmap, lay, xyz[0], xyz[1], xyz[2]);
		}
		else
			out[i] = 0.0;
	}
	p_output_real64_vec3(output, out);
	return P_COMPUTE_DONE;
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("getpixel");
	p_init_input(0, P_VALUE_MODULE,      "bitmap", P_INPUT_DESC("The first bitmap found in the "
								    "input set of nodes will be accessed."), P_INPUT_DONE);
	p_init_input(1, P_VALUE_REAL64_VEC3, "position", P_INPUT_DESC("The (x,y,z) position in the bitmap to access. "
								      "Coordinates are rounded to integer before the access."), P_INPUT_DONE);
	p_init_input(2, P_VALUE_BOOLEAN,     "red",   P_INPUT_DESC("Include red color component in the result?"),   P_INPUT_DEFAULT(1), P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(3, P_VALUE_BOOLEAN,     "green", P_INPUT_DESC("Include green color component in the result?"), P_INPUT_DEFAULT(1), P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(4, P_VALUE_BOOLEAN,     "blue",  P_INPUT_DESC("Include blue color component in the result?"),  P_INPUT_DEFAULT(1), P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_meta("authors", "Emil Brink");
	p_init_meta("copyright", "2005 PDC, KTH");
	p_init_meta("desc/purpose", "Retrieves the color of a given pixel in a bitmap, and outputs it as a 3D vector. Inputs control which color channel(s) are to be included.");
	p_init_compute(compute);
}
