/*
 * 
*/

#include "purple.h"
#include "purple-plugin.h"

/* It's more fun to compute. */
static void compute(PPInput *input, PPOutput output, void *user)
{
	if(user == NULL)
		printf("Hello!\n");
	else if(user == (void *) 1)
		printf("Hola!\n");
	else if(user == (void *) 2)
		printf("Morsning!\n");
}

void init(void)
{
	p_init_create("hello");
	p_init_input(0, P_INPUT_INT32, "mode",
		     P_INPUT_MIN(0),
		     P_INPUT_MAX(17),
		     P_INPUT_DEFAULT(5),
		     P_INPUT_REQUIRED,
		     P_INPUT_DONE);
	p_init_input(1, P_INPUT_STRING, "text", P_INPUT_DEFAULT_STR("monster"), P_INPUT_DONE);
	p_init_meta("desc/purpose", "Say hello to the world");
	p_init_meta("author", "Emil Brink");
	p_init_compute(compute, NULL);

	p_init_create("hola");
	p_init_meta("author", "Emil Brink");
	p_init_meta("desc/purpose", "Say hola to the world");
	p_init_compute(compute, (void *) 1);

	p_init_create("morsning");
	p_init_compute(compute, (void *) 2);
}
