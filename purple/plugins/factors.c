/*
 * Count number of factors in a number. If result is 1, number is prime. Grossely
 * naive, mainly to really use up a lot of time for doing the computation.
*/

#include "purple.h"
#include "purple-plugin.h"

typedef struct
{
	uint32	number;
	uint32	index;
	int32	factors;
} State;

static PComputeStatus compute(PPInput *input, PPOutput output, void *state_typeless)
{
	State	*state = state_typeless;

	if(state->factors < 0)		/* Not yet initialized? */
	{
		state->number  = p_input_uint32(input[0]);
		state->index   = 2;
		state->factors = 1;
	}
	if(state->index >= state->number)	/* Done? */
	{
		p_output_int32(output, state->factors);
		state->factors = -1;
		return P_COMPUTE_DONE;
	}
	if(state->number % state->index == 0)
		state->factors++;
	state->index++;
	return P_COMPUTE_AGAIN;
}

static void ctor(void *state_typeless)
{
	State	*state = state_typeless;

	state->number = state->index = 0U;
	state->factors = -1;			/* Signals non-active state. */
}
	
void init(void)
{
	p_init_create("factors");
	p_init_state(sizeof (State), ctor, NULL);
	p_init_input(0, P_VALUE_UINT32, "n", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_compute(compute);
}
