/*
 * nodedb-b.h
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Bitmap node database implementation.
*/

#include <stdarg.h>

/* This is used to avoid copying tiles back and forth when sychronizing, by
 * instead providing enough access information to let it at the bytes.
*/
typedef struct
{
	struct
	{
		uint16	x, y, z;
	}	in;
	struct
	{
		uint8	*ptr;
		size_t	mod_row;
		size_t	mod_tile;
		size_t	width;		/* Effective width; last column can be cropped. In bytes. */
		size_t	height;		/* Effective height; last row can be cropped. */
	}	out;
} NdbBTileDesc;

typedef struct
{
	uint16		id;
	char		name[16];
	VNBLayerType	type;
	void		*framebuffer;
} NdbBLayer;

typedef enum {
	NDB_B_FILTER_NEAREST = 0
} NdbBFilterMode;

typedef struct
{
	PNode	node;
	uint16	width, height, depth;
	DynArr	*layers;
} NodeBitmap;

extern void		nodedb_b_construct(NodeBitmap *n);
extern void		nodedb_b_copy(NodeBitmap *n, const NodeBitmap *src);
extern void		nodedb_b_set(NodeBitmap *n, const NodeBitmap *src);
extern void		nodedb_b_destruct(NodeBitmap *n);

extern int		nodedb_b_set_dimensions(NodeBitmap *node, uint16 width, uint16 height, uint16 depth);
extern void		nodedb_b_get_dimensions(const NodeBitmap *node, uint16 *width, uint16 *height, uint16 *depth);

extern unsigned int	nodedb_b_layer_num(const NodeBitmap *node);
extern NdbBLayer *	nodedb_b_layer_nth(const NodeBitmap *node, unsigned int n);
extern NdbBLayer *	nodedb_b_layer_find(const NodeBitmap *node, const char *name);

extern NdbBLayer *	nodedb_b_layer_create(NodeBitmap *node, VLayerID layer_id, const char *name, VNBLayerType type);

extern real64		nodedb_b_layer_pixel_read(const NodeBitmap *node, const NdbBLayer *layer, real64 x, real64 y, real64 z);
extern real64		nodedb_b_layer_pixel_read_filtered(const NodeBitmap *node, const NdbBLayer *layer, NdbBFilterMode mode, real64 x, real64 y, real64 z);

extern void		nodedb_b_layer_pixel_write(NodeBitmap *node, NdbBLayer *layer, uint16 x, uint16 y, uint16 z, real64 pixel);

extern void *		nodedb_b_layer_access_begin(NodeBitmap *node, NdbBLayer *layer);
extern void		nodedb_b_layer_access_end(NodeBitmap *node, NdbBLayer *layer, void *framebuffer);

extern const void *	nodedb_b_layer_read_multi_begin(NodeBitmap *node, VNBLayerType format, va_list layers);
extern void		nodedb_b_layer_read_multi_end(NodeBitmap *node, const void *framebuffer);

extern void *		nodedb_b_layer_write_multi_begin(NodeBitmap *node, VNBLayerType format, va_list layers);
extern void		nodedb_b_layer_write_multi_end(NodeBitmap *node, void *framebuffer);

extern void		nodedb_b_layer_foreach_set(NodeBitmap *node, NdbBLayer *layer,
						real64 (*pixel)(uint32 x, uint32 y, uint32 z, void *user), void *user);

extern void		nodedb_b_tile_describe(const NodeBitmap *node, const NdbBLayer *layer, NdbBTileDesc *desc);

extern void		nodedb_b_register_callbacks(void);
