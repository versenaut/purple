/*
 * A module for managing plug-in loading. 
*/

#include <stdarg.h>

#include "dynstr.h"

#include "purple.h"

typedef struct Library		Library;
typedef struct Plugin		Plugin;

typedef struct PInputSet	PInputSet;

extern void	plugins_init(const char *paths);

extern Plugin *	plugin_new(const char *name);
extern void	plugin_set_input(Plugin *p, int index, PInputType type, const char *name, va_list taglist);
extern void	plugin_set_meta(Plugin *p, const char *category, const char *text);
extern void	plugin_set_compute(Plugin *p, void (*compute)(PPInput *input, PPOutput output, void *user), void *user);
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

extern PInputSet *	plugin_inputset_new(const Plugin *p);
extern void		plugin_inputset_set(PInputSet *is, unsigned int index, const PInputValue *value);
extern void		plugin_inputset_set_va(PInputSet *is, unsigned int index, PInputType type, va_list arg);
extern void		plugin_inputset_clear(PInputSet *is, unsigned int index);
extern boolean		plugin_inputset_is_set(const PInputSet *is, unsigned int index);
extern void		plugin_inputset_destroy(PInputSet *is);
