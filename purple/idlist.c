/*
 * A list of IDs, used for dependent-tracking by graph modules. Tries to be at least marginally
 * clever and store IDs below a certain (see header) limit efficiently as bits set in a mask,
 * and resorting to an actual linked list for bigger IDs. Might work out okay.
*/

#include "verse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"
#include "list.h"

#include "idlist.h"

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
	{
		memset(il->quick, 0, sizeof il->quick);
		il->slow = NULL;
	}
}

/* Compare IDs, for sorted insert in list. */
static int cmp_id(const void *data1, const void *data2)
{
	unsigned int	a = (unsigned int) data1, b = (unsigned int) data2;

	return a < b ? -1 : a > b;
}

void idlist_insert(IdList *il, unsigned int id)
{
	if(il == NULL)
		return;
	if(id < QUICK_MAX)
		il->quick[id / 32] |= 1 << (id % 32);
	else if(list_find_sorted(il->slow, (void *) id, cmp_id) == NULL)	/* No dupes. */
		il->slow = list_insert_sorted(il->slow, (void *) id, cmp_id);
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

void idlist_destruct(IdList *il)
{
	if(il != NULL)
		list_destroy(il->slow);
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

static int cb_test_as_string(unsigned int id, void *data)
{
	void	**d = data;
	char	num[32];
	size_t	len;

	len = sprintf(num, "%s%u", d[2] ? " " : "", id);
	d[2] = (void *) 1;
	if((size_t) d[1] >= len)
	{
		strcat(d[0], num);
		d[1] -= len;
		return TRUE;
	}
	return FALSE;
}

void idlist_test_as_string(const IdList *il, char *buf, size_t buf_max)
{
	void	*data[3];

	data[0] = buf;
	data[1] = (void *) buf_max;
	data[2] = 0;

	buf[0] = '\0';
	idlist_foreach(il, cb_test_as_string, data);
}
