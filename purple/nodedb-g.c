/*
 * 
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

void nodedb_g_construct(NodeGeometry *n)
{
	n->layers = NULL;
	n->bones  = NULL;
	n->crease_vertex.layer[0] = '\0';
	n->crease_vertex.def = 0;
	n->crease_edge.layer[0] = '\0';
	n->crease_edge.def = 0;
}

static void cb_copy_layer(void *d, const void *s, void *user)
{
	const NdbGLayer		*src = s;
	NdbGLayer		*dst = d;
	const NodeGeometry	*node = user;
}

void nodedb_g_copy(NodeGeometry *n, const NodeGeometry *src)
{
	if(src->layers != NULL)
		n->layers = dynarr_new_copy(src->layers, cb_copy_layer, n);
	if(src->bones != NULL)
		n->bones = dynarr_new_copy(src->bones, NULL, NULL);	/* Self-contained. */
	n->crease_vertex = src->crease_vertex;
	n->crease_edge   = src->crease_edge;
}

void nodedb_g_destruct(NodeGeometry *n)
{
	if(n->layers != NULL)
	{
		unsigned int	i;
		NdbGLayer	*layer;

		for(i = 0; i < dynarr_size(n->layers); i++)
		{
			if((layer = dynarr_index(n->layers, i)) == NULL || layer->name[0] == '\0')
				continue;
			mem_free(layer->data);
		}
		dynarr_destroy(n->layers);
	}
	if(n->bones != NULL)
		dynarr_destroy(n->bones);	/* Bones contain no pointers. */
}

NdbGLayer * nodedb_g_layer_lookup_id(const NodeGeometry *node, VLayerID layer_id)
{
	if(node == NULL)
		return NULL;
	return dynarr_index(node->layers, layer_id);
}

/* ----------------------------------------------------------------------------------------- */

static void cb_layer_default(unsigned int index, void *element)
{
	NdbGLayer	*layer = element;

	layer->id = -1;
	layer->name[0] = '\0';
	layer->data = NULL;
}

static void cb_g_layer_create(void *user, VNodeID node_id, VLayerID layer_id, const char *name,
			      VNGLayerType type, uint32 def_uint, real64 def_real)
{
	NodeGeometry	*node;
	NdbGLayer	*layer;

	if((node = nodedb_lookup_with_type(node_id, V_NT_GEOMETRY)) == NULL)
		return;
	if(node->layers == NULL)
		node->layers = dynarr_new(sizeof *layer, 2);
	if(node->layers != NULL)
	{
		if((layer = dynarr_set(node->layers, layer_id, NULL)) == NULL)
		{
			if(layer->name[0] != '\0')
			{
				if(layer->data != NULL)
				{
					dynarr_destroy(layer->data);
					layer->data = NULL;
				}
			}
			layer->id = layer_id;
			stu_strncpy(layer->name, sizeof layer->name, name);
			layer->type = type;
			layer->data = NULL;
			layer->def_uint = def_uint;
			layer->def_real = def_real;
		}
	}
}

static void cb_g_layer_destroy(void *user, VNodeID node_id, VLayerID layer_id)
{
	NodeGeometry	*node;
	NdbGLayer	*layer;

	if((node = nodedb_lookup_with_type(node_id, V_NT_GEOMETRY)) == NULL)
		return;
	if((layer = nodedb_g_layer_lookup_id(node, layer_id)) == NULL)
		return;
	layer->name[0] = '\0';
	if(layer->data != NULL)
		dynarr_destroy(layer->data);
}

/* ----------------------------------------------------------------------------------------- */

void nodedb_g_register_callbacks(void)
{
	verse_callback_set(verse_send_g_layer_create,	cb_g_layer_create,	NULL);
	verse_callback_set(verse_send_g_layer_destroy,	cb_g_layer_destroy,	NULL);
}
