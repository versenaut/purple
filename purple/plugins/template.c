/*
 * Purple plug-in template. Does nothing, in an informative way.
*/

#include "purple.h"

typedef struct
{
	unsigned int	value;	/* Just to prevent an empty struct. */
} State;

static void compute(PPInput *input, PPOutput output, void *state_typeless)
{
	State	*state = state_typeless;

	state->value++;
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

void init(void)
{
	p_init_create("template");
	/* Initialize inputs. */
	p_init_input(0, P_INPUT_REAL32, "x", P_INPUT_REQUIRED, P_INPUT_DONE);
	/* Set meta information. */
	p_init_meta("desc/purpose", "Shows how to use Purple API");
	p_init_meta("author", "Randomm Developer");
	/* Register state structure size, and constructor/destructor functions. */
	p_init_state(sizeof (State), ctor, dtor);
	/* Finally register the compute() callback. */
	p_init_compute(compute);
}
