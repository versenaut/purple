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

/* consider alternate behavior for each NULL parameter  */
extern int diff_compare(const void *a, int aoff, int n, const void *b, int boff, int m,
		int dmax, DynArr *ses, int *sn, DynArr *buf);
