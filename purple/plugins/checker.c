/*
 * A bitmap Purple plug-in that creates a checkerboard pattern. Silly.
*/

#include "purple.h"
#include "purple-plugin.h"

/* Determine if (x,y) is in two-dimensional checker board square of size <user> or not. */
static real64 cb_check(uint32 x, uint32 y, uint32 z, void *user)
{
	uint32	size = (uint32) user, row = y / size, col = x / size;
	int	v = (row & 1) ? (col & 1) : !(col & 1);

	return v ? 1.0 : 0.0;
}

static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	const char	*lname[] = { "col_r", "col_g", "col_b" };
	PONode		*node;
	PNBLayer	*layer;
	uint32		side = p_input_uint32(input[0]), size = p_input_uint32(input[1]), i;

	node = p_output_node_create(output, V_NT_BITMAP, 0);
	p_node_b_dimensions_set(node, side, side, 1);
	for(i = 0; i < sizeof lname / sizeof *lname; i++)
	{
		layer = p_node_b_layer_create(node, lname[i], VN_B_LAYER_UINT8);
		p_node_b_layer_foreach_set(node, layer, cb_check, (void *) size);
	}
	return P_COMPUTE_DONE;
}

void init(void)
{
	p_init_create("checker");
	p_init_input(0, P_VALUE_UINT32, "side", P_INPUT_REQUIRED, P_INPUT_DONE);	/* Side of entire checker. */
	p_init_input(1, P_VALUE_UINT32, "size", P_INPUT_REQUIRED, P_INPUT_DONE);	/* Size of each square. */
	p_init_compute(compute);
}
