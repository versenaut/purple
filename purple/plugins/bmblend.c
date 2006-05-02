/*
 * A bitmap Purple plug-in that blends two bitmap nodes together, using standard alpha-blending.
*/

#include <stdio.h>

#include "purple.h"

#define	MIN(a, b)	(((a) < (b)) ? (a) : (b))

static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	PINode		*in1, *in2;
	PONode		*out;
	uint16		dim1[3], dim2[3], width, height, depth;
	real32		alpha, alphainv;
	const uint8	*fb1 = NULL, *fb2 = NULL;
	uint8		*fbout = NULL;

	in1 = p_input_node(input[0]);
	in2 = p_input_node(input[1]);
	if(p_node_get_type(in1) != V_NT_BITMAP || p_node_get_type(in2) != V_NT_BITMAP)
	{
		printf(" bad node input %d %d\n", p_node_get_type(in1), p_node_get_type(in2));
		return P_COMPUTE_DONE;
	}
	alpha = p_input_real32(input[2]);
	if(alpha < 0.0 || alpha > 1.0)
	{
		printf(" bad input alpha %g\n", alpha);
		return P_COMPUTE_DONE;
	}
	alphainv = 1.0 - alpha;

	/* Compute output size; only operate on common pixels of inputs. No scaling. */
	p_node_b_get_dimensions(in1, dim1, dim1 + 1, dim1 + 2);
	p_node_b_get_dimensions(in2, dim2, dim2 + 1, dim2 + 2);
	width  = MIN(dim1[0], dim2[0]);
	height = MIN(dim1[1], dim2[1]);
	depth  = MIN(dim1[2], dim2[2]);

	/* Create output node. */
	out = p_output_node_create(output, V_NT_BITMAP, 0);
	p_node_b_set_dimensions(out, width, height, depth);
	p_node_b_layer_create(out, "color_r", VN_B_LAYER_UINT8);
	p_node_b_layer_create(out, "color_g", VN_B_LAYER_UINT8);
	p_node_b_layer_create(out, "color_b", VN_B_LAYER_UINT8);

	/* Begin access to RGB layers in sources and destination. */
	if((fb1 = p_node_b_layer_read_multi_begin((PONode *) in1, VN_B_LAYER_UINT8, "color_r", "color_g", "color_b", NULL)) != NULL &&
	   (fb2 = p_node_b_layer_read_multi_begin((PONode *) in2, VN_B_LAYER_UINT8, "color_r", "color_g", "color_b", NULL)) != NULL &&
	   (fbout = p_node_b_layer_write_multi_begin(out, VN_B_LAYER_UINT8, "color_r", "color_g", "color_b", NULL)) != NULL)
	{
		int	x, y, z;

		for(z = 0; z < depth; z++)
		{
			for(y = 0; y < height; y++)
			{
				const uint8	*r1  = fb1 + 3 * z * dim1[0] * dim1[1] + y * 3 * dim1[0],
						*r2  = fb2 + 3 * z * dim2[0] * dim2[1] + y * 3 * dim2[0];
				uint8		*put = fbout + 3 * z * width * height + y * 3 * width;

				for(x = 0; x < 3 * width; x++)
					*put++ = alpha * *r2++ + alphainv * *r1++;	/* Simple blend equation. */
			}
		}
	}
	else
		printf("blblend: couldn't set up operation (%p %p %p)\n", fb1, fb2, fbout);
	/* Finalize accesses. */
	if(fb1 != NULL)
		p_node_b_layer_read_multi_end(in1, fb1);
	if(fb2 != NULL)
		p_node_b_layer_read_multi_end(in2, fb2);
	if(fbout != NULL)
		p_node_b_layer_write_multi_end(out, fbout);

	return P_COMPUTE_DONE;
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("bmblend");
	p_init_input(0, P_VALUE_MODULE, "bitmap1", P_INPUT_REQUIRED, P_INPUT_DESC("The first bitmap is input here."), P_INPUT_DONE);
	p_init_input(1, P_VALUE_MODULE, "bitmap2", P_INPUT_REQUIRED, P_INPUT_DESC("The second bitmap is input here."), P_INPUT_DONE);
	p_init_input(2, P_VALUE_REAL32, "alpha",   P_INPUT_REQUIRED, P_INPUT_MIN(0), P_INPUT_MAX(1.0),
		     P_INPUT_DESC("Controls how much of each bitmap is used in the output. "
				  "the blending equation is: out = (1 - alpha) * bitmap1 + alpha * bitmap2. "
				  "So, in other words, the output goes from bitmap1 to bitmap2 as the alpha "
				  "goes from zero to one."),
		     P_INPUT_DONE);
	p_init_meta("authors", "Emil Brink");
	p_init_meta("desc/purpose", "Computes a straightforward \"alpha blend\" between two input images. Uses a third input, the alpha, to control "
				    "how much of each is to appear on the output.");
	p_init_compute(compute);
}
