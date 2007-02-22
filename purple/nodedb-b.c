/*
 * nodedb-b.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Bitmap node databasing. Bitmaps are stored by allocating separate contigious blocks of memory
 * for each required layer, nothing very fancy at all. One-bit-per-pixel layers are stored using
 * 8-bit bytes as the smallest unit of allocation, and never using any single byte for pixels
 * from two different rows, so a 10x10 layer requires 20 bytes.
 * 
 * Layers are stored with space for only the required number of pixels, which means that for
 * many widths, the final tile will be truncated.
*/

#include <stdarg.h>
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

#if defined _WIN32
# define va_copy(ap1, ap2) (*(ap1) = *(ap2))
#endif

/* ----------------------------------------------------------------------------------------- */

#define	MIN(a,b)	(((a) < (b)) ? (a) : (b))	/* Handy in tile width computations. */

static real64	layer_get_pixel(const NodeBitmap *node, const NdbBLayer *layer, const unsigned char *framebuffer, int x, int y, int z);

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

static size_t tile_modulo(UNUSED(const NodeBitmap *node), const NdbBLayer *layer)
{
	static const size_t	mod[] = { 1, VN_B_TILE_SIZE, 2 * VN_B_TILE_SIZE, 4 * VN_B_TILE_SIZE, 8 * VN_B_TILE_SIZE };

	return mod[layer->type];
}

/* Compute width of tile in *bytes*, for copying. */
static size_t tile_width(const NodeBitmap *node, const NdbBLayer *layer, uint16 tile_x)
{
	size_t	tw = MIN(VN_B_TILE_SIZE, node->width - tile_x * VN_B_TILE_SIZE);

	tw *= pixel_size(layer->type);
	tw /= 8;
	return tw > 0 ? tw : 1;
}

static size_t layer_modulo(const NodeBitmap *node, const NdbBLayer *layer)
{
	static const size_t	bpp[] = { 0, 1, 2, 4, 8 };

	if(layer->type == VN_B_LAYER_UINT1)
		return (node->width + 7) / 8;
	return node->width * bpp[layer->type];
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

	layer_size = ((node->width * ps + 7) / 8) * node->height * node->depth;
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

void nodedb_b_set(NodeBitmap *n, const NodeBitmap *src)
{
	nodedb_b_destruct(n);
	nodedb_b_copy(n, src);
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
		n->layers = NULL;
	}
}

/* ----------------------------------------------------------------------------------------- */

int nodedb_b_set_dimensions(NodeBitmap *node, uint16 width, uint16 height, uint16 depth)
{
	NdbBLayer	*layer;
	size_t		i, y, z, ps, layer_size, dss, sss, cw, ch;
	unsigned char	*fb;

	if(width == node->width && height == node->height && depth == node->depth)
		return 0;
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
		memset(fb, 0, layer_size);
		if(fb == NULL)
		{
			LOG_WARN(("Couldn't allocate new framebuffer for layer %u.%u (%s)--out of memory",
				  node->node.id, layer->id, layer->name));
			continue;
		}
		/* Copy scanlines from old buffer into new. */
		cw = MIN(node->width, width);
		ch = MIN(node->height, height);
		if(layer->type == VN_B_LAYER_UINT1)
		{
			sss = (node->width * ps + 7) / 8;	/* Round sizes up to whole bytes. */
			dss = (cw * ps + 7) / 8;
		}
		else
		{
			sss = node->width * ps / 8;
			dss = cw * ps / 8;
		}
/*		printf("node is resizing from %ux%ux%u to %ux%ux%u -- copying\n",
		       node->width, node->height, node->depth,
		       width, height, depth);
		printf(" common width is %u, common height is %u, source scanline size=%u dest scanline size=%u\n", cw, ch, sss, dss);
*/		for(z = 0; z < depth; z++)
		{
			for(y = 0; y < ch; y++)
				memcpy(fb + z * (dss * height) + y * dss,
				       (char *) layer->framebuffer + z * sss * height + y * sss, dss);
		}
		mem_free(layer->framebuffer);
		layer->framebuffer = fb;
	}
	node->width  = width;
	node->height = height;
	node->depth  = depth;
	return 1;
}

void nodedb_b_get_dimensions(const NodeBitmap *node, uint16 *width, uint16 *height, uint16 *depth)
{
	if(node == NULL)
		return;
	if(width != NULL)
		*width = node->width;
	if(height != NULL)
		*height = node->height;
	if(depth != NULL)
		*depth = node->depth;
}

