/*
 * Initialize a library, letting it describe the plug-ins it defines.
*/

#include <stdarg.h>
#include <stdio.h>

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

void p_init_create(const char *name)
{
	plugin_flush();
	init_info.plugin = plugin_new(name);
}

void p_init_meta(const char *category, const char *text)
{
	plugin_set_meta(init_info.plugin, category, text);
}

void p_init_input(int index, PInputType type, const char *name, ...)
{
	va_list	args;

	va_start(args, name);
	plugin_set_input(init_info.plugin, index, type, name, args);
	va_end(args);
}

void p_init_compute(void (*compute)(PPInput *input, PPOutput output, void *user), void *user)
{
	plugin_set_compute(init_info.plugin, compute, user);
}
