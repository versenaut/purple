/*
 * nodedb-m.h
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Material node databasing.
*/

typedef struct
{
	VNMFragmentID	id;
	VNMFragmentType	type;
	VMatFrag	frag;
} NdbMFragment;

typedef struct
{
	Node	node;
	DynArr	*fragments;
} NodeMaterial;

extern void		nodedb_m_construct(NodeMaterial *n);
extern void		nodedb_m_copy(NodeMaterial *n, const NodeMaterial *src);
extern void		nodedb_m_destruct(NodeMaterial *n);

extern unsigned int	nodedb_m_fragment_num(const NodeMaterial *node);
extern NdbMFragment *	nodedb_m_fragment_nth(const NodeMaterial *node, unsigned int n);

extern void		nodedb_m_register_callbacks(void);
