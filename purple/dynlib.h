/*
 * A simple little wrapper for loading dynamic libraries of code. This is needed because
 * there is no platform-independent API for doing this, it's different between e.g. Unix
 * (dlopen) and Windows (LoadLibrary).
*/

#define	DYNLIB_VALID(d)	((d) != NULL)

#if defined  __linux
#define	HAVE_DLOPEN
#include <dlfcn.h>
typedef void *	DynLib;
#elif defined _WIN32
typedef HMODULE	DynLib;
#endif

/* Set a set of directory paths that will be searched for libraries. Separate entries
 * in set with vertical bar ("pipe") characters, e.g. "/lib/|/usr/lib".
*/
void	dynlib_path_set(const char *path);

DynLib	dynlib_load(const char *name);
void *	dynlib_resolve(const DynLib lib, const char *symbol);
void	dynlib_unload(DynLib lib);
