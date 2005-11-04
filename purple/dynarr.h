/*
 * dynarr.h
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * A dynamic array, to save me some boring realloc() logic all over the place.
 *
 * NOTE: This doesn't pay attention to alignment restrictions very much, but
 * typially that shouldn't be a problem. Might bite when compiling on non-x86
 * hardware, perhaps...
*/

#if !defined DYNARR_H
#define	DYNARR_H

typedef struct DynArr	DynArr;

/* Initialize dynarr module. This must be called before any of the below functions are used. */
extern void	dynarr_init(void);

/* Create a new dynamic array with the given element and page sizes. Arrays hold actual
 * elements, not just pointers to them, and grow by <page_size> elements at a time.
*/
extern DynArr *	dynarr_new(size_t elem_size, size_t page_size);

/* Create a new dynamic array that holds a copy of the <src> array and its data. The element_copy
 * function is called with each element, and should return a pointer to a copy to insert at the
 * present index, or NULL if the element should be left undefined. Elements that are not assigned
 * in the source will be *copied*, defaulting is not used during copy. The <user> pointer is handy
 * for context needed to create the copy, and simply passed along.
*/
extern DynArr *	dynarr_new_copy(const DynArr *src, void (*element_copy)(void *dst, const void *src, void *user), void *user);

/* Return elem_size from creation. Handy for external indexing. */
extern size_t	dynarr_get_elem_size(const DynArr *da);

/* Set a default element value, which will be copied into newly allocated elements. Set
 * to NULL (the default) to not have a default element. The pointer to the element is
 * retained, the default element itself is not copied at this point.
*/
extern void	dynarr_set_default(DynArr *da, const void *element);

/* Register a callback that is used to initialize newly allocated elements. */
extern void	dynarr_set_default_func(DynArr *da, void (*set)(unsigned int index, void *element, void *user), void *user);

/* Index into the dynamic array. Returns a pointer to the indicated element, or NULL if
 * it is out of bounds. Does not cause the array to grow, use set for that.
*/
extern void *	dynarr_index(const DynArr *da, unsigned index);

/* Grow the given array to end with the <index>. This will set any newly allocated elements
 * to the default value (if one exists). If <default_at_index> is 0, the indicated index
 * will not be initialized to the default. This limiting *ONLY* affects "simple" arrays,
 * i.e. arrays that do not use a a defaulting function. Defaulting functions are always
 * called as new elements are allocated, regardless of the <default_at_index> value.
*/
extern void	dynarr_grow(DynArr *da, unsigned int index, int default_at_index);

/* Set the given index to the given content. Causes the array to re-allocate itself if
 * the indicated position is outside its current bounds. Returns pointer to element in
 * array. The initializer <element> may be NULL to do allocation only.
*/
extern void *	dynarr_set(DynArr *da, unsigned int index, const void *element);

/* Does a set on the next non-allocated index. Returns pointer to storage, sets <index> is non-NULL. */
extern void *	dynarr_append(DynArr *da, const void *element, unsigned int *index);

/* Returns the number of slots in the array. */
extern size_t	dynarr_size(const DynArr *da);

/* Sort the array. The comparison function is defined as for qsort(). */
extern void	dynarr_sort(DynArr *da, int (*cmp)(const void *el1, const void *el2));

/* Clear an existing dynamic array, making the next append have index 0. */
extern void	dynarr_clear(DynArr *da);

/* Destroy an array an all the elements held in it. */
extern void	dynarr_destroy(DynArr *da);

#endif		/* DYNARR_H */
