/*
 * nodedb-g.h
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Geometry node database implementation.
*/

typedef struct
{
	uint16		id;
	char		name[16];
	VNGLayerType	type;
	DynArr		*data;
	uint32		def_uint;
	real64		def_real;
} NdbGLayer;

typedef struct
{
	char	weight[16];
	char	reference[16];
	uint32	parent;
	real64	pos[3];		/* XYZ (duh). */
	real64	rot[4];		/* XYZW */
} NdbGBone;

typedef struct
{
	Node	node;
	DynArr	*layers;
	DynArr	*bones;
	struct {
	char	layer[16];
	uint32	def;
	}	crease_vertex, crease_edge;
} NodeGeometry;

extern void		nodedb_g_construct(NodeGeometry *n);
extern void		nodedb_g_copy(NodeGeometry *n, const NodeGeometry *src);
extern void		nodedb_g_destruct(NodeGeometry *n);

extern unsigned int	nodedb_g_layer_num(const NodeGeometry *n);
extern NdbGLayer *	nodedb_g_layer_nth(const NodeGeometry *n, unsigned int i);
extern NdbGLayer *	nodedb_g_layer_find(const NodeGeometry *n, const char *name);

extern size_t		nodedb_g_layer_get_size(const NdbGLayer *layer);
extern const char *	nodedb_g_layer_get_name(const NdbGLayer *layer);

extern void		nodedb_g_layer_create(NodeGeometry *n, VLayerID layer_id, const char *name, VNGLayerType type);

extern void		nodedb_g_vertex_set_xyz(NdbGLayer *layer, uint32 vertex_id, real64 x, real64 y, real64 z);
extern void		nodedb_g_vertex_get_xyz(const NdbGLayer *layer, uint32 vertex_id, real64 *x, real64 *y, real64 *z);
extern void		nodedb_g_polygon_set_corner_uint32(NdbGLayer *layer, uint32 polygon_id,
							   uint32 v0, uint32 v1, uint32 v2, uint32 v3);

extern void		nodedb_g_crease_set_vertex(NodeGeometry *node, const char *layer, uint32 def);
extern void		nodedb_g_crease_set_edge(NodeGeometry *node, const char *layer, uint32 def);

extern void		nodedb_g_register_callbacks(void);
