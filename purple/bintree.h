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

extern BinTree *	bintree_new(int (*compare)(const void *key1, const void *key2));

extern void		bintree_insert(BinTree *tree, const void *key, void *element);
extern void		bintree_remove(BinTree *tree, const void *key);

extern void *		bintree_lookup(const BinTree *tree, const void *key);

extern size_t		bintree_size(const BinTree *tree);

extern void		bintree_print(const BinTree *tree);

#endif		/* BINTREE_H */
