/*
 * dynlib.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Dynamic library abstraction. Really rather thin.
*/

#include <unistd.h>
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
	if((l = dlopen(name, RTLD_LOCAL | RTLD_NOW)) != NULL)
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
#error Not implemented
#endif
