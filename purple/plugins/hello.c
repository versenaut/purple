/*
 * 
*/

#include <stdio.h>

#include "purple.h"

/* It's more fun to compute. */
static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
{
	printf("Hello!\n");

	return P_COMPUTE_DONE;
}

PURPLE_PLUGIN void init(void)
{
	p_init_create("hello");
	p_init_input(0, P_VALUE_INT32, "mode",
		     P_INPUT_MIN(0),
		     P_INPUT_MAX(17),
		     P_INPUT_DEFAULT(5),
		     P_INPUT_REQUIRED,
		     P_INPUT_DONE);
	p_init_input(1, P_VALUE_STRING, "text", P_INPUT_DEFAULT_STR("monster"), P_INPUT_DONE);
	p_init_meta("desc/purpose", "Say hello to the world.");
	p_init_meta("authors", "Emil Brink");
	p_init_compute(compute);
}
