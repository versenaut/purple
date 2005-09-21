/*
 * api-iter.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 *
 * Iterator framework, for iterating over dynarrays and lists easily.
 * Intention is to save plug-ins from the O(n^2) time it takes to use
 * the (dumb) _num() and _nth() calls to iterate, and instead do it
 * directly for something closer to ~O(n) access performance.
 * 
 * This file is kind of unique; it rolls together both behind-the-scenes
 * iter code (the iter_* calls) and the plug-in ("user") visible iter
 * API calls, p_iter_*. It simply seemed silly to split it up further,
 * although there are two headers (internal iter.h and external in
 * parts of purple.h).
*/

#include <stdlib.h>

#define PURPLE_INTERNAL

#include "purple.h"

#include "dynarr.h"
#include "list.h"

#include "iter.h"

/* ----------------------------------------------------------------------------------------- */

#define	ITER_DYNARR		(1 << 0)
#define	ITER_LIST		(1 << 1)

/* ----------------------------------------------------------------------------------------- */

static unsigned int validate_dynarr_string(PIter *iter, unsigned int index)
{
	const char	*el;

	for(; (el = dynarr_index(iter->data.dynarr.arr, index)) != NULL; index++)
	{
		if(el[iter->offset] != '\0')
			break;
	}
	return index;
}

static unsigned int validate_dynarr_enum_negative(PIter *iter, unsigned int index)
{
	const char	*el;
	const int	*e;		/* Enums are ints, really. */

	for(; (el = dynarr_index(iter->data.dynarr.arr, index)) != NULL; index++)
	{
		e = (const int *) (el + iter->offset);
		if(*e >= 0)
			break;
	}
	return index;
}

static unsigned int validate_dynarr_uint16_ffff(PIter *iter, unsigned int index)
{
	const char	*el;
	const uint16	*id;

	for(; (el = dynarr_index(iter->data.dynarr.arr, index)) != NULL; index++)
	{
		id = (const uint16 *) (el + iter->offset);
		if(*id != (uint16) ~0u)
			break;
	}
	return index;
}

/* Initialize iterator over dynamic array (dynarr.h). Rather pointless, included for completeness. */
void iter_init_dynarr(PIter *iter, const DynArr *arr)
{
	iter->flags = ITER_DYNARR;
	iter->index = 0;
	iter->data.dynarr.arr   = arr;
	iter->data.dynarr.index = 0;
	iter->data.dynarr.validate = NULL;
}

/* Initialize iterator over dynamic array (dynarr.h) in string mode. This simply indexes into the
 * array using dynarr_index (an O(1) operation), and uses the provided offset to probe for a
 * zero-length string. Elements with zero-length strings are considered invalid, and are never
 * returned by iter_get(). Typically used to exclude destroyed layers/tag groups/curves etc.
*/
void iter_init_dynarr_string(PIter *iter, const DynArr *arr, size_t offset)
{
	if(iter == NULL)
		return;
	iter_init_dynarr(iter, arr);
	iter->offset = offset;
	iter->data.dynarr.validate = validate_dynarr_string;
	iter->data.dynarr.index = iter->data.dynarr.validate(iter, 0);
}

/* Initialize iter that tests for legal dynarr elements by checking the sign of an enum field. */
void iter_init_dynarr_enum_negative(PIter *iter, const DynArr *arr, size_t offset)
{
	if(iter == NULL)
		return;
	iter_init_dynarr(iter, arr);
	iter->offset = offset;
	iter->data.dynarr.validate = validate_dynarr_enum_negative;
	iter->data.dynarr.index = iter->data.dynarr.validate(iter, 0);
}

/* Initialize iter that tests for legal dynarr elements by checking a uint16 field against ~0. */
void iter_init_dynarr_uint16_ffff(PIter *iter, const DynArr *arr, size_t offset)
{
	if(iter == NULL)
		return;
	iter_init_dynarr(iter, arr);
	iter->offset = offset;
	iter->data.dynarr.validate = validate_dynarr_uint16_ffff;
	iter->data.dynarr.index = iter->data.dynarr.validate(iter, 0);
}

/* Initialize iterator over standard linked list. Should be fairly cheap wrapping. */
void iter_init_list(PIter *iter, List *list)
{
	iter->flags = ITER_LIST;
	iter->index = 0;
	iter->data.list = list;
}

/* ----------------------------------------------------------------------------------------- */

/** \defgroup api_iter Iterator Functions
 * 
 * These functions, together with the \c PIter data structure, provide functionality for
 * \b iterating over sequences of objects. Iterating is a means to traverse over the set
 * of items in some order, that guarantees that all items will be visited exactly once.
 * 
 * Iterating works by initializing a data structure with the state describing the current
 * sequence member. Functions are defined to get a pointer to the actual member out of
 * this data structure, and also for stepping to the next member.
 * 
 * Iterators allows code that needs to traverse some sequence to always look the same,
 * regardless of the type of data in the sequence. It also saves time, since the other
 * access functions in the Purple API (of the \c p_something_nth() variety) often have
 * O(n) performance. Using such a function to iterate over \e n items would be O(n*n)
 * total, but with iterators it will be O(n) at worst.
 * 
 * As an example, here is how to iterate over a node's tag groups:
 * \code
 * PIter      iter;
 * PNTagGroup *group;
 * 
 * for(p_node_tag_group_iter(node, &iter);
       (group = p_iter_data(&iter)) != NULL;
 *     p_iter_next(&iter))
 * {
 * 	// ... process group here ...
 * }
 * \endcode
 * 
 * Because the \c p_iter_data() function returns a \c void pointer, there is no need to
 * explicitly cast it to the desired \c PNTagGroup pointer type; C's automatic pointer
 * promotion rules does the conversion automatically.
 * @{
*/


/**
 * Return the current index, counting from 0, of an iterator.
 * 
 * While the point of iterators is to iterate over a sequence of data pieces without actually
 * using an integer index to identify each one, it is sometimes useful to have such an index
 * anyway. Therefore, the iterator data structure contains an integer internally, which is
 * updated whenever \c p_iter_next() is called.
 * 
*/
PURPLEAPI unsigned int p_iter_index(const PIter *iter	/** The iterator whose index is sought. */)
{
	if(iter != NULL)
		return iter->index;
	return 0;
}

/**
 * Return the data piece pointed at by the an iterator. Returns \c NULL if given an invalid
 * iterator.
*/
PURPLEAPI void * p_iter_data(PIter *iter	/** The iterator whose data is to be accessed. */)
{
	if(iter == NULL)
		return NULL;
	if(iter->flags & ITER_DYNARR)
		return dynarr_index(iter->data.dynarr.arr, iter->data.dynarr.index);
	else if(iter->flags & ITER_LIST)
		return list_data(iter->data.list);
	return NULL;
}

/**
 * Step an iterator to the next sequence member. This is the core functionality of iterators, the one
 * that does the actual iterating. If it succeeds, this will as a side-effect increase the index returned
 * by \c p_iter_index().
*/
PURPLEAPI void p_iter_next(PIter *iter	/** The iterator to be stepped forward. */)
{
	if(iter == NULL)
		return;
	if(iter->flags & ITER_DYNARR)
	{
		if(iter->data.dynarr.validate)
			iter->data.dynarr.index = iter->data.dynarr.validate(iter, iter->data.dynarr.index + 1);
		else
			iter->data.dynarr.index++;
	}
	else if(iter->flags & ITER_LIST)
		iter->data.list = list_next(iter->data.list);
	else
		return;
	iter->index++;
}

/** @} */
