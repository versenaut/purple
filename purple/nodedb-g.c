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

	printf("now in g_layer_create\n");
	if((node = (NodeGeometry *) nodedb_lookup_with_type(node_id, V_NT_GEOMETRY)) == NULL)
		return;
	if(node->layers == NULL)
		node->layers = dynarr_new(sizeof *layer, 2);
	if(node->layers != NULL)
	{
		if((layer = dynarr_set(node->layers, layer_id, NULL)) != NULL)
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
			NOTIFY(node, STRUCTURE);
			verse_send_g_layer_subscribe(node_id, layer_id, VN_FORMAT_REAL64);
			printf("created layer %u.%u (%s), type %d\n", node_id, layer_id, name, type);
		}
	}
}

static void cb_g_layer_destroy(void *user, VNodeID node_id, VLayerID layer_id)
{
	NodeGeometry	*node;
	NdbGLayer	*layer;

	if((node = (NodeGeometry *) nodedb_lookup_with_type(node_id, V_NT_GEOMETRY)) == NULL)
		return;
	if((layer = nodedb_g_layer_lookup_id(node, layer_id)) == NULL || layer->name[0] == '\0')
		return;
	layer->name[0] = '\0';
	if(layer->data != NULL)
		dynarr_destroy(layer->data);
	NOTIFY(node, STRUCTURE);
}

/* Macro to define a vertex XYZ handler function. */
#define	VERTEX_XYZ(t)	\
	static void cb_g_vertex_set_##t ##_xyz(void *user, VNodeID node_id, VLayerID layer_id, uint32 vertex_id,\
						       t x, t y, t z)\
	{\
		NodeGeometry	*node;\
		NdbGLayer	*layer;\
		real32		*vtx;\
		if((node = (NodeGeometry *) nodedb_lookup_with_type(node_id, V_NT_GEOMETRY)) == NULL)\
			return;\
		if((layer = nodedb_g_layer_lookup_id(node, layer_id)) == NULL || layer->name[0] == '\0')\
			return;\
		if(layer->data == NULL)\
			layer->data = dynarr_new(3 * sizeof *vtx, 16);	/* FIXME: Should care about defaults. */\
		if((vtx = dynarr_set(layer->data, vertex_id, NULL)) != NULL)\
		{\
			vtx[0] = x;\
			vtx[1] = y;\
			vtx[2] = z;\
		}\
	}

/* Use macro to define the needed real32 and real64 varieties. */
VERTEX_XYZ(real32)
VERTEX_XYZ(real64)

/* Macro to define a vertex scalar handler function. */
#define	VERTEX_SCALAR(t)	\
	static void cb_g_vertex_set_ ##t(void *user, VNodeID node_id, VLayerID layer_id, uint32 vertex_id, t value)\
	{\
		NodeGeometry	*node;\
		NdbGLayer	*layer;\
		t		*v;\
		\
		if((node = (NodeGeometry *) nodedb_lookup_with_type(node_id, V_NT_GEOMETRY)) == NULL)\
			return;\
		if((layer = nodedb_g_layer_lookup_id(node, layer_id)) == NULL || layer->name[0] == '\0')\
			return;\
		if(layer->data == NULL)\
			layer->data = dynarr_new(sizeof *v, 16);	/* FIXME: Should care about defaults. */\
		if((v = dynarr_set(layer->data, vertex_id, NULL)) != NULL)\
			*v = value;\
	}

VERTEX_SCALAR(uint32)
VERTEX_SCALAR(real32)
VERTEX_SCALAR(real64)

/* Macro to define a polygon corner value handler function. */
#define POLYGON_CORNER(t)	\
	static void cb_g_polygon_set_corner_ ##t(void *user, VNodeID node_id, VLayerID layer_id, uint32 polygon_id,\
							 t v0, t v1, t v2, t v3)\
	{\
		NodeGeometry	*node;\
		NdbGLayer	*layer;\
		t		*v;\
		\
		if((node = (NodeGeometry *) nodedb_lookup_with_type(node_id, V_NT_GEOMETRY)) == NULL)\
			return;\
		if((layer = nodedb_g_layer_lookup_id(node, layer_id)) == NULL || layer->name[0] == '\0')\
			return;\
		if(layer->data == NULL)\
			layer->data = dynarr_new(4 * sizeof *v, 16);	/* FIXME: Should care about defaults. */\
		if((v = dynarr_set(layer->data, polygon_id, NULL)) != NULL)\
		{\
			v[0] = v0;\
			v[1] = v1;\
			v[2] = v2;\
			v[3] = v3;\
		}\
	}

POLYGON_CORNER(uint32)
POLYGON_CORNER(real32)
POLYGON_CORNER(real64)

/* Macro to define a polygon face value handler function. */
#define POLYGON_FACE(t)	\
	static void cb_g_polygon_set_face_ ##t(void *user, VNodeID node_id, VLayerID layer_id, uint32 polygon_id, t value)\
	{\
		NodeGeometry	*node;\
		NdbGLayer	*layer;\
		t		*v;\
		\
		if((node = (NodeGeometry *) nodedb_lookup_with_type(node_id, V_NT_GEOMETRY)) == NULL)\
			return;\
		if((layer = nodedb_g_layer_lookup_id(node, layer_id)) == NULL || layer->name[0] == '\0')\
			return;\
		if(layer->data == NULL)\
			layer->data = dynarr_new(sizeof *v, 16);	/* FIXME: Should care about defaults. */\
		if((v = dynarr_set(layer->data, polygon_id, NULL)) != NULL)\
			*v = value;\
	}

POLYGON_FACE(uint8)
POLYGON_FACE(uint32)
POLYGON_FACE(real32)
POLYGON_FACE(real64)

/* ----------------------------------------------------------------------------------------- */

void nodedb_g_register_callbacks(void)
{
	verse_callback_set(verse_send_g_layer_create,		cb_g_layer_create,	NULL);
	verse_callback_set(verse_send_g_layer_destroy,		cb_g_layer_destroy,	NULL);

	verse_callback_set(verse_send_g_vertex_set_real32_xyz,	cb_g_vertex_set_real32_xyz,	NULL);
	verse_callback_set(verse_send_g_vertex_set_real64_xyz,	cb_g_vertex_set_real64_xyz,	NULL);
	verse_callback_set(verse_send_g_vertex_set_uint32,	cb_g_vertex_set_uint32,		NULL);
	verse_callback_set(verse_send_g_vertex_set_real32,	cb_g_vertex_set_real32,		NULL);
	verse_callback_set(verse_send_g_vertex_set_real64,	cb_g_vertex_set_real64,		NULL);

	verse_callback_set(verse_send_g_polygon_set_corner_uint32,	cb_g_polygon_set_corner_uint32,	NULL);
	verse_callback_set(verse_send_g_polygon_set_corner_real32,	cb_g_polygon_set_corner_real32,	NULL);
	verse_callback_set(verse_send_g_polygon_set_corner_real64,	cb_g_polygon_set_corner_real64,	NULL);

	verse_callback_set(verse_send_g_polygon_set_face_uint8,		cb_g_polygon_set_face_uint8,	NULL);
	verse_callback_set(verse_send_g_polygon_set_face_uint32,	cb_g_polygon_set_face_uint32,	NULL);
	verse_callback_set(verse_send_g_polygon_set_face_real32,	cb_g_polygon_set_face_real32,	NULL);
	verse_callback_set(verse_send_g_polygon_set_face_real64,	cb_g_polygon_set_face_real64,	NULL);
}
