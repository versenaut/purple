/*
 * Editable text. This could well be implemented at least semi-cleverly, using
 * e.g. "gapped storage", but here is the most naive approach thinkable. Almost.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"

#include "textbuf.h"

/* ----------------------------------------------------------------------------------------- */

#define ALLOC_EXTRA	1024

struct TextBuf
{
	char	*buf;
	size_t	length;
	size_t	alloc;
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
}

void textbuf_delete(TextBuf *tb, size_t offset, size_t length)
{
	if(tb == NULL)
		return;
	if(offset > tb->length)
		offset = tb->length - 1;
	if(offset + length > tb->length)
		length = tb->length - offset;
	if(length == 0)
		return;
	printf("moving %u bytes of tail\n", tb->length - (offset + length));
	memmove(tb->buf + offset, tb->buf + offset + length, tb->length - (offset + length));
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
