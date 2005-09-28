/*
 * idtree.c
 * 
 * Copyright (C) 2005 PDC, KTH. See COPYING for license details.
 * 
 * Read the header for details.
*/

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"
#include "memchunk.h"

#include "idtree.h"

#define	MEMSET_NULL				/* Define this if memset() to 0 makes valid NULL pointers. */

typedef struct Page	Page;

struct Page
{
	unsigned short	lsb;			/* Index of the least significant bit of IDs used to index this page. Constant once the page is created. */
	unsigned short	size;			/* Number of used slots in this page. */
	Page		*parent;
	void		**ptr;
	/* Array of (1 << IdTree.bits) pointers goes here. */
};

struct IdTree
{
	MemChunk	*chunk_page;
	MemChunk	*chunk_element;		/* Chunk for elements. */
	unsigned int	bits;
	unsigned int	depth;			/* Current depth, number of indirections to reach objects. A function of the 'next' field. */
	unsigned int	depth_max;
	unsigned int	next_id;		/* Next free, unused, maximum, id. */
	size_t		size;			/* Number of inserted items. */
	size_t		page_alloc;

	Page		*root;			/* A root page. */
};

static size_t page_size(unsigned int bits)
{
	return sizeof (Page) + (1 << bits) * sizeof (void *);
}

/* Extract and return the <width> bits of <x> that have their lsb at <pos> <width> bits
 * wide slots, counting from the right (i.e., pos == 0 returns true lsbs of x).
 * The result is returned shifted down to the <width> lower-most bits of an unsigned int.
 * 
 * Example:	mask(0xfead, 2, 4) -> 0xe, i.e. the 3rd nibble, from the right.
*/
static unsigned int mask(unsigned int x, unsigned int pos, unsigned int width)
{
	return (x >> (pos * width)) & ((1u << width) - 1);
}

/* Simply horrible. :) Got this idea from Googling, and I guess it makes more sense
 * than using a for-loop. Could perhaps optimize (?) a bit by using a lookup table?
*/
static unsigned int msb(unsigned int x)
{
	unsigned int	m = 0;

	if(x & 0xffff0000)
	{
		m = 16;
		x >>= 16;
	}
	if(x & 0xff00)
	{
		if(x & 0xf000)
		{
			if(x & 0xc000)
				return m + ((x & 0x8000) ? 15 : 14);
			return m + ((x & 0x2000) ? 13 : 12);
		}
		if(x & 0xc00)
			return m + ((x & 0x800) ? 11 : 10);
		return m + ((x & 0x200) ? 9 : 8);
	}
	if(x & 0xf0)
	{
		if(x & 0xc0)
			return m + ((x & 0x80) ? 7 : 6);
		return m + ((x & 0x20) ? 5 : 4);
	}
	if(x & 0x3)
		return m + ((x & 2) ? 1 : 0);
	return m + ((x & 8) ? 3 : 2);
}

static Page * page_alloc(IdTree *tree, int level)
{
	Page	*p = memchunk_alloc(tree->chunk_page);

	p->lsb  = level * tree->bits;
/*	printf("page allocated at %p, lsb %u\n", p, p->lsb);*/
	p->size = 0;
	p->ptr  = NULL;
	p->ptr  = (void **) (p + 1);
#if defined MEMSET_NULL
	memset(p->ptr, 0, (1u << tree->bits) * sizeof *p->ptr);
#else
	{
		unsigned int	i;

		for(i = 0; i < (1u << tree->bits); i++)
			p->ptr[i] = NULL;
	}
#endif
	tree->page_alloc++;

	return p;
}

static void page_free(IdTree *tree, Page *p)
{
	memchunk_free(tree->chunk_page, p);
	tree->page_alloc--;
}

