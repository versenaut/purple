/*
 * A module for managing plug-in loading. 
*/

#include <stdarg.h>

#include "dynstr.h"

#include "purple.h"

typedef struct Library	Library;
typedef struct Plugin	Plugin;

typedef struct PPortSet	PPortSet;

typedef struct
{
	PPortSet	*inputs;
	void		*state;
} PInstance;

extern void	plugins_init(const char *paths);

extern Plugin *	plugin_new(const char *name);
extern void	plugin_set_input(Plugin *p, int index, PValueType type, const char *name, va_list taglist);
extern void	plugin_set_meta(Plugin *p, const char *category, const char *text);
extern void	plugin_set_state(Plugin *p, size_t size, void (*ctor)(void *state), void (*dtor)(void *state));
extern void	plugin_set_compute(Plugin *p, void (*compute)(PPInput *input, PPOutput output, void *state));
extern char *	plugin_describe(const Plugin *p);
extern void	plugin_describe_append(const Plugin *p, DynStr *ds);
extern void	plugin_destroy(Plugin *p);

extern void	plugins_libraries_load(void);
extern void	plugins_libraries_init(void);
extern void	plugins_register(Library *owner, Plugin *p);	/* Used internally. Frees p on failure. */

extern size_t	plugins_amount(void);
extern char *	plugins_build_xml(void);

extern const Plugin *	plugin_lookup(unsigned int id);
extern unsigned int	plugin_id(const Plugin *p);
extern const char *	plugin_name(const Plugin *p);

extern PPortSet *	plugin_portset_new(const Plugin *p);
extern PPInput *	plugin_portset_ports(PPortSet *is);
extern void		plugin_portset_set_va(PPortSet *is, unsigned int index, PValueType type, va_list arg);
extern void		plugin_portset_clear(PPortSet *is, unsigned int index);
extern boolean		plugin_portset_is_set(const PPortSet *is, unsigned int index);
extern size_t		plugin_portset_size(const PPortSet *is);
extern boolean		plugin_portset_get_module(const PPortSet *is, unsigned int index, uint32 *module_id);
extern void		plugin_portset_describe(const PPortSet *is, DynStr *d);
extern void		plugin_portset_destroy(PPortSet *is);

extern int		plugin_instance_init(Plugin *p, PInstance *inst);
extern void		plugin_instance_compute(Plugin *p, const PInstance *inst);
extern void		plugin_instance_free(Plugin *p, PInstance *inst);
