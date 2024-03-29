/*
 * textbuf.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Editable text. This could well be implemented at least semi-cleverly, using
 * e.g. "gapped storage", but here is the most naive approach thinkable. We simply
 * allocate a slab of memory, and keep the text in there. The text is always kept
 * as a continous, properly terminated C string, with no holes or gaps. The only
 * allowance done for performance is allocating some extra storage to keep every
 * insert() from being a realloc().
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "mem.h"

#include "textbuf.h"

/* ----------------------------------------------------------------------------------------- */

#define ALLOC_EXTRA	1024

struct TextBuf
{
	char	*buf;		/* Buffer space. */
	size_t	length;		/* Exact length of text. */
	size_t	alloc;		/* Bytes allocated at <buf>. Has margin to reduce realloc()ing. */
};

/* ----------------------------------------------------------------------------------------- */

TextBuf * textbuf_new(size_t initial_size)
{
	TextBuf	*tb;

	if((tb = mem_alloc(sizeof *tb)) != NULL)
	{
		if(initial_size > 0)
		{
			initial_size += ALLOC_EXTRA;
			if((tb->buf = mem_alloc(initial_size)) != NULL)
			{
				tb->length = 0;
				tb->alloc = initial_size;
				return tb;
			}
			/* Fall-through if initial alloc failed. */
		}
		tb->buf = NULL;
		tb->length = 0;
		tb->alloc = 0;
		return tb;
	}
	return NULL;
}

/* ----------------------------------------------------------------------------------------- */

void textbuf_insert(TextBuf *tb, size_t offset, const char *text)
{
	size_t	len;

	if(tb == NULL || text == NULL)
		return;
	if(offset > tb->length)
		offset = tb->length;
	len = strlen(text);
	if(len == 0)
		return;

	if(tb->length + len + 1 > tb->alloc)
	{
		char	*nb;

		nb = mem_realloc(tb->buf, tb->length + len + ALLOC_EXTRA);
		if(nb != NULL)
		{
			tb->buf = nb;
			tb->alloc = tb->length + len + ALLOC_EXTRA;
		}
		else
		{
			LOG_ERR(("Out of memory when growing text buffer"));
			return;
		}
	}
	memmove(tb->buf + offset + len, tb->buf + offset, tb->length - offset);
	memcpy(tb->buf + offset, text, len);	/* Using strcpy() would add '\0 in mid-buffer; bad. */
	tb->length += len;
	tb->buf[tb->length] = '\0';
}

void textbuf_delete(TextBuf *tb, size_t offset, size_t length)
{
	if(tb == NULL)
		return;
	if(offset > tb->length)
		return;
	if(offset + length > tb->length)
		length = tb->length - offset;
	if(length == 0)
		return;
	memmove(tb->buf + offset, tb->buf + offset + length, tb->length - (offset + length));
	tb->length -= length;
	tb->buf[tb->length] = '\0';

	if(tb->alloc >= 4 * ALLOC_EXTRA && tb->alloc * 2 > tb->length)
	{
		char	*nb;

		nb = mem_realloc(tb->buf, tb->length + ALLOC_EXTRA);
		if(nb == NULL)
		{
			LOG_WARN(("Couldn't shrink textbuf after delete"));
			return;
		}
		tb->buf = nb;
		tb->alloc = tb->length + ALLOC_EXTRA;
	}
}

void textbuf_truncate(TextBuf *tb, size_t length)
{
	if(tb == NULL)
		return;
	if(length > tb->length)
		return;
	tb->length = length;
	tb->buf[tb->length] = '\0';
}

/* ----------------------------------------------------------------------------------------- */

const char * textbuf_text(TextBuf *tb)
{
	if(tb == NULL)
		return NULL;
	return tb->buf;
}

size_t textbuf_length(const TextBuf *tb)
{
	if(tb == NULL)
		return 0;
	return tb->length;
}

void textbuf_destroy(TextBuf *tb)
{
	if(tb != NULL)
	{
		mem_free(tb->buf);
		mem_free(tb);
	}
}