IdTree * idtree_new(size_t elem_size, size_t elem_chunk_size, unsigned int bits)
{
	IdTree	*tree;

	tree = mem_alloc(sizeof *tree);
	if(tree == NULL)
		return NULL;
	tree->chunk_page    = memchunk_new("idtree.pagechunk", page_size(bits), 8);
	tree->chunk_element = memchunk_new("idtree.elemchunk", elem_size, elem_chunk_size);
	tree->bits = bits;
	tree->next_id = 0;
	tree->depth = 0;
	tree->depth_max = (CHAR_BIT * sizeof (unsigned int) + bits - 1) / bits;
	tree->page_alloc = 0;
	tree->root = NULL;

	return tree;
}

static void recursive_copy_page(IdTree *dst, const IdTree *src, const Page *page, unsigned int id, void (*element_copy)(void *dst, unsigned int id, const void *src, void *user), void *user)
{
	unsigned int	i;

	if(page->lsb == 0)	/* Final level element page? */
	{
		for(i = 0; i < (1u << src->bits); i++)
		{
			if(page->ptr[i] != NULL)
			{
				if(element_copy == NULL)
					idtree_set(dst, id | i, page->ptr[i]);
				else
				{
					void	*el;

					el = idtree_set(dst, id | i, NULL);
					element_copy(el, id | i, page->ptr[i], user);
				}
			}
		}
	}
	else			/* Internal index page. */
	{
		for(i = 0; i < (1u << src->bits); i++)
		{
			if(page->ptr[i] != NULL)
				recursive_copy_page(dst, src, page->ptr[i], id | (i << page->lsb), element_copy, user);
		}
	}
}

/* Copy an IdTree. This simply (?) recurses down to the level 0 pages, while maintaining the ID path to get there,
 * and does set()s on the new tree with the old one's data. Optionally uses a callback to do the copy.
*/
IdTree * idtree_new_copy(const IdTree *src, void (*element_copy)(void *dst, unsigned int id, const void *src, void *user), void *user)
{
	IdTree		*nt;
	unsigned int	i;

	if(src == NULL)
		return NULL;

	nt = idtree_new(memchunk_chunk_size(src->chunk_element), memchunk_growth(src->chunk_element), src->bits);

	for(i = 0; i < (1u << src->bits); i++)
	{
		if(src->root->ptr[i] != NULL)
			recursive_copy_page(nt, src, src->root, i << src->root->lsb, element_copy, user);
	}
/*	printf("copy has %u elements in %u pages\n", idtree_size(nt), nt->page_alloc);*/

	return nt;
}

#define	PAGE_SET(p, s, e)	do { p->ptr[s] = e; p->size++;    } while(0)
#define	PAGE_CLR(p, s)		do { p->ptr[s] = NULL; p->size--; } while(0)

