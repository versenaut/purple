/*
 * Basic math functions. Used as a development sketching area, mainly.
*/

#include "purple.h"
#include "purple-plugin.h"

static PComputeStatus compute_power_of_two(PPInput *input, PPOutput output, void *state)
{
	uint32	x;

	x = p_input_uint32(input[0]);
	printf("computing 2^%u\n", x);
	p_output_uint32(output, 1 << x);

	return P_COMPUTE_DONE;
}

static void fact_ctor(void *state)
{
	int	*s = state;

	s[0] = 1;	/* Accumulated result. */
	s[1] = 0;	/* Counter, counts down from n to 0. */
}

static PComputeStatus compute_fact(PPInput *input, PPOutput output, void *state)
{
	uint32	*s = state;

	if(s[1] == 0)	/* First run? */
		s[1] = p_input_uint32(input[0]);
	if(s[1] <= 1)
	{
		printf("fact done, emitting %u\n", s[0]);
		p_output_uint32(output, s[0]);
		fact_ctor(state);	/* Reset state. */
		return P_COMPUTE_DONE;
	}
/*	printf("computing faculty, scaling %u by %u\n", s[0], s[1]);*/
	s[0] *= s[1]--;
	return P_COMPUTE_AGAIN;
}

static PComputeStatus compute_add_real32(PPInput *input, PPOutput output, void *state)
{
	real32	a, b;

/*	{
		const char	*as, *bs;
		if((as = p_input_string(input[0])) != NULL &&
		   (bs = p_input_string(input[1])) != NULL)
			printf("Inputs are '%s' and '%s'\n", as, bs);
	}
*/	a = p_input_real32(input[0]);
	b = p_input_real32(input[1]);
	printf("math: adding %g and %g\n", a, b);
	p_output_real32(output, a + b);

	return P_COMPUTE_DONE;
}

static PComputeStatus compute_sub_real32(PPInput *input, PPOutput output, void *state)
{
	real32	a, b;

	a = p_input_real32(input[0]);
	b = p_input_real32(input[1]);
	printf("math: substracting %g from %g\n", b, a);
	p_output_real32(output, a - b);

	return P_COMPUTE_DONE;
}

void init(void)
{
	p_init_create("power_of_two");
	p_init_input(0, P_VALUE_UINT32, "x", P_INPUT_DONE);
	p_init_compute(compute_power_of_two);

	p_init_create("fact");
	p_init_state(2 * sizeof (uint32), fact_ctor, NULL);
	p_init_input(0, P_VALUE_UINT32, "n", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_compute(compute_fact);

	p_init_create("add_r32");
	p_init_input(0, P_VALUE_REAL32, "a", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(1, P_VALUE_REAL32, "b", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_meta("desc/purpose", "Adds two real32 numbers together");
	p_init_meta("author", "Emil Brink");
	p_init_compute(compute_add_real32);

	p_init_create("sub_r32");
	p_init_input(0, P_VALUE_REAL32, "a", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_input(1, P_VALUE_REAL32, "b", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_meta("desc/purpose", "Computes difference of two real32 numbers");
	p_init_meta("author", "Emil Brink");
	p_init_compute(compute_sub_real32);
}
