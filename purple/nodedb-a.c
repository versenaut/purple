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
#include "mem.h"
#include "strutil.h"

#include "nodedb.h"
#include "nodedb-internal.h"

/* ----------------------------------------------------------------------------------------- */

/* Return length of a block in samples, given its type. */
static size_t block_len(VNALayerType type)
{
	static const size_t	len[] = {
		VN_A_BLOCK_SIZE_INT8, VN_A_BLOCK_SIZE_INT16, VN_A_BLOCK_SIZE_INT24,
		VN_A_BLOCK_SIZE_INT32, VN_A_BLOCK_SIZE_REAL32, VN_A_BLOCK_SIZE_REAL64 
	};

	return len[type];
}

/* Return size of a block in bytes, given its type. */
static size_t block_size(VNALayerType type)
{
	static const size_t	size[] = {
		VN_A_BLOCK_SIZE_INT8   * sizeof (uint8),
		VN_A_BLOCK_SIZE_INT16  * sizeof (uint16), 
		VN_A_BLOCK_SIZE_INT24  * 3 * sizeof (uint8),
		VN_A_BLOCK_SIZE_INT32  * sizeof (uint32),
		VN_A_BLOCK_SIZE_REAL32 * sizeof (real32),
		VN_A_BLOCK_SIZE_REAL64 * sizeof (real64)
	};

	return size[type];
}

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

NdbALayer * nodedb_a_layer_nth(const NodeAudio *node, unsigned int n)
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
}

static void cb_a_layer_destroy(void *user, VNodeID node_id, VLayerID layer_id)
{
	NodeAudio	*n;

	if((n = (NodeAudio *) nodedb_lookup_with_type(node_id, V_NT_AUDIO)) != NULL)
	{
		NdbALayer	*al;

		if((al = dynarr_index(n->layers, layer_id)) != NULL)
		{
			NOTIFY(n, STRUCTURE);
		}
	}
}

static int block_compare(const void *key1, const void *key2)
{
	return key1 < key2 ? -1 : key1 > key2;
}

static void cb_a_block_set(void *user, VNodeID node_id, VLayerID layer_id, uint32 block_index,
			   VNALayerType type, const VNASample *data)
{
	NodeAudio	*n;

	if((n = (NodeAudio *) nodedb_lookup_with_type(node_id, V_NT_AUDIO)) != NULL)
	{
		NdbALayer	*al;

		if((al = dynarr_index(n->layers, layer_id)) != NULL)
		{
			NdbABlk	*blk;

			if(al->blocks == NULL)
				al->blocks = bintree_new(block_compare);

			/* Is there an existing block? */
			if((blk = bintree_lookup(al->blocks, (void *) block_index)) != NULL)
			{
				if(blk->type != type)
				{
					printf("Got audio block set of type %d on known block of type %d\n", type, blk->type);
					return;
				}
			}
			else	/* Allocate and insert a new block. */
			{
				blk = mem_alloc(sizeof *blk + block_size(type));
				blk->data = blk + 1;
				bintree_insert(al->blocks, (void *) block_index, blk);
				printf("Set audio block %u.%u.%u\n", node_id, layer_id, block_index);
			}
			/* Copy the data into the block, either replacing old or setting new. */
			memcpy(blk->data, data, block_size(type));
			NOTIFY(n, DATA);
		}
	}
}

static void cb_a_block_clear(void *user, VNodeID node_id, VLayerID layer_id, uint32 block_index)
{
	NodeAudio	*n;

	if((n = (NodeAudio *) nodedb_lookup_with_type(node_id, V_NT_AUDIO)) != NULL)
	{
		NdbALayer	*al;

		if((al = dynarr_index(n->layers, layer_id)) != NULL)
		{
			NdbABlk	*blk;

			printf("Clearing audio block %u.%u.%u\n", node_id, layer_id, block_index);
			if((blk = bintree_lookup(al->blocks, (void *) block_index)) != NULL)
			{
				bintree_remove(al->blocks, (void *) block_index);
				mem_free(blk);
				NOTIFY(n, DATA);
			}
			else
				printf("Can't clear unknown block %u\n", block_index);
		}
	}
}

/* ----------------------------------------------------------------------------------------- */

void nodedb_a_register_callbacks(void)
{
	verse_callback_set(verse_send_a_layer_create,	cb_a_layer_create, NULL);
	verse_callback_set(verse_send_a_layer_destroy,	cb_a_layer_destroy, NULL);
	verse_callback_set(verse_send_a_block_set,	cb_a_block_set, NULL);
	verse_callback_set(verse_send_a_block_clear,	cb_a_block_clear, NULL);
}