void * idtree_set(IdTree *tree, unsigned int id, const void *el)
{
	unsigned int	m, nd, slot;
	void		*ne;
	Page		*p, *np, *pp;
	int		i;

	if(tree == NULL)
		return NULL;

	m = msb(id);
/*	printf("msb of %x is %u\n", id, m);*/
	nd = (m + tree->bits - 1) / tree->bits;
/*	printf(" so we need %u levels\n", nd);*/
	if(nd < tree->depth)
		nd = tree->depth;

	if((ne = memchunk_alloc(tree->chunk_element)) == NULL)
		return NULL;
	if(el != NULL)
		memcpy(ne, el, memchunk_chunk_size(tree->chunk_element));

	if(nd > tree->depth)
	{
/*		printf("new required depth %u is larger than existing %u -- this means we need a new root\n", nd, tree->depth);*/
		/* Build missing pages, "backwards", i.e. down to level 0. */
		for(i = tree->depth, pp = tree->root; i < (int) nd; i++)
		{
/*			printf(" building page at level %d\n", i);*/
			np = page_alloc(tree, i);
/*			printf("  lsb=%u\n", np->lsb);*/
			slot = (tree->depth > 0) ? 0 : mask(id, i, tree->bits);
			if(pp != NULL)
			{
				PAGE_SET(np, slot, pp);
/*				printf("  page with lsb %u linked from slot %x\n", pp->lsb, slot);*/
				pp->parent = np;
			}
			pp = np;
		}
		tree->root = pp;
/*		printf("tree deepened, %u pages allocated\n", tree->page_alloc);*/
		tree->depth = nd;
	}

	/* Walk the tree from the root and on down, adding new internal index pages as/if needed. */
	for(i = nd - 1, p = tree->root; i > 0; i--)
	{
/*		printf("take a look at depth %d\n", i);*/
		slot = mask(id, i, tree->bits);
/*		printf(" slot=%x\n", slot);*/
		if(p->ptr[slot] == NULL)
		{
/*			printf("  next level page missing, adding at level %d\n", i - 1);*/
			PAGE_SET(p, slot, page_alloc(tree, i - 1));
/*			p->ptr[slot] = page_alloc(tree, i - 1);
			p->size++;
*/		}
		p = p->ptr[slot];
	}
/*	printf("final page: %p at %d, size %u\n", p, p->lsb, p->size);*/
	slot = mask(id, 0, tree->bits);
	if(p->ptr[slot] == NULL)
	{
		tree->size++;
		p->size++;
	}
	p->ptr[slot] = ne;
	if(id >= tree->next_id)
		tree->next_id = id + 1;
/*	printf("insert at %u, next now %u\n", id, tree->next_id);*/
/*	printf("insert at %u done, size=%u in %u pages\n", id, tree->size, tree->page_alloc);*/
	return ne;
}

void * idtree_append(IdTree *tree, const void *el, unsigned int *id)
{
	unsigned int	nid;

	if(tree == NULL)
		return NULL;
	nid = tree->next_id;
	if(id != NULL)
		*id = nid;
	return idtree_set(tree, nid, el);
}

void * idtree_get(const IdTree *tree, unsigned int id)
{
	unsigned int	m;
	int		i;
	const Page	*p;

	if(tree == NULL)
		return NULL;
	m = (msb(id) + tree->bits - 1) / tree->bits;
	if(m > tree->depth)
		return NULL;
	/* Drill down to final page. This should be reasonably quick. */
	for(i = tree->depth - 1, p = tree->root; i > 0; i--)
	{
		if(p == NULL)
			return NULL;
		p = p->ptr[mask(id, i, tree->bits)];
	}
	return p->ptr[mask(id, 0, tree->bits)];
}

void idtree_remove(IdTree *tree, unsigned int id)
{
	int		i;
	unsigned int	slot, k;
	Page		*p, *path[CHAR_BIT * sizeof (unsigned int)];
	void		*el;

	if(tree == NULL || id >= tree->next_id)
		return;
	for(i = tree->depth - 1, p = tree->root; i > 0; i--)
	{
		path[i] = p;
		p = p->ptr[mask(id, i, tree->bits)];
		if(p == NULL)
			return;
	}
	slot = mask(id, 0, tree->bits);
	if((el = p->ptr[slot]) != NULL)	/* Does an element exist in final page? */
	{
		memchunk_free(tree->chunk_element, el);
		PAGE_CLR(p, slot);
/*		printf("size of page at %p now %u\n", p, p->size);*/
		if(p->size == 0)	/* If page got empty, track backwards and remove any useless pages. */
		{
/*			printf("tail page at %p cleared, tracking path\n", p);*/
			page_free(tree, p);
			for(k = 1; k <= tree->depth - 1; k++)
			{
				slot = mask(id, k, tree->bits);
				PAGE_CLR(path[k], slot);
/*				path[k]->ptr[slot] = NULL;*/
/*				printf("removing from slot %x in page at %p, size=%u\n", slot, path[k], path[k]->size);*/
				if(path[k]->size != 0)
				{
/*					printf("page at %p still has %u valid pointers, not removing\n", path[k], path[k]->size);*/
					break;
				}
				page_free(tree, path[k]);
/*				printf("page %d is at %p, lsb %u\n", k, path[k], path[k]->lsb);*/
			}
		}
		tree->size--;
/*		printf("remove done, size=%u in %u pages\n", tree->size, tree->page_alloc);*/
	}
}

