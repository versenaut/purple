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

static Node * tree_minimum(const Node *node)
{
	if(node == NULL)
		return NULL;
	while(node->left != NULL)
	{
		node = node->left;
	}
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
