/*
 * A dynamic array, to save me some realloc() logic all over the place.
*/

#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "mem.h"
#include "memchunk.h"

#include "dynarr.h"

/* ----------------------------------------------------------------------------------------- */

struct DynArr
{
	void		*data;
	size_t		elem_size;
	size_t		page_size;
	size_t		alloc;
	size_t		next;
	const void	*def;
};

static MemChunk *the_chunk = NULL;

/* ----------------------------------------------------------------------------------------- */

void dynarr_init(void)
{
	the_chunk = memchunk_new("DynArr", sizeof (DynArr), 4);
}

/* ----------------------------------------------------------------------------------------- */

DynArr * dynarr_new(size_t elem_size, size_t page_size)
{
	DynArr	*da;

	if(elem_size < 1 || page_size < 1)
		return NULL;

	da = memchunk_alloc(the_chunk);
	da->data = NULL;
	da->elem_size = elem_size;
	da->page_size = page_size;
	da->alloc = 0;
	da->next  = 0;
	da->def = NULL;

	return da;
}

void dynarr_set_default(DynArr *da, const void *def)
{
	if(da != NULL)
		da->def = def;
}

void * dynarr_index(const DynArr *da, unsigned index)
{
	if(da == NULL)
		return NULL;
	if(index < da->alloc)
		return (char *) da->data + index * da->elem_size;
	return NULL;
}

void dynarr_set(DynArr *da, unsigned int index, const void *element)
{
	if(da == NULL || element == NULL)
		return;
	if(index >= da->alloc)
	{
		size_t	size;
		void	*nd;

		/* Compute new size, might grow more than one page. */
		size = da->page_size * ((da->alloc + index + 1 + da->page_size - 1) / da->page_size);
		nd = mem_realloc(da->data, size * da->elem_size);

		if(nd != NULL)
		{
			LOG_MSG(("Dynarr: Grew array to %u elements of size %u", size, da->elem_size));
			if(da->def != NULL)
			{
				unsigned int	i;

				for(i = da->alloc; i < size; i++)
				{
					if(i == index)	/* Don't initialize the element we're about to set. */
						continue;
					memcpy((char *) nd + i * da->elem_size, da->def, da->elem_size);
				}
			}
			da->data  = nd;
			da->alloc = size;
		}
		else
			LOG_ERR(("Dynamic array couldn't grow to %u elements; realloc() failed", da->alloc + da->page_size));
	}
	memcpy((char *) da->data + index * da->elem_size, element, da->elem_size);
	if(index >= da->next)
		da->next = index + 1;
}

void dynarr_append(DynArr *da, const void *element)
{
	dynarr_set(da, da->next, element);
}

size_t dynarr_size(const DynArr *da)
{
	return da == NULL ? 0 : da->next;
}

void dynarr_sort(DynArr *da, int (*cmp)(const void *el1, const void *el2))
{
	if(da == NULL || cmp == NULL)
		return;
	if(da->next == 0)
		return;
	qsort(da->data, da->next, da->elem_size, cmp);
}

void dynarr_destroy(DynArr *da)
{
	if(da != NULL)
	{
		if(da->data != NULL)
			mem_free(da->data);
		memchunk_free(the_chunk, da);
	}
}