/* ----------------------------------------------------------------------------------------- */

unsigned int nodedb_b_layer_num(const NodeBitmap *node)
{
	unsigned int	i, num;
	const NdbBLayer	*layer;

	if(node == NULL)
		return 0;
	for(i = num = 0; (layer = dynarr_index(node->layers, i)) != NULL; i++)
	{
		if(layer->name[0] == '\0')
			continue;
		num++;
	}
	return num;
}

NdbBLayer * nodedb_b_layer_nth(const NodeBitmap *node, unsigned int n)
{
	unsigned int	i;
	NdbBLayer	*layer;

	if(node == NULL)
		return NULL;
	for(i = 0; (layer = dynarr_index(node->layers, i)) != NULL; i++)
	{
		if(layer->name[0] == '\0')
			continue;
		if(n == 0)
			return layer;
		n--;
	}
	return NULL;
}

NdbBLayer * nodedb_b_layer_find(const NodeBitmap *node, const char *name)
{
	size_t		i;
	NdbBLayer	*layer;

	if(node == NULL || name == NULL || *name == '\0')
		return NULL;
	for(i = 0; (layer = dynarr_index(node->layers, i)) != NULL; i++)
	{
		if(layer->name[0] == '\0')
			continue;
		if(strcmp(layer->name, name) == 0)
			return layer;
	}
	return NULL;
}

static void cb_def_layer(UNUSED(unsigned int index), void *element, UNUSED(void *user))
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

real64 nodedb_b_layer_pixel_read(const NodeBitmap *node, const NdbBLayer *layer, real64 x, real64 y, real64 z)
{
	uint32	ix = x, iy = y, iz = z;
	if(node == NULL || layer == NULL || layer->framebuffer == NULL || ix >= node->width || iy >= node->height || z >= node->depth)
		return 0.0;
	return layer_get_pixel(node, layer, layer->framebuffer, ix, iy, iz);
}

real64 nodedb_b_layer_pixel_read_filtered(const NodeBitmap *node, const NdbBLayer *layer, UNUSED(NdbBFilterMode mode), real64 x, real64 y, real64 z)
{
	if(node == NULL || layer == NULL)
		return 0.0;
	x *= node->width;
	y *= node->height;
	z *= node->depth;
	if(x < 0 || y < 0 || z < 0 || x >= node->width || y >= node->height || z >= node->depth)
		return 0.0;
	return layer_get_pixel(node, layer, layer->framebuffer, x, y, z);
}

