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
