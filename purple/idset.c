/*
 * 
*/

#include <stdlib.h>

#include "dynarr.h"
#include "list.h"
#include "mem.h"

#include "idset.h"

/* ----------------------------------------------------------------------------------------- */

struct IdSet
{
	DynArr	*arr;
	List	*removed;
	size_t	size;		/* Number of active references. */
};

IdSet * idset_new(void)
{
	IdSet	*is;

	is = mem_alloc(sizeof *is);
	is->arr = dynarr_new(sizeof (void *), 4);
	is->removed = NULL;
	is->size = 0;

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
		index = dynarr_append(is->arr, &object);
	is->size++;
	return index;
}

void idset_remove(IdSet *is, unsigned int id)
{
	void	**objp;

	if(is == NULL)
		return;
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
	for(id = 0; id < dynarr_size(is->arr); id++)
	{
		if((objp = dynarr_index(is->arr, id)) != NULL && *objp != NULL)
			return id;
	}
	return 0;
}

unsigned int idset_foreach_next(const IdSet *is, unsigned int id)
{
	void	**objp;

	if(is == NULL)
		return 0;
	while((objp = dynarr_index(is->arr, ++id)) != NULL)
	{
		if(*objp != NULL)
			return id;
	}
	return id;
}

void idset_destroy(IdSet *is)
{
	if(is == NULL)
		return;

	dynarr_destroy(is->arr);
	list_destroy(is->removed);
	mem_free(is);
}
