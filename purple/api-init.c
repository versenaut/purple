/*
 * api-init.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Initialize a library, letting it describe the plug-ins it defines.
*/

#include <stdarg.h>
#include <stdio.h>

#define PURPLE_INTERNAL

#include "log.h"
#include "purple.h"
#include "plugins.h"

/* ----------------------------------------------------------------------------------------- */

static struct
{
	Library	*owner;
	Plugin	*plugin;
} init_info = { NULL };

/* ----------------------------------------------------------------------------------------- */

/* Begin use of Init API, expecting calls to come from code held in <owner>. If the owning
 * library is NULL, a "magical" plug-in that is part of the Purple core is being initialized.
*/
void api_init_begin(Library *owner)
{
	init_info.owner = owner;
	if(init_info.plugin != NULL)
		LOG_WARN(("Previous plug-in pointer not reset (was %p)", init_info.plugin));
	init_info.plugin = NULL;
}

static void plugin_flush(void)
{
	if(init_info.plugin != NULL)
		plugins_register(init_info.owner, init_info.plugin);
	init_info.plugin = NULL;
}

void api_init_end(void)
{
	plugin_flush();
	init_info.owner = NULL;
}

/* ----------------------------------------------------------------------------------------- */

/* These are the plug-in-visible actual Purple API functions. */

/** \defgroup api_init Plug-In Initialization Functions
 * 
 * Functions in this group are used to initialize a plug-in. They are always used exclusively from
 * a library's \c init() function, and never from the \c compute() callback. There are five functions,
 * three of which are optional.
 * 
 * The most important functions are \c p_init_create() and \c p_init_compute(), they must be used
 * in every library's \c init() function. Any plug-in that needs inputs to function, which will be
 * vast majority, needs to have them defined using a number of calls to \c p_init_input(). Using
 * \c p_init_meta() to register meta information is a very good idea, but not mandatory at the time
 * of writing. If per-instance persistent state is desired, use the \c p_init_state() function to
 * define it.
 * 
 * Here's an example of a library \c init() function that defines three separate plug-ins that all
 * share the same \c compute() code:
 * 
 * \code
 * static PComputeStatus compute(PPInput *input, PPOutput output, void *state)
 * {
 * 	// ... code here
 * 	return P_COMPUTE_DONE;
 * }
 * 
 * void init(void)
 * {
 * 	p_init_create("foo");
 * 	p_init_compute(compute);
 * 
 * 	p_init_create("bar");
 * 	p_init_compute(compute);
 * 
 * 	p_init_create("baz");
 * 	p_init_compute(compute);
 * }
 * \endcode
 * 
 * Things to note:
 * - A new plug-in is created by each call to the \c p_init_create() function.
 * - The \c compute() callback is set by \c p_init_compute().
 * - A plug-in does \b not "inherit" any initialization from the previous one.
 * - The \c compute() function does not need to be seen from the outside, it can
 * be defined as \c static. This is true for \b all functions a library might
 * contain, except \c init().
 * @{
*/

/**
 * This function is called from within the \c init() function of a plug-in library, to register a new
 * plug-in with the Purple engine. You can do this any number of times, since a single library (i.e.
 * the shared object or DLL file on disk) can define many actual plug-ins. This is often useful when
 * one wants to share code between two plug-ins in an easy manner.
 * 
 * All calls to other init functions affect the current plug-in, which is the one that was registered last.
 * 
 * There \b must be at least one call to this function in a library's \c init() function, or the
 * library will be ignored by the Purple engine.
*/
PURPLEAPI void p_init_create(const char *name /** The name of the plug-in to create. Plug-in names should
					       be short and descriptive, and not include any whitespace.
					       Use a dash or underscore instead of a space. */)
{
	plugin_flush();
	init_info.plugin = plugin_new(name);
}

/**
 * Register a piece of "meta information" associated with the current plug-in. Such meta information is
 * intented to be presented to end users by user interface clients, and might also provide grouping support
 * so that, for instance, related tools can be grouped together in the interface.
 * 
 * Each piece of meta information is defined as a pair of strings, one being called the category and the
 * other the text. These can be considered "assignments", like so: \a category = \a text.
 * 
 * There should be a list of suggested/recommended/standardized meta categories here, but curiously there
 * isn't.
*/
PURPLEAPI void p_init_meta(const char *category	/** The category to register meta text in. */,
			   const char *text	/** The meta text to register. */)
{
	plugin_set_meta(init_info.plugin, category, text);
}

/**
 * Add an input to the current plug-in. Inputs are "ports" where the plug-in can receive data from other
 * plug-ins, or directly from the user.
 * 
 * This function is varargs to support the specification of further detail. You can use the \c P_INPUT macros
 * to build a list of extra information, including minimum, maximum and default values, make the input
 * required, and so on. To end the list, use \c P_INPUT_DONE.
*/
PURPLEAPI void p_init_input(int index		/** Index of the input to add. Must begin at zero and monotonically increase. */,
			    PValueType type	/** Preferred type of input. This information is published in the XML description. */,
			    const char *name	/** Name of the input. */,
			    ...)
{
	va_list	args;

	va_start(args, name);
	plugin_set_input(init_info.plugin, index, type, name, args);
	va_end(args);
}

/**
 * Register persistent per-instance state for a plug-in. Every instance of the plug-in will have the state
 * automatically allocated by the Purple engine, and passed in as an argument to the \c compute() callback.
 * 
 * The Purple engine does not touch the buffer in any way, its contents are totally up to the plug-in to
 * read and write.
 * 
 * The state buffer will be automatically deallocated once the plug-in instance is destroyed, of course.
*/
PURPLEAPI void p_init_state(size_t size				/** The number of bytes of state required. Typically computed by \c sizeof on a plug-in internal structure. */,
			    void (*constructor)(void *state)	/** Optional function to initialize the state buffer, before it's first seen by the \c compute() callback. */,
			    void (*destructor)(void *state)	/** Optional function to destroy the state buffer, before it's deallocated. */)
{
	plugin_set_state(init_info.plugin, size, constructor, destructor);
}

/**
 * This function registers the main computational callback function. This is the function that the Purple
 * engine will call when it needs the plug-in to re-compute its output.
*/
PURPLEAPI void p_init_compute(PComputeStatus (*compute)(PPInput *input, PPOutput output, void *state) /** The callback function pointer. */)
{
	plugin_set_compute(init_info.plugin, compute);
}

/** @} */
