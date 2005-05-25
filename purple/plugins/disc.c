/*
 * A bitmap Purple plug-in that creates a filled cirle. Silly.
*/

#include "purple.h"

/* Answer whether (x,y) is within circle centered on a <user>-pixel square.
 * Uses simplistic integer math only, and thus isn't too precise.
*/
static real64 cb_disc(uint32 x, uint32 y, uint32 z, void *user)
{
	uint32	size = ((uint32) user) / 2;

	return (x - size) * (x - size) + (y - size) * (y - size) <= size * size;
}

static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	const char	*lname[] = { "col_r", "col_g", "col_b" };
	PONode		*node;
	PNBLayer	*layer;
	uint32		size = p_input_uint32(input[0]), i;

	node = p_output_node_create(output, V_NT_BITMAP, 0);
	p_node_b_set_dimensions(node, size, size, 1);
	for(i = 0; i < sizeof lname / sizeof *lname; i++)
	{
		layer = p_node_b_layer_create(node, lname[i], VN_B_LAYER_UINT8);
		p_node_b_layer_foreach_set(node, layer, cb_disc, (void *) size);
	}
	return P_COMPUTE_DONE;
}

void init(void)
{
	p_init_create("disc");
	p_init_input(0, P_VALUE_UINT32, "size", P_INPUT_REQUIRED, P_INPUT_MAX(256), P_INPUT_DONE);
	p_init_input(1, P_VALUE_BOOLEAN, "filled", P_INPUT_DONE);
	p_init_compute(compute);
}
