/*
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

NdbTBuffer * nodedb_t_buffer_lookup(const NodeText *node, const char *name)
{
	unsigned int	i;
	NdbTBuffer	*b;

	if(node == NULL || name == NULL || *name == '\0')
		return NULL;
	for(i = 0; i < dynarr_size(node->buffers); i++)
	{
		if((b = dynarr_index(node->buffers, i)) != NULL)
		{
			if(strcmp(b->name, name) == 0)
				return b;
		}
	}
	return NULL;
}

NdbTBuffer * nodedb_t_buffer_lookup_id(const NodeText *node, uint16 buffer_id)
{
	if(node == NULL)
		return NULL;
	return dynarr_index(node->buffers, buffer_id);
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
