/*
 * Magical built-in output plug-in.
*/

#include <stdio.h>

#include "purple.h"

#include "dynarr.h"
#include "list.h"
#include "plugins.h"
#include "textbuf.h"
#include "nodedb.h"
#include "synchronizer.h"

#include "api-init.h"

/* ----------------------------------------------------------------------------------------- */

/* Called when our solitary input changes. Somehow send the output to Verse. */
static PComputeStatus compute(PPInput *input, PPOutput output, void *state_typeless)
{
	PINode	*node = p_input_node(input[0]);

	printf("now in plugin-output\n");
	if(node != NULL)
	{
		printf("Output: processing node \"%s\" at %p\n", p_node_name_get(node), node);
		sync_node_add(node);	/* Don't convert to "output" type, just do it. */
	}
	return P_COMPUTE_DONE;
}

/* ----------------------------------------------------------------------------------------- */

/* This works just as a "real" (library-based) plug-in's init() function. */
void plugin_output_init(void)
{
	api_init_begin(NULL);	/* Normal plug-in init()s don't do this! */

	p_init_create("node-output");
	p_init_input(0, P_VALUE_MODULE, "x", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_meta("desc/purpose", "Built-in plug-in, writes input to Verse server.");
	p_init_meta("author", "Emil Brink");
	p_init_compute(compute);

	api_init_end();
}
