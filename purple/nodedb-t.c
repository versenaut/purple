/*
 * nodedb-t.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 *
*/

#include <stdio.h>
#include <string.h>

#include "verse.h"
#include "purple.h"

#include "dynarr.h"
#include "list.h"
#include "strutil.h"
#include "textbuf.h"

#include "nodedb.h"
#include "nodedb-internal.h"

/* ----------------------------------------------------------------------------------------- */

void nodedb_t_construct(NodeText *n)
{
	n->buffers = dynarr_new(sizeof (NdbTBuffer), 2);
}

static void cb_copy_buffer(void *d, const void *s, void *user)
{
	const NdbTBuffer	*src = s;
	NdbTBuffer		*dst = d;

	dst->id = src->id;
	strcpy(dst->name, src->name);
	dst->text = textbuf_new(textbuf_length(src->text));
	textbuf_insert(dst->text, 0, textbuf_text(src->text));
}

void nodedb_t_copy(NodeText *n, const NodeText *src)
{
	n->buffers = dynarr_new_copy(src->buffers, cb_copy_buffer, NULL);
}

void nodedb_t_destruct(NodeText *n)
{
	unsigned int	i, num;

	num = dynarr_size(n->buffers);
	for(i = 0; i < num; i++)
	{
		NdbTBuffer	*b;

		if((b = dynarr_index(n->buffers, i)) != NULL && b->name[0] != '\0')
		{
			printf("destroying buffer %u\n", i);
			textbuf_destroy(b->text);
		}
	}
	dynarr_destroy(n->buffers);
}

const char * nodedb_t_language_get(const NodeText *node)
{
	if(node == NULL || node->node.type != V_NT_TEXT)
		return NULL;
	return node->language;
}

size_t nodedb_t_buffer_get_count(const NodeText *node)
{
	unsigned int	i, count = 0;
	NdbTBuffer	*b;

	if(node == NULL || node->node.type != V_NT_TEXT)
		return 0;
	for(i = 0; (b = dynarr_index(node->buffers, i)) != NULL; i++)
	{
		if(b->name[0] == '\0')
			continue;
		count++;
	}
	return count;
}

NdbTBuffer * nodedb_t_buffer_get_nth(const NodeText *node, unsigned int n)
{
	unsigned int	i;
	NdbTBuffer	*b;

	if(node == NULL || node->node.type != V_NT_TEXT)
		return 0;
	for(i = 0; (b = dynarr_index(node->buffers, i)) != NULL; i++)
	{
		if(b->name[0] == '\0')
			continue;
		if(n == 0)
			return b;
		n--;
	}
	return NULL;
}

NdbTBuffer * nodedb_t_buffer_get_named(const NodeText *node, const char *name)
{
	unsigned int	i;
	NdbTBuffer	*b;

	if(node == NULL || name == NULL || *name == '\0')
		return NULL;
	for(i = 0; (b = dynarr_index(node->buffers, i)) != NULL; i++)
	{
		if(strcmp(b->name, name) == 0)
			return b;
	}
	return NULL;
}

const char * nodedb_t_buffer_read_begin(NdbTBuffer *buffer)
{
	if(buffer != NULL)
		return textbuf_text(buffer->text);
	return NULL;
}

void nodedb_t_buffer_read_end(NdbTBuffer *buffer)
{
	/* Not much to do, here. */
}

