/*
 * nodedb-a.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Audio node support. Limited to just buffers, no real-time streaming.
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
static size_t block_len(VNABlockType type)
{
	static const size_t	len[] = {
		VN_A_BLOCK_SIZE_INT8, VN_A_BLOCK_SIZE_INT16, VN_A_BLOCK_SIZE_INT24,
		VN_A_BLOCK_SIZE_INT32, VN_A_BLOCK_SIZE_REAL32, VN_A_BLOCK_SIZE_REAL64 
	};

	return len[type];
}

/* Return size of a block in bytes, given its type. */
static size_t block_size(VNABlockType type)
{
	static const size_t	size[] = {
		VN_A_BLOCK_SIZE_INT8   * sizeof (uint8),
		VN_A_BLOCK_SIZE_INT16  * sizeof (uint16),
		VN_A_BLOCK_SIZE_INT24  * sizeof (uint32),
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
	n->buffers = NULL;
}

static NdbABlk * block_new(const NdbABuffer *buffer)
{
	NdbABlk	*blk;

	if((blk = mem_alloc(sizeof *blk + block_size(buffer->type))) != NULL)
	{
		blk->type = buffer->type;
		blk->data = blk + 1;
	}
	return blk;
}

static void * block_copy(const void *key, const void *element, void *user)
{
	const NdbABuffer	*ref = (NdbABuffer *) user;
	NdbABlk		*blk;

	blk = block_new(ref);
	memcpy(blk->data, ((NdbABlk *) element)->data, block_size(ref->type));

	return blk;
}

static void block_destroy(NdbABlk *blk)
{
	mem_free(blk);
}

static void cb_copy_buffer(void *d, const void *s, void *user)
{
	const NdbABuffer	*src = s;
	NdbABuffer	*dst = d;

	dst->id = src->id;
	strcpy(dst->name, src->name);
	dst->type = src->type;
	dst->frequency = src->frequency;
	dst->blocks = bintree_new_copy(src->blocks, block_copy, dst);
}

void nodedb_a_copy(NodeAudio *n, const NodeAudio *src)
{
	n->buffers = dynarr_new_copy(src->buffers, cb_copy_buffer, NULL);
}

/* Set <n> to equal contents of <src>. */
void nodedb_a_set(NodeAudio *n, const NodeAudio *src)
{
	/* FIXME: This can't quite claim to be efficent. :/ */
	nodedb_a_destruct(n);
	nodedb_a_copy(n, src);
}

static void cb_block_destroy(const void *key, void *element)
{
	block_destroy(element);
}

void nodedb_a_destruct(NodeAudio *n)
{
	unsigned int	i, num;

	num = dynarr_size(n->buffers);
	for(i = 0; i < num; i++)
	{
		NdbABuffer	*la;

		if((la = dynarr_index(n->buffers, i)) != NULL && la->name[0] != '\0')
		{
			bintree_destroy(la->blocks, cb_block_destroy);
		}
	}
	dynarr_destroy(n->buffers);
	n->buffers = NULL;
}

unsigned int nodedb_a_buffer_num(const NodeAudio *node)
{
	unsigned int	i, num;
	NdbABuffer	*la;

	if(node == NULL || node->node.type != V_NT_AUDIO)
		return 0;
	for(i = num = 0; (la = dynarr_index(node->buffers, i)) != NULL; i++)
	{
		if(la->name[0] == '\0')
			continue;
		num++;
	}
	return num;
}

NdbABuffer * nodedb_a_buffer_nth(const NodeAudio *node, unsigned int n)
{
	unsigned int	i;
	NdbABuffer	*la;

	if(node == NULL || node->node.type != V_NT_AUDIO)
		return 0;
	for(i = 0; (la = dynarr_index(node->buffers, i)) != NULL; i++)
	{
		if(la->name[0] == '\0')
			continue;
		if(n == 0)
			return la;
		n--;
	}
	return NULL;
}

NdbABuffer * nodedb_a_buffer_find(const NodeAudio *node, const char *name)
{
	unsigned int	i;
	NdbABuffer	*la;

	if(node == NULL || name == NULL || *name == '\0')
		return NULL;
	for(i = 0; (la = dynarr_index(node->buffers, i)) != NULL; i++)
	{
		if(strcmp(la->name, name) == 0)
			return la;
	}
	return NULL;
}

static void cb_def_buffer(unsigned int index, void *element, void *user)
{
	NdbABuffer	*la = element;

	la->name[0] = '\0';
	la->blocks  = NULL;
}

NdbABuffer * nodedb_a_buffer_create(NodeAudio *node, VBufferID buffer_id, const char *name, VNABlockType type, real64 frequency)
{
	NdbABuffer	*la;

	if(node->buffers == NULL)
	{
		node->buffers = dynarr_new(sizeof *la, 2);
		dynarr_set_default_func(node->buffers, cb_def_buffer, NULL);
	}
	if(node->buffers == NULL)
		return NULL;
	if(buffer_id == (VLayerID) ~0)
		la = dynarr_append(node->buffers, NULL, NULL);
	else
	{
		if((la = dynarr_set(node->buffers, buffer_id, NULL)) != NULL)
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
	la->id = buffer_id;
	stu_strncpy(la->name, sizeof la->name, name);
	la->type = type;
	la->frequency = frequency;
	la->blocks = NULL;

	return la;
}

/* ----------------------------------------------------------------------------------------- */

int nodedb_a_blocks_equal(VNABlockType type, const NdbABlk *blk1, const NdbABlk *blk2)
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
			*samples++ = ((real64) *get++) / (real64) (1 << (bits - 1));\
	}\
	break

#define	READREAL(bits)	\
	{\
		real ## bits	*get = blk->data;\
		for(i = 0, get += offset; i < chunk; i++)\
			*samples++ = *get++;\
	}\
	break

unsigned int nodedb_a_buffer_read_samples(const NdbABuffer *buffer, unsigned int start, real64 *samples, unsigned int length)
{
	unsigned int	index, pos, offset, bl, i, chunk, to_go = length, max;
	const NdbABlk	*blk;

	if(buffer == NULL || buffer == NULL || length == 0)
		return 0;
	bl = block_len(buffer->type);

	max = (unsigned long) bintree_key_maximum(buffer->blocks);

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
		if((blk = bintree_lookup(buffer->blocks, (void *) index)) != NULL)
		{
			switch(buffer->type)
			{
			case VN_A_BLOCK_INT8:
				READINT(8);
			case VN_A_BLOCK_INT16:
				READINT(16);
			case VN_A_BLOCK_INT24:
				{
					uint32	*get = blk->data;
					for(i = 0, get += offset; i < chunk; i++)
						*samples++ = (*get++ >> 8) * (((real64) (1 << 24)) / 4294967296.0);
				}
				break;
			case VN_A_BLOCK_INT32:
				READINT(32);
			case VN_A_BLOCK_REAL32:
				READREAL(32);
			case VN_A_BLOCK_REAL64:
				READREAL(64);
			}
		}
		else
		{
			printf(" Block is clear, setting %u zeroes\n", chunk);
			for(i = 0; i < chunk; i++)
				*samples++ = 0.0;
		}
	}
	return pos - start;
}

