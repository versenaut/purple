/*
 * A bitmap Purple plug-in that filters two bitmaps, supporting plenty of "industry-
 * standard" filtering modes.
 *
 * Thanks to <http://www.pegtop.net/delphi/blendmodes/> for the list of modes.
*/

#include "purple.h"

#define	MAX(a, b)	(((a) > (b)) ? (a) : (b))
#define	MIN(a, b)	(((a) < (b)) ? (a) : (b))

static void filter_normal(uint8 *put, const uint8 *a, const uint8 *b)
{
	put[0] = a[0];
	put[1] = a[1];
	put[2] = a[2];
}

static void filter_average(uint8 *put, const uint8 *a, const uint8 *b)
{
	put[0] = (a[0] + b[0]) / 2;
	put[1] = (a[1] + b[1]) / 2;
	put[2] = (a[2] + b[2]) / 2;
}

static void filter_multiply(uint8 *put, const uint8 *a, const uint8 *b)
{
	put[0] = (a[0] * b[0]) / 256;
	put[1] = (a[1] * b[1]) / 256;
	put[2] = (a[2] * b[2]) / 256;
}

static void filter_screen(uint8 *put, const uint8 *a, const uint8 *b)
{
	put[0] = 255 - ((255 - a[0]) * (255 - b[0])) / 256;
	put[1] = 255 - ((255 - a[1]) * (255 - b[1])) / 256;
	put[2] = 255 - ((255 - a[2]) * (255 - b[2])) / 256;
}

static void filter_darken(uint8 *put, const uint8 *a, const uint8 *b)
{
	put[0] = MIN(a[0], b[0]);
	put[1] = MIN(a[1], b[1]);
	put[2] = MIN(a[2], b[2]);
}

static void filter_lighten(uint8 *put, const uint8 *a, const uint8 *b)
{
	put[0] = MAX(a[0], b[0]);
	put[1] = MAX(a[1], b[1]);
	put[2] = MAX(a[2], b[2]);
}

static void filter_difference(uint8 *put, const uint8 *a, const uint8 *b)
{
	put[0] = a[0] < b[0] ? b[0] - a[0] : a[0] - b[0];
	put[1] = a[1] < b[1] ? b[1] - a[1] : a[1] - b[1];
	put[2] = a[2] < b[2] ? b[2] - a[2] : a[2] - b[2];
}

static void filter_negation(uint8 *put, const uint8 *a, const uint8 *b)
{
	register unsigned int	t;

	t = 255 - a[0] - b[0];
	if(t & (1 << 31))
		t = ~t + 1;
	put[0] = 255 - t;
	t = 255 - a[1] - b[1];
	if(t & (1 << 31))
		t = ~t + 1;
	put[1] = 255 - t;
	t = 255 - a[2] - b[2];
	if(t & (1 << 31))
		t = ~t + 1;
	put[2] = 255 - t;
}

static void filter_exclusion(uint8 *put, const uint8 *a, const uint8 *b)
{
	put[0] = a[0] + b[0]  - (a[0] * b[0]) / 128;
	put[1] = a[1] + b[1]  - (a[1] * b[1]) / 128;
	put[2] = a[2] + b[2]  - (a[2] * b[2]) / 128;
}

static void filter_overlay(uint8 *put, const uint8 *a, const uint8 *b)
{
	register int	i;

	for(i = 0; i < 3; i++)
	{
		if(a[i] < 128)
			put[i] = (a[i] * b[i]) / 128;
		else	
			put[i] = 255 - ((255 - a[i]) * (255 - b[i]) / 128);
	}
}

static void filter_hard_light(uint8 *put, const uint8 *a, const uint8 *b)
{
	register int	i;

	for(i = 0; i < 3; i++)
	{
		if(b[i] < 128)
			put[i] = (a[i] * b[i]) / 128;
		else
			put[i] = 255 - ((255 - b[i]) * (255 - a[i]) / 128);
	}
}

static void filter_xfader_hard_light(uint8 *put, const uint8 *a, const uint8 *b)
{
	register unsigned int	i, c;

	for(i = 0; i < 3; i++)
	{
		c = (a[i] * b[i]) / 256;
		put[i] = c + a[i] * (255 - ((255 - a[i]) * (255 - b[i]) / 256) - c) / 256;
	}
}

static void filter_color_dodge(uint8 *put, const uint8 *a, const uint8 *b)
{
	register unsigned int	i, c;

	for(i = 0; i < 3; i++)
	{
		if(b[i] == 255)
			put[i] = 255;
		else
		{
			c = (a[i] * 256) / (255 - b[i]);
			if(c > 255)
				put[i] = 255;
			else
				put[i] = c;
		}
	}
}

static void filter_color_burn(uint8 *put, const uint8 *a, const uint8 *b)
{
	register int	i, c;

	for(i = 0; i < 3; i++)
	{
		if(b == 0)
			put[i] = 0;
		else
		{
			c = 255 - (((255 - a[0]) * 256) / b[i]);
			if(c < 0)
				put[i] = 0;
			else
				put[i] = c;
		}
	}
}