char * nodedb_t_buffer_read_line(NdbTBuffer *buffer, unsigned int line, char *put, size_t putmax)
{
	const char	*text;

	if((text = nodedb_t_buffer_read_begin(buffer)) != NULL)
	{
		/* Fast forward to line <line>, with a wide understanding idea of what is a line break:
		 * a lone CR (Mac), lone LF (Unix), CR followed by any number of LFs, and LF followed
		 * by any number of CRs all work as a single line break.
		*/
		while(*text != '\0')
		{
			if(line == 0)
				break;
			if(*text == '\n' || *text == '\r')
			{
				const char	other = *text == '\n' ? '\r' : '\n';
				text++;
				while(*text == other)
					text++;
				line--;
			}
			else
				text++;
		}
		/* Now copy text to user's buffer, until line or buffer ends. */
		if(*text != '\0')
		{
			char	*p = put;

			putmax--;
			while(*text != '\0' && *text != '\n' && *text != '\r' && putmax > 0)
				*p++ = *text++;
			*p = '\0';
		}
		else
			put = NULL;
		nodedb_t_buffer_read_end(buffer);
		return put;
	}
	return NULL;
}

void nodedb_t_buffer_insert(NdbTBuffer *buffer, size_t pos, const char *text)
{
	if(buffer != NULL)
		textbuf_insert(buffer->text, pos, text);
}

void nodedb_t_buffer_delete(NdbTBuffer *buffer, size_t pos, size_t length)
{
	if(buffer != NULL)
		textbuf_delete(buffer->text, pos, length);
}

/* ----------------------------------------------------------------------------------------- */

static void cb_t_set_language(void *user, VNodeID node_id, const char *language)
{
	NodeText	*n;

	if((n = nodedb_lookup_text(node_id)) != NULL)
	{
		stu_strncpy(n->language, sizeof n->language, language);
		NOTIFY(n, DATA);
	}
}

static void cb_t_buffer_create(void *user, VNodeID node_id, uint16 buffer_id, uint16 index, const char *name)
{
	NodeText	*n;

	if((n = nodedb_lookup_text(node_id)) != NULL)
	{
		NdbTBuffer	*tb;

		if((tb = dynarr_set(n->buffers, buffer_id, NULL)) != NULL)
		{
			tb->id = buffer_id;
			stu_strncpy(tb->name, sizeof tb->name, name);
			tb->text = textbuf_new(1024);
			printf("Text buffer %u.%u %s created\n", node_id, buffer_id, name);
			NOTIFY(n, STRUCTURE);
		}
	}
}

static void cb_t_buffer_destroy(void *user, VNodeID node_id, uint16 buffer_id, uint16 index, const char *name)
{
	NodeText	*n;

	if((n = nodedb_lookup_text(node_id)) != NULL)
	{
		NdbTBuffer	*tb;

		if((tb = dynarr_index(n->buffers, buffer_id)) != NULL)
		{
			tb->id = 0;
			tb->name[0] = '\0';
			textbuf_destroy(tb->text);
			tb->text = NULL;
			NOTIFY(n, STRUCTURE);
		}
	}
}

static void cb_t_text_set(void *user, VNodeID node_id, uint16 buffer_id, uint32 pos, uint32 len, const char *text)
{
	NodeText	*n;

	if((n = nodedb_lookup_text(node_id)) != NULL)
	{
		NdbTBuffer	*tb;

		if((tb = dynarr_index(n->buffers, buffer_id)) != NULL)
		{
			printf("text set in buffer %u.%u\n", node_id, buffer_id);
			textbuf_delete(tb->text, pos, len);
			textbuf_insert(tb->text, pos, text);
			NOTIFY(n, DATA);

			{
				char	line[1024];
				int	i;

				printf("First 8 lines:\n");
				for(i = 0; i < 8; i++)
				{
					if(nodedb_t_buffer_read_line(tb, i, line, sizeof line) != NULL)
						printf("%d: '%s'\n", i, line);
				}
			}
		}
	}
}

/* ----------------------------------------------------------------------------------------- */

void nodedb_t_register_callbacks(void)
{
	verse_callback_set(verse_send_t_set_language,	cb_t_set_language, NULL);
	verse_callback_set(verse_send_t_buffer_create,	cb_t_buffer_create, NULL);
	verse_callback_set(verse_send_t_buffer_destroy,	cb_t_buffer_destroy, NULL);
	verse_callback_set(verse_send_t_text_set,	cb_t_text_set, NULL);
}
