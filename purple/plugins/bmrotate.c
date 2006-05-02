/*
 * A bitmap Purple plug-in that rotates a bitmap. Does not grow the bitmap, so it's perhaps easier to
 * think of it as a rotating window that looks into a static bitmap. Or a bitmap rotating around it's
 * center, or something.
*/

#define _USE_MATH_DEFINES
#include <math.h>

#include <stdio.h>

#include "purple.h"

/* Macro to boundary-check and then sample a single pixel. Adds color to 'p' while increasing 'c'. */
#define	SAMPLE(fb,w,h,x,y,ch,p,c)	\
	do {\
		if(x < 0 || y < 0 || x >= w || y >= h)\
			;\
		else\
		{\
			p += fb[((int) (y)) * 3 * w + 3 * ((int) (x)) + ch];\
			c++;\
		}\
	} while(0)

static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	PINode		*in;
	PONode		*out;
	uint16		dim[3], width, height, depth;
	real32		angle;
	const real32	*fb;
	uint8		*fbout = NULL;
	char		nbuf[128];

	in = p_input_node(input[0]);
	if(p_node_get_type(in) != V_NT_BITMAP)
	{
		printf(" bad node input %d\n", p_node_get_type(in));
		return P_COMPUTE_DONE;
	}
	angle = p_input_real32(input[1]);
	if(angle < -180.0)
		angle = -180.0;
	else if(angle > 180.0)
		angle = 180.0;
	angle *= M_PI / 180.0;

	/* Compute output size; only operate on common pixels of inputs. No scaling. */
	p_node_b_get_dimensions(in, &width, &height, &depth);
	if(depth != 1)
	{
		printf("rotate: not rotating 3D bitmap, sorry\n");
		return P_COMPUTE_DONE;
	}

	/* Create output node. */
	out = p_output_node_create(output, V_NT_BITMAP, 0);
	snprintf(nbuf, sizeof nbuf, "rotated %s", p_node_get_name(in));
	p_node_set_name(out, nbuf);
	p_node_b_set_dimensions(out, width, height, depth);
	p_node_b_layer_create(out, "color_r", VN_B_LAYER_UINT8);
	p_node_b_layer_create(out, "color_g", VN_B_LAYER_UINT8);
	p_node_b_layer_create(out, "color_b", VN_B_LAYER_UINT8);

	/* Begin access to RGB layers in sources and destination. */
	if((fb = p_node_b_layer_read_multi_begin((PONode *) in, VN_B_LAYER_REAL32, "color_r", "color_g", "color_b", NULL)) != NULL &&
	   (fbout = p_node_b_layer_write_multi_begin(out, VN_B_LAYER_UINT8, "color_r", "color_g", "color_b", NULL)) != NULL)
	{
		int	x, y, cx = width / 2, cy = height / 2, j, cnt;
		real32	dx, dy, rx, ry, sx, sy, pix;
		uint8	*put = fbout;

		dx = cos(angle);
		dy = -sin(angle);

		for(y = 0; y < height; y++)
		{
			for(x = 0; x < width; x++)
			{
				/* Classic (expensive) rotation by reverse matrix, and then sampling the source. */
				rx = x - cx;
				ry = y - cy;
				sx = rx *  dx + ry * dy;
				sy = rx * -dy + ry * dx;
				sx += cx;
				sy += cy;
				for(j = 0; j < 3; j++)
				{
					pix = 0.0f;
					cnt = 0;
					SAMPLE(fb, width, height, sx + 0.5f, sy + 0.5f, j, pix, cnt);
					SAMPLE(fb, width, height, sx - 1.0f, sy, j, pix, cnt);
					SAMPLE(fb, width, height, sx + 1.0f, sy, j, pix, cnt);
					SAMPLE(fb, width, height, sx, sy - 1.0f, j, pix, cnt);
					SAMPLE(fb, width, height, sx, sy + 1.0f, j, pix, cnt);
					*put++ = cnt ? 255.0 * pix / cnt : 0u;
				}
			}
		}
	}
	else
		printf("rotate: couldn't set up operation (%p %p)\n", fb, fbout);
	/* Finalize accesses. */
	if(fb != NULL)
		p_node_b_layer_read_multi_end(in, fb);
	if(fbout != NULL)
		p_node_b_layer_write_multi_end(out, fbout);

	return P_COMPUTE_DONE;
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("bmrotate");
	p_init_input(0, P_VALUE_MODULE, "bitmap", P_INPUT_REQUIRED, P_INPUT_DESC("The bitmap to rotate."), P_INPUT_DONE);
	p_init_input(1, P_VALUE_REAL32, "angle",   P_INPUT_REQUIRED, P_INPUT_DEFAULT(0.0), P_INPUT_MIN(-180.0), P_INPUT_MAX(180.0),
		     P_INPUT_DESC("Rotation angle, in degrees."),
		     P_INPUT_DONE);
	p_init_meta("authors", "Emil Brink");
	p_init_meta("desc/purpose", "Rotates the input bitmap by the given amount, and outputs the result. Does not resize the bitmap.");
	p_init_compute(compute);
}
