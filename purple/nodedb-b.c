*
 * Bitmap node databasing. Bitmaps are stored by allocating contiguous blocks of memory for
 * the required layers, nothing very fancy at all. One-bit-per-pixel layers are stored using
 * 8-bit bytes as the smallest unit of allocation, and never using any single byte for pixels
 * from two different rows, so a 10x10 layer requires 20 bytes.
 * 
 * Layers are always stored with space for a whole number of tiles horizontally, but the number
 * of scanlines is always the proper one (node->height), no rounding takes place there. This
 * means we can waste at most 7 pixels per scanline (for a REAL64 layer), i.e. 56 bytes.
*/

#include <stdio.h>
#include <string.h>

#include "verse.h"
#include "purple.h"

#include "dynarr.h"
#include "list.h"
#include "log.h"
#include "mem.h"
#include "strutil.h"
#include "textbuf.h"

#include "nodedb.h"
#include "nodedb-internal.h"

/* ----------------------------------------------------------------------------------------- */

/* Return size in *bits* for a pixel in a given layer type. */
static size_t pixel_size(VNBLayerType type)
{
	switch(type)
	{
	case VN_B_LAYER_UINT1:	return 1;
	case VN_B_LAYER_UINT8:	return 8;
	case VN_B_LAYER_UINT16:	return 16;
	case VN_B_LAYER_REAL32:	return 32;
	case VN_B_LAYER_REAL64:	return 64;
	}
	return 0;
}

static size_t tile_modulo(const NodeBitmap *node, const NdbBLayer *layer)
{
	static const size_t	mod[] = { 1, VN_B_TILE_SIZE, 2 * VN_B_TILE_SIZE, 4 * VN_B_TILE_SIZE, 8 * VN_B_TILE_SIZE };

	return mod[layer->type];
}

static size_t layer_modulo(const NodeBitmap *node, const NdbBLayer *layer)
{
	static const size_t	bpp[] = { 0, 1, 2, 4, 8 };
	size_t			wit;	/* Width in tiles, geddit? */

	wit = VN_B_TILE_SIZE * ((node->width + VN_B_TILE_SIZE - 1) / VN_B_TILE_SIZE);
	if(layer->type == VN_B_LAYER_UINT1)
		return wit / 8;		/* WARNING: Basically assumes that VN_B_TILE_SIZE % 8 == 0. */
	return wit * bpp[layer->type];
}

/* ----------------------------------------------------------------------------------------- */

void nodedb_b_construct(NodeBitmap *n)
{
	n->width  = n->height = n->depth = 0U;
	n->layers = NULL;
}

static void cb_copy_layer(void *d, const void *s, void *user)
{
	const NdbBLayer	*src = s;
	NdbBLayer	*dst = d;
	const NodeBitmap*node = user;
	size_t		ps = pixel_size(src->type), layer_size;

	dst->id = src->id;
	strcpy(dst->name, src->name);
	dst->type = src->type;

	layer_size = (node->width * ps + 7 / 8) * node->height * node->depth;
	if((dst->framebuffer = mem_alloc(layer_size)) != NULL)
		memcpy(dst->framebuffer, src->framebuffer, layer_size);
}

void nodedb_b_copy(NodeBitmap *n, const NodeBitmap *src)
{
	n->width  = src->width;
	n->height = src->height;
	n->depth  = src->depth;

	if(src->layers != NULL)
		n->layers = dynarr_new_copy(src->layers, cb_copy_layer, n);
}

void nodedb_b_destruct(NodeBitmap *n)
{
	if(n->layers != NULL)
	{
		unsigned int	i;
		NdbBLayer	*layer;

		for(i = 0; i < dynarr_size(n->layers); i++)
		{
			if((layer = dynarr_index(n->layers, i)) == NULL || layer->name[0] == '\0')
				continue;
			mem_free(layer->framebuffer);
		}
		dynarr_destroy(n->layers);
	}
}

/* ----------------------------------------------------------------------------------------- */

