/*
 * mem.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * 
*/

#include <stdio.h>
#include <stdlib.h>

#include "log.h"
#include "mem.h"

static enum MemMode the_mode = MEM_NULL_ERROR;

void mem_mode_set(enum MemMode mode)
{
	if(mode < 0 || mode >= 3)
	{
		LOG_WARN(("Can't set mode %d, not defined", mode));
		return;
	}
	the_mode = mode;
}

static void * do_return(const char *src, void *p, size_t size)
{
	if(p != NULL)
		return p;
	switch(the_mode)
	{
	case MEM_NULL_RETURN:
		return NULL;
	case MEM_NULL_WARN:
		LOG_WARN(("Failed to %s %u bytes, returning NULL", src, size));
		return NULL;
	case MEM_NULL_ERROR:
		LOG_ERR(("Failed to %s %u bytes, aborting", src, size));
		abort();
	}
	return NULL;
}

void * mem_alloc(size_t size)
{
	return do_return("malloc", malloc(size), size);
}

void * mem_realloc(void *ptr, size_t size)
{
	return do_return("realloc", realloc(ptr, size), size);
}

void mem_free(void *ptr)
{
	free(ptr);
}
