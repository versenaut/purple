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

/* Compare audio blocks, for bintree. */
static int block_compare(const void *key1, const void *key2)
{
	return key1 < key2 ? -1 : key1 > key2;
}

/* ----------------------------------------------------------------------------------------- */

void nodedb_a_construct(NodeAudio *n)
{
	n->layers = NULL;
}

static NdbABlk * block_new(const NdbALayer *layer)
{
	NdbABlk	*blk;

	if((blk = mem_alloc(sizeof *blk + block_size(layer->type))) != NULL)
	{
		blk->type = layer->type;
		blk->data = blk + 1;
	}
	return blk;
}

static void * block_copy(const void *key, const void *element, void *user)
{
	const NdbALayer	*ref = (NdbALayer *) user;
	NdbABlk		*blk;

	blk = block_new(ref);
	memcpy(blk->data, ((NdbABlk *) element)->data, block_size(ref->type));

	return blk;
}

static void cb_copy_layer(void *d, const void *s, void *user)
{
	const NdbALayer	*src = s;
	NdbALayer	*dst = d;

	dst->id = src->id;
	strcpy(dst->name, src->name);
	dst->type = src->type;
	dst->frequency = src->frequency;
	dst->blocks = bintree_new_copy(src->blocks, block_copy, dst);
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

	if(node == NULL || node->node.type != V_NT_AUDIO)
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
	la->type = type;
	la->frequency = frequency;
	la->blocks = NULL;

	return la;
}

/* ----------------------------------------------------------------------------------------- */

int nodedb_a_blocks_equal(VNALayerType type, const NdbABlk *blk1, const NdbABlk *blk2)
{
	if(blk1 == blk2)
		return 1;
	if(blk1 == NULL || blk2 == NULL)
		return 0;
	return memcmp(blk1->data, blk2->data, block_size(type)) == 0;
}

/* ----------------------------------------------------------------------------------------- */

#define	READINT(bits)	\
	{\
		int ## bits	*get = blk->data;\
		for(i = 0, get += offset; i < chunk; i++)\
			*buffer++ = ((real64) *get++) / (real64) (1 << (bits - 1));\
	}\
	break

#define	READREAL(bits)	\
	{\
		real ## bits	*get = blk->data;\
		for(i = 0, get += offset; i < chunk; i++)\
			*buffer++ = *get++;\
	}\
	break

unsigned int nodedb_a_layer_read_samples(const NdbALayer *layer, unsigned int start, real64 *buffer, unsigned int length)
{
	unsigned int	index, pos, offset, bl, i, chunk, to_go = length, max;
	const NdbABlk	*blk;

	if(layer == NULL || buffer == NULL || length == 0)
		return 0;
	bl = block_len(layer->type);

	max = (unsigned long) bintree_key_maximum(layer->blocks);

/*	printf("Reading out %u samples from position %u (max=%u)\n", length, start, max);*/

	for(pos = start; pos < start + length; pos += chunk, to_go -= chunk)
	{
		index  = pos / bl;
		offset = pos % bl;
		chunk  = bl - offset;
		if(chunk > to_go)
			chunk = to_go;

/*		printf("Getting audio data from position %u -> block %u, offset %u, chunk %u\n", pos, index, offset, chunk);*/
		if(index > max)
		{
/*			printf(" Block index too large, there is no more data. Aborting\n");*/
			return pos - start;
		}
		if((blk = bintree_lookup(layer->blocks, (void *) index)) != NULL)
		{
			switch(layer->type)
			{
			case VN_A_LAYER_INT8:
				READINT(8);
			case VN_A_LAYER_INT16:
				READINT(16);
			case VN_A_LAYER_INT24:
				{
					uint32	*get = blk->data;
					printf("  converting 24-bit data (FIXME: BROKEN)\n");	/* FIXME! */
					for(i = 0, get += offset; i < chunk; i++)
						*buffer++ = ((real64) *get++) / 8388608.0;
				}
				break;
			case VN_A_LAYER_INT32:
				READINT(32);
			case VN_A_LAYER_REAL32:
				READREAL(32);
			case VN_A_LAYER_REAL64:
				READREAL(64);
			}
		}
		else
		{
			printf(" Block is clear, setting %u zeroes\n", chunk);
			for(i = 0; i < chunk; i++)
				*buffer++ = 0.0;
		}
	}
	return pos - start;
}

/* Handy dandy macros for writing data back into blocks, and make my life a bit easier. */
#define	WRITEINT(bits)	\
	{\
		int ## bits	*put = blk->data;\
		for(i = 0, put += offset; i < chunk; i++)\
			*put++ = *buffer++ * (real64) (1 << (bits - 1));\
	}\
	break

#define	WRITEREAL(bits)	\
	{\
		real ## bits	*put = blk->data;\
		for(i = 0, put += offset; i < chunk; i++)\
			*put++ = *buffer++;\
	}\
	break


void nodedb_a_layer_write_samples(NdbALayer *layer, unsigned int start, const real64 *buffer, unsigned int length)
{
	unsigned int	index, pos, offset, bl, i, chunk, to_go = length;
	NdbABlk		*blk;

	if(layer == NULL || buffer == NULL || length == 0)
		return;
	bl = block_len(layer->type);

	if(layer->blocks == NULL)
		layer->blocks = bintree_new(block_compare);

/*	printf("Writing back %u samples from position %u\n", length, start);*/
	for(pos = start, to_go = length; pos < start + length; pos += chunk, to_go -= chunk)
	{
		index  = pos / bl;
		offset = pos % bl;
		chunk  = bl - offset;
		if(chunk > to_go)
			chunk = to_go;
		
/*		printf("Setting audio data from position %u -> block %u, offset %u, chunk %u\n", pos, index, offset, chunk);*/
		if((blk = bintree_lookup(layer->blocks, (void *) index)) == NULL)
		{
			printf("no block %u found, creating\n", index);
			blk = block_new(layer);
			bintree_insert(layer->blocks, (void *) index, blk);
		}
		switch(layer->type)
		{
		case VN_A_LAYER_INT8:
			WRITEINT(8);
		case VN_A_LAYER_INT16:
			WRITEINT(16);
		case VN_A_LAYER_INT32:
			WRITEINT(32);
		case VN_A_LAYER_REAL32:
			WRITEREAL(32);
		case VN_A_LAYER_REAL64:
			WRITEREAL(64);
		default:	/* FIXME: Code missing here. */
			LOG_ERR(("Unhandled layer type %d in write_samples()", layer->type));
		}
	}
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
			al->name[0] = '\0';
			printf("Missing code to destroy audio layer\n");	/* FIXME: Write more. */
			NOTIFY(n, STRUCTURE);
		}
	}
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
					printf("Got audio block set, index=%u, type %d on known block of type %d\n", block_index, type, blk->type);
					return;
				}
			}
			else	/* Allocate and insert a new block. */
			{
				blk = block_new(al);
				bintree_insert(al->blocks, (void *) block_index, blk);
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
