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
	const Node	*node;		/* Used only by light and texture fragments. Messy. See sync. */
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
extern NdbMFragment *	nodedb_m_fragment_lookup(const NodeMaterial *node, VNMFragmentID id);

extern const NdbMFragment * nodedb_m_fragment_find_equiv(const NodeMaterial *node,
							 const NodeMaterial *source, const NdbMFragment *f);
extern int		nodedb_m_fragment_resolve(VNMFragmentID *id,  const NodeMaterial *node,
						  const NodeMaterial *source,  VNMFragmentID f);

extern NdbMFragment *	nodedb_m_fragment_create(NodeMaterial *node, VNMFragmentID fragment_id,
						 VNMFragmentType type, const VMatFrag *fragment);
extern NdbMFragment *	nodedb_m_fragment_create_color(NodeMaterial *node, real64 red, real64 green, real64 blue);
extern NdbMFragment *	nodedb_m_fragment_create_light(NodeMaterial *node, VNMLightType type,
						       real64 normal_falloff, const Node *brdf,
						       const char *brdf_r, const char *brdf_g, const char *brdf_b);
extern NdbMFragment *	nodedb_m_fragment_create_output(NodeMaterial *node, const char *label,
							const NdbMFragment *front, const NdbMFragment *back);

extern void		nodedb_m_register_callbacks(void);
