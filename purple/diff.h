/*
 * diff.h
 *
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Header for diff:ing module based on original code by Michael B.
 * Allen but sufficiently "Purple-ized" to warrant its own copyright
 * now I think.
*/

typedef int		(*cmp_fn)(const void *object1, const void *object2, void *context);
typedef const void *	(*idx_fn)(const void *s, int idx, void *context);

typedef enum { DIFF_MATCH = 1, DIFF_DELETE, DIFF_INSERT } diff_op;

struct diff_edit
{
	short	op;
	int	off;	/* Offset into a if MATCH or DELETE but b if INSERT. */
	int	len;
};

/* consider alternate behavior for each NULL parameter  */
extern int diff_compare(const void *a, int aoff, int n, const void *b, int boff, int m,
		idx_fn idx, cmp_fn cmp, void *context, int dmax,
		DynArr *ses, int *sn,
		DynArr *buf);
