/* diff - compute a shortest edit script (SES) given two sequences
 * Copyright (c) 2004 Michael B. Allen <mba2000 ioplex.com>
 *
 * The MIT License
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/* This algorithm is basically Myers' solution to SES/LCS with
 * the Hirschberg linear space refinement as described in the
 * following publication:
 *
 *   E. Myers, ``An O(ND) Difference Algorithm and Its Variations,''
 *   Algorithmica 1, 2 (1986), 251-266.
 *   http://www.cs.arizona.edu/people/gene/PAPERS/diff.ps
 *
 * This is the same algorithm used by GNU diff(1).
 */

/* Purple changes:
 * 
 * - Code found at http://www.ioplex.com/~miallen/libmba/dl/src/diff.c.
 * - Adopted for use in Purple, heavy edits and support code porting.
 * - Removed ~55% of the total number of code lines by coalescing the
 *   dependencies in the original's mba/ folder (headers).
 * 
*/

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dynarr.h"

#include "diff.h"

#define FV(k)	v_get(ctx, (k), 0)
#define RV(k)	v_get(ctx, (k), 1)

typedef struct
{
	DynArr	*buf, *ses;
	int	si;
	int	dmax;
} Context;

typedef struct
{
	int	x, y, u, v;
} Snake;

static void v_set(Context *ctx, int k, int r, int val)
{
	int	j = (k <= 0) ? -k * 4 + r : k * 4 + (r - 2);	/* Pack -N to N into 0 to N * 2. */

	dynarr_set(ctx->buf, j, &val);
}

static int v_get(Context *ctx, int k, int r)
{
	int	j = (k <= 0) ? -k * 4 + r : k * 4 + (r - 2);	/* Pack -N to N into 0 to N * 2. */

	return *((int *) dynarr_index(ctx->buf, j));
}

static int find_middle_snake(const void *a, int aoff, int n,
		const void *b, int boff, int m,
		Context *ctx,
		Snake *ms)
{
	int			delta, odd, mid, d;
	const unsigned char	*a0, *b0;

	delta = n - m;
	odd = delta & 1;
	mid = (n + m) / 2;
	mid += odd;

	v_set(ctx, 1, 0, 0);
	v_set(ctx, delta - 1, 1, n);

	for(d = 0; d <= mid; d++)
	{
		int k, x, y;

		if((2 * d - 1) >= ctx->dmax)
			return ctx->dmax;

		for(k = d; k >= -d; k -= 2)
		{
			if(k == -d || (k != d && FV(k - 1) < FV(k + 1)))
				x = FV(k + 1);
			else
				x = FV(k - 1) + 1;
			y = x - k;

			ms->x = x;
			ms->y = y;
			for(a0 = a + aoff, b0 = b + boff; x < n && y < m && a0[x] == b0[y]; x++, y++)
				;
			v_set(ctx, k, 0, x);

			if(odd && k >= (delta - (d - 1)) && k <= (delta + (d - 1)))
			{
				if(x >= RV(k))
				{
					ms->u = x;
					ms->v = y;
					return 2 * d - 1;
				}
			}
		}
		for(k = d; k >= -d; k -= 2)
		{
			int kr = (n - m) + k;

			if(k == d || (k != -d && RV(kr - 1) < RV(kr + 1)))
				x = RV(kr - 1);
			else
				x = RV(kr + 1) - 1;
			y = x - kr;

			ms->u = x;
			ms->v = y;
			for(a0 = a + aoff, b0 = b + boff;x > 0 && y > 0 && a0[x - 1] == b0[y - 1]; x--, y--)
				;
			v_set(ctx, kr, 1, x);

			if(!odd && kr >= -d && kr <= d)
			{
				if(x <= FV(kr))
				{
					ms->x = x;
					ms->y = y;
					return 2 * d;
				}
			}
		}
	}
	return -1;
}

