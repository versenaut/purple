/*
 * Geometry node database implementation.
*/

typedef struct
{
	uint16		id;
	char		name[16];
	VNGLayerType	type;
	void		*data;
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

extern NdbGLayer *	nodedb_g_layer_lookup(const NodeGeometry *n, const char *name);
extern void		nodedb_g_layer_create(NodeGeometry *n, VLayerID layer_id, const char *name, VNGLayerType type);

extern void		nodedb_g_vertex_set_xyz(NodeGeometry *node, NdbGLayer *layer, uint32 vertex_id, real64 x, real64 y, real64 z);
extern void		nodedb_g_polygon_set_corner_uint32(NodeGeometry *node, NdbGLayer *layer, uint32 polygon_id,
							   uint32 v0, uint32 v1, uint32 v2, uint32 v3);

extern void		nodedb_g_register_callbacks(void);
