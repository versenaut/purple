/*
 * nodedb-a.h
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Audio node databasing module.
*/

#include "bintree.h"

/* Held in binary tree, indexed by block index which is not stored here but in tree. */
typedef struct
{
	VNABlockType	type;
	void		*data;		/* Data in type-specific format, co-allocated with block. */
} NdbABlk;

typedef struct
{
	uint16		id;
	char		name[16];
	VNABlockType	type;
	real64		frequency;
	BinTree		*blocks;
} NdbABuffer;

typedef struct
{
	Node	node;
	DynArr	*buffers;
} NodeAudio;

extern void		nodedb_a_construct(NodeAudio *n);
extern void		nodedb_a_copy(NodeAudio *n, const NodeAudio *src);
extern void		nodedb_a_set(NodeAudio *n, const NodeAudio *src);
extern void		nodedb_a_destruct(NodeAudio *n);

extern unsigned int	nodedb_a_buffer_num(const NodeAudio *node);
extern NdbABuffer *	nodedb_a_buffer_nth(const NodeAudio *node, unsigned int n);
extern NdbABuffer *	nodedb_a_buffer_find(const NodeAudio *node, const char *name);

extern int		nodedb_a_blocks_equal(VNABlockType type, const NdbABlk *blk1, const NdbABlk *blk2);

extern unsigned int	nodedb_a_buffer_read_samples(const NdbABuffer *buffer, unsigned int start, real64 *samples, unsigned int length);
extern void		nodedb_a_buffer_write_samples(NdbABuffer *buffer, unsigned int start, const real64 *samples, unsigned int length);

extern NdbABuffer *	nodedb_a_buffer_create(NodeAudio *node, VBufferID buffer_id, const char *name, VNABlockType type, real64 frequency);

extern void		nodedb_a_register_callbacks(void);