static void edit(Context *ctx, int op, int off, int len)
{
	DiffEdit *e;

	if(len == 0 || ctx->ses == NULL)
		return;
        /* Add an edit to the SES (or coalesce if the op is the same). */
	e = dynarr_set(ctx->ses, ctx->si, NULL);
	if(e->op != op)
	{
		if(e->op)
			e = dynarr_set(ctx->ses, ++ctx->si, NULL);
		e->op  = op;
		e->off = off;
		e->len = len;
	}
	else
		e->len += len;
}

static int ses_compute(const void *a, int aoff, int n, const void *b, int boff, int m, Context *ctx)
{
	Snake	ms;
	int	d;

	if(n == 0)
	{
		edit(ctx, DIFF_INSERT, boff, m);
		d = m;
	}
	else if(m == 0)
	{
		edit(ctx, DIFF_DELETE, aoff, n);
		d = n;
	}
	else
	{
                /* Find the middle "snake" around which we recursively solve the sub-problems. */
		d = find_middle_snake(a, aoff, n, b, boff, m, ctx, &ms);
		if(d == -1)
			return -1;
		else if(d >= ctx->dmax)
			return ctx->dmax;
		else if(ctx->ses == NULL)
			return d;
		else if(d > 1)
		{
			if(ses_compute(a, aoff, ms.x, b, boff, ms.y, ctx) == -1)
				return -1;

			edit(ctx, DIFF_MATCH, aoff + ms.x, ms.u - ms.x);
			aoff += ms.u;
			boff += ms.v;
			n -= ms.u;
			m -= ms.v;
			if(ses_compute(a, aoff, n, b, boff, m, ctx) == -1)
				return -1;
		}
		else
		{
			int x = ms.x;
			int u = ms.u;

			/* Resolve four base cases for edit distance d = 1. */
			if(m > n)
			{
				if(x == u)
				{
					edit(ctx, DIFF_MATCH,  aoff, n);
					edit(ctx, DIFF_INSERT, boff + (m - 1), 1);
				}
				else
				{
					edit(ctx, DIFF_INSERT, boff, 1);
					edit(ctx, DIFF_MATCH,  aoff, n);
				}
			}
			else
			{
				if(x == u)
				{
					edit(ctx, DIFF_MATCH,  aoff, m);
					edit(ctx, DIFF_DELETE, aoff + (n - 1), 1);
				}
				else
				{
					edit(ctx, DIFF_DELETE, aoff, 1);
					edit(ctx, DIFF_MATCH,  aoff + 1, m);
				}
			}
		}
	}
	return d;
}

int diff_compare(const void *a, int aoff, int n,
	 const void *b, int boff, int m,
	 int dmax, DynArr *edits, DynArr *buf)
{
	Context			ctx;
	int			d = 0, x, y;
	DiffEdit 		*e = NULL;
	DynArr			*tmp;
	const unsigned char	*a0, *b0;

	if(buf != NULL)
		ctx.buf = buf;
	else
	{
		tmp = dynarr_new(sizeof (int), 32);
		ctx.buf = tmp;
	}
	ctx.ses = edits;
	ctx.si = 0;
	ctx.dmax = dmax ? dmax : INT_MAX;

	if(edits)
	{
		if((e = dynarr_set(edits, 0, NULL)) == NULL)
		{
			if(buf == NULL)
				dynarr_destroy(tmp);
			return -1;
		}
		e->op = 0;
	}

         /* The ses_compute() function assumes the SES will begin or end with a delete
          * or insert. The following will insure this is true by eating any
          * beginning matches. This is also a quick to process sequences
          * that match entirely.
          */
	x = y = 0;
	for(a0 = a + aoff, b0 = b + boff; x < n && y < m && a0[x] == b0[y]; x++, y++)
		;
	edit(&ctx, DIFF_MATCH, aoff, x);

	if((d = ses_compute(a, aoff + x, n - x, b, boff + y, m - y, &ctx)) == -1)
	{
		if(buf == NULL)
			dynarr_destroy(tmp);
		return -1;
	}

	if(buf == NULL)
		dynarr_destroy(tmp);
	return d;
}

int diff_compare_simple(const void *a, size_t n, const void *b, size_t m, DynArr *edits)
{
	return diff_compare(a, 0, n,  b, 0, m,  0, edits, NULL);
}
