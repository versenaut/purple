/*
 * 
*/

#include <stdlib.h>

#include "mem.h"

#include "textbuf.h"

/* ----------------------------------------------------------------------------------------- */

struct TextBuf
{
	char	*buf;
	size_t	buf_size;
	char	*gap;
	size_t	gap_size;
	size_t	gap_size_min;
};

/* ----------------------------------------------------------------------------------------- */

TextBuf * textbuf_new(void)
{
	TextBuf	*t;

	t = mem_alloc(sizeof *t);

	t->gap_size_min = 1024;
	t->gap = mem_alloc(t->gap_size_min);
	t->gap_size = t->gap_size_min;

	t->buf = t->gap;
	t->buf_size = 0;

	return t;
}

void textbuf_insert(TextBuf *tb, unsigned int offset, const char *text, size_t len)
{
}
