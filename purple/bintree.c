/*
 * bintree.c
 * 
 * Copyright (C) 2005 PDC, KTH. See COPYING for license details.
 * 
 * A binary tree.
*/

#include <stdio.h>
#include <stdlib.h>

#include "mem.h"
#include "memchunk.h"

#include "bintree.h"

/* ----------------------------------------------------------------------------------------- */

typedef struct Node	Node;

struct Node
{
	const void	*key;
	void		*element;
	Node		*left, *right, *parent;	/* Children and parent pointers. */
};

struct BinTree
{
	Node	*root;
	size_t	size;
	int	(*compare)(const void *key1, const void *key2);
};

static MemChunk	*the_chunk = NULL;

/* ----------------------------------------------------------------------------------------- */

void bintree_init(void)
{
	the_chunk = memchunk_new("BinTree", sizeof (Node), 16);
}

/* ----------------------------------------------------------------------------------------- */

static Node * tree_minimum(const Node *node)
{
	if(node == NULL)
		return NULL;
	while(node->left != NULL)
		node = node->left;
	return (Node *) node;
}

static Node * tree_maximum(const Node *node)
{
	if(node == NULL)
		return NULL;
	while(node->right != NULL)
		node = node->right;
	return (Node *) node;
}

static Node * tree_successor(const Node *node)
{
	Node	*y;

	if(node->right != NULL)
		return tree_minimum(node->right);
	y = node->parent;
	while(y != NULL && node == y->right)
	{
		node = y;
		y = y->parent;
	}
	return y;
}

/* ----------------------------------------------------------------------------------------- */

BinTree * bintree_new(int (*compare)(const void *key1, const void *key2))
{
	BinTree	*t;

	if(compare == NULL)
		return NULL;

	t = mem_alloc(sizeof *t);
	t->root = NULL;
	t->size = 0;
	t->compare = compare;

	return t;
}

/* ----------------------------------------------------------------------------------------- */

BinTree * bintree_new_copy(const BinTree *src, void * (*element_copy)(const void *key, const void *element, void *user), void *user)
{
	BinTree	*t;
	Node	*n;

	if(src == NULL || element_copy == NULL)
		return NULL;
	t = mem_alloc(sizeof *t);
	t->root    = NULL;
	t->size    = 0;
	t->compare = src->compare;

	printf("copying binary tree at %p\n", src);
	/* FIXME: This **really** unbalances the tree, and thus is pessimal. */
	for(n = tree_minimum(src->root); n != NULL; n = tree_successor(n))
	{
		void	*el = element_copy(n->key, n->element, user);

		bintree_insert(t, n->key, el);
	}
	return t;
}

/* ----------------------------------------------------------------------------------------- */

static const Node * tree_lookup(const BinTree *tree, const Node *root, const void *key)
{
	int	cmp;

	if(root == NULL)
		return NULL;
	cmp = tree->compare(key, root->key);
	if(cmp == 0)
		return root;
	else if(cmp < 0)
		return tree_lookup(tree, root->left, key);
	return tree_lookup(tree, root->right, key);
}

void * bintree_lookup(const BinTree *tree, const void *key)
{
	const Node	*x;

	if(tree == NULL)
		return NULL;
	if((x = tree_lookup(tree, tree->root, key)) != NULL)
		return x->element;
	return NULL;
}

void bintree_insert(BinTree *tree, const void *key, void *element)
{
	Node	*y, *x, *z;

	y = NULL;
	x = tree->root;

	while(x != NULL)
	{
		y = x;
		if(tree->compare(key, x->key) < 0)
			x = x->left;
		else
			x = x->right;
	}
	z = memchunk_alloc(the_chunk);
	z->key = key;
	z->element = element;
	z->left = z->right = NULL;
	z->parent = y;
	if(y == NULL)
		tree->root = z;
	else if(tree->compare(key, y->key) < 0)
		y->left = z;
	else
		y->right = z;
	tree->size++;
}

static void do_print(const Node *root)
{
	if(root == NULL)
	{
		printf("-");
		return;
	}
	printf("[%p ", root->key);
	do_print(root->left);
	printf(" ");
	do_print(root->right);
	printf("]");
}

/* ----------------------------------------------------------------------------------------- */

void bintree_remove(BinTree *tree, const void *key)
{
	Node	*x, *y, *z;

	z = (Node *) tree_lookup(tree, tree->root, key);
	if(z == NULL)
		return;
	if(z->left == NULL || z->right == NULL)
		y = z;
	else
		y = tree_successor(z);

	if(y->left != NULL)
		x = y->left;
	else
		x = y->right;

	if(x != NULL)
		x->parent = y->parent;
	if(y->parent == NULL)
		tree->root = x;
	else
	{
		if(y == y->parent->left)
			y->parent->left = x;
		else
			y->parent->right = x;
	}
	if(y != z)
	{
		z->key = y->key;
		z->element = y->element;
	}
	memchunk_free(the_chunk, y);
	tree->size--;
}

/* ----------------------------------------------------------------------------------------- */

size_t bintree_size(const BinTree *tree)
{
	return tree != NULL ? tree->size : 0;
}

const void * bintree_key_minimum(const BinTree *tree)
{
	if(tree != NULL)
	{
		Node	*min = tree_minimum(tree->root);

		if(min != NULL)
			return min->key;
	}
	return NULL;
}

const void * bintree_key_maximum(const BinTree *tree)
{
	if(tree != NULL)
	{
		Node	*max = tree_maximum(tree->root);

		if(max != NULL)
			return max->key;
	}
	return NULL;
}

/* ----------------------------------------------------------------------------------------- */

void bintree_iter_init(const BinTree *tree, BinTreeIter *iter)
{
	if(tree == NULL || iter == NULL)
		return;
	*iter = tree_minimum(tree->root);
}

int bintree_iter_valid(const BinTreeIter iter)
{
	return iter != NULL;
}

const void * bintree_iter_key(const BinTreeIter iter)
{
	if(iter != NULL)
		return ((Node *) iter)->key;
	return NULL;
}

void * bintree_iter_element(const BinTreeIter iter)
{
	if(iter != NULL)
		return ((Node *) iter)->element;
	return NULL;
}

void bintree_iter_next(BinTreeIter *iter)
{
	if(iter == NULL || *iter == NULL)
		return;

	*iter = tree_successor(*iter);
}

/* ----------------------------------------------------------------------------------------- */

void bintree_print(const BinTree *tree)
{
	do_print(tree->root);
	printf("\n");
}

/* ----------------------------------------------------------------------------------------- */

void bintree_destroy(BinTree *tree, void (*callback)(const void *key, void *element))
{
	Node	*iter, *next;

	if(tree == NULL)
		return;

	for(iter = tree_minimum(tree->root); iter != NULL; iter = next)
	{
		next = tree_successor(iter);
		if(callback != NULL)
			callback(iter->key, iter->element);
		memchunk_free(the_chunk, iter);
	}
	mem_free(tree);
}