size_t idtree_size(const IdTree *tree)
{
	if(tree == NULL)
		return 0;
	return tree->size;
}

/* Compute ID of first element in tree. */
unsigned int idtree_foreach_first(const IdTree *tree)
{
	unsigned int	i, j, id = 0, ps;
	const Page	*p;

	if(tree == NULL)
		return 0;
	if(tree->size == 0)
		return 0;
	ps = 1u << tree->bits;
	p = tree->root;
	for(i = 0; i < tree->depth; i++)
	{
		for(j = 0; i < ps; j++)
		{
			if(p->ptr[j] != NULL)
			{
				id |= j << p->lsb;
				if(p->lsb == 0)
					return id;
				p = p->ptr[j];
				break;
			}
		}
	}
	return 0;
}

/* Drawing a simple tree really helps in understanding this routine. It's
 * basically a rather involved tree traversal. This is where that 'parent'
 * pointer in Pages start to pay off, in a big way.
*/
unsigned int idtree_foreach_next(const IdTree *tree, unsigned int id)
{
	int		i;
	unsigned int	d, j, k, ps, id0 = id;
	const Page	*p;

	if(tree == NULL)
		return id;
	if(id >= tree->next_id)
		return id;

	ps = 1u << tree->bits;				/* Buffer the page size; really handy. */
	d = (msb(id) + tree->bits - 1) / tree->bits;	/* Compute depth required by id. */
	if(d > tree->depth)
		return id;
	/* Drill down to the expected page. This should be reasonably quick. */
	for(i = tree->depth - 1, p = tree->root; i > 0; i--)
	{
		if(p == NULL)
			return id;
		p = p->ptr[mask(id, i, tree->bits)];
	}
	/* Look for element in same page as previous. */
	for(j = (id & (ps - 1)) + 1; j < ps; j++)
	{
		if(p->ptr[j] != NULL)
			return (id & ~(ps - 1)) | j;	/* Assemble the resulting ID. */
	}
	/* If there was no hit there, we need to traverse up to a parent page, then
	 * down again as soon as we can. This is ... slightly less expensive than a
	 * brand new car.
	*/
	do
	{
		p = p->parent;			/* Take the step up to a parent page. */
		if(p == NULL)
			break;
		id += 1u << p->lsb;		/* Increase in the required bit. */
		id &= ~((1 << p->lsb) - 1);	/* Mask away lowest bits, they're old. */
		for(j = ((id >> p->lsb) & (ps - 1)) + 1; j < ps; j++)
		{
			if(p->ptr[j] != NULL)
			{
				/* Found inner page with non-NULL slot, drill down again to first leaf. */
				for(id = j << p->lsb, p = p->ptr[j]; p != NULL;)
				{
					for(k = 0; k < ps; k++)
					{
						if(p->ptr[k] != NULL)
						{
							id |= k << p->lsb;	/* Add bits to the ID. */
							if(p->lsb == 0)
								return id;
							p = p->ptr[k];		/* Keep walking if not leaf. */
						}
					}
				}
				/* This can't happen. If an inner page has non-
				 * NULL slot, there MUST be a leaf page there.
				 */
				return id0 + 1;
			}
		}
	} while(p != NULL);
	return id0 + 1;	/* This is a failing ID. */
}

void idtree_destroy(IdTree *tree)
{
	if(tree == NULL)
		return;
	/* No need here to traverse, just free the MemChunks and the tree itself, and that's that. */
	memchunk_destroy(tree->chunk_element);
	memchunk_destroy(tree->chunk_page);
	mem_free(tree);
}
