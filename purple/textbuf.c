/*
 * Editable text. This could well be implemented at least semi-cleverly, using
 * e.g. "gapped storage", but here is the most naive approach thinkable. Almost.
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
	size_t	len;

	if(tb == NULL || text == NULL)
		return;
	if(offset > tb->length)
		offset = tb->length ? tb->length - 1 : 0;
	len = strlen(text);
	if(len == 0)
		return;

	printf("insert %u chars at %u\n", len, offset);
	if(tb->length + len + 1 > tb->alloc)
	{
		char	*nb;

		printf(" growing buffer\n");
		nb = mem_realloc(tb->buf, tb->length + len + ALLOC_EXTRA);
		if(nb != NULL)
		{
			tb->buf = nb;
			tb->alloc = tb->length + len + ALLOC_EXTRA;
			printf("  done, got %u bytes\n", tb->alloc);
		}
		else
		{
			LOG_ERR(("Out of memory when growing text buffer"));
			return;
		}
	}
	memmove(tb->buf + offset + len, tb->buf + offset, tb->length - offset);
	memcpy(tb->buf + offset, text, len);
	tb->length += len;
	tb->buf[tb->length] = '\0';
	printf(" done, final length is %u\n", tb->length);
}

void textbuf_delete(TextBuf *tb, size_t offset, size_t length)
{
	if(tb == NULL)
		return;
	if(offset > tb->length)
		offset = tb->length != 0 ? tb->length - 1 : 0;
	if(offset + length > tb->length)
		length = tb->length - offset;
	if(length == 0)
		return;
	printf("deleting %u at %u\n", length, offset);
	printf(" moving %u bytes of tail\n", tb->length - (offset + length));
	memmove(tb->buf + offset, tb->buf + offset + length, tb->length - (offset + length));
	tb->length -= length;
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
