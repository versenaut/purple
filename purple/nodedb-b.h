/*
 * Bitmap node database implementation.
*/

typedef struct
{
	uint16		id;
	char		name[16];
	VNBLayerType	type;
	void		*framebuffer;
} NdbBLayer;

typedef struct
{
	Node	node;
	uint16	width, height, depth;
	DynArr	*layers;
} NodeBitmap;

extern void		nodedb_b_construct(NodeBitmap *n);
extern void		nodedb_b_copy(NodeBitmap *n, const NodeBitmap *src);
extern void		nodedb_b_destruct(NodeBitmap *n);

extern NdbBLayer *	nodedb_b_layer_lookup(const NodeBitmap *node, const char *name);
extern NdbBLayer *	nodedb_b_layer_lookup_id(const NodeBitmap *node, uint16 buffer_id);

extern void		nodedb_b_register_callbacks(void);
