/*
 * nodedb-g.h
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Geometry node database implementation.
*/

typedef struct
{
	VLayerID	id;
	char		name[16];
	VNGLayerType	type;
	DynArr		*data;
	uint32		def_uint;
	real64		def_real;
} NdbGLayer;

typedef struct NdbGBone	NdbGBone;
struct NdbGBone
{
	uint16		id;
	char		weight[16];
	char		reference[16];
	uint16		parent;
	real64		pos[3];		/* XYZ (duh). */
	char		pos_curve[16];
	real64		rot[4];		/* XYZW */
	char		rot_curve[16];
	boolean		pending;
};

typedef struct
{
	PNode	node;
	DynArr	*layers;
	DynArr	*bones;
	struct {
	char	layer[16];
	uint32	def;
	}	crease_vertex, crease_edge;
} NodeGeometry;

extern void		nodedb_g_construct(NodeGeometry *n);
extern void		nodedb_g_copy(NodeGeometry *n, const NodeGeometry *src);
extern void		nodedb_g_set(NodeGeometry *n, const NodeGeometry *src);
extern void		nodedb_g_destruct(NodeGeometry *n);

extern unsigned int	nodedb_g_layer_num(const NodeGeometry *n);
extern NdbGLayer *	nodedb_g_layer_nth(const NodeGeometry *n, unsigned int i);
extern NdbGLayer *	nodedb_g_layer_find(const NodeGeometry *n, const char *name);

extern size_t		nodedb_g_layer_get_size(const NdbGLayer *layer);
extern const char *	nodedb_g_layer_get_name(const NdbGLayer *layer);

extern NdbGLayer *	nodedb_g_layer_create(NodeGeometry *node, VLayerID layer_id, const char *name, VNGLayerType type);
extern void		nodedb_g_layer_destroy(NodeGeometry *node, NdbGLayer *layer);

extern void		nodedb_g_vertex_set_xyz(NdbGLayer *layer, uint32 vertex_id, real64 x, real64 y, real64 z);
extern void		nodedb_g_vertex_get_xyz(const NdbGLayer *layer, uint32 vertex_id, real64 *x, real64 *y, real64 *z);
extern void		nodedb_g_vertex_set_uint32(NdbGLayer *layer, uint32 vertex_id, uint32 value);
extern uint32		nodedb_g_vertex_get_uint32(const NdbGLayer *layer, uint32 vertex_id);
extern void		nodedb_g_polygon_set_corner_uint32(NdbGLayer *layer, uint32 polygon_id,
							   uint32 v0, uint32 v1, uint32 v2, uint32 v3);
extern void		nodedb_g_polygon_get_corner_uint32(const NdbGLayer *layer, uint32 polygon_id,
							   uint32 *v0, uint32 *v1, uint32 *v2, uint32 *v3);
extern void		nodedb_g_polygon_set_corner_real32(NdbGLayer *layer, uint32 polygon_id,
							   real32 v0, real32 v1, real32 v2, real32 v3);
extern void		nodedb_g_polygon_get_corner_real32(const NdbGLayer *layer, uint32 polygon_id,
							   real32 *v0, real32 *v1, real32 *v2, real32 *v3);
extern void		nodedb_g_polygon_set_corner_real64(NdbGLayer *layer, uint32 polygon_id,
							   real64 v0, real64 v1, real64 v2, real64 v3);
extern void		nodedb_g_polygon_get_corner_real64(const NdbGLayer *layer, uint32 polygon_id,
							   real64 *v0, real64 *v1, real64 *v2, real64 *v3);
extern void		nodedb_g_polygon_set_face_uint8(NdbGLayer *layer, uint32 polygon_id, uint8 value);
extern uint8		nodedb_g_polygon_get_face_uint8(const NdbGLayer *layer, uint32 polygon_id);
extern void		nodedb_g_polygon_set_face_uint32(NdbGLayer *layer, uint32 polygon_id, uint32 value);
extern uint32		nodedb_g_polygon_get_face_uint32(const NdbGLayer *layer, uint32 polygon_id);
extern void		nodedb_g_polygon_set_face_real64(NdbGLayer *layer, uint32 polygon_id, real64 value);
extern real64		nodedb_g_polygon_get_face_real64(const NdbGLayer *layer, uint32 polygon_id);

extern NdbGBone *	nodedb_g_bone_lookup(const NodeGeometry *node, uint16 id);
extern const NdbGBone *	nodedb_g_bone_find_equal(const NodeGeometry *n, const NodeGeometry *source, const NdbGBone *bone);
extern int		nodedb_g_bone_resolve(uint16 *id, const NodeGeometry *n, const NodeGeometry *source, uint16 b);
extern NdbGBone *	nodedb_g_bone_create(NodeGeometry *n, uint16 id,
					     const char *weights, const char *references,
					     uint16 parent,
					     real64 px, real64 py, real64 pz, const char *pos_curve,
					     real64 rx, real64 ry, real64 rz, real64 rw, const char *rot_curve);
extern void		nodedb_g_bone_destroy(NodeGeometry *n, NdbGBone *bone);

extern void		nodedb_g_crease_set_vertex(NodeGeometry *node, const char *layer, uint32 def);
extern void		nodedb_g_crease_set_edge(NodeGeometry *node, const char *layer, uint32 def);

extern void		nodedb_g_register_callbacks(void);
