/*
 * Basic math functions. Used as a development sketching area, mainly.
*/

#include "purple.h"

static void compute_add_real32(PPInput *input, PPOutput output, void *state)
{
	real32	a, b;

	{
		char	abuf[256], bbuf[256];

		if(p_input_string(input[0], abuf, sizeof abuf) &&
		   p_input_string(input[1], bbuf, sizeof bbuf))
			printf("Inputs are '%s' and '%s'\n", abuf, bbuf);
	}
	a = p_input_real32(input[0]);
	b = p_input_real32(input[1]);
	printf("math: adding %g and %g\n", a, b);
/*	p_output_real32(a + b);*/
}

static void compute_sub_real32(PPInput *input, PPOutput output, void *state)
{
	real32	a, b;

	a = p_input_real32(input[0]);
	b = p_input_real32(input[1]);
	printf("math: substracting %g from %g\n", b, a);
/*	p_output_real32(a - b);*/
}

void init(void)
{
	p_init_create("add_r32");
	p_init_input(0, P_INPUT_REAL32, "a", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(1, P_INPUT_REAL32, "b", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_meta("desc/purpose", "Adds two real32 numbers together");
	p_init_meta("author", "Emil Brink");
	p_init_compute(compute_add_real32);

	p_init_create("sub_r32");
	p_init_input(0, P_INPUT_REAL32, "a", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(1, P_INPUT_REAL32, "b", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_meta("desc/purpose", "Computes difference of two real32 numbers");
	p_init_meta("author", "Emil Brink");
	p_init_compute(compute_sub_real32);
}
