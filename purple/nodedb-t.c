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
#include "log.h"
#include "strutil.h"
#include "textbuf.h"

#include "nodedb.h"
#include "nodedb-internal.h"

/* ----------------------------------------------------------------------------------------- */

void nodedb_t_construct(NodeText *n)
{
	n->language[0] = '\0';
	n->buffers = NULL;
}

static void cb_copy_buffer(void *d, const void *s, UNUSED(void *user))
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
	strcpy(n->language, src->language);
	n->buffers = dynarr_new_copy(src->buffers, cb_copy_buffer, NULL);
}

void nodedb_t_set(NodeText *n, const NodeText *src)
{
	/* FIXME: This could be quicker. */
	nodedb_t_destruct(n);
	nodedb_t_copy(n, src);
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
	n->buffers = NULL;
}

const char * nodedb_t_language_get(const NodeText *node)
{
	if(node == NULL || node->node.type != V_NT_TEXT)
		return NULL;
	return node->language;
}

void nodedb_t_language_set(NodeText *node, const char *language)
{
	if(node == NULL || language == NULL)
		return;
	stu_strncpy(node->language, sizeof node->language, language);
}

unsigned int nodedb_t_buffer_num(const NodeText *node)
{
	unsigned int	i, num;
	NdbTBuffer	*b;

	if(node == NULL || node->node.type != V_NT_TEXT)
		return 0;
	for(i = num = 0; (b = dynarr_index(node->buffers, i)) != NULL; i++)
	{
		if(b->name[0] == '\0')
			continue;
		num++;
	}
	return num;
}

NdbTBuffer * nodedb_t_buffer_nth(const NodeText *node, unsigned int n)
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

NdbTBuffer * nodedb_t_buffer_find(const NodeText *node, const char *name)
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

static void cb_def_buffer(UNUSED(unsigned int index), void *element, UNUSED(void *user))
{
	NdbTBuffer	*buffer = element;

	buffer->name[0] = '\0';
	buffer->text = NULL;
}

NdbTBuffer * nodedb_t_buffer_create(NodeText *node, VLayerID buffer_id, const char *name)
{
	NdbTBuffer	*buffer;

	if(node->buffers == NULL)
	{
		node->buffers = dynarr_new(sizeof *buffer, 2);
		dynarr_set_default_func(node->buffers, cb_def_buffer, NULL);
	}
	if(node->buffers == NULL)
		return NULL;
	if(buffer_id == (VLayerID) ~0)
		buffer = dynarr_append(node->buffers, NULL, NULL);
	else
	{
		if((buffer = dynarr_index(node->buffers, buffer_id)) != NULL)
		{
			if(buffer->name[0] != '\0')
			{
				if(buffer->text != NULL)
				{
					textbuf_destroy(buffer->text);
					buffer->text = NULL;
				}
			}
		}
		buffer = dynarr_set(node->buffers, buffer_id, NULL);
	}
	buffer->id = buffer_id;
	stu_strncpy(buffer->name, sizeof buffer->name, name);
	buffer->text = NULL;

	return buffer;
}

void nodedb_t_buffer_destroy(NodeText *node, NdbTBuffer *buffer)
{
	if(node == NULL || buffer == NULL)
		return;
	buffer->id = 0;
	buffer->name[0] = '\0';
	if(buffer->text != NULL)
	{
		textbuf_destroy(buffer->text);
		buffer->text = NULL;
	}
}

const char * nodedb_t_buffer_read_begin(NdbTBuffer *buffer)
{
	if(buffer != NULL)
		return textbuf_text(buffer->text);
	return NULL;
}

void nodedb_t_buffer_read_end(UNUSED(NdbTBuffer *buffer))
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
	else
		printf("skabr�p\n");
	return NULL;
}

void nodedb_t_buffer_insert(NdbTBuffer *buffer, size_t pos, const char *text)
{
	if(buffer != NULL)
	{
		if(buffer->text == NULL)
			buffer->text = textbuf_new(1024);
		textbuf_insert(buffer->text, pos, text);
	}
}

void nodedb_t_buffer_append(NdbTBuffer *buffer, const char *text)
{
	if(buffer == NULL || text == NULL || *text == '\0')
		return;
	nodedb_t_buffer_insert(buffer, textbuf_length(buffer->text), text);
}

void nodedb_t_buffer_delete(NdbTBuffer *buffer, size_t pos, size_t length)
{
	if(buffer != NULL)
		textbuf_delete(buffer->text, pos, length);
}

void nodedb_t_buffer_clear(NdbTBuffer *buffer)
{
	if(buffer != NULL)
		textbuf_truncate(buffer->text, 0);
}

/* ----------------------------------------------------------------------------------------- */

static void cb_t_language_set(UNUSED(void *user), VNodeID node_id, const char *language)
{
	NodeText	*n;

	if((n = nodedb_lookup_text(node_id)) != NULL)
	{
		stu_strncpy(n->language, sizeof n->language, language);
		NOTIFY(n, DATA);
	}
}

static void cb_t_buffer_create(UNUSED(void *user), VNodeID node_id, uint16 buffer_id, const char *name)
{
	NodeText	*n;
	NdbTBuffer	*buffer;

	printf("text callback: create buffer %u (%s) in %u\n", buffer_id, name, node_id);
	if((n = (NodeText *) nodedb_lookup_with_type(node_id, V_NT_TEXT)) == NULL)
		return;
	if((buffer = dynarr_index(n->buffers, buffer_id)) != NULL && buffer->name[0] != '\0')
	{
		LOG_WARN(("Missing code, text buffer needs to be reborn"));
		return;
	}
	else
	{
		printf("text buffer %s created\n", name);
		nodedb_t_buffer_create(n, buffer_id, name);
		NOTIFY(n, STRUCTURE);
		verse_send_t_buffer_subscribe(node_id, buffer_id);
	}
/*	if((n = nodedb_lookup_text(node_id)) != NULL)
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
*/
}

static void cb_t_buffer_destroy(UNUSED(void *user), VNodeID node_id, uint16 buffer_id)
{
	NodeText	*n;

	if((n = nodedb_lookup_text(node_id)) != NULL)
	{
		NdbTBuffer	*tb;

		if((tb = dynarr_index(n->buffers, buffer_id)) != NULL)
		{
			nodedb_t_buffer_destroy(n, tb);
			NOTIFY(n, STRUCTURE);
		}
	}
}

static void cb_t_text_set(UNUSED(void *user), VNodeID node_id, uint16 buffer_id, uint32 pos, uint32 len, const char *text)
{
	NodeText	*n;

	if((n = nodedb_lookup_text(node_id)) != NULL)
	{
		NdbTBuffer	*tb;

		if((tb = dynarr_index(n->buffers, buffer_id)) != NULL)
		{
/*			printf("text set in buffer %u.%u\n", node_id, buffer_id);*/
			nodedb_t_buffer_delete(tb, pos, len);
			nodedb_t_buffer_insert(tb, pos, text);
			NOTIFY(n, DATA);
		}
	}
}

/* ----------------------------------------------------------------------------------------- */

void nodedb_t_register_callbacks(void)
{
	verse_callback_set(verse_send_t_language_set,	cb_t_language_set, NULL);
	verse_callback_set(verse_send_t_buffer_create,	cb_t_buffer_create, NULL);
	verse_callback_set(verse_send_t_buffer_destroy,	cb_t_buffer_destroy, NULL);
	verse_callback_set(verse_send_t_text_set,	cb_t_text_set, NULL);
}
