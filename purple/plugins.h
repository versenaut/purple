/*
 * plugins.h
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * A module for managing plug-in loading. 
*/

#include <stdarg.h>

#include "dynstr.h"

#include "purple.h"

/* ----------------------------------------------------------------------------------------- */

typedef struct Library	Library;
typedef struct Plugin	Plugin;

typedef struct PPortSet	PPortSet;

typedef struct
{
	Plugin		*plugin;
	PPortSet	*inputs;
	PPOutput	output;
	void		*state;
	PPOutput	(*resolver)(uint32 module_id, void *data);
	void		*resolver_data;
} PInstance;

/* ----------------------------------------------------------------------------------------- */

extern void	plugins_init(const char *paths);

/* Calls used by the Init API functions, used by plug-ins in their init() phase. */
extern Plugin *	plugin_new(const char *name);
extern void	plugin_set_input(Plugin *p, int index, PValueType type, const char *name, va_list taglist);
extern void	plugin_set_meta(Plugin *p, const char *category, const char *text);
extern void	plugin_set_state(Plugin *p, size_t size, void (*ctor)(void *state), void (*dtor)(void *state));
extern void	plugin_set_compute(Plugin *p, PComputeStatus (*compute)(PPInput *input, PPOutput output, void *state));
extern char *	plugin_describe(const Plugin *p);
extern void	plugin_describe_append(const Plugin *p, DynStr *ds);
extern void	plugin_destroy(Plugin *p);

extern void	plugins_libraries_load(void);
extern void	plugins_libraries_init(void);
extern void	plugins_register(Library *owner, Plugin *p);	/* Used internally. Frees p on failure. */

extern size_t	plugins_amount(void);
extern char *	plugins_build_xml(void);

extern Plugin *		plugin_lookup(unsigned int id);
extern Plugin *		plugin_lookup_by_name(const char *name);
extern unsigned int	plugin_id(const Plugin *p);
extern const char *	plugin_name(const Plugin *p);

/* Portset sub-API. A portset is a collection of (input) ports, always created by providing
 * a plug-in as a template. The portset contains as many value slots as the provided plug-in
 * has inputs.
*/
extern PPortSet *	plugin_portset_new(const Plugin *p);
extern PPInput *	plugin_portset_ports(PPortSet *ps);
extern void		plugin_portset_set_va(PPortSet *ps, unsigned int index, PValueType type, va_list arg);
extern void		plugin_portset_clear(PPortSet *ps, unsigned int index);
extern boolean		plugin_portset_is_set(const PPortSet *ps, unsigned int index);
extern size_t		plugin_portset_size(const PPortSet *ps);
extern boolean		plugin_portset_get_module(const PPortSet *ps, unsigned int index, uint32 *module_id);
extern void		plugin_portset_describe(const PPortSet *ps, DynStr *d);
extern void		plugin_portset_destroy(PPortSet *ps);

typedef enum
{
	PLUGIN_RETRY_INCOMPLETE = -1,			/* Computation is proceeding, please call again. */
	PLUGIN_STOP = 0,				/* General stopping condition, all values >= this mean stop. */
	PLUGIN_STOP_COMPLETE = PLUGIN_STOP,		/* Computation is done! */
	PLUGIN_STOP_INPUT_MISSING,			/* Required inputs are missing, no go. */
	PLUGIN_STOP_FAILURE,				/* Generic, probably permanent, failure occured. */
} PluginStatus;

/* Plug-in instances are used to represent instances of plug-ins ("modules"). It's a
 * transparent structure holding an inputset and a state pointer, basically.
*/
extern int		plugin_instance_init(Plugin *p, PInstance *inst);
extern void		plugin_instance_set_output(PInstance *inst, PPOutput output);
extern void		plugin_instance_set_link_resolver(PInstance *inst,
							  PPOutput (*get_output)(uint32 module_id, void *data), void *data);
extern PluginStatus	plugin_instance_compute(PInstance *inst);
extern void		plugin_instance_free(PInstance *inst);
