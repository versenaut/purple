/*
 * Bitmap node databasing. Bitmaps are stored by allocating contiguous blocks of memory for
 * the required layers, nothing very fancy at all. One-bit-per-pixel layers are stored using
 * 8-bit bytes as the smallest unit of allocation, and never using any single byte for pixels
 * from two different rows, so a 10x10 layer requires 20 bytes.
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

static void cb_b_dimensions_set(void *user, VNodeID node_id, uint16 width, uint16 height, uint16 depth)
{
	NodeBitmap	*node;
	NdbBLayer	*layer;
	size_t		i, y, z, ps, layer_size, dss, sss;
	unsigned char	*fb;

	if((node = (NodeBitmap *) nodedb_lookup_with_type(node_id, V_NT_BITMAP)) == NULL)
		return;
	if(width == node->width && height == node->height && depth == node->depth)
		return;
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
				  node_id, layer->id, layer->name));
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
}

static void cb_b_layer_create(void *user, VNodeID node_id, VLayerID layer_id, const char *name, VNBLayerType type)
{
	NodeBitmap	*node;
	NdbBLayer	*layer;

	if((node = (NodeBitmap *) nodedb_lookup_with_type(node_id, V_NT_BITMAP)) == NULL)
		return;
	if((layer = dynarr_index(node->layers, layer_id)) != NULL && strcmp(layer->name, name) != 0)
	{
		LOG_WARN(("Layer already exists--unhandled case"));
		return;
	}
	if(node->layers == NULL)
		node->layers = dynarr_new(sizeof *layer, 4);
	if((layer = dynarr_set(node->layers, layer_id, NULL)) != NULL)
	{
		layer->id   = layer_id;
		stu_strncpy(layer->name, sizeof layer->name, name);
		layer->type = type;
		layer->framebuffer = NULL;
		verse_send_b_layer_subscribe(node_id, layer_id, 0);
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
}

static void cb_b_tile_set(void *user, VNodeID node_id, VLayerID layer_id, uint16 tile_x, uint16 tile_y, uint16 tile_z,
			  VNBLayerType type, const VNBTile *tile)
{
	NodeBitmap	*node;
	NdbBLayer	*layer;
	size_t		ps;

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
		size_t	layer_size = ((node->width * ps + 7) / 8) * node->height * node->depth;

		layer->framebuffer = mem_alloc(layer_size);
		memset(layer->framebuffer, layer_size, 0);
	}
	if(layer->framebuffer == NULL)
	{
		LOG_WARN(("No framebuffer in layer %u (%s)--out of memory?", layer->id, layer->name));
		return;
	}
	if(type == VN_B_LAYER_UINT1)	/* Copying a 1bpp tile involves nibble operations. Yummy. */
	{
		size_t	line_size = (node->width * ps + 7) / 8,
			z_size = line_size * node->height;
		uint8	*put = layer->framebuffer + tile_z * z_size + tile_y * 4 * line_size + tile_x / 2, nibble, y, th;
/*		printf("line_size is %u, z_size is %u\n", line_size, z_size);
		printf("put of tile (%u,%u,%u), %04x, at %u bytes into frame\n", tile_x, tile_y, tile_z, tile->vuint1,
		       (uint8 *) put - (uint8 *) layer->framebuffer);
*/		th = (tile_y * 4 + 3 >= node->height) ? node->height % 4 : 4;	/* Clamp tile height against node. */
		for(y = 0; y < th; y++, put += line_size)
		{
			nibble = (tile->vuint1 >> (12 - 4 * y)) & 0xf;
			if(!(tile_x & 1))	/* Left? (High) */
			{
				*put &= 0x0f;
				*put |= nibble << 4;
			}
			else			/* Right? (Low) */
			{
				*put &= 0xf0;
				*put |= nibble;
			}
		}

/*		{
			const uint8	*get = layer->framebuffer;
			int		x, y;

			printf("Bitmap now:\n");
			for(y = 0; y < node->height; y++)
			{
				for(x = 0; x < (node->width + 7) / 8; x++)
					printf("%02X", *get++);
				printf("\n");
			}
		}
*/	}
	else
	{
		uint8	*put, y, th;

		ps /= 8;	/* We know the size is a whole number of bytes. */
		put = layer->framebuffer + 4 * ps * tile_x +
				4 * ps * node->width * tile_y +
				ps * node->width * node->height * tile_z;
		th = (tile_y * 4 + 3 >= node->height) ? node->height % 4 : 4;	/* Clamp tile height against node. */
		for(y = 0; y < th; y++)
		{
			switch(type)
			{
			case VN_B_LAYER_UINT8:
				memcpy(put, tile->vuint8 + 4 * y, 4 * ps);
				break;
			case VN_B_LAYER_UINT16:
				memcpy(put, tile->vuint16 + 4 * y, 4 * ps);
				break;
			case VN_B_LAYER_REAL32:
				memcpy(put, tile->vreal32 + 4 * y, 4 * ps);
				break;
			case VN_B_LAYER_REAL64:
				memcpy(put, tile->vreal64 + 4 * y, 4 * ps);
				break;
			default:
				;
			}
			put += 4 * ps;
		}
	}
}

/* ----------------------------------------------------------------------------------------- */

void nodedb_b_register_callbacks(void)
{
	verse_callback_set(verse_send_b_dimensions_set,	cb_b_dimensions_set, NULL);
	verse_callback_set(verse_send_b_layer_create,	cb_b_layer_create, NULL);
	verse_callback_set(verse_send_b_tile_set,	cb_b_tile_set, NULL);
	verse_callback_set(verse_send_b_layer_destroy,	cb_b_layer_destroy, NULL);
}
