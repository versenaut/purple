/*
 * plugin-clock.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 *
 * This is another built-in plug-in, which does something totally magical in the Purple
 * world: it creates change. Basically, it works like a "metronome"; you set a period,
 * and it will alter its output periodically until stopped.
 * 
 * The value that is output is the time passed since the clock was started (or its
 * period last changed), delivered as a real64 (in seconds).
 * 
 * Precision of this clock will vary with the overall load of the Purple engine;
 * remember that it's all a big cooperative system.
 * 
*/

#include <stdarg.h>
#include <stdio.h>

#include "purple.h"

#include "cron.h"
#include "nodedb.h"
#include "value.h"
#include "graph.h"
#include "plugins.h"
#include "timeval.h"

#include "api-init.h"

/* ----------------------------------------------------------------------------------------- */

typedef struct {
	TimeVal		start;
	unsigned int	handle;		/* Handle to cron. */
	PPOutput	output;
} State;

/* ----------------------------------------------------------------------------------------- */

static int cb_cron(void *data)
{
	State		*state = data;

	graph_port_output_begin(state->output);
	p_output_real64(state->output, timeval_elapsed(&state->start, NULL));
	graph_port_output_end(state->output);

	return 1;
}

static PComputeStatus compute(PPInput *input, PPOutput output, void *state_typeless)
{
	State	*state = state_typeless;
	real32	period = p_input_real32(input[0]);
	boolean	on = p_input_boolean(input[1]);

	if(period > 0.0 && on)
	{
		state->output = output;
		if(state->handle > 0)
			cron_set(state->handle, period, cb_cron, state);
		else
			state->handle = cron_add(CRON_PERIODIC_SOON, period, cb_cron, state);
		timeval_now(&state->start);
	}
	else if(period <= 0.0 || !on)
	{
		if(state->handle > 0)
			cron_remove(state->handle);
		state->handle = 0;
	}
	return P_COMPUTE_DONE;
}

/* ----------------------------------------------------------------------------------------- */

/* Constructor, that simply initializes the state of a clock plug-in. */
static void ctor(void *state_typeless)
{
	State	*state = state_typeless;

	state->handle = 0;
}

/* The destructor is responsible for freeing any resources allocated in the state. */
static void dtor(void *state_typeless)
{
	State	*state = state_typeless;

	if(state->handle > 0)
	{
		cron_remove(state->handle);
	}
}

/* ----------------------------------------------------------------------------------------- */

void plugin_clock_init(void)
{
	api_init_begin(NULL);

	p_init_create("clock");
	p_init_input(0, P_VALUE_REAL32,  "period",  P_INPUT_REQUIRED, P_INPUT_MIN(0.05), P_INPUT_DESC("Period of the clock, in seconds."), P_INPUT_DONE);
	p_init_input(1, P_VALUE_BOOLEAN, "enabled", P_INPUT_DEFAULT(1), P_INPUT_DESC("Whether or not this clock is enabled."), P_INPUT_DONE);
	p_init_meta("authors", "Emil Brink");
	p_init_meta("desc/purpose", "Built-in plug-in, creates a spontaneously changing \"clock\" signal");
	p_init_state(sizeof (State), ctor, dtor);
	p_init_compute(compute);

	api_init_end();
}
