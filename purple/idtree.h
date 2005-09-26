/*
 * idtree.h
 * 
 * Copyright (C) 2005 PDC, KTH. See COPYING for license details.
 * 
 * A hierarchical data-structure that associates an unsigned integer ID with the
 * elements it stores. Low overhead for when sparse IDs are used, lookups should
 * run in O(ln id) time. Elements, once allocated, do not move, even if the tree
 * grows or shrinks.
*/

typedef struct IdTree	IdTree;

/* Create a new IdTree. The two first arguments control the MemChunk used internally
 * to allocate space for elements. Elements are always stored internally by the tree;
 * to make a tree of pointers specify a suitable element size. The <bits> argument
 * controls the "page size" of the tree; a large value gives a short but wide tree,
 * which uses more memory for index pages. Suggested values to use are in the [4,8]
 * range, depending on the expected size of the ID space.
*/
extern IdTree *	idtree_new(size_t elem_size, size_t elem_chunk_size, unsigned int bits);

/* Create a new IdTree, by copying an existing one. All parameters of the copy will
 * be the same as those of the source. Element data will be copied bit-by-bit over
 * to the new one, or by the user-supplied callback if so desired.
*/
extern IdTree *	idtree_new_copy(const IdTree *src, void (*element_copy)(void *dst, unsigned int id, const void *src, void *user), void *user);

/* This inserts an element into the tree. It's not called "insert", since it allows
 * the user to specify the ID, thus being a bit more like an array set. The specified
 * ID *will* be used. Existing data is overwritten. If <element> is NULL, no copying
 * into the tree is done. In any case, a pointer to the tree-owned memory holding the
 * element is returned, applications can copy data in there after the call.
*/
extern void *	idtree_set(IdTree *tree, unsigned int id, const void *element);

/* Insert a element, using the next free ID to do so. The next free ID is the largest
 * valid ID used so far, plus one. For an empty tree, the next free ID is 0. The <id>
 * variable will be set to the ID used, and the tree-owned storage will be returned.
*/
extern void *	idtree_append(IdTree *tree, const void *element, unsigned int *id);

/* Like append(), with without a user-supplied element. Returns pointer to uninitialized
 * tree-owned element storage.
*/
extern void *	idtree_append2(IdTree *tree, unsigned int *id);

/* Look up an ID, and return the corresponding tree-owned element pointer. */
extern void *	idtree_get(const IdTree *tree, unsigned int id);

/* Remove the indicated element from the tree, causing future lookups to return NULL.
 * This will compress the tree when possible, to flush unused index pages.
*/
extern void	idtree_remove(IdTree *tree, unsigned int id);

/* Return the number of currently held elements. This increases by one on append(),
 * (or set() with a "new" ID), and decreases by one on remove() with a valid ID.
*/
extern size_t	idtree_size(const IdTree *tree);
