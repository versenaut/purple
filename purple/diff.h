/*
 * diff.h
 *
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Header for diff:ing module based on original code by Michael B.
 * Allen but sufficiently "Purple-ized" to warrant its own copyright
 * now I think.
*/

typedef enum { DIFF_MATCH = 1, DIFF_DELETE, DIFF_INSERT } DiffOp;

typedef struct
{
	DiffOp	op;
	int	off;	/* Offset into a if MATCH or DELETE but b if INSERT. */
	int	len;
} DiffEdit;

/* Simple comparison, cuts down on number of arguments needed. */
extern int	diff_compare_simple(const void *a, size_t n, const void *b, size_t m, DynArr *edits);

/* Expose all bells and whistles of the diffing algorithm. */
extern int	diff_compare(const void *a, int aoff, int n, const void *b, int boff, int m,
				int dmax, DynArr *edits, DynArr *buf);
