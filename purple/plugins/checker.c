/*
 * A bitmap Purple plug-in that creates a checkerboard pattern. Silly.
*/

#include "purple.h"
#include "purple-plugin.h"

static uint8 check(uint32 side, uint32 size, uint32 x, uint32 y)
{
	uint32	row = y / size, col = x / size;

	if((row & 1) == 0)
		return !(col & 1);
	return (col & 1);
}

static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	const char	*lname[] = { "col_r", "col_g", "col_b" };
	PONode		*node;
	PNBLayer	*layer;
	uint32		side = p_input_uint32(input[0]), size = p_input_uint32(input[1]);
	uint8		*frame, i;

	node = p_output_node_create(output, V_NT_BITMAP, 0);
	p_node_b_dimensions_set(node, side, side, 1);
	for(i = 0; i < sizeof lname / sizeof *lname; i++)
	{
		layer = p_node_b_layer_create(node, lname[i], VN_B_LAYER_UINT8);
		if((frame = p_node_b_layer_access_begin(node, layer)) != NULL)
		{
			uint32	x, y;

			for(y = 0; y < side; y++)
			{
				uint8	*row = frame + y * side;

				for(x = 0; x < side; x++)
					*row++ = check(side, size, x, y) ? 0xFF : 0x00;
			}
			p_node_b_layer_access_end(node, layer, frame);
		}
	}
	return P_COMPUTE_DONE;
}

void init(void)
{
	p_init_create("checker");
	p_init_input(0, P_VALUE_UINT32, "side", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(1, P_VALUE_UINT32, "size", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_compute(compute);
}