static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	PINode		*in1, *in2;
	PONode		*out;
	uint16		dim1[3], dim2[3], width, height, depth;
	uint32		mode;
	const uint8	*fb1 = NULL, *fb2 = NULL;
	uint8		*fbout = NULL;
	void		(*filter)(uint8 *put, const uint8 *a, const uint8 *b);

	in1 = p_input_node(input[0]);
	in2 = p_input_node(input[1]);
	if(p_node_get_type(in1) != V_NT_BITMAP || p_node_get_type(in2) != V_NT_BITMAP)
	{
		printf(" bad node input %d %d\n", p_node_get_type(in1), p_node_get_type(in2));
		return P_COMPUTE_DONE;
	}

	mode = p_input_uint32(input[2]);
	switch(mode)
	{
	case 0:	filter = filter_normal;		break;
	case 1: filter = filter_average;	break;
	case 2: filter = filter_multiply;	break;
	case 3: filter = filter_screen;		break;
	case 4: filter = filter_darken;		break;
	case 5: filter = filter_lighten;	break;
	case 6: filter = filter_difference;	break;
	case 7: filter = filter_negation;	break;
	case 8: filter = filter_exclusion;	break;
	case 9: filter = filter_overlay;	break;
	case 10: filter = filter_hard_light;	break;
	case 11: filter = filter_xfader_hard_light;	break;
	case 12: filter = filter_color_dodge;	break;
	default:	filter = NULL;
	}
	if(filter == NULL)
	{
		printf("bmfilter: Unknown mode %u\n", mode);
		return P_COMPUTE_DONE;
	}


	/* Compute output size; only operate on common pixels of inputs. No scaling. */
	p_node_b_get_dimensions(in1, dim1, dim1 + 1, dim1 + 2);
	p_node_b_get_dimensions(in2, dim2, dim2 + 1, dim2 + 2);
	width  = MIN(dim1[0], dim2[0]);
	height = MIN(dim1[1], dim2[1]);
	depth  = MIN(dim1[2], dim2[2]);

	/* Create output node. */
	out = p_output_node_create(output, V_NT_BITMAP, 0);
	p_node_b_set_dimensions(out, width, height, depth);
	p_node_b_layer_create(out, "col_r", VN_B_LAYER_UINT8);
	p_node_b_layer_create(out, "col_g", VN_B_LAYER_UINT8);
	p_node_b_layer_create(out, "col_b", VN_B_LAYER_UINT8);

	/* Begin access to RGB layers in sources and destination. */
	if((fb1 = p_node_b_layer_read_multi_begin((PONode *) in1, VN_B_LAYER_UINT8, "col_r", "col_g", "col_b", NULL)) != NULL &&
	   (fb2 = p_node_b_layer_read_multi_begin((PONode *) in2, VN_B_LAYER_UINT8, "col_r", "col_g", "col_b", NULL)) != NULL &&
	   (fbout = p_node_b_layer_write_multi_begin(out, VN_B_LAYER_UINT8, "col_r", "col_g", "col_b", NULL)) != NULL)
	{
		int	x, y, z;

		for(z = 0; z < depth; z++)
		{
			for(y = 0; y < height; y++)
			{
				const uint8	*r1  = fb1 + 3 * z * dim1[0] * dim1[1] + y * 3 * dim1[0],
						*r2  = fb2 + 3 * z * dim2[0] * dim2[1] + y * 3 * dim2[0];
				uint8		*put = fbout + 3 * z * width * height + y * 3 * width;

				for(x = 0; x < width; x++)
				{
					filter(put, r1, r2);
					put += 3;
					r1 += 3;
					r2 += 3;
				}
			}
		}
	}
	else
		printf("bmfilter: couldn't set up operation (%p %p %p)\n", fb1, fb2, fbout);
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
	p_init_create("bmfilter");
	p_init_input(0, P_VALUE_MODULE, "bitmap1", P_INPUT_REQUIRED, P_INPUT_DESC("The first bitmap is input here."), P_INPUT_DONE);
	p_init_input(1, P_VALUE_MODULE, "bitmap2", P_INPUT_REQUIRED, P_INPUT_DESC("The second bitmap is input here."), P_INPUT_DONE);
	p_init_input(2, P_VALUE_UINT32, "mode", P_INPUT_REQUIRED, P_INPUT_DEFAULT(0.0),
		     P_INPUT_ENUM("0:Normal|"
				  "1:Average|"
				  "2:Multiply|"
				  "3:Screen|"
				  "4:Darken|"
				  "5:Lighten|"
				  "6:Difference|"
				  "7:Negation|"
				  "8:Exclusion|"
				  "9:Overlay|"
				  "10:Hard Light|"
				  "11:Hard Light2|"
				  "12:Color Dodge"), P_INPUT_DONE);
	p_init_meta("authors", "Emil Brink");
	p_init_meta("desc/purpose", "Computes bitmap filter operation, on two sources. Supports several \"modes\" that affect how the result will look.");
	p_init_compute(compute);
}