void nodedb_b_layer_pixel_write(NodeBitmap *node, NdbBLayer *layer, uint16 x, uint16 y, uint16 z, real64 pixel)
{
	void	*fb;
	uint32	rw;

	if(node == NULL || layer == NULL || x >= node->width || y >= node->height || z >= node->depth)
		return;
	if((fb = nodedb_b_layer_access_begin(node, layer)) != NULL)
	{
		switch(layer->type)
		{
		case VN_B_LAYER_UINT1:
			rw = ((node->width + 7) / 8);
			if(pixel <= 0)
				((uint8 *) fb)[z * rw * node->height + y * rw + x / 8] &= ~(1 << (7 - (x % 8)));
			else
				((uint8 *) fb)[z * rw * node->height + y * rw + x / 8] |= 1 << (7 - (x % 8));
			break;
		case VN_B_LAYER_UINT8:
			if(pixel < 0)		/* Clamp 8-bit pixels. */
				pixel = 0.0;
			else if(pixel > 1.0)
				pixel = 1.0;
			((uint8 *) fb)[z * node->width * node->height + y * node->width + x] = 255.0 * pixel;
			break;
		case VN_B_LAYER_UINT16:
			if(pixel < 0)		/* Clamp 16-bit pixels. */
				pixel = 0.0;
			else if(pixel > 1.0)
				pixel = 1.0;
			((uint16 *) fb)[z * node->width * node->height + y * node->width + x] = 65535.0 * pixel;
			break;
		case VN_B_LAYER_REAL32:
			((real32 *) fb)[z * node->width * node->height + y * node->width + x] = pixel;
			break;
		case VN_B_LAYER_REAL64:
			((real64 *) fb)[z * node->width * node->height + y * node->width + x] = pixel;
			break;
		}
		nodedb_b_layer_access_end(node, layer, fb);
	}
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

void nodedb_b_layer_access_end(UNUSED(NodeBitmap *node), UNUSED(NdbBLayer *layer), UNUSED(void *framebuffer))
{
	/* Nothing much to do, here. */
}

/* ----------------------------------------------------------------------------------------- */

/* Information used to keep track of a multi-layer framebuffer. */
struct multi_info
{
	unsigned char	*fb;			/* Points at first byte past structure, framebuffer base. */
	unsigned char	*put;			/* Used when reading and writing, loops over scanlines. */
	size_t		mask;			/* Used for 1bpp layer access. */
	VNBLayerType	format;			/* Format of each layer in this multi-layer buffer. */
	NodeBitmap	*node;			/* Source node for the layers. */
	size_t		num;			/* Number of layers in buffer, at least 1. */
	NdbBLayer	*layer[16];		/* Source layer pointers. */
	void		*access[16];		/* Source layer framebuffer pointers. */
	size_t		row_size, sheet_size;	/* Handy sizes. */
	int		y, z;			/* Current scanline coordinates. */
	/* Framebuffer for multi-layer interleaved image goes here. */
};

/* Prepare to process a scanline of a multi-layer framebuffer. */
static void multi_scanline_init(struct multi_info *mi, int y, int z)
{
	mi->put = mi->fb + z * mi->sheet_size + y * mi->row_size;
/*	printf("z=%u row %u begins at %p (offset %u, row=%u)\n", z, y, mi->put, mi->put - mi->fb, mi->row_size);*/
	mi->mask = 0x80;		/* Scanlines always begin at even byte boundaries. */
	mi->y = y;
	mi->z = z;
}

/* This slows things down, but hey. */
static real64 layer_get_pixel(const NodeBitmap *node, const NdbBLayer *layer, const unsigned char *framebuffer, int x, int y, int z)
{
	if(layer->type == VN_B_LAYER_UINT1)
	{
		size_t	row = (node->width + 7) / 8, off;

		off = z * row * node->height + y * row + x / 8;
		return ((uint8 *) framebuffer)[off] & (128 >> (x % 8)) ? 1.0 : 0.0;
	}
	else if(layer->type == VN_B_LAYER_UINT8)
		return (real64) ((uint8 *) framebuffer)[z * node->width * node->height + y * node->width + x] / 255.0;
	else if(layer->type == VN_B_LAYER_UINT16)
		return (real64) ((uint16 *) framebuffer)[z * node->width * node->height + y * node->width + x] / 65535.0;
	else if(layer->type == VN_B_LAYER_REAL32)
		return ((real32 *) framebuffer)[z * node->width * node->height + y * node->width + x];
	else if(layer->type == VN_B_LAYER_REAL64)
		return ((real64 *) framebuffer)[z * node->width * node->height + y * node->width + x];
	return 0.0;
}

/* Write <pixel> to <layer>'s <framebuffer> at (x,y,z), converting it to the layer's type as needed. */
static void layer_put_pixel(const NodeBitmap *node, NdbBLayer *layer, unsigned char *framebuffer,
			    int x, int y, int z, real64 pixel)
{
	if(layer->type == VN_B_LAYER_UINT1)
	{
		size_t	row = (node->width + 7) / 8, off;
		uint8	mask;

		off  = z * row * node->height + y * row + x / 8;
		mask = 128 >> (x % 8);
		if(pixel > 0.0)
			framebuffer[off] |= mask;
		else
			framebuffer[off] &= ~mask;
	}
	else if(layer->type == VN_B_LAYER_UINT8)
		((uint8 *) framebuffer)[z * node->width * node->height + y * node->width + x] = pixel * 255.0;
	else if(layer->type == VN_B_LAYER_UINT16)
		((uint16 *) framebuffer)[z * node->width * node->height + y * node->width + x] = pixel * 65535.0;
	else if(layer->type == VN_B_LAYER_REAL32)
		((real32 *) framebuffer)[z * node->width * node->height + y * node->width + x] = pixel;
	else if(layer->type == VN_B_LAYER_REAL64)
		((real64 *) framebuffer)[z * node->width * node->height + y * node->width + x] = pixel;
}

/* Initialize multi framebuffer by reading out and writing pixel <x> from the current scanline in all layers. */
static void multi_put_pixel(struct multi_info *mi, int x)
{
	unsigned  int	i;
	real64		pix;

	if(mi->format == VN_B_LAYER_UINT1)
	{
		for(i = 0; i < mi->num; i++)
		{
			if(mi->mask == 0)
			{
				mi->put++;
				mi->mask = 0x80;
			}
			pix = layer_get_pixel(mi->node, mi->layer[i], mi->access[i], x, mi->y, mi->z);
			*mi->put &= ~mi->mask;
			if(pix > 0.0)
				*mi->put |= mi->mask;
			mi->mask >>= 1;
		}
	}
	else
	{
		for(i = 0; i < mi->num; i++)
		{
			pix = layer_get_pixel(mi->node, mi->layer[i], mi->access[i], x, mi->y, mi->z);
			switch(mi->format)
			{
			case VN_B_LAYER_UINT8:
				*mi->put++ = pix * 255.0;
				break;
			case VN_B_LAYER_UINT16:
				{
					uint16	*p = (uint16 *) mi->put;
					*p++ = pix;
					mi->put = (unsigned char *) p;
				}
				break;
			case VN_B_LAYER_REAL32:
				{
					real32	*p = (real32 *) mi->put;
					*p++ = pix;
					mi->put = (unsigned char *) p;
				}
				break;
			case VN_B_LAYER_REAL64:
				{
					real64	*p = (real64 *) mi->put;
					*p++ = pix;
					mi->put = (unsigned char *) p;
				}
				break;
			default:;
			}
		}
	}
}

const void * nodedb_b_layer_read_multi_begin(NodeBitmap *node, VNBLayerType format, va_list layers)
{
	const size_t		mul[] = { 1,  1,  2,  4,  8 };
	size_t			num = 0, x, y, z, row_size, sheet_size;
	const char		*name;
	va_list			copy;
	struct multi_info	*mi;
	NdbBLayer		*layer[sizeof mi->layer / sizeof *mi->layer];

	va_copy(copy, layers);
	for(num = 0; ((name = va_arg(copy, const char *)) != NULL); num++)
	{
		if((layer[num] = nodedb_b_layer_find(node, name)) == NULL)
			break;
	}
	va_end(copy);
/*	printf("counted layers, num=%u last=%p\n", num, name);*/
	if(name != NULL)		/* Lookup failed? Then we fail, for now. */
	{
		printf(" multi_begin(): couldn't lookup layer '%s', aborting\n", name);
		return NULL;
	}
	if(num > sizeof layer / sizeof *layer)
	{
		LOG_ERR(("Can't multi-access more than %u layers", sizeof mi->layer / sizeof *mi->layer));
		return NULL;
	}

	row_size = num * node->width;
	row_size *= mul[format];
	if(format == VN_B_LAYER_UINT1)
	{
		row_size += 7;
		row_size /= 8;
	}
	sheet_size = row_size * node->height;
/*	printf("row size: %u, sheet size: %u\n", row_size, sheet_size);*/
	if((mi = mem_alloc(sizeof *mi + sheet_size * node->depth)) == NULL)
		return NULL;
	mi->fb = (unsigned char *) (mi + 1);
	mi->put = NULL;
	mi->format = format;
	mi->node = node;
	mi->num = num;
	for(x = 0; x < num; x++)
	{
		mi->layer[x] = layer[x];
		mi->access[x] = nodedb_b_layer_access_begin(node, mi->layer[x]);
	}
	mi->row_size = row_size;
	mi->sheet_size = sheet_size;
	for(z = 0; z < node->depth; z++)
	{
		for(y = 0; y < node->height; y++)
		{
			multi_scanline_init(mi, y, z);
			for(x = 0; x < node->width; x++)
				multi_put_pixel(mi, x);
		}
	}
	return mi->fb;
}

void * nodedb_b_layer_write_multi_begin(NodeBitmap *node, VNBLayerType format, va_list layers)
{
	return (void *) nodedb_b_layer_read_multi_begin(node, format, layers);
}

void nodedb_b_layer_read_multi_end(NodeBitmap *node, const void *framebuffer)
{
	struct multi_info	*mi = (struct multi_info *) ((char *) framebuffer - (sizeof *mi));
	size_t			i;

	/* Stop accessing layers. */
	for(i = 0; i < mi->num; i++)
		nodedb_b_layer_access_end(node, mi->layer[i], mi->access[i]);
	mem_free(mi);
}

void nodedb_b_layer_write_multi_end(NodeBitmap *node, void *framebuffer)
{
	struct multi_info	*mi = (struct multi_info *) ((char *) framebuffer - (sizeof *mi));
	size_t			i, x, y, z, off;
	real64			pixel;

	/* Write contents back to individual layers. Lots of work. */
	for(i = 0; i < mi->num; i++)
	{
		for(z = 0; z < node->depth; z++)
		{
			for(y = 0; y < node->height; y++)
			{
				multi_scanline_init(mi, y, z);
				for(x = 0; x < node->width; x++)
				{
					off = mi->num * x + i;

					/* Read out pixel from multi-buffer. */
					if(mi->format == VN_B_LAYER_UINT1)
						pixel = (mi->put[off / 8] & (128 >> (off % x))) ? 1.0 : 0.0;
					else if(mi->format == VN_B_LAYER_UINT8)
						pixel = mi->put[off] / 255.0;
					else if(mi->format == VN_B_LAYER_UINT16)
						pixel = ((const uint16 *) mi->put)[off];
					else if(mi->format == VN_B_LAYER_REAL32)
						pixel = ((const real32 *) mi->put)[off];
					else if(mi->format == VN_B_LAYER_REAL64)
						pixel = ((const real64 *) mi->put)[off];
					/* Write pixel into source layer. */
					layer_put_pixel(mi->node, mi->layer[i], mi->access[i], x, y, z, pixel);
				}
			}
		}
	}
	nodedb_b_layer_read_multi_end(node, framebuffer);
}

/* ----------------------------------------------------------------------------------------- */

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
		return (char *) layer->framebuffer + tile_y * VN_B_TILE_SIZE * mod +
			(tile_x * VN_B_TILE_SIZE * ps + 7) / 8 + size * tile_z;
	}
	return NULL;
}

