/*
 * This is a "magical" plug-in, that does a lookup of a named node in the node database.
 * It does not limit itself to the official Purple plug-in API, but uses core calls
 * directly since it is part of the application. It also uses the Purple API and has a
 * similar public init() function, ín order to be registerable as an externally visible
 * plug-in. For technical reasons (public symbols), it'd probably be possible to write
 * an actual library-based plug-in using these calls, too. *Don't* do that.
*/

#include <stdio.h>

#include "verse.h"

#include "dynarr.h"
#include "list.h"
#include "textbuf.h"
#include "nodedb.h"

#include "purple.h"

/* ----------------------------------------------------------------------------------------- */

/* A struct like this is associated with every instance of this plug-in. */
typedef struct
{
	Node	*notify;
	void	*notify_handle;
} State;

/* ----------------------------------------------------------------------------------------- */

/* This gets called when the node we're watching changes. We need to cause a
 * re-computation of any modules having this one as an input dependency.
*/
static void cb_notify(Node *node, NodeNotifyEvent ev, void *user)
{
	printf("Notification in input, node at %p\n", node);
/*	p_output_node(node);*/
}

/* Called when our solitary input changes. Look up the named node,
 * and set up change-notification on it, if found.
*/
static void compute(PPInput *input, PPOutput output, void *state_typeless)
{
	const char	*name;
	State		*state = state_typeless;

	if((name = p_input_string(input[0])) != NULL)
	{
		Node	*node;

		node = nodedb_lookup_by_name(name);
		if(node != state->notify)	/* Different from current? */
		{
			/* De-register current. */
			nodedb_notify_node_remove(state->notify, state->notify_handle);
			if(node != NULL)	/* Register new. */
				state->notify_handle = nodedb_notify_node_add(node, cb_notify, NULL);
			else
				state->notify_handle = NULL;
			state->notify = node;
		}
	}
	else
		printf("Input couldn't get node name to watch\n");
}

/* This gets called when the plug-in is instantiated. Initialize the state. */
static void ctor(void *state_typeless)
{
	State	*state = state_typeless;

	state->notify = NULL;
	state->notify_handle = NULL;
}

/* This gets called when the plug-in is de-instantiated. Remove any notification. */
static void dtor(void *state_typeless)
{
	State	*state = state_typeless;

	if(state->notify)
		nodedb_notify_node_remove(state->notify, state->notify_handle);
}

/* ----------------------------------------------------------------------------------------- */

/* This works just as a "real" (library-based) plug-in's init() function. */
void plugin_input_init(void)
{
	p_init_create("node-input");
	p_init_input(0, P_VALUE_STRING, "name", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_meta("desc/purpose", "Built-in plug-in, outputs the single node whose name is given");
	p_init_meta("author", "Emil Brink");
	p_init_state(sizeof (State), ctor, dtor);
	p_init_compute(compute);
}
