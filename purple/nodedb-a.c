/*
 * nodedb-a.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Audio node support. Limited to just layers (buffers), no real-time
 * streaming.
*/

#include <stdio.h>
#include <string.h>

#include "verse.h"
#include "purple.h"

#include "dynarr.h"
#include "list.h"
#include "log.h"
#include "strutil.h"

#include "nodedb.h"
#include "nodedb-internal.h"

/* ----------------------------------------------------------------------------------------- */

void nodedb_a_construct(NodeAudio *n)
{
	n->layers = NULL;
}

static void cb_copy_layer(void *d, const void *s, void *user)
{
#if 0
	const NdbTBuffer	*src = s;
	NdbTBuffer		*dst = d;

	dst->id = src->id;
	strcpy(dst->name, src->name);
	dst->text = textbuf_new(textbuf_length(src->text));
	textbuf_insert(dst->text, 0, textbuf_text(src->text));
#endif
}

void nodedb_a_copy(NodeAudio *n, const NodeAudio *src)
{
	n->layers = dynarr_new_copy(src->layers, cb_copy_layer, NULL);
}

void nodedb_a_destruct(NodeAudio *n)
{
	unsigned int	i, num;

	num = dynarr_size(n->layers);
	for(i = 0; i < num; i++)
	{
		NdbALayer	*la;

		if((la = dynarr_index(n->layers, i)) != NULL && la->name[0] != '\0')
		{
			printf("destroying layer %u\n", i);
		}
	}
	dynarr_destroy(n->layers);
}

unsigned int nodedb_a_layer_num(const NodeAudio *node)
{
	unsigned int	i, num;
	NdbALayer	*la;

	if(node == NULL || node->node.type != V_NT_AUDIO)
		return 0;
	for(i = num = 0; (la = dynarr_index(node->layers, i)) != NULL; i++)
	{
		if(la->name[0] == '\0')
			continue;
		num++;
	}
	return num;
}

NdbALayer * nodedb_a_buffer_nth(const NodeAudio *node, unsigned int n)
{
	unsigned int	i;
	NdbALayer	*la;

	if(node == NULL || node->node.type != V_NT_TEXT)
		return 0;
	for(i = 0; (la = dynarr_index(node->layers, i)) != NULL; i++)
	{
		if(la->name[0] == '\0')
			continue;
		if(n == 0)
			return la;
		n--;
	}
	return NULL;
}

NdbALayer * nodedb_a_layer_find(const NodeAudio *node, const char *name)
{
	unsigned int	i;
	NdbALayer	*la;

	if(node == NULL || name == NULL || *name == '\0')
		return NULL;
	for(i = 0; (la = dynarr_index(node->layers, i)) != NULL; i++)
	{
		if(strcmp(la->name, name) == 0)
			return la;
	}
	return NULL;
}

static void cb_def_layer(unsigned int index, void *element, void *user)
{
	NdbALayer	*la = element;

	la->name[0] = '\0';
	la->blocks  = NULL;
}

NdbALayer * nodedb_a_layer_create(NodeAudio *node, VLayerID layer_id, const char *name, VNALayerType type, real64 frequency)
{
	NdbALayer	*la;

	if(node->layers == NULL)
	{
		node->layers = dynarr_new(sizeof *la, 2);
		dynarr_set_default_func(node->layers, cb_def_layer, NULL);
	}
	if(node->layers == NULL)
		return NULL;
	if(layer_id == (VLayerID) ~0)
		la = dynarr_append(node->layers, NULL, NULL);
	else
	{
		if((la = dynarr_set(node->layers, layer_id, NULL)) != NULL)
		{
			if(la->name[0] != '\0')
			{
				if(la->blocks != NULL)
				{
					/*bintree_destroy(); */
				}
			}
		}
	}
	la->id = layer_id;
	stu_strncpy(la->name, sizeof la->name, name);
	la->blocks = NULL;

	return la;
}

/* ----------------------------------------------------------------------------------------- */

static void cb_a_layer_create(void *user, VNodeID node_id, VLayerID layer_id, const char *name,
			      VNALayerType type, real64 frequency)
{
	NodeAudio	*n;
	NdbALayer	*layer;

	printf("audio callback: create layer %u (%s) in %u\n", layer_id, name, node_id);
	if((n = (NodeAudio *) nodedb_lookup_with_type(node_id, V_NT_AUDIO)) == NULL)
		return;
	if((layer = dynarr_index(n->layers, layer_id)) != NULL && layer->name[0] != '\0')
	{
		LOG_WARN(("Missing code, audio layer needs to be reborn"));
		return;
	}
	else
	{
		printf("audio layer %s created\n", name);
		nodedb_a_layer_create(n, layer_id, name, type, frequency);
		NOTIFY(n, STRUCTURE);
		verse_send_a_layer_subscribe(node_id, layer_id);
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

static void cb_a_layer_destroy(void *user, VNodeID node_id, VLayerID buffer_id)
{
	NodeAudio	*n;

	if((n = (NodeAudio *) nodedb_lookup_with_type(node_id, V_NT_AUDIO)) != NULL)
	{
		NdbALayer	*al;

		if((al = dynarr_index(n->layers, buffer_id)) != NULL)
		{
			NOTIFY(n, STRUCTURE);
		}
	}
}

static void cb_a_block_set(void *user, VNodeID node_id, VLayerID layer_id, uint32 pos, ...)
{
	NodeAudio	*n;

	if((n = (NodeAudio *) nodedb_lookup_with_type(node_id, V_NT_AUDIO)) != NULL)
	{
		NdbALayer	*al;

		if((al = dynarr_index(n->layers, layer_id)) != NULL)
		{
			NOTIFY(n, DATA);
		}
	}
}

/* ----------------------------------------------------------------------------------------- */

void nodedb_a_register_callbacks(void)
{
	verse_callback_set(verse_send_a_layer_create,	cb_a_layer_create, NULL);
	verse_callback_set(verse_send_a_layer_destroy,	cb_a_layer_destroy, NULL);
	verse_callback_set(verse_send_a_block_set,	cb_a_block_set, NULL);
}