void nodedb_b_tile_describe(const NodeBitmap *node, const NdbBLayer *layer, NdbBTileDesc *desc)
{
	desc->out.mod_row  = layer_modulo(node, layer);
	desc->out.mod_tile = tile_modulo(node, layer);
	desc->out.width    = MIN(VN_B_TILE_SIZE, node->width - desc->in.x * VN_B_TILE_SIZE);
	if(layer->type == VN_B_LAYER_UINT1)
		desc->out.width_bytes = (desc->out.width + 7) / 8;
	else
		desc->out.width_bytes = (desc->out.width * pixel_size(layer->type)) / 8;
	desc->out.height   = (desc->in.y * VN_B_TILE_SIZE + 7 >= node->height) ?
				node->height % VN_B_TILE_SIZE : VN_B_TILE_SIZE;
	desc->out.ptr = nodedb_b_layer_tile_find(node, layer, desc->in.x, desc->in.y, desc->in.z);
}

/* ----------------------------------------------------------------------------------------- */

static void cb_b_dimensions_set(UNUSED(void *user), VNodeID node_id, uint16 width, uint16 height, uint16 depth)
{
	NodeBitmap	*node;

	if((node = (NodeBitmap *) nodedb_lookup_with_type(node_id, V_NT_BITMAP)) == NULL)
		return;
	if(nodedb_b_set_dimensions(node, width, height, depth))
		NOTIFY(node, STRUCTURE);
}

