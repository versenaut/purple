/*
 * api-iter.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 *
 * Iterator framework, for iterating over dynarrays and lists easily.
 * Intention is to save plug-ins from the O(n^2) time it takes to use
 * the (dumb) _num() and _nth() calls to iterate, and instead do it
 * directly for something closer to ~O(1) access performance.
 * 
 * This file is kind of unique; it rolls together both behind-the-scenes
 * iter code (the iter_* calls) and the plug-in ("user") visible iter
 * API calls, p_iter_*. It simply seemed silly to split it up further,
 * although there are two headers (internal iter.h and external in
 * parts of purple.h).
*/

#include <stdlib.h>

#include "dynarr.h"
#include "list.h"

#include "purple.h"
#include "iter.h"

/* ----------------------------------------------------------------------------------------- */

#define	ITER_DYNARR	(1 << 0)
#define	ITER_LIST	(1 << 1)
#define	ITER_STRING	(1 << 15)

/* ----------------------------------------------------------------------------------------- */

/* Return next valid index, which will be <index> or higher if there are (string-mode) gaps. */
static unsigned int validate_dynarr_index(PIter *iter, unsigned int index)
{
	if(iter->flags & ITER_STRING)	/* In string mode, skip array elements with '\0' at [offset]. */
	{
		const char	*el;

		for(; (el = dynarr_index(iter->data.dynarr.arr, index)) != NULL; index++)
		{
			if(el[iter->offset] != '\0')
				break;
		}
	}
	return index;
}

/* Initialize iterator over dynamic array (dynarr.h). Rather pointless, included for completeness. */
void iter_init_dynarr(PIter *iter, DynArr *arr)
{
	iter->flags = ITER_DYNARR;
	iter->index = 0;
	iter->data.dynarr.arr   = arr;
	iter->data.dynarr.index = 0;
}

/* Initialize iterator over dynamic array (dynarr.h) in string mode. This simply indexes into the
 * array using dynarr_index (an O(1) operation), and uses the provided offset to probe for a
 * zero-length string. Elements with zero-length strings are considered invalid, and are never
 * returned by iter_get(). Typically used to exclude destroyed layers/tag groups/curves etc.
*/
void iter_init_dynarr_string(PIter *iter, DynArr *arr, size_t offset)
{
	if(iter == NULL)
		return;
	iter_init_dynarr(iter, arr);
	iter->flags |= ITER_STRING;
	iter->offset = offset;
	iter->data.dynarr.index = validate_dynarr_index(iter, 0);
}

/* Initialize iterator over standard linked list. Should be fairly cheap wrapping. */
void iter_init_list(PIter *iter, List *list)
{
	iter->flags = ITER_LIST;
	iter->index = 0;
	iter->data.list = list;
}

/* ----------------------------------------------------------------------------------------- */

unsigned int p_iter_index(const PIter *iter)
{
	if(iter != NULL)
		return iter->index;
	return 0;
}

void * p_iter_data(PIter *iter)
{
	if(iter == NULL)
		return NULL;
	if(iter->flags & ITER_DYNARR)
		return dynarr_index(iter->data.dynarr.arr, iter->data.dynarr.index);
	else if(iter->flags & ITER_LIST)
		return list_data(iter->data.list);
	return NULL;
}

void p_iter_next(PIter *iter)
{
	if(iter == NULL)
		return;
	if(iter->flags & ITER_DYNARR)
	{
		if(iter->flags & ITER_STRING)
			iter->data.dynarr.index = validate_dynarr_index(iter, iter->data.dynarr.index + 1);
		else
			iter->data.dynarr.index++;
	}
	else if(iter->flags & ITER_LIST)
		iter->data.list = list_next(iter->data.list);
	else
		return;
	iter->index++;
}
