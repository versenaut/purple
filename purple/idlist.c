/*
 * 
*/

#include "verse.h"

#include <stdlib.h>
#include <string.h>

#include "mem.h"
#include "list.h"

#include "idlist.h"

#define	QUICK_MAX	64

struct IdList
{
	uint32	quick[QUICK_MAX/32];	/* *Must* be 32-bit, assumed in code below. */
	List	*slow;
};

IdList * idlist_new(void)
{
	IdList	*il;

	il = mem_alloc(sizeof *il);
	memset(il->quick, 0, sizeof il->quick);
	il->slow = NULL;

	return il;
}

void idlist_insert(IdList *il, unsigned int id)
{
	if(il == NULL)
		return;
	if(id < QUICK_MAX)
		il->quick[id / 32] |= 1 << (id % 32);
	else
		il->slow = list_append(il->slow, (void *) id);
}

void idlist_remove(IdList *il, unsigned int id)
{
	if(il == NULL)
		return;
	if(id < QUICK_MAX)
		il->quick[id / 32] &= ~(1 << (id % 32));
	else
		il->slow = list_remove(il->slow, (void *) id);
}

void idlist_foreach(const IdList *il, int (*callback)(unsigned int id, void *data), void *data)
{
	int		i, j;
	const List	*iter;

	if(il == NULL || callback == NULL)
		return;
	for(i = 0; i < sizeof il->quick / sizeof *il->quick; i++)
	{
		if(il->quick[i] == 0)
			continue;
		for(j = 0; j < 32; j++)
		{
			if(il->quick[i] & (1 << j))
				if(!callback(32 * i + j, data))
					return;
		}
	}
	for(iter = il->slow; iter != NULL; iter = list_next(iter))
		if(!callback((unsigned int) list_data(iter), data))
			return;
}

void idlist_destroy(IdList *il)
{
	if(il != NULL)
	{
		list_destroy(il->slow);
		mem_free(il);
	}
}
