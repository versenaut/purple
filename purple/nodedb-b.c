/*
 * Bitmap node databasing.
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

void nodedb_b_construct(NodeBitmap *n)
{
	n->width  = n->height = n->depth = 0U;
	n->layers = NULL;
}

void nodedb_b_copy(NodeBitmap *n, const NodeBitmap *src)
{
	n->width  = src->width;
	n->height = src->height;
	n->depth  = src->depth;
	/* FIXME: Copy layers. */
}

void nodedb_b_destruct(NodeBitmap *n)
{
}

/* ----------------------------------------------------------------------------------------- */

static size_t pixel_size(VNBLayerType type)
{
	switch(type)
	{
	case VN_B_LAYER_UINT1:	return 1;
	case VN_B_LAYER_UINT8:	return sizeof (uint8);
	case VN_B_LAYER_UINT16:	return sizeof (uint16);
	case VN_B_LAYER_REAL32:	return sizeof (real32);
	case VN_B_LAYER_REAL64:	return sizeof (real64);
	}
	return 0;
}

static void cb_b_dimensions_set(VNodeID node_id, uint16 width, uint16 height, uint16 depth)
{
	NodeBitmap	*node;
	NdbBLayer	*layer;
	size_t		i, y, z, ps, dss, sss;
	unsigned char	*fb;

	if((node = (NodeBitmap *) nodedb_lookup_with_type(node_id, V_NT_BITMAP)) == NULL)
		return;
	if(width == node->width && height == node->height && depth == node->depth)
		return;
	ps = pixel_size(layer->type);
	for(i = 0; i < dynarr_size(node->layers); i++)
	{
		if((layer = dynarr_index(node->layers, i)) == NULL || layer->name[0] == '\0')
			continue;
		fb = mem_alloc(ps * width * height * depth);
		if(fb == NULL)
		{
			LOG_WARN(("Couldn't allocate new framebuffer for layer %u.%u (%s)--out of memory",
				  node_id, layer->id, layer->name));
			continue;
		}
		/* Copy scanlines from old buffer into new. */
		sss = ps * node->width;
		dss = ps * width;
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

static void cb_b_layer_create(VNodeID node_id, VLayerID layer_id, const char *name, VNBLayerType type)
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
		strncpy(layer->name, sizeof layer->name, name);
		layer->type = type;
		layer->framebuffer = NULL;
	}
}

static void cb_b_layer_destroy(VNodeID node_id, VLayerID layer_id)
{
	NodeBitmap	*node;
	NdbBLayer	*layer;

	if((node = (NodeBitmap *) nodedb_lookup_with_type(node_id, V_NT_BITMAP)) == NULL)
		return;
	if((layer = dynarr_index(node->layers, layer_id)) == NULL || layer->name[0] == '\0')
		return;
	layer->type = -1;
	layer->name[0] = '\0';
	mem_free(layer->framebuffer);
}

static void cb_b_tile_set(VNodeID node_id, VLayerID layer_id, uint16 tile_x, uint16 tile_y, uint16 tile_z, VNBLayerType type, VNBTile *tile)
{
	NodeBitmap	*node;
	NdbBLayer	*layer;

	if((node = (NodeBitmap *) nodedb_lookup_with_type(node_id, V_NT_BITMAP)) == NULL)
		return;
	if((layer = dynarr_index(node->layers, layer_id)) == NULL || layer->name[0] == '\0')
		return;
	if(layer->type != type)
	{
		LOG_WARN(("Received type %d data for type %d layer--ignoring", type, layer->type));
		return;
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