/* Handy dandy macros for writing data back into blocks, and make my life a bit easier. */
#define	WRITEINT(bits)	\
	{\
		int ## bits	*put = blk->data;\
		for(i = 0, put += offset; i < chunk; i++)\
			*put++ = *samples++ * (real64) (1 << (bits - 1));\
	}\
	break

#define	WRITEREAL(bits)	\
	{\
		real ## bits	*put = blk->data;\
		for(i = 0, put += offset; i < chunk; i++)\
			*put++ = *samples++;\
	}\
	break


void nodedb_a_buffer_write_samples(NdbABuffer *buffer, unsigned int start, const real64 *samples, unsigned int length)
{
	unsigned int	index, pos, offset, bl, i, chunk, to_go = length;
	NdbABlk		*blk;

	if(buffer == NULL || buffer == NULL || length == 0)
		return;
	bl = block_len(buffer->type);

	if(buffer->blocks == NULL)
		buffer->blocks = bintree_new(block_compare);

/*	printf("Writing back %u samples from position %u\n", length, start);*/
	for(pos = start, to_go = length; pos < start + length; pos += chunk, to_go -= chunk)
	{
		index  = pos / bl;
		offset = pos % bl;
		chunk  = bl - offset;
		if(chunk > to_go)
			chunk = to_go;
		
/*		printf("Setting audio data from position %u -> block %u, offset %u, chunk %u\n", pos, index, offset, chunk);*/
		if((blk = bintree_lookup(buffer->blocks, (void *) index)) == NULL)
		{
			blk = block_new(buffer);
			bintree_insert(buffer->blocks, (void *) index, blk);
		}
		switch(buffer->type)
		{
		case VN_A_BLOCK_INT8:
			WRITEINT(8);
		case VN_A_BLOCK_INT16:
			WRITEINT(16);
		case VN_A_BLOCK_INT24:
			printf("Can't write back 24-bit integer samples, code missing\n");	/* FIXME. */
			break;
		case VN_A_BLOCK_INT32:
			WRITEINT(32);
		case VN_A_BLOCK_REAL32:
			WRITEREAL(32);
		case VN_A_BLOCK_REAL64:
			WRITEREAL(64);
		default:	/* FIXME: Code missing here. */
			LOG_ERR(("Unhandled buffer type %d in write_samples()", buffer->type));
		}
	}
}

