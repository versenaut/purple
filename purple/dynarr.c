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
	void		(*def_func)(unsigned int index, void *element, void *user);
	void		*def_func_user;
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
	da->def_func = NULL;

	return da;
}

DynArr * dynarr_new_copy(const DynArr *src, void (*element_copy)(void *dst, const void *src, void *user), void *user)
{
	DynArr		*da;
	unsigned int	i;

	if(src == NULL)
		return NULL;
	da = dynarr_new(src->elem_size, src->page_size);
	dynarr_set_default(da, src->def);
	dynarr_set_default_func(da, src->def_func, src->def_func_user);
	if(src->next > 0)		/* Sneakily "pre-warm" the array, to minimize allocations in loop below. */
		dynarr_set(da, src->next - 1, NULL);
	for(i = 0; i < src->next; i++)
	{
		void	*dst;

		if(element_copy != NULL)
		{
			if((dst = dynarr_set(da, i, NULL)) != NULL)
 				element_copy(dst, dynarr_index(src, i), user);
		}
		else
			dynarr_set(da, i, dynarr_index(src, i));
	}
	return da;
}

size_t dynarr_get_elem_size(const DynArr *da)
{
	return da != NULL ? da->elem_size : 0;
}

void dynarr_set_default(DynArr *da, const void *def)
{
	if(da != NULL)
		da->def = def;
}

void dynarr_set_default_func(DynArr *da, void (*func)(unsigned int, void *element, void *user), void *user)
{
	if(da != NULL)
	{
		da->def_func = func;
		da->def_func_user = user;
	}
}

void * dynarr_index(const DynArr *da, unsigned index)
{
	if(da == NULL)
		return NULL;
	if(index < da->alloc)
		return (unsigned char *) da->data + index * da->elem_size;
	return NULL;
}

void * dynarr_set(DynArr *da, unsigned int index, const void *element)
{
	if(da == NULL)
		return NULL;
	if(index >= da->alloc)
	{
		size_t	size;
		void	*nd;

		/* Compute new size, might grow more than one page. */
		size = da->page_size * ((da->alloc + index + 1 + da->page_size - 1) / da->page_size);
		nd = mem_realloc(da->data, size * da->elem_size);

		if(nd != NULL)
		{
/*			LOG_MSG(("Dynarr: Grew array to %u elements of size %u", size, da->elem_size));*/
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
			else if(da->def_func != NULL)
			{
				unsigned int	i;

				for(i = da->alloc; i < size; i++)
				{
					if(i == index)
						continue;
					da->def_func(i, (char *) nd + i * da->elem_size, da->def_func_user);
				}
			}
			da->data  = nd;
			da->alloc = size;
		}
		else
			LOG_ERR(("Dynamic array couldn't grow to %u elements; realloc() failed", da->alloc + da->page_size));
	}
	if(element != NULL)
		memcpy((char *) da->data + index * da->elem_size, element, da->elem_size);
	if(index >= da->next)
		da->next = index + 1;
	return (char *) da->data + index * da->elem_size;
}

void * dynarr_append(DynArr *da, const void *element, unsigned int *index)
{
	unsigned int	i = da->next;

	if(index != NULL)
		*index = i;

	return dynarr_set(da, i, element);
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