int nodedb_b_dimensions_set(NodeBitmap *node, uint16 width, uint16 height, uint16 depth)
{
	NdbBLayer	*layer;
	size_t		i, y, z, ps, layer_size, dss, sss;
	unsigned char	*fb;

	if(width == node->width && height == node->height && depth == node->depth)
		return 0;
	printf("setting dimensions to %ux%ux%u\n", width, height, depth);
	/* Resize all layers. Heavy lifting. */
	for(i = 0; i < dynarr_size(node->layers); i++)
	{
		if((layer = dynarr_index(node->layers, i)) == NULL || layer->name[0] == '\0')
			continue;
		if(layer->framebuffer == NULL)				/* Don't copy what's not there. */
			continue;
		ps = pixel_size(layer->type);
		layer_size = ((width * ps + 7) / 8) * height * depth;	/* Convert to whole bytes, for uint1. */
		fb = mem_alloc(layer_size);
		if(fb == NULL)
		{
			LOG_WARN(("Couldn't allocate new framebuffer for layer %u.%u (%s)--out of memory",
				  node->node.id, layer->id, layer->name));
			continue;
		}
		/* Copy scanlines from old buffer into new. */
		if(layer->type == VN_B_LAYER_UINT1)
		{
			sss = (node->width * ps + 7) / 8;	/* Round sizes up to whole bytes. */
			dss = (width * ps + 7) / 8;
		}
		else
		{
			sss = node->width * ps / 8;
			dss = width * ps / 8;
		}
		for(z = 0; z < depth; z++)
		{
			for(y = 0; y < height; y++)
				memcpy(fb + z * (dss * height) + y * dss,
				       layer->framebuffer + z * sss * height + y * sss, dss);
		}
		mem_free(layer->framebuffer);
		layer->framebuffer = fb;
	}
	node->width  = width;
	node->height = height;
	node->depth  = depth;
	return 1;
}

static void cb_def_layer(unsigned int index, void *element, void *user)
{
	NdbBLayer	*layer = element;

	layer->name[0] = '\0';
	layer->framebuffer = NULL;
}

NdbBLayer * nodedb_b_layer_create(NodeBitmap *node, VLayerID layer_id, const char *name, VNBLayerType type)
{
	NdbBLayer	*layer;

	if(node == NULL || name == NULL)
		return NULL;
	if(node->layers == NULL)
	{
		node->layers = dynarr_new(sizeof *layer, 4);
		dynarr_set_default_func(node->layers, cb_def_layer, NULL);
	}
	if(layer_id == (VLayerID) ~0)
		layer = dynarr_append(node->layers, NULL, NULL);
	else
		layer = dynarr_set(node->layers, layer_id, NULL);
	if(layer != NULL)
	{
		layer->id   = layer_id;
		stu_strncpy(layer->name, sizeof layer->name, name);
		layer->type = type;
		layer->framebuffer = NULL;
	}
	return layer;
}

NdbBLayer * nodedb_b_layer_lookup(const NodeBitmap *node, const char *name)
{
	size_t		i;
	NdbBLayer	*layer;

	if(node == NULL || name == NULL)
		return NULL;
	for(i = 0; (layer = dynarr_index(node->layers, i)) != NULL; i++)
	{
		if(strcmp(layer->name, name) == 0)
			return layer;
	}
	return NULL;
}

void * nodedb_b_layer_access_begin(NodeBitmap *node, NdbBLayer *layer)
{
	if(node == NULL || layer == NULL)
		return NULL;
	if(layer->framebuffer == NULL)
	{
		size_t	ps, layer_size;

		ps = pixel_size(layer->type);
		layer_size = ((node->width * ps + 7) / 8) * node->height * node->depth;
		layer->framebuffer = mem_alloc(layer_size);
	}
	return layer->framebuffer;
}

void nodedb_b_layer_access_end(NodeBitmap *node, NdbBLayer *layer, void *framebuffer)
{
	/* Nothing much to do, here. */
}