static void cb_b_layer_create(UNUSED(void *user), VNodeID node_id, VLayerID layer_id, const char *name, VNBLayerType type)
{
	NodeBitmap	*node;
	NdbBLayer	*layer;

	if((node = (NodeBitmap *) nodedb_lookup_with_type(node_id, V_NT_BITMAP)) == NULL)
		return;
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

static void cb_b_layer_destroy(UNUSED(void *user), VNodeID node_id, VLayerID layer_id)
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

static void cb_b_tile_set(UNUSED(void *user), VNodeID node_id, VLayerID layer_id, uint16 tile_x, uint16 tile_y, uint16 tile_z,
			  VNBLayerType type, const VNBTile *tile)
{
	NodeBitmap	*node;
	NdbBLayer	*layer;
	size_t		ps, tw, th, ht, y, mod_src, mod_dst;
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
		memset(layer->framebuffer, 0, layer_size);
	}
	if(layer->framebuffer == NULL)
	{
		LOG_WARN(("No framebuffer in layer %u (%s)--out of memory?", layer->id, layer->name));
		return;
	}

	ht = (node->height + VN_B_TILE_SIZE - 1) / VN_B_TILE_SIZE;
	th = (tile_y == ht - 1) && (node->height % VN_B_TILE_SIZE) != 0 ? node->height % VN_B_TILE_SIZE : VN_B_TILE_SIZE;
	
	get = (uint8 *) tile;		/* It's a union. */
	put = nodedb_b_layer_tile_find(node, layer, tile_x, tile_y, tile_z);
	mod_src = tile_modulo(node, layer);
	tw = tile_width(node, layer, tile_x);
	mod_dst = layer_modulo(node, layer);
	for(y = 0; y < th; y++, get += mod_src, put += mod_dst)
		memcpy(put, get, tw);
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
