/*
 * nodedb-a.h
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Audio node databasing module.
*/

#include "bintree.h"

typedef struct
{
	uint32		index;		/* Index. Serves as key in binary tree, and orders blocks. */
	VNALayerType	type;
	VNASample	*data;
} NdbABlk;

typedef struct
{
	uint16	id;
	char	name[16];
	real64	frequency;
	BinTree	*blocks;
} NdbALayer;

typedef struct
{
	Node	node;
	DynArr	*layers;
} NodeAudio;

extern void		nodedb_a_construct(NodeAudio *n);
extern void		nodedb_a_copy(NodeAudio *n, const NodeAudio *src);
extern void		nodedb_a_destruct(NodeAudio *n);

extern unsigned int	nodedb_a_buffer_num(const NodeAudio *node);
extern NdbALayer *	nodedb_a_buffer_nth(const NodeAudio *node, unsigned int n);
extern NdbALayer *	nodedb_a_buffer_find(const NodeAudio *node, const char *name);

extern NdbALayer *	nodedb_a_layer_create(NodeAudio *node, VLayerID layer_id, const char *name, VNALayerType type, real64 frequency);
/*extern char *		nodedb_a_buffer_read_line(NdbTBuffer *buffer, unsigned int line, char *put, size_t putmax);
extern void		nodedb_t_buffer_insert(NdbTBuffer *buffer, size_t pos, const char *text);
extern void		nodedb_t_buffer_append(NdbTBuffer *buffer, const char *text);
extern void		nodedb_t_buffer_delete(NdbTBuffer *buffer, size_t pos, size_t length);
extern void		nodedb_t_buffer_clear(NdbTBuffer *buffer);
*/
extern void		nodedb_a_register_callbacks(void);
