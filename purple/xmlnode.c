/*
 * xmlnode.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * A very trivial and non-compliant with anything "XML parser" workalike.
*/

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dynstr.h"
#include "list.h"
#include "log.h"
#include "mem.h"

#include "xmlnode.h"

/* ----------------------------------------------------------------------------------------- */

typedef enum { TAG, TAGEMPTY, TEXT, ERROR } TokenStatus;

typedef struct
{
	const char	*name;
	const char	*value;
} Attrib;

struct XmlNode
{
	const char	*element;
	char		*text;
	size_t		attrib_num;	/* Number of attributes. */
	Attrib		*attrib;
	XmlNode		*parent;
	List		*children;
};

/* ----------------------------------------------------------------------------------------- */

#define	TOKEN_STATUS_RETURN(s, r)	*status = s; return r;

/* Look up entity (on the form &ENTITY;) from start of <buffer>, and append the corresponding
 * actual text to <token>. Returns pointer to first char of <buffer> past entity.
*/
static const char * append_entity(const char *buffer, DynStr *token)
{
	static const struct
	{
		char	*entity;
		size_t	len;
		char	replace;
	} entity[] = { { "&lt;", 4, '<' }, { "&gt;", 4, '>' }, { "&amp;", 5, '&' }, { "&quot;", 6, '"' } };
	unsigned int	i;

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

/* Parse off a "token" from <buffer>, storing it in <token> which is created if necessary.
 * A token is simply either a tag, or some text. Uses <status> to report the type of the
 * token, angle brackets are stripped from tags. Returns new buffer pointer.
*/
static const char * token_get(const char *buffer, DynStr **token, TokenStatus *status)
{
	TokenStatus	dummy;
	DynStr		*d;

	if(buffer == NULL || token == NULL)
		return NULL;
	if(status == NULL)
		status = &dummy;	/* No need to check it, now. */

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

/* A qsort() comparison callback for attribute name ordering. */
static int cmp_attr(const void *a, const void *b)
{
	const Attrib	*aa = a, *ab = b;

	return strcmp(aa->name, ab->name);
}

/* FIXME: This function is way too long. Could be refactored, some day. */
static Attrib * attribs_build(const char *token, size_t *attrib_num)
{
	size_t		num = 0, name_size = 0, value_size = 0;
	const char	*src = token;

	if(token == NULL)
		return NULL;

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
	if(num > 0 && name_size > 0 && value_size > 0)
	{
		Attrib	*attr;

		if((attr = mem_alloc(num * sizeof *attr + (name_size + value_size + 2 * num))) != NULL)
		{
			int	index = 0;
			char	*put = (char *) (attr + num);

			src = token;
			while(*src)
			{
				while(isspace(*src))
					src++;
				if(isalpha(*src))
				{
					attr[index].name = put;
					while(isalpha(*src) || *src == '-')
						*put++ = tolower(*src++);
					*put++ = '\0';
					if(*src == '=')
					{
						char	quot;

						src++;
						quot = *src;
						if(quot == '\'' || quot == '"')
						{
							src++;
							attr[index].value = put;
							while(*src && *src != quot)
								*put++ = *src++;
							*put++ = '\0';
							if(*src == quot)
								src++;
							index++;
						}
					}
				}
			}
			qsort(attr, num, sizeof *attr, cmp_attr);
			*attrib_num = num;
			return attr;
		}
	}
	return NULL;
}

/* Create a new node, from the given <token>. If token is NULL, node is anonymous. */
static XmlNode * node_new(const char *token)
{
	size_t	elen = 0;
	XmlNode	*node;

	if(token != NULL)
	{
		for(elen = 0; token[elen] && !isspace(token[elen]); elen++)
			;
		if(elen > 0)
			elen++;		/* Count in the terminator. */
	}
	if((node = mem_alloc(sizeof *node + elen)) != NULL)
	{
		if(elen > 0)
		{
			char		*put = (char *) (node + 1);
			unsigned int	i;

			for(i = 0; i < elen - 1; i++)
				*put++ = token[i];
			*put = '\0';
			node->element  = (const char *) (node + 1);
		}
		else
			node->element = NULL;
		node->text     = NULL;
		node->attrib_num = 0;
		node->attrib   = attribs_build(token ? token + elen : NULL, &node->attrib_num);
		node->parent   = NULL;
		node->children = NULL;
		return node;
	}
	return NULL;
}

/* Add a <child> node to a <parent>. Thin. */
static void node_child_add(XmlNode *parent, XmlNode *child)
{
	if(parent == NULL || child == NULL)
		return;
	parent->children = list_append(parent->children, child);
	child->parent    = parent;
}

/* Add <text> content to a <parent> node. This is a bit weird, since text is not totally symmetrically
 * handled: the first text child of a node is stored directly in the node, while any additional text
 * children will be linked as individual XmlNodes.
*/
static void node_text_add(XmlNode *parent, DynStr *text)
{
	if(parent->text == NULL)
		parent->text = dynstr_destroy(text, 0);
	else
	{
		XmlNode	*tc;

		tc = node_new(NULL);
		tc->text = dynstr_destroy(text, 0);
		node_child_add(parent, tc);
	}
}

/* Determine if <tag> is the closing tag for the <parent> node. This means that if <parent>
 * is the element "foo", 1 is returned if <tag> is "/foo" and 0 otherwise.
*/
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
	return *src == *tag;
}

/* Traverse <buffer>, extracting tokens. Build nodes from tokens, and add to <parent> as fit. Recurse. */
static XmlNode * build_tree(XmlNode *parent, const char **buffer, int *complete)
{
	DynStr	*token = NULL;

	if(complete == NULL)
	{
		static int	fake;

		complete = &fake;
	}

	*complete = 1;

	for(; *complete && **buffer;)
	{
		TokenStatus	st;

		*buffer = token_get(*buffer, &token, &st);
		if(st == ERROR)
		{
			LOG_WARN(("XML parse error detected, aborting"));
			*complete = 0;
			return parent;
		}
		if(token != NULL)
		{
			if(st == TAG || st == TAGEMPTY)
			{
				const char	*tag = dynstr_string(token);

				if(tag[0] == '?' || strncmp(tag, "!--", 3) == 0)
					dynstr_truncate(token, 0);
				else if(tag[0] == '/')
				{
					if(node_closes(parent, tag))
						return parent;
					LOG_ERR(("Element nesting error in XML source, <%s> vs <%s>--aborting", tag, parent->element));
					*complete = 0;
					return parent;
				}
				else
				{
					XmlNode	*child = node_new(tag), *subtree;
					dynstr_destroy(token, 1);
					token = NULL;

					if(st != TAGEMPTY)
						subtree = build_tree(child, buffer, complete);
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
				dynstr_trim(token);
				if(dynstr_length(token) > 0)
				{
					if(parent != NULL)
						node_text_add(parent, token);
					else
					{
						LOG_WARN(("Ignoring top-level text"));
						dynstr_destroy(token, 1);
					}
					token = NULL;
				}
			}
		}
		else
			break;
	}
	return parent;
}

XmlNode * xmlnode_new(const char *buffer)
{
	XmlNode	*root;
	int	complete;

	if(buffer == NULL)
		return NULL;
	root = build_tree(NULL, &buffer, &complete);
	if(!complete)
	{
		xmlnode_destroy(root);
		return NULL;
	}
	return root;
}

const char * xmlnode_get_name(const XmlNode *node)
{
	return node != NULL ? node->element : NULL;
}

const char * xmlnode_attrib_get_value(const XmlNode *node, const char *name)
{
	int	lo, hi;

	if(node == NULL || name == NULL)
		return NULL;

	for(lo = 0, hi = node->attrib_num; lo <= hi;)
	{
		int	mid = (lo + hi) / 2, rel;

		rel = strcmp(name, node->attrib[mid].name);
		if(rel == 0)
			return node->attrib[mid].value;
		else if(rel < 0)
			hi = mid - 1;
		else
			lo = mid + 1;
	}
	return NULL;
}

/* ----------------------------------------------------------------------------------------- */

static int filter_node(const XmlNode *node, void **filter, size_t filter_size)
{
	size_t	i;

	for(i = 0; i < filter_size; i++)
	{
		void	*cmd = filter[i];

/*		printf("read command %p\n", cmd);*/
		if(cmd == (void *) XMLNODE_FILTER_ACCEPT)
			break;
		else if(cmd == (void *) XMLNODE_FILTER_NAME)
		{
			cmd = filter[++i];
			if(strcmp(node->element, cmd) != 0)
				return 0;
		}
		else if(cmd == (void *) XMLNODE_FILTER_ATTRIB)
		{
			cmd = filter[++i];
			if(xmlnode_attrib_get_value(node, cmd) == NULL)
				return 0;
		}
		else if(cmd == (void *) XMLNODE_FILTER_ATTRIB_VALUE)
		{
			void		*value;
			const char	*v;

			cmd   = filter[++i];
			value = filter[++i];
			if(((v = xmlnode_attrib_get_value(node, cmd)) == NULL) || (strcmp(v, value) != 0))
				return 0;
		}
	}
	return 1;
}

List * xmlnode_nodeset_get(const XmlNode *node, XmlNodeAxis axis, ...)
{
	void	*filter[32];
	size_t	filter_size;
	va_list	va;

	va_start(va, axis);
	for(filter_size = 0; filter_size < sizeof filter / sizeof *filter; filter_size++)
	{
		filter[filter_size] = va_arg(va, void *);
		if(filter[filter_size] == (void *) 0)
			break;
	}
	va_end(va);
/*	printf("filter size is %u, axis is %d\n", filter_size, axis);*/

	if(axis == XMLNODE_AXIS_ANCESTOR)
	{
		List	*list = NULL;

		for(node = node->parent; node != NULL; node = node->parent)
		{
			if(filter_node(node, filter, filter_size))
				list = list_prepend(list, (void *) node);
		}
		return list;
	}
	else if(axis == XMLNODE_AXIS_CHILD)
	{
		List	*list = NULL, *iter;

/*		printf("traverse children\n");*/
		for(iter = node->children; iter != NULL; iter = list_next(iter))
		{
			if(filter_node(list_data(iter), filter, filter_size))
				list = list_append(list, list_data(iter));
		}
		return list;
	}
	return NULL;
}

/* Evaluate micro-minimalistic "dialect" (I use the term loosely) xpath-like expression.
 * The "single" means that this is guaranteed not to return a nodeset, it will simply
 * take the first result it finds. Preferrably used where the single result is the only.
*/
const char * xmlnode_eval_single(const XmlNode *node, const char *path)
{
	XmlNodeAxis	axis = XMLNODE_AXIS_CHILD;
	char		part[256], *put;			/* Static limits rule. */
	List		*res;

	while(*path)
	{
		for(put = part; *path && *path != '/' && (size_t) (put - part) < sizeof part - 1;)
			*put++ = *path++;
		*put = '\0';
		if(*path == '/')
			path++;
		if(*part == '\0')
			break;
		if(part[0] == '@')
			return xmlnode_attrib_get_value(node, part + 1);
		res = xmlnode_nodeset_get(node, axis, XMLNODE_NAME(part), XMLNODE_DONE);
		if(res != NULL)
		{
			node = list_data(res);
			list_destroy(res);
		}
		else
			return NULL;
	}
	return node != NULL ? node->text : NULL;
}

/* ----------------------------------------------------------------------------------------- */

/* Worker function to print outline of a node hierarchy. */
static void do_print_outline(const XmlNode *root, int indent)
{
	int		i;
	const List	*iter;

	for(i = 0; i < indent; i++)
		putchar(' ');
	if(root->element != NULL)
		printf("%s%s", root->element, root->text != NULL ? " : " : "");
	if(root->text != NULL)
		printf("\"%s\"", root->text);
	if(root->attrib_num > 0)
	{
		size_t	i;

		printf(" [");
		for(i = 0; i < root->attrib_num; i++)
			printf(" %s=\"%s\"", root->attrib[i].name, root->attrib[i].value);
		printf(" ]");
	}
	putchar('\n');
	for(iter = root->children; iter != NULL; iter = list_next(iter))
		do_print_outline(list_data(iter), indent + 1);
}

void xmlnode_print_outline(const XmlNode *root)
{
	if(root != NULL)
		do_print_outline(root, 0);
}

void xmlnode_destroy(XmlNode *root)
{
	List	*iter;

	if(root == NULL)
		return;

	for(iter = root->children; iter != NULL; iter = list_next(iter))
		xmlnode_destroy(list_data(iter));
	list_destroy(root->children);
	mem_free(root->text);
	mem_free(root->attrib);
	mem_free(root);
}
