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
#include "mem.h"

#include "xmlnode.h"

/* ----------------------------------------------------------------------------------------- */

typedef enum { TAG, TAGEMPTY, TEXT, END, ERROR } TokenStatus;

typedef struct
{
	const char	*name;
	const char	*value;
} Attrib;

struct XmlNode
{
	const char	*element;
	const char	*text;
	Attrib		**attrib;	/* NULL-terminated array, all allocated in one piece. */
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

		if((d = *token) == NULL)
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
		if(last != '/')
			*status = TAG;
		else
		{
			dynstr_truncate(d, dynstr_length(d) - 1);
			*status = TAGEMPTY;
		}
		*token  = d;
		return buffer;
	}
	else
	{
		if((d = *token) == NULL)
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

/* ----------------------------------------------------------------------------------------- */

static int cmp_attr(const void *a, const void *b)
{
	const Attrib	*aa = *(Attrib **) a, *ab = *(Attrib **) b;

	return strcmp(aa->name, ab->name);
}

static Attrib ** attribs_build(const char *token)
{
	size_t		num = 0, name_size = 0, value_size = 0;
	const char	*src = token;

	while(*src)
	{
		while(isspace(*src))
			src++;
		if(isalpha(*src))
		{
			while(isalpha(*src) || *src == '-')
			{
				name_size++;
				src++;
			}
			if(*src == '=')
			{
				char	quot;

				src++;
				quot = *src;
				if(quot == '\'' || quot == '"')
				{
					src++;
					while(*src && *src != quot)
					{
						value_size++;
						src++;
					}
					if(*src == quot)
						src++;
					else
						return NULL;
					num++;
				}
				else
					return NULL;
			}
			else
				return NULL;
		}
		else
			return NULL;
	}
	printf("found %d attribs, %u bytes in names and %u in values\n", num, name_size, value_size);
	if(num > 0 && name_size > 0 && value_size > 0)
	{
		Attrib	**attr;

		if((attr = mem_alloc((num + 1) * sizeof *attr + num * sizeof **attr + (name_size + value_size + 2 * num))) != NULL)
		{
			int	index = 0;
			Attrib	*a = (Attrib *) (attr + (num + 1));
			char	*put = (char *) (a + num);

			printf("%u and %u (%u %u)\n", (char *) a - (char *) attr, put - (char *) a, num, sizeof *a);
			src = token;
			while(*src)
			{
				while(isspace(*src))
					src++;
				if(isalpha(*src))
				{
					attr[index] = a;
					a->name = put;
					while(isalpha(*src) || *src == '-')
						*put++ = *src++;
					*put++ = '\0';
					if(*src == '=')
					{
						char	quot;

						src++;
						quot = *src;
						if(quot == '\'' || quot == '"')
						{
							src++;
							a->value = put;
							while(*src && *src != quot)
								*put++ = *src++;
							*put++ = '\0';
							if(*src == quot)
								src++;
							index++;
							a++;
						}
					}
				}
			}
			attr[index] = NULL;
			qsort(attr, num, sizeof *attr, cmp_attr);
			return attr;
		}
	}
	return NULL;
}

static XmlNode * node_new(const char *token)
{
	size_t	elen;

	for(elen = 0; token[elen] && !isspace(token[elen]); elen++)
		;
	if(elen > 0)
	{
		XmlNode	*node;

		if((node = mem_alloc(sizeof *node + elen + 1)) != NULL)
		{
			char	*put = (char *) (node + 1);
			int	i;

			for(i = 0; i < elen; i++)
				*put++ = token[i];
			*put = '\0';
			node->element  = (const char *) (node + 1);
			node->text     = NULL;
			node->attrib   = attribs_build(token + elen);
			node->children = NULL;

			if(node->attrib != NULL)
			{
				int	i;

				for(i = 0; node->attrib[i] != NULL; i++)
					printf("%s='%s'\n", node->attrib[i]->name, node->attrib[i]->value);
			}
			return node;
		}
	}
	return NULL;
}

static int node_closes(const XmlNode *parent, const char *tag)
{
	const char	*src;

	if(parent == NULL || tag == NULL || *tag == '\0' || *tag != '/')
		return 0;
	src = parent->element;
	tag++;		/* Skip the slash. */
	for(; *src && *tag && *src == *tag; src++, tag++)
	{
		if(isspace(*tag))
			return 0;
	}
	return 1;
}

static void node_child_add(XmlNode *parent, XmlNode *child)
{
	if(parent == NULL || child == NULL)
		return;
	parent->children = list_append(parent->children, child);
}

static XmlNode * build_tree(XmlNode *parent, const char **buffer)
{
	DynStr	*token = NULL;

	for(; **buffer;)
	{
		TokenStatus	st;

		*buffer = token_get(*buffer, &token, 0, &st);
		if(token != NULL)
		{
			if(st == TAG || st == TAGEMPTY)
			{
				const char	*tag = dynstr_string(token);

				printf("got %s tag: '%s'\n", st == TAG ? "regular" : "empty", tag);
				if(tag[0] == '?' || strncmp(tag, "!--", 3) == 0)
					dynstr_truncate(token, 0);
				else if(tag[0] == '/')
				{
					if(node_closes(parent, tag))
						return parent;
					LOG_ERR(("Element nesting error in XML source, aborting"));
					return parent;
				}
				else
				{
					XmlNode	*child = node_new(tag), *subtree;
					dynstr_destroy(token, 1);
					token = NULL;

					if(st != TAGEMPTY)
						subtree = build_tree(child, buffer);
					else
						subtree = child;
					if(parent != NULL)
						node_child_add(parent, subtree);
					else
						parent = child;
				}
			}
			else if(st == TEXT)
			{
				printf("got text: '%s'\n", dynstr_string(token));
				dynstr_destroy(token, 1);
				token = NULL;
			}
		}
		else
			break;
	}
	return parent;
}

XmlNode * xmlnode_new(const char *buffer)
{
	if(buffer == NULL)
		return NULL;
	return build_tree(NULL, &buffer);
}

const char * xmlnode_attrib_get(const XmlNode *node, const char *name)
{
/*	const List	*iter;

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
*/	return NULL;
}

static void do_print_outline(const XmlNode *root, int indent)
{
	int		i;
	const List	*iter;

	for(i = 0; i < indent; i++)
		putchar(' ');
	printf("%s\n", root->element);
	for(iter = root->children; iter != NULL; iter = list_next(iter))
		do_print_outline(list_data(iter), indent + 1);
}

void xmlnode_print_outline(const XmlNode *root)
{
	do_print_outline(root, 0);
}
