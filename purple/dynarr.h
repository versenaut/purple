/*
 * A dynamic array, to save me some boring realloc() logic all over the place.
*/

typedef struct DynArr	DynArr;

/* Initialize dynarr module. This must be called before any of the below functions are used. */
extern void	dynarr_init(void);

/* Create a new dynamic array with the given element and page sizes. Arrays hold actual
 * elements, not just pointers to them, and grow by <page_size> elements at a time.
*/
extern DynArr *	dynarr_new(size_t elem_size, size_t page_size);

/* Set a default element value, which will be copied into newly allocated elements. Set
 * to NULL (the default) to not have a default element. The pointer to the element is
 * retained, the default element itself is not copied at this point.
*/
extern void	dynarr_set_default(DynArr *da, const void *element);

/* Index into the dynamic array. Returns a pointer to the indicated element, or NULL if
 * it is out of bounds. Does not cause the array to grow, use set for that.
*/
extern void *	dynarr_index(const DynArr *da, unsigned index);

/* Set the given index to the given content. Causes the array to re-allocate itself if
 * the indicated position is outside its current bounds. Returns pointer to element in
 * array. The initializer <element> may be NULL to do allocation only.
*/
extern void *	dynarr_set(DynArr *da, unsigned int index, const void *element);

/* Does a set on the next non-allocated index. Returns index. */
extern unsigned int	dynarr_append(DynArr *da, const void *element);

/* Returns the number of slots in the array. */
extern size_t	dynarr_size(const DynArr *da);

/* Sort the array. The comparison function is defined as for qsort(). */
extern void	dynarr_sort(DynArr *da, int (*cmp)(const void *el1, const void *el2));

/* Destroy an array an all the elements held in it. */
extern void	dynarr_destroy(DynArr *da);
