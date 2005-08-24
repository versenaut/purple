/*
 * plugin-input.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * This is a "magical" plug-in, that does a lookup of a named node in the node database.
 * It does not limit itself to the official Purple plug-in API, but uses core calls
 * directly since it is part of the application. It also uses the Purple API and has a
 * similar public init() function, ín order to be registerable as an externally visible
 * plug-in. For technical reasons (public symbols), it'd probably be possible to write
 * an actual library-based plug-in using these calls, too. *Don't* do that.
*/

#include <stdarg.h>
#include <stdio.h>

#include "verse.h"

#include "purple.h"

#include "dynarr.h"
#include "value.h"
#include "list.h"
#include "plugins.h"
#include "textbuf.h"
#include "nodedb.h"

#include "api-init.h"

#include "plugin-input.h"

/* ----------------------------------------------------------------------------------------- */

/* A struct like this is associated with every instance of this plug-in. */
typedef struct
{
	PNode		*notify;
	void		*notify_handle;
	PPOutput	output;
} State;

/* ----------------------------------------------------------------------------------------- */

/* This gets called when the node we're watching changes. We need to cause a
 * re-computation of any modules having this one as an input dependency.
*/
static void cb_notify(PNode *node, NodeNotifyEvent ev, void *user)
{
	State	*state = user;

	printf("node-input: Notification in input, node at %p\n", node);
	graph_port_output_begin(state->output);
	p_output_node(state->output, node);
	graph_port_output_end(state->output);
}

/* Called when our solitary input changes. Look up the named node,
 * and set up change-notification on it, if found.
*/
static PComputeStatus compute(PPInput *input, PPOutput output, void *state_typeless)
{
	const char	*name;
	State		*state = state_typeless;

	state->output = output;

	if((name = p_input_string(input[0])) != NULL)
	{
		PNode	*node;

		node = nodedb_lookup_by_name(name);
		if(node != state->notify)	/* Different from current? */
		{
			/* De-register current. */
			nodedb_notify_node_remove(state->notify, state->notify_handle);
			if(node != NULL)	/* Register new. */
			{
				state->notify_handle = nodedb_notify_node_add(node, cb_notify, state);
				cb_notify(node, NODEDB_NOTIFY_DATA, state);
			}
			else
				state->notify_handle = NULL;
			state->notify = node;
		}
	}
	else
		printf("Input couldn't get node name to watch\n");
	return P_COMPUTE_DONE;
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
	api_init_begin(NULL);	/* Normal plug-in init()s don't do this! */

	p_init_create("node-input");
	p_init_input(0, P_VALUE_STRING, "name", P_INPUT_REQUIRED, P_INPUT_DESC("Name of single Verse node to import."), P_INPUT_DONE);
	p_init_meta("class", "input/output");
	p_init_meta("authors", "Emil Brink");
	p_init_meta("desc/purpose", "Built-in plug-in, outputs the single node whose name is given");
	p_init_state(sizeof (State), ctor, dtor);
	p_init_compute(compute);

	api_init_end();
}
