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
	const PNode	*node;		/* Used only by light and texture fragments. Messy. See sync. */
	unsigned int	pending;	/* Create-call has been issued, and is pending. */
} NdbMFragment;

typedef struct
{
	PNode	node;
	DynArr	*fragments;
} NodeMaterial;

extern void		nodedb_m_construct(NodeMaterial *n);
extern void		nodedb_m_copy(NodeMaterial *n, const NodeMaterial *src);
extern void		nodedb_m_set(NodeMaterial *n, const NodeMaterial *src);
extern void		nodedb_m_destruct(NodeMaterial *n);

extern unsigned int	nodedb_m_fragment_num(const NodeMaterial *node);
extern NdbMFragment *	nodedb_m_fragment_nth(const NodeMaterial *node, unsigned int n);
extern NdbMFragment *	nodedb_m_fragment_lookup(const NodeMaterial *node, VNMFragmentID id);

extern const NdbMFragment * nodedb_m_fragment_find_equal(const NodeMaterial *node,
							 const NodeMaterial *source, const NdbMFragment *f);
extern int		nodedb_m_fragment_resolve(VNMFragmentID *id,  const NodeMaterial *node,
						  const NodeMaterial *source,  VNMFragmentID f);

extern NdbMFragment *	nodedb_m_fragment_create(NodeMaterial *node, VNMFragmentID fragment_id,
						 VNMFragmentType type, const VMatFrag *fragment);
extern NdbMFragment *	nodedb_m_fragment_create_color(NodeMaterial *node, real64 red, real64 green, real64 blue);
extern NdbMFragment *	nodedb_m_fragment_create_light(NodeMaterial *node, VNMLightType type,
						       real64 normal_falloff, const PNode *brdf,
						       const char *brdf_r, const char *brdf_g, const char *brdf_b);
extern NdbMFragment *	nodedb_m_fragment_create_reflection(NodeMaterial *node, real64 normal_falloff);
extern NdbMFragment *	nodedb_m_fragment_create_transparency(NodeMaterial *node, real64 normal_fallof, real64 refract);
extern NdbMFragment *	nodedb_m_fragment_create_volume(NodeMaterial *node, real64 diffusion,
							real64 col_r, real64 col_g, real64 col_b,
							const NdbMFragment *color);
extern NdbMFragment *	nodedb_m_fragment_create_geometry(NodeMaterial *node,
							  const char *layer_r, const char *layer_g, const char *layer_b);
extern NdbMFragment *	nodedb_m_fragment_create_texture(NodeMaterial *node, const PNode *bitmap,
							 const char *layer_r, const char *layer_g, const char *layer_b,
							 const NdbMFragment *mapping);
extern NdbMFragment *	nodedb_m_fragment_create_noise(NodeMaterial *node, VNMNoiseType type, const NdbMFragment *mapping);
extern NdbMFragment *	nodedb_m_fragment_create_blender(NodeMaterial *node, VNMBlendType type,
						const PNMFragment *data_a, const PNMFragment *data_b, const PNMFragment *ctrl);
extern NdbMFragment *	nodedb_m_fragment_create_matrix(NodeMaterial *node, const real64 *matrix, const PNMFragment *data);
extern NdbMFragment *	nodedb_m_fragment_create_ramp(NodeMaterial *node, VNMRampType type, uint8 channel,
						      const NdbMFragment *mapping, uint8 point_count,
						      const VNMRampPoint *ramp);
extern NdbMFragment *	nodedb_m_fragment_create_animation(NodeMaterial *node, const char *label);
extern NdbMFragment *	nodedb_m_fragment_create_alternative(NodeMaterial *node,
							     const NdbMFragment *alt_a, const NdbMFragment *alt_b);
extern NdbMFragment *	nodedb_m_fragment_create_output(NodeMaterial *node, const char *label,
							const NdbMFragment *front, const NdbMFragment *back);

extern void		nodedb_m_register_callbacks(void);