void nodedb_b_layer_foreach_set(NodeBitmap *node, NdbBLayer *layer,
				real64 (*pixel)(uint32 x, uint32 y, uint32 z, void *user), void *user)
{
	uint8	*frame;

	if(node == NULL || layer == NULL || pixel == NULL)
		return;
	if((frame = nodedb_b_layer_access_begin(node, layer)) != NULL)
	{
		uint32	z, y, x;
		uint8	*put8, px, shift;
		uint16	*put16;
		real32	*put32;
		real64	*put64;

		for(z = 0; z < node->depth; z++)
		{
			for(y = 0; y < node->height; y++)
			{
				switch(layer->type)
				{
				case VN_B_LAYER_UINT1:
					put8 = frame;
					for(x = 0, px = 0; x < node->width; x++)
					{
						if(x > 0 && (x % 8) == 0)
						{
/*							printf(" x=%u, writing %02X\n", x, px);*/
							*put8++ = px;
							px = 0;
						}
						px <<= 1;
						px |= pixel(x, y, z, user) >= 0.5;
					}
					shift = (8 - (x % 8)) % 8;
/*					printf("scanline done, x=%u, writing %02X << %u = %02X\n", x, px, shift, px << shift);*/
					*put8++ = px << shift;
					frame += (node->width + 7) / 8;
					break;
				case VN_B_LAYER_UINT8:
					put8 = frame;
					for(x = 0; x < node->width; x++)
						*put8++ = 255.0 * pixel(x, y, z, user);
					frame += node->width;
					break;
				case VN_B_LAYER_UINT16:
					put16 = (uint16 *) frame;
					for(x = 0; x < node->width; x++)
						*put16++ = 65535.0 * pixel(x, y, z, user);
					frame += 2 * node->width;
					break;
				case VN_B_LAYER_REAL32:	
					put32 = (real32 *) frame;
					for(x = 0; x < node->width; x++)
						*put32++ = pixel(x, y, z, user);
					frame += 4 * node->width;
					break;
				case VN_B_LAYER_REAL64:
					put64 = (real64 *) frame;
					for(x = 0; x < node->width; x++)
						*put64++ = pixel(x, y, z, user);
					frame += 8 * node->width;
					break;
				}
			}
		}
		nodedb_b_layer_access_end(node, layer, frame);
	}
}

void * nodedb_b_layer_tile_find(const NodeBitmap *node, const NdbBLayer *layer,
			     uint16 tile_x, uint16 tile_y, uint16 tile_z)
{
	if(layer->framebuffer != NULL)
	{
		size_t	ps = pixel_size(layer->type),
			mod = layer_modulo(node, layer),
			size = (node->width * ps + 7 / 8) * node->height;
		return layer->framebuffer + tile_y * VN_B_TILE_SIZE * mod +
			(tile_x * VN_B_TILE_SIZE * ps + 7) / 8 + size * tile_z;
	}
	return NULL;
}

void nodedb_b_tile_describe(const NodeBitmap *node, const NdbBLayer *layer, NdbBTileDesc *desc)
{
	desc->out.mod_row  = layer_modulo(node, layer);
	desc->out.mod_tile = tile_modulo(node, layer);
	desc->out.height   = (desc->in.y * VN_B_TILE_SIZE + 3 >= node->height) ?
				node->height % VN_B_TILE_SIZE : VN_B_TILE_SIZE;
	desc->out.ptr = nodedb_b_layer_tile_find(node, layer, desc->in.x, desc->in.y, desc->in.z);
}

/* ----------------------------------------------------------------------------------------- */

static void cb_b_dimensions_set(void *user, VNodeID node_id, uint16 width, uint16 height, uint16 depth)
{
	NodeBitmap	*node;

	if((node = (NodeBitmap *) nodedb_lookup_with_type(node_id, V_NT_BITMAP)) == NULL)
		return;
	if(nodedb_b_dimensions_set(node, width, height, depth))
		NOTIFY(node, STRUCTURE);
}

static void cb_b_layer_create(void *user, VNodeID node_id, VLayerID layer_id, const char *name, VNBLayerType type)
{
	NodeBitmap	*node;
	NdbBLayer	*layer;

	printf("now in cb_layer_create, node_id=%u layer_id=%u (%s)\n", node_id, layer_id, name);
	if((node = (NodeBitmap *) nodedb_lookup_with_type(node_id, V_NT_BITMAP)) == NULL)
	{
		printf(" node not found in database\n");
		return;
	}
	if((layer = dynarr_index(node->layers, layer_id)) != NULL && layer->name[0] != '\0' && strcmp(layer->name, name) != 0)
	{
		LOG_WARN(("Layer already exists--unhandled case"));
		return;
	}
	if((layer = nodedb_b_layer_create(node, layer_id, name, type)) != NULL)
	{
		verse_send_b_layer_subscribe(node_id, layer_id, 0);
		NOTIFY(node, STRUCTURE);
	}	
}

