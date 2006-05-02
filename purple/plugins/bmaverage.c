/*
 * A bitmap Purple plug-in that averages the contents of a bitmap, thus graying it out.
*/

#include <stdio.h>

#include "purple.h"

static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	PINode	*inbmp;
	PONode	*bitmap;
	void	*fb;
	uint8	*fb8;

	inbmp = p_input_node(input[0]);
	if(p_node_get_type(inbmp) != V_NT_BITMAP)
		return P_COMPUTE_DONE;
	bitmap = p_output_node_copy(output, inbmp, 0);

/*	{
		PNBLayer	*lay = p_node_b_layer_lookup(bitmap, "color_r");

		if((fb8 = p_node_b_layer_access_begin(bitmap, lay)) != NULL)
		{
			uint16	w, h, d, x, y;

			p_node_b_get_dimensions(bitmap, &w, &h, &d);
			printf("dimensions are %ux%ux%u, framebuffer at %p\n", w, h, d, fb8);
			for(y = 0; y < w; y++)
			{
				for(x = 0; x < h; x++)
				{
					printf(" %02X", fb8[y * w + x]);
				}
				printf("\n");
			}
			p_node_b_layer_access_end(bitmap, lay, fb8);
		}
	}
*/
	if((fb = p_node_b_layer_write_multi_begin(bitmap, VN_B_LAYER_UINT8, "color_r", "color_g", "color_b", NULL)) != NULL)
	{
		uint16	w, h, d, x, y, here;
		uint8	*get;

		p_node_b_get_dimensions(bitmap, &w, &h, &d);
		for(y = 0; y < w; y++)
		{
			get = (uint8 *) fb + y * 3* w;
			for(x = 0; x < h; x++, get += 3)
			{
				here = (get[0] + get[1] + get[2]) / 3;
				get[0] = here;
				get[1] = here;
				get[2] = here;
/*				printf(" (%02X,%02X,%02X)", get[0], get[1], get[2]);*/
			}
/*			printf("\n");*/
		}
		p_node_b_layer_write_multi_end(bitmap, fb);
	}

	{
		PNBLayer	*lay = p_node_b_layer_find(bitmap, "color_r");

		if((fb8 = p_node_b_layer_access_begin(bitmap, lay)) != NULL)
		{
			uint16	w, h, d, x, y;

			p_node_b_get_dimensions(bitmap, &w, &h, &d);
			printf("dimensions are %ux%ux%u, framebuffer at %p\n", w, h, d, fb8);
			for(y = 0; y < w; y++)
			{
				for(x = 0; x < h; x++)
					printf(" %02X", fb8[y * w + x]);
				printf("\n");
			}
			p_node_b_layer_access_end(bitmap, lay, fb8);
		}
	}
	return P_COMPUTE_DONE;
}

void init(void)
{
	p_init_create("bmaverage");
	p_init_input(0, P_VALUE_MODULE, "bitmap", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_compute(compute);
}
