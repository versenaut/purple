/*
 * This is a "magical" plug-in, that does a lookup of a named node in the node database.
 * It does not limit itself to the official Purple plug-in API, but uses core calls
 * directly since it is part of the application. It also uses the Purple API and has a
 * similar public init() function, ín order to be registerable as an externally visible
 * plug-in.
*/

#include <stdio.h>

#include "verse.h"

#include "dynarr.h"
#include "list.h"
#include "textbuf.h"
#include "nodedb.h"

#include "purple.h"

typedef struct
{
	int	init;
	Node	*notify;
	void	*notify_handle;
} State;

static void cb_notify(Node *node, NodeNotifyEvent ev, void *user)
{
	printf("Notification in input, node at %p\n", node);
}

static void compute(PPInput *input, PPOutput output, void *state_typeless)
{
	char	name[64];
	State	*state = state_typeless;

	printf("Input::compute() running, state at %p\n", state);
	if(!state->init)
	{
		printf("First time in input::compute\n");
		state->init   = 1;
		state->notify = NULL;
	}
	if(p_input_string(input[0], name, sizeof name) != NULL)
	{
		Node	*node;

		printf("Input looking up node '%s'\n", name);
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

/* This works just as a "real" (library-based) plug-in's init() function. */
void plugin_input_init(void)
{
	p_init_create("node-input");
	p_init_input(0, P_INPUT_STRING, "name", P_INPUT_REQUIRED, P_INPUT_DONE);
	p_init_meta("desc/purpose", "Built-in plug-in, outputs the single node whose name is given");
	p_init_meta("author", "Emil Brink");
	p_init_state(sizeof (State));
	p_init_compute(compute);
}
