/*
 * bintree.h
 * 
 * Copyright (C) 2005 PDC, KTH. See COPYING for license details.
 * 
 * A binary tree. Requires that keys be ordered, and that the 'compare'
 * function return -1, 0 or 1 for < == > respectively.
*/

#if !defined BINTREE_H
#define	BINTREE_H

typedef struct BinTree	BinTree;

typedef const void *	BinTreeIter;

extern void		bintree_init(void);

extern BinTree *	bintree_new(int (*compare)(const void *key1, const void *key2));

extern BinTree *	bintree_new_copy(const BinTree *src, void * (*element_copy)(const void *key, const void *element, void *user), void *user);

extern void		bintree_insert(BinTree *tree, const void *key, void *element);
extern void		bintree_remove(BinTree *tree, const void *key);

extern void *		bintree_lookup(const BinTree *tree, const void *key);

extern size_t		bintree_size(const BinTree *tree);

extern const void *	bintree_key_minimum(const BinTree *tree);
extern const void *	bintree_key_maximum(const BinTree *tree);

/* Iterator. Uses a BinTreeIter reference, and sets it to point at internal
 * data structure to support iterating. Use like this:
 * BinTree *tree;
 * BinTreeIter iter;	# NOT a pointer, an instance is needed.
 * void *key;
 * for(key = bintree_iter_first(tree, &iter); key != NULL; key = bintree_iter_next(tree, &iter)
 * { use bintree_iter_element(iter) to access key's element. }
*/
extern const void *	bintree_iter_first(const BinTree *tree, BinTreeIter *iter);
extern void *		bintree_iter_element(const BinTreeIter iter);
extern const void *	bintree_iter_next(BinTreeIter *iter);

extern void		bintree_print(const BinTree *tree);

extern void		bintree_destroy(BinTree *tree, void (*callback)(const void *key, void *element));

#endif		/* BINTREE_H */
