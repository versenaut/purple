/*
 * Bitmap node databasing.
*/

#include <stdio.h>
#include <string.h>

#include "verse.h"
#include "purple.h"

#include "dynarr.h"
#include "list.h"
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

static void cb_b_dimensions_set(VNodeID node_id, uint16 width, uint16 height, uint16 depth)
{
}

static void cb_b_layer_create(VNodeID node_id, VLayerID layer_id, const char *name, VNBLayerType type)
{
}

static void cb_b_layer_destroy(VNodeID node_id, VLayerID layer_id)
{
}

static void cb_b_tile_set(VNodeID node_id, VLayerID layer_id, uint16 tile_x, uint16 tile_y, uint16 tile_z, VNBLayerType type, VNBTile *tile)
{
}

/* ----------------------------------------------------------------------------------------- */

void nodedb_b_register_callbacks(void)
{
	verse_callback_set(verse_send_b_dimensions_set,	cb_b_dimensions_set, NULL);
	verse_callback_set(verse_send_b_layer_create,	cb_b_layer_create, NULL);
	verse_callback_set(verse_send_b_tile_set,	cb_b_tile_set, NULL);
	verse_callback_set(verse_send_b_layer_destroy,	cb_b_layer_destroy, NULL);
}
