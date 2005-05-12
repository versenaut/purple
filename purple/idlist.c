/*
 * idlist.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * A list of IDs, used for dependent-tracking by graph modules. To reduce memory allocation
 * overhead, we simply stuff both ID and the required count (this is a multiset) into the
 * list data pointer. This limits both ID and count ranges, but... Should be OK anyway.
 * 
 * The layout used is: a void * is assumed to hold at least 32 bits. The lower 16 bits are
 * used for the ID, on the theory it's quicker to access it that way. The upper 16 bits are
 * used for the count, which is less-often accessed. The entry list is kept sorted on ID.
*/

#include "verse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"
#include "memchunk.h"
#include "list.h"
#include "log.h"
#include "strutil.h"

#include "idlist.h"

/* ----------------------------------------------------------------------------------------- */

#define	MASK_ID		((1 << 16) - 1)
#define	SHIFT_ID	0
#define	MASK_COUNT	(~MASK_ID)
#define	SHIFT_COUNT	16

/* ----------------------------------------------------------------------------------------- */

IdList * idlist_new(void)
{
	IdList	*il;

	if((il = mem_alloc(sizeof *il)) != NULL)
		idlist_construct(il);
	return il;
}

void idlist_construct(IdList *il)
{
	if(il != NULL)
		il->entries = NULL;
}

static int cmp_find(const void *listdata, const void *data)
{
	uint32	listid = ((uint32) listdata & MASK_ID) >> SHIFT_ID,
		newid  = ((uint32) data & MASK_ID) >> SHIFT_ID;

	return listid < newid ? -1 : listid > newid;
}

static int cmp_insert(const void *data1, const void *data2)
{
	uint32	id1 = ((uint32) data1 & MASK_ID) >> SHIFT_ID,
		id2  = ((uint32) data2 & MASK_ID) >> SHIFT_ID;

	return id1 < id2 ? -1 : id1 > id2;
}

void idlist_insert(IdList *il, unsigned int id)
{
	List	*iter;

	if(il == NULL)
		return;
	if((iter = list_find_sorted(il->entries, (void *) id, cmp_find)) != NULL)
	{
		uint32	d = (uint32) list_data(iter);
		uint16	count;

		count = (d & MASK_COUNT) >> SHIFT_COUNT;
		count++;
		d &= ~MASK_COUNT;
		d |= count << SHIFT_COUNT;

		list_data_set(iter, (void *) d);
	}
	else
	{
		uint32	d = (id << SHIFT_ID) | (1 << SHIFT_COUNT);

		il->entries = list_insert_sorted(il->entries, (void *) d, cmp_insert);
		if(id > 1 << 16)
			LOG_WARN(("Insertion of module ID %u in IdSet breaks implementation assumption, failing", id));
	}
}

void idlist_remove(IdList *il, unsigned int id)
{
	List	*el;

	if(il == NULL)
		return;
	if((el = list_find_sorted(il->entries, (void *) id, cmp_find)) != NULL)
	{
		uint32	d = (uint32) list_data(el);
		uint16	count;

		count = (d & MASK_COUNT) >> SHIFT_COUNT;
		count--;
		if(count > 0)		/* Writeback required? */
		{
			d &= ~MASK_COUNT;
			d |= count << SHIFT_COUNT;
			list_data_set(el, (void *) d);
		}
		else
		{
			il->entries = list_unlink(il->entries, el);
			list_destroy(el);
		}
	}
}

void idlist_foreach_init(const IdList *il, IdListIter *iter)
{
	if(il == NULL || iter == NULL)
		return;
	iter->iter = il->entries;
}

boolean idlist_foreach_step(const IdList *il, IdListIter *iter)
{
	if(il == NULL || iter == NULL)
		return FALSE;
	if(iter->iter == NULL)
		return FALSE;
	iter->id = (((uint32) list_data(iter->iter)) & MASK_ID) >> SHIFT_ID;
	iter->iter = list_next(iter->iter);
	return TRUE;
}

void idlist_destruct(IdList *il)
{
	if(il != NULL)
		list_destroy(il->entries);
}

void idlist_destroy(IdList *il)
{
	if(il != NULL)
	{
		idlist_destruct(il);
		mem_free(il);
	}
}

/* ----------------------------------------------------------------------------------------- */

void idlist_test_as_string(const IdList *il, char *buf, size_t buf_max)
{
	List	*prev, *iter;
	char	piece[32], *put = buf;
	size_t	size;

	*put++ = '[';
	*put = '\0';
	buf_max -= 2;
	for(prev = NULL, iter = il->entries; iter != NULL; prev = iter, iter = list_next(iter))
	{
		uint32	d = (uint32) list_data(iter), id, count;

		id    = (d & MASK_ID) >> SHIFT_ID;
		count = (d & MASK_COUNT) >> SHIFT_COUNT;
		size = snprintf(piece, sizeof piece, "%s(%u;%u)", prev != NULL ? " " : "", id, count);
		if(buf_max >= size)
		{
			strcat(put, piece);
			put += size;
			buf_max -= size;
		}
		else
			break;
	}
	if(buf_max)
		strcpy(put, "]");
}