/* ----------------------------------------------------------------------------------------- */

static void cb_a_buffer_create(void *user, VNodeID node_id, VLayerID buffer_id, const char *name,
			      VNABlockType type, real64 frequency)
{
	NodeAudio	*n;
	NdbABuffer	*buffer;

	printf("audio callback: create buffer %u (%s) in %u\n", buffer_id, name, node_id);
	if((n = (NodeAudio *) nodedb_lookup_with_type(node_id, V_NT_AUDIO)) == NULL)
		return;
	if((buffer = dynarr_index(n->buffers, buffer_id)) != NULL && buffer->name[0] != '\0')
	{
		LOG_WARN(("Missing code, audio buffer needs to be reborn"));
		return;
	}
	else
	{
		printf("audio buffer %s created\n", name);
		nodedb_a_buffer_create(n, buffer_id, name, type, frequency);
		NOTIFY(n, STRUCTURE);
		verse_send_a_buffer_subscribe(node_id, buffer_id);
	}
}

static void cb_a_buffer_destroy(void *user, VNodeID node_id, VLayerID buffer_id)
{
	NodeAudio	*n;

	if((n = (NodeAudio *) nodedb_lookup_with_type(node_id, V_NT_AUDIO)) != NULL)
	{
		NdbABuffer	*al;

		if((al = dynarr_index(n->buffers, buffer_id)) != NULL)
		{
			al->name[0] = '\0';
			printf("Missing code to destroy audio buffer\n");	/* FIXME: Write more. */
			NOTIFY(n, STRUCTURE);
		}
	}
}

static void cb_a_block_set(void *user, VNodeID node_id, VBufferID buffer_id, uint32 block_index,
			   VNABlockType type, const VNABlock *data)
{
	NodeAudio	*n;

	if((n = (NodeAudio *) nodedb_lookup_with_type(node_id, V_NT_AUDIO)) != NULL)
	{
		NdbABuffer	*al;

		if((al = dynarr_index(n->buffers, buffer_id)) != NULL)
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

static void cb_a_block_clear(void *user, VNodeID node_id, VLayerID buffer_id, uint32 block_index)
{
	NodeAudio	*n;

	if((n = (NodeAudio *) nodedb_lookup_with_type(node_id, V_NT_AUDIO)) != NULL)
	{
		NdbABuffer	*al;

		if((al = dynarr_index(n->buffers, buffer_id)) != NULL)
		{
			NdbABlk	*blk;

			printf("Clearing audio block %u.%u.%u\n", node_id, buffer_id, block_index);
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
	verse_callback_set(verse_send_a_buffer_create,	cb_a_buffer_create, NULL);
	verse_callback_set(verse_send_a_buffer_destroy,	cb_a_buffer_destroy, NULL);
	verse_callback_set(verse_send_a_block_set,	cb_a_block_set, NULL);
	verse_callback_set(verse_send_a_block_clear,	cb_a_block_clear, NULL);
}
