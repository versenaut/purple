/*
 * Dynamic string. This is not at all UTF8-secure. :/
*/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "mem.h"
#include "strutil.h"

#include "dynstr.h"

/* ----------------------------------------------------------------------------------------- */

#define	APPEND_MARGIN	32

struct DynStr
{
	char	*str;
	size_t	len;		/* Length of string, in characters not including terminator. */
	size_t	alloc;		/* Allocated bytes. Often more than len, when appending. */
};

/* ----------------------------------------------------------------------------------------- */

DynStr * dynstr_new(const char *text)
{
	DynStr	*ds;

	if((ds = mem_alloc(sizeof *ds)) != NULL)
	{
		if(text == NULL)
		{
			ds->str = NULL;
			ds->alloc = ds->len = 0;
			return ds;
		}
		else if((ds->str = stu_strdup(text)) != NULL)
		{
			ds->len = strlen(ds->str);
			ds->alloc = ds->len;
			return ds;
		}
		mem_free(ds);
	}
	return NULL;
}

void dynstr_assign(DynStr *ds, const char *text)
{
	size_t	len;
	char	*nb;

	if(ds == NULL || text == NULL)
		return;
	len = strlen(text);
	if(len < ds->len && len < 128)
	{
		strcpy(ds->str, text);
		ds->len = len;
		return;
	}
	if((nb = mem_alloc(len + 1)) != NULL)
	{
		mem_free(ds->str);
		ds->str   = nb;
		ds->alloc = len + 1;
		ds->len   = len;
	}
	else
		LOG_WARN(("Couldn't assign string, out of memory"));
}

/* Compute size needed for string formatting. A portability thing. If run on Linux
 * with a pre-2.0 C runtime, this will fail horribly.
*/
static size_t format_size(const char *fmt, va_list args)
{
#if defined __win32
	return _vscprintf(fmt, args);
#else
	char	buf;
	int	n;

	n = vsnprintf(&buf, 1, fmt, args);
	if(n >= 0)	/* Very quick on modern glibc runtimes. */
		return n;
	else
	{
		LOG_ERR(("Negative return handler for vsnprintf(), not implemented"));
	}
#endif
	LOG_ERR(("Failing, falling through to return 0"));
	return 0;
}

void dynstr_printf(DynStr *ds, const char *fmt, ...)
{
	va_list	args;
	size_t	need;
	char	*nb;

	if(ds == NULL || fmt == NULL)
		return;

	va_start(args, fmt);
	need = format_size(fmt, args);
	va_end(args);

	if(need == 0)	/* Size computation failed, or string is really boring. */
		return;
	if(need + 1 < ds->alloc)
	{
		ds->len = vsprintf(ds->str, fmt, args);
		return;
	}
	if((nb = mem_realloc(ds->str, need + 1)) != NULL)
	{
		ds->str = nb;
		vsprintf(ds->str, fmt, args);
		ds->len = need;
		ds->alloc = need + 1;
	}
}

void dynstr_append(DynStr *ds, const char *text)
{
	size_t	len;
	char	*nb;

	if(ds == NULL || text == NULL || *text == '\0')
		return;
	len = strlen(text);
	if(ds->len + len + 1 <= ds->alloc)
	{
		strcpy(ds->str + ds->len, text);
		ds->len += len;
		return;
	}
	if((nb = mem_realloc(ds->str, ds->alloc + len + 1 + APPEND_MARGIN)) != NULL)
	{
		ds->str = nb;
		strcpy(ds->str + ds->len, text);
		ds->len += len;
		ds->alloc = ds->alloc + len + 1 + APPEND_MARGIN;
/*		LOG_MSG(("String grew to %u bytes, with %u allocated", ds->len, ds->alloc));*/
		return;
	}
	LOG_ERR(("Couldn't allocate new buffer for append"));
}

void dynstr_append_c(DynStr *ds, char c)
{
	if(ds == NULL || c == '\0')
		return;
	if(ds->alloc > ds->len + 1)
		ds->str[ds->len++] = c;
	else
	{
		char	buf[2];

		buf[0] = c;
		buf[1] = '\0';
		dynstr_append(ds, buf);
	}
}

void dynstr_append_printf(DynStr *ds, const char *fmt, ...)
{
	va_list	args;
	int	need;

	if(ds == NULL || fmt == NULL || *fmt == '\0')
		return;

	va_start(args, fmt);
	need = format_size(fmt, args);
	va_end(args);
	if(need == 0)
		return;
	else if(need <= (int) (ds->alloc - ds->len - 1))
	{
		vsprintf(ds->str + ds->len, fmt, args);
		ds->len += need;
		return;
	}
	else
	{
		char	*nb;

		if((nb = mem_realloc(ds->str, ds->alloc + need + 1 + APPEND_MARGIN)) != NULL)
		{
			vsprintf(ds->str + ds->len, fmt, args);
			ds->len   += need;
			ds->alloc += need + 1 + APPEND_MARGIN;
			return;
		}
	}
	LOG_ERR(("Couldn't allocate new buffer for append printf"));
}

const char * dynstr_string(const DynStr *ds)
{
	if(ds == NULL)
		return NULL;
	return ds->str;
}

size_t dynstr_length(const DynStr *ds)
{
	if(ds == NULL)
		return 0;
	return ds->len;
}

char * dynstr_destroy(DynStr *ds, int destroy_buffer)
{
	char	*ret = NULL;

	if(ds == NULL)
		return NULL;
	if(!destroy_buffer)
		ret = ds->str;
	else
		mem_free(ds->str);
	mem_free(ds);

	return ret;
}
