/*
 * dynlib.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Dynamic library abstraction. Really rather thin.
*/

#if !defined _WIN32
#include <unistd.h>
#endif

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "dynlib.h"

#define	DYNLIB_INVALID	NULL

#if defined HAVE_DLOPEN

DynLib dynlib_load(const char *name)
{
	DynLib	l;

	if(name == NULL)
		return DYNLIB_INVALID;
	if((l = dlopen(name, RTLD_NOW)) != NULL)	/* Local symbol resolution (default). */
		return l;
	puts(dlerror());

	return DYNLIB_INVALID;	
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

DynLib dynlib_load(const char *name)
{
	return LoadLibrary(name);
}

void * dynlib_resolve(const DynLib lib, const char *symbol)
{
	return GetProcAddress(lib, symbol);
}

void dynlib_unload(DynLib lib)
{
	FreeLibrary(lib);
}

#endif
