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
	VNALayerType	type;
	void		*data;		/* Data in type-specific format, co-allocated with block. */
} NdbABlk;

typedef struct
{
	uint16		id;
	char		name[16];
	VNALayerType	type;
	real64		frequency;
	BinTree		*blocks;
} NdbALayer;

typedef struct
{
	Node	node;
	DynArr	*layers;
} NodeAudio;

extern void		nodedb_a_construct(NodeAudio *n);
extern void		nodedb_a_copy(NodeAudio *n, const NodeAudio *src);
extern void		nodedb_a_set(NodeAudio *n, const NodeAudio *src);
extern void		nodedb_a_destruct(NodeAudio *n);

extern unsigned int	nodedb_a_layer_num(const NodeAudio *node);
extern NdbALayer *	nodedb_a_layer_nth(const NodeAudio *node, unsigned int n);
extern NdbALayer *	nodedb_a_layer_find(const NodeAudio *node, const char *name);

extern int		nodedb_a_blocks_equal(VNALayerType type, const NdbABlk *blk1, const NdbABlk *blk2);

extern unsigned int	nodedb_a_layer_read_samples(const NdbALayer *layer, unsigned int start, real64 *buffer, unsigned int length);
extern void		nodedb_a_layer_write_samples(NdbALayer *layer, unsigned int start, const real64 *buffer, unsigned int length);

extern NdbALayer *	nodedb_a_layer_create(NodeAudio *node, VLayerID layer_id, const char *name, VNALayerType type, real64 frequency);

extern void		nodedb_a_register_callbacks(void);
