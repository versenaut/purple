/*
 * idset.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * 
*/

#include <stdlib.h>

#include "dynarr.h"
#include "list.h"
#include "log.h"
#include "mem.h"

#include "idset.h"

/* ----------------------------------------------------------------------------------------- */

struct IdSet
{
	unsigned int	offset;		/* Bias all IDs by this amount. */
	DynArr		*arr;
	List		*removed;
	size_t		size;		/* Number of active references. */
	unsigned int	max;		/* Known maximum ID. */
};

IdSet * idset_new(unsigned int offset)
{
	IdSet		*is;
	static void	*def = NULL;

	is = mem_alloc(sizeof *is);
	is->offset = offset;
	is->arr = dynarr_new(sizeof (void *), 4);
	dynarr_set_default(is->arr, &def);
	is->removed = NULL;
	is->size = 0;
	is->max  = 0;

	return is;
}

unsigned int idset_insert(IdSet *is, void *object)
{
	unsigned int	index;

	if(is == NULL || object == NULL)
		return 0;
	if(is->removed != NULL)
	{
		index = (unsigned int) list_data(is->removed);
		is->removed = list_tail(is->removed);
		dynarr_set(is->arr, index, &object);
	}
	else
		dynarr_append(is->arr, &object, &index);
	is->size++;
	is->max = index > is->max ? index : is->max;
	return index + is->offset;
}

unsigned int idset_insert_with_id(IdSet *is, unsigned int id, void *object)
{
	unsigned int	dist, i;

	if(is == NULL || object == NULL)
		return 0;
	id -= is->offset;
	if(id <= is->max)
	{
		List	*iter;
		/* The requested ID is inside the space of previously used IDs. Hopefully,
		 * it's because it's been used and then removed, in which case we can reuse it.
		*/
		for(iter = is->removed; iter != NULL; iter = list_next(iter))
		{
			if(((unsigned int) list_data(iter)) == id)
			{
				dynarr_set(is->arr, id, &object);
				is->removed = list_unlink(is->removed, iter);
				list_destroy(iter);
				return id + is->offset;
			}
		}
		if(is->max > 0)	/* Complain if not found and set not empty. */
			LOG_WARN(("idset_insert_with_id id=%u (offset=%u max=%u) collides with existing data--appending instead", id + is->offset, is->offset, is->max));
		return idset_insert(is, object);
	}
	/* Desired ID is beyond the current range. Extend with fake removes. */
	dist = id - is->max;
	LOG_MSG(("Inserting in idset with known ID %u, distance from max %u is %u\n", id, is->max, dist));
	i = id - 1;
	do
	{
		is->removed = list_prepend(is->removed, (void *) i);
	} while(i-- > is->max);
	dynarr_set(is->arr, id, &object);
	is->max = id;
	return id + is->offset;
}

void idset_remove(IdSet *is, unsigned int id)
{
	void	**objp;

	if(is == NULL)
		return;
	id -= is->offset;
	if((objp = dynarr_index(is->arr, id)) != NULL)
	{
		void	*obj;

		if((obj = *objp) != NULL)
		{
			*objp = NULL;	/* Mark slot as empty. */
			is->removed = list_prepend(is->removed, (void *) id);
			is->size--;
		}
	}
}

size_t idset_size(const IdSet *is)
{
	if(is == NULL)
		return 0;
	return is->size;
}

void * idset_lookup(const IdSet *is, unsigned int id)
{
	void	**objp;

	if(is == NULL)
		return NULL;
	id -= is->offset;
	if(id > is->max)
		return NULL;
	if((objp = dynarr_index(is->arr, id)) != NULL)
		return *objp;
	return NULL;
}

unsigned int idset_foreach_first(const IdSet *is)
{
	unsigned int	id;
	void		**objp;

	if(is == NULL)
		return 0;
	for(id = 0; id <= is->max && id < dynarr_size(is->arr); id++)
	{
		if((objp = dynarr_index(is->arr, id)) != NULL && *objp != NULL)
			return id + is->offset;
	}
	return 0;
}

unsigned int idset_foreach_next(const IdSet *is, unsigned int id)
{
	void	**objp;

	if(is == NULL)
		return 0;
	id -= is->offset;
	while((objp = dynarr_index(is->arr, ++id)) != NULL)
	{
		if(*objp != NULL)
			break;
	}
	return id + is->offset;
}

void idset_destroy(IdSet *is)
{
	if(is == NULL)
		return;

	dynarr_destroy(is->arr);
	list_destroy(is->removed);
	mem_free(is);
}
