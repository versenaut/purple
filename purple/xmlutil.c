/*
 * Some simple XML helper functions.
*/

#include <stdlib.h>

#include "xmlutil.h"

/* ----------------------------------------------------------------------------------------- */

static char * buf_flush(DynStr *d, char *buf, char *put, const char *escape)
{
	if(put > buf)
	{
		*put = '\0';
		dynstr_append(d, buf);
	}
	if(escape != NULL)
		dynstr_append(d, escape);
	return buf;
}

void xml_dynstr_append(DynStr *d, const char *text)
{
	char	buf[32], *end = buf + sizeof buf - 1, *put, here;

	if(d == NULL || text == NULL || *text == '\0')
		return;

	for(put = buf; *text;)
	{
		here = *text++;
		if(here == '<')
			put = buf_flush(d, buf, put, "&lt;");
		else if(here == '>')
			put = buf_flush(d, buf, put, "&gt;");
		else if(here == '&')
			put = buf_flush(d, buf, put, "&amp;");
		else if(here == '"')
			put = buf_flush(d, buf, put, "&quot;");
		else
		{
			if(put >= end)
				put = buf_flush(d, buf, put, NULL);
			*put++ = here;
		}
	}
	buf_flush(d, buf, put, NULL);
}
