/*
 * 
*/

#include "purple.h"

static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	PONode		*out;
	PNCCurve	*curve;
	const uint32	pre_pos[] = { 0x80000000 }, post_pos[] = { 0x80000000 };
	const real64	val1 = p_input_real64(input[0]), val2 = 0.0, pre_value[] = { 10.0 }, post_value[] = {  10.0 };

	out = p_output_node_create(output, V_NT_CURVE, 0);
	curve = p_node_c_curve_create(out, "test", 1);
	p_node_c_curve_key_create(curve, 0.5, &val1, pre_pos, pre_value, post_pos, post_value);
	p_node_c_curve_key_create(curve, 0.2, &val2, pre_pos, pre_value, post_pos, post_value);

	return P_COMPUTE_DONE;
}

void init(void)
{
	p_init_create("curvetest");
	p_init_input(0, P_VALUE_REAL64, "v", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_compute(compute);
}
