/*
 * A module for managing plug-in loading. 
*/

#include <stdarg.h>

#include "purple.h"

typedef struct Library	Library;
typedef struct Plugin	Plugin;

extern void	plugins_init(const char *paths);

extern Plugin *	plugin_new(const char *name);
extern void	plugin_set_input(Plugin *p, int index, PInputType type, const char *name, va_list taglist);
extern void	plugin_set_meta(Plugin *p, const char *category, const char *text);
extern void	plugin_set_compute(Plugin *p, void (*compute)(PPInput *input, PPOutput output, void *user), void *user);
extern char *	plugin_describe(const Plugin *p);
extern void	plugin_destroy(Plugin *p);

extern void	plugins_libraries_load(void);
extern void	plugins_libraries_init(void);
extern void	plugins_register(Library *owner, Plugin *p);	/* Used internally. Frees p on failure. */

extern size_t	plugins_amount(void);
extern void	plugins_build_xml(void);