static void cb_b_layer_destroy(void *user, VNodeID node_id, VLayerID layer_id)
{
	NodeBitmap	*node;
	NdbBLayer	*layer;

	if((node = (NodeBitmap *) nodedb_lookup_with_type(node_id, V_NT_BITMAP)) == NULL)
		return;
	if((layer = dynarr_index(node->layers, layer_id)) == NULL || layer->name[0] == '\0')
		return;
	layer->name[0] = '\0';
	layer->type = -1;
	mem_free(layer->framebuffer);
	NOTIFY(node, STRUCTURE);
}

static void cb_b_tile_set(void *user, VNodeID node_id, VLayerID layer_id, uint16 tile_x, uint16 tile_y, uint16 tile_z,
			  VNBLayerType type, const VNBTile *tile)
{
	NodeBitmap	*node;
	NdbBLayer	*layer;
	size_t		ps, th, y, mod_src, mod_dst;
	const uint8	*get;
	uint8		*put;

/*	printf("got tile_set in %u.%u.(%u,%u,%u)\n", node_id, layer_id, tile_x, tile_y, tile_z);*/
	if((node = (NodeBitmap *) nodedb_lookup_with_type(node_id, V_NT_BITMAP)) == NULL)
		return;
	if((layer = dynarr_index(node->layers, layer_id)) == NULL || layer->name[0] == '\0')
		return;
	if(layer->type != type)
	{
		LOG_WARN(("Received type %d data for type %d layer--ignoring", type, layer->type));
		return;
	}
	ps = pixel_size(layer->type);
	if(layer->framebuffer == NULL)
	{
		size_t	layer_size = layer_modulo(node, layer) * node->height * node->depth;
		layer->framebuffer = mem_alloc(layer_size);
		memset(layer->framebuffer, layer_size, 0);
	}
	if(layer->framebuffer == NULL)
	{
		LOG_WARN(("No framebuffer in layer %u (%s)--out of memory?", layer->id, layer->name));
		return;
	}

	th = (tile_y * VN_B_TILE_SIZE + 3 >= node->height) ?
			node->height % VN_B_TILE_SIZE : VN_B_TILE_SIZE;	/* Clamp tile height against node. */
	get = (uint8 *) tile;		/* It's a union. */
	put = nodedb_b_layer_tile_find(node, layer, tile_x, tile_y, tile_z);
	mod_src = tile_modulo(node, layer);
	mod_dst = layer_modulo(node, layer);
	for(y = 0; y < th; y++, get += mod_src, put += mod_dst)
		memcpy(put, get, mod_src);
	NOTIFY(node, DATA);

/*	if(layer->type == VN_B_LAYER_UINT1)
	{
		const uint8	*ptr = layer->framebuffer;
		uint32		x, y;

		for(y = 0; y < node->height; y++)
		{
			for(x = 0; x < mod_dst; x++)
				printf(" %02X", *ptr++);
			printf("\n");
		}
	}
	else if(layer->type == VN_B_LAYER_REAL64)
	{
		const real64	*ptr = layer->framebuffer;
		uint32		x, y;

		for(y = 0; y < node->height; y++)
		{
			for(x = 0; x < node->width; x++)
				printf(" %g", *ptr++);
			printf("\n");
		}
	}
*/
}

/* ----------------------------------------------------------------------------------------- */

void nodedb_b_register_callbacks(void)
{
	verse_callback_set(verse_send_b_dimensions_set,	cb_b_dimensions_set, NULL);
	verse_callback_set(verse_send_b_layer_create,	cb_b_layer_create, NULL);
	verse_callback_set(verse_send_b_tile_set,	cb_b_tile_set, NULL);
	verse_callback_set(verse_send_b_layer_destroy,	cb_b_layer_destroy, NULL);
}
