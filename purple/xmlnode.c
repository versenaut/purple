/*
 * 
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dynstr.h"
#include "list.h"
#include "log.h"

#include "xmlnode.h"

/* ----------------------------------------------------------------------------------------- */

typedef enum { TAG, TEXT, END, ERROR } TokenStatus;

typedef struct
{
	const char	*name;
	const char	*value;
} Attrib;

struct XmlNode
{
	const char	*element;
	const char	*text;
	List		*attribs;
	List		*children;
};

/* ----------------------------------------------------------------------------------------- */

#define	TOKEN_STATUS_RETURN(s, r)	*status = s; return r;

static const char * append_entity(const char *buffer, DynStr *token)
{
	static const struct
	{
		char	*entity;
		size_t	len;
		char	replace;
	} entity[] = {
		{ "&lt;", 4, '<' }, { "&gt;", 4, '>' }, { "&amp;", 5, '&' }, { "&quot;", 6, '"' }
	};
	int	i;

	for(i = 0; i < sizeof entity / sizeof *entity; i++)
	{
		if(strncmp(buffer, entity[i].entity, entity[i].len) == 0)
		{
			dynstr_append_c(token, entity[i].replace);
			return buffer + entity[i].len;
		}
	}
	return buffer;
}

static const char * token_get(const char *buffer, DynStr **token, int skip_space, TokenStatus *status)
{
	TokenStatus	dummy;
	DynStr		*d;
	int		in_tag = 0;

	if(buffer == NULL || token == NULL)
		return NULL;
	if(status == NULL)
		status = &dummy;	/* No need to check it, now. */

	if(skip_space)
	{
		while(*buffer && isspace(*buffer))
			buffer++;
		if(!*buffer)
			return buffer;
	}
	if(*buffer == '<')
	{
		char	here, last = 0;
		int	in_str = 0;

		d = dynstr_new_sized(32);
		for(buffer++; *buffer; last = here)
		{
			here = *buffer++;
			if(here == '\n' || here == '\t' || here == '\r')
				here = ' ';
			if(!in_str && (here == ' ' && last == ' '))
				continue;
			if((in_str && (here == '<' || here == '>')) || here == '<')
			{
				LOG_ERR(("Bracket in string not legal, use entities"));
				*status = ERROR;
				return buffer;
			}
			else if(!in_str && here == '>')
				break;
			else if(here == '&')
			{
				const char	*b2 = append_entity(buffer - 1, d);
				if(b2 > buffer)
				{
					buffer = b2;
					continue;
				}
			}
			dynstr_append_c(d, here);
			if(!in_str && (here == '\'' || here == '"'))
				in_str = here;
			else if(in_str && (here == in_str))
				in_str = 0;
		}
		*token  = d;
		*status = TAG;
		return buffer;
	}
	else
	{
		d = dynstr_new_sized(16);
		for(; *buffer;)
		{
			char	here = *buffer;

			if(here == '<')
				break;
			else if(here == '&')
			{
				const char	*b2 = append_entity(buffer, d);
				if(b2 > buffer)
				{
					buffer = b2;
					continue;
				}
			}
			dynstr_append_c(d, here);
			buffer++;
		}
		*token  = d;
		*status = TEXT;
		return buffer;
	}
	return NULL;
}

XmlNode * xmlnode_new(const char *buffer)
{
	DynStr		*token = NULL;
	TokenStatus	status;

	if(buffer == NULL)
		return NULL;
	for(; *buffer;)
	{
		buffer = token_get(buffer, &token, 1, &status);
		if(token != NULL)
			printf("got token: '%s'\n", dynstr_string(token));
		dynstr_destroy(token, 1);
	}
	return NULL;
}

const char * xmlnode_attrib_get(const XmlNode *node, const char *name)
{
	const List	*iter;

	if(node == NULL || name == NULL || *name == '\0')
		return NULL;
	for(iter = node->attribs; iter != NULL; iter = list_next(iter))
	{
		const Attrib	*a = list_data(iter);
		int		rel;

		if((rel = strcmp(a->name, name)) == NULL)
			return a->value;
		else if(rel > 0)
			break;
	}
	return NULL;
}
