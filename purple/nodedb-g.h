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

extern void		nodedb_g_register_callbacks(void);
