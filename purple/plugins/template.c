/*
 * Purple plug-in template. Does nothing, in an informative way.
*/

#include "purple.h"

typedef struct
{
	unsigned int	value;	/* Just to prevent an empty struct. */
} State;

static PComputeStatus compute(PPInput *input, PPOutput output, void *state_typeless)
{
	State	*state = state_typeless;

	state->value++;

	return P_COMPUTE_DONE;		/* Return P_COMPUTE_AGAIN if we need to run again. */
}

static void ctor(void *state_typeless)
{
	State	*state = state_typeless;

	state->value = 0;
}

static void dtor(void *state_typeless)
{
	State	*state = state_typeless;

	printf("We compute()d %u times\n", state->value);
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("template");
	/* Initialize inputs. */
	p_init_input(0, P_VALUE_REAL32, "x", P_INPUT_REQUIRED, P_INPUT_DONE);
	/* Set meta information. */
	p_init_meta("authors", "Random Developer");
	p_init_meta("desc/purpose", "Shows wanna-be developers how to use the Purple API.");
	/* Register state structure size, and constructor/destructor functions. */
	p_init_state(sizeof (State), ctor, dtor);
	/* Finally register the compute() callback. */
	p_init_compute(compute);
}
