/*
 * Dynamic library abstraction.
*/

#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "mem.h"

#include "dynlib.h"

#define	PATH_SEPARATOR	'|'
#define	DIR_SEPARATOR	'/'

#define	DYNLIB_INVALID	NULL

static struct {
	char	**paths;
	int	num_paths;
} path_info;

void dynlib_path_set(const char *path)
{
	int	i, j, num = 1;
	char	*put;

	if(path == NULL)
		return;

	for(i = 0; path[i] != '\0'; i++)
		if(path[i] == PATH_SEPARATOR)
			num++;
	path_info.paths = mem_realloc(path_info.paths, num * sizeof *path_info.paths + i + i);
	path_info.num_paths = num;
	put = (char *) path_info.paths + num * sizeof *path_info.paths;
	for(i = j = 0; i < num && path[j] != '\0'; i++)
	{
		path_info.paths[i] = put;
		for(; path[j] != '\0' && path[j] != PATH_SEPARATOR; j++)
			*put++ = path[j];
		*put++ = '\0';
		if(path[j] == PATH_SEPARATOR)
			j++;
	}
}

#if defined HAVE_DLOPEN

DynLib dynlib_load_absolute(const char *name)
{
	DynLib	l;

	if((l = dlopen(name, RTLD_LOCAL | RTLD_NOW)) != NULL)
		return l;
	puts(dlerror());
	return DYNLIB_INVALID;	
}

DynLib dynlib_load(const char *name)
{
	char	buf[PATH_MAX];
	int	i;

	if(name == NULL)
		return DYNLIB_INVALID;
	if(name[0] == DIR_SEPARATOR)
		return dynlib_load_absolute(name);

	for(i = 0; i < path_info.num_paths; i++)
	{
		DynLib	l;

		snprintf(buf, sizeof buf, "%s/lib%s.so", path_info.paths[i], name);
		if(DYNLIB_VALID(l = dynlib_load_absolute(buf)))
			return l;
	}
	return NULL;
}

void * dynlib_resolve(const DynLib lib, const char *symbol)
{
	if(lib != NULL)
		return dlsym(lib, symbol);
	return NULL;
}

void dynlib_unload(DynLib lib)
{
	if(lib != NULL)
		dlclose(lib);
}

#elif defined _WIN32
#error Not implemented
#endif
