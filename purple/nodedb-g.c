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

	dst->id = src->id;
	strcpy(dst->name, src->name);
	dst->type = src->type;
	dst->data = dynarr_new_copy(src->data, NULL, NULL);
	dst->def_uint = src->def_uint;
	dst->def_real = src->def_real;
}

void nodedb_g_copy(NodeGeometry *n, const NodeGeometry *src)
{
	if(src->layers != NULL)
		n->layers = dynarr_new_copy(src->layers, cb_copy_layer, NULL);
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

		for(i = 0; (layer = dynarr_index(n->layers, i)) != NULL; i++)
		{
			if(layer->name[0] == '\0' || layer->data == NULL)
				continue;
			dynarr_destroy(layer->data);
		}
		dynarr_destroy(n->layers);
	}
	if(n->bones != NULL)
		dynarr_destroy(n->bones);	/* Bones contain no pointers. */
}

/* ----------------------------------------------------------------------------------------- */

void nodedb_g_layer_create(NodeGeometry *node, VLayerID layer_id, const char *name, VNGLayerType type)
{
	NdbGLayer	*layer;

	if(node->layers == NULL)
		node->layers = dynarr_new(sizeof *layer, 2);
	if(node->layers == NULL)
		return;
	if(layer_id == (uint16) ~0)
		layer = dynarr_append(node->layers, NULL, NULL);
	else
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
		}
	}
	layer->id = layer_id;
	stu_strncpy(layer->name, sizeof layer->name, name);
	layer->type = type;
	layer->data = NULL;
	layer->def_uint = 0;
	layer->def_real = 0.0;
}

NdbGLayer * nodedb_g_layer_lookup(const NodeGeometry *node, const char *name)
{
	unsigned int	i;
	NdbGLayer	*layer;

	if(node == NULL)
		return NULL;
	for(i = 0; (layer = dynarr_index(node->layers, i)) != NULL; i++)
	{
		if(strcmp(layer->name, name) == 0)
			return layer;
	}
	return NULL;
}

size_t nodedb_g_layer_size(const NodeGeometry *node, const NdbGLayer *layer)
{
	return dynarr_size(layer->data);
}

NdbGLayer * nodedb_g_layer_lookup_id(const NodeGeometry *node, VLayerID layer_id)
{
	if(node == NULL)
		return NULL;
	return dynarr_index(node->layers, layer_id);
}

/* ----------------------------------------------------------------------------------------- */

static void cb_g_layer_create(void *user, VNodeID node_id, VLayerID layer_id, const char *name,
			      VNGLayerType type, uint32 def_uint, real64 def_real)
{
	NodeGeometry	*node;
	NdbGLayer	*layer;

	if((node = (NodeGeometry *) nodedb_lookup_with_type(node_id, V_NT_GEOMETRY)) == NULL)
		return;
	if((layer = nodedb_g_layer_lookup(node, name)) != NULL)
	{
		if(layer->type == type)
		{
			layer->id = layer_id;
			layer->def_uint = def_uint;
			layer->def_real = def_real;
		}
		else
			fprintf(stderr, "Missing code here, geo layer needs to be reborn\n");	/* FIXME */
	}
	else
	{
		nodedb_g_layer_create(node, layer_id, name, type);
		if((layer = nodedb_g_layer_lookup(node, name)) != NULL)
		{
			layer->id = layer_id;
			stu_strncpy(layer->name, sizeof layer->name, name);
			layer->type = type;
			layer->data = NULL;
			layer->def_uint = def_uint;
			layer->def_real = def_real;
			NOTIFY(node, STRUCTURE);
			verse_send_g_layer_subscribe(node_id, layer_id, VN_FORMAT_REAL64);
		}
	}
	if(node->layers == NULL)
		node->layers = dynarr_new(sizeof *layer, 2);
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

void nodedb_g_vertex_set_xyz(NodeGeometry *node, NdbGLayer *layer, uint32 vertex_id, real64 x, real64 y, real64 z)
{
	real64	*vtx;

	if(layer->data == NULL)\
		layer->data = dynarr_new(3 * sizeof *vtx, 16);	/* FIXME: Should care about defaults. */\
	if((vtx = dynarr_set(layer->data, vertex_id, NULL)) != NULL)
	{
		vtx[0] = x;
		vtx[1] = y;
		vtx[2] = z;
/*		printf(" vertex set to (%g,%g,%g)\n", x, y, z);*/
	}
}

void nodedb_g_vertex_get_xyz(const NodeGeometry *node, const NdbGLayer *layer, uint32 vertex_id, real64 *x, real64 *y, real64 *z)
{
	real64	*vtx;

	if(layer->type != VN_G_LAYER_VERTEX_XYZ || layer->data == NULL)
		return;
	if((vtx = dynarr_index(layer->data, vertex_id)) != NULL)
	{
		*x = vtx[0];
		*y = vtx[1];
		*z = vtx[2];
	}
}

/* Macro to define a vertex XYZ handler function. */
#define	VERTEX_XYZ(t)	\
	static void cb_g_vertex_set_##t ##_xyz(void *user, VNodeID node_id, VLayerID layer_id, uint32 vertex_id,\
						       t x, t y, t z)\
	{\
		NodeGeometry	*node;\
		NdbGLayer	*layer;\
		t		*vtx;\
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
			NOTIFY(node, DATA);\
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
		{\
			*v = value;\
			NOTIFY(node, DATA);\
		}\
	}

VERTEX_SCALAR(uint32)
VERTEX_SCALAR(real32)
VERTEX_SCALAR(real64)

/* Macro to define a polygon corner value handler function. */
#define POLYGON_CORNER(t)	\
	void nodedb_g_polygon_set_corner_ ##t(NodeGeometry *node, NdbGLayer *layer, uint32 polygon_id,\
						      t v0, t v1, t v2, t v3)\
	{\
		t		*v;\
		\
		if(layer->data == NULL)\
			layer->data = dynarr_new(4 * sizeof *v, 16);	/* FIXME: Should care about defaults. */\
		if((v = dynarr_set(layer->data, polygon_id, NULL)) != NULL)\
		{\
			v[0] = v0;\
			v[1] = v1;\
			v[2] = v2;\
			v[3] = v3;\
			NOTIFY(node, DATA);\
		}\
	}\
	\
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
			NOTIFY(node, DATA);\
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
		{\
			*v = value;\
			NOTIFY(node, DATA);\
		}\
	}

POLYGON_FACE(uint8)
POLYGON_FACE(uint32)
POLYGON_FACE(real32)
POLYGON_FACE(real64)

/* ----------------------------------------------------------------------------------------- */

void nodedb_g_crease_set_vertex(NodeGeometry *n, const char *layer, uint32 def)
{
	if(n->node.type != V_NT_GEOMETRY)
		return;
	stu_strncpy(n->crease_vertex.layer, sizeof n->crease_vertex.layer, layer);
	n->crease_vertex.def = def;
}

void nodedb_g_crease_set_edge(NodeGeometry *n, const char *layer, uint32 def)
{
	if(n->node.type != V_NT_GEOMETRY)
		return;
	stu_strncpy(n->crease_edge.layer, sizeof n->crease_edge.layer, layer);
	n->crease_edge.def = def;
}

/* ----------------------------------------------------------------------------------------- */

static void cb_g_bone_create(void *user, VNodeID node_id, uint16 bone_id, const char *weight, const char *reference,
			     uint32 parent, real64 pos_x, real64 pos_y, real64 pos_z,
			     real64 rot_x, real64 rot_y, real64 rot_z, real64 rot_w)
{
	NodeGeometry	*node;
	NdbGBone	*bone;

	if((node = (NodeGeometry *) nodedb_lookup_with_type(node_id, V_NT_GEOMETRY)) == NULL)
		return;
	if(node->bones == NULL)
		node->bones = dynarr_new(sizeof *bone, 8);
	if(node->bones != NULL)
	{
		if((bone = dynarr_set(node->bones, bone_id, NULL)) != NULL)
		{
			stu_strncpy(bone->weight, sizeof bone->weight, weight);
			stu_strncpy(bone->reference, sizeof bone->reference, reference);
			bone->parent = parent;
			bone->pos[0] = pos_x;
			bone->pos[1] = pos_y;
			bone->pos[2] = pos_z;
			bone->rot[0] = rot_x;
			bone->rot[1] = rot_y;
			bone->rot[2] = rot_z;
			bone->rot[3] = rot_w;
			NOTIFY(node, DATA);
		}
	}
}

/* ----------------------------------------------------------------------------------------- */

static void cb_g_crease_set_vertex(void *user, VNodeID node_id, const char *layer, uint32 def_crease)
{
	NodeGeometry	*node;

	if((node = (NodeGeometry *) nodedb_lookup_with_type(node_id, V_NT_GEOMETRY)) == NULL)
		return;
	stu_strncpy(node->crease_vertex.layer, sizeof node->crease_vertex.layer, layer);
	node->crease_vertex.def = def_crease;
	NOTIFY(node, DATA);
}

static void cb_g_crease_set_edge(void *user, VNodeID node_id, const char *layer, uint32 def_crease)
{
	NodeGeometry	*node;

	if((node = (NodeGeometry *) nodedb_lookup_with_type(node_id, V_NT_GEOMETRY)) == NULL)
		return;
	stu_strncpy(node->crease_edge.layer, sizeof node->crease_edge.layer, layer);
	node->crease_edge.def = def_crease;
	NOTIFY(node, DATA);
}

/* ----------------------------------------------------------------------------------------- */

void nodedb_g_register_callbacks(void)
{
	verse_callback_set(verse_send_g_layer_create,			cb_g_layer_create,	NULL);
	verse_callback_set(verse_send_g_layer_destroy,			cb_g_layer_destroy,	NULL);

	verse_callback_set(verse_send_g_vertex_set_real32_xyz,		cb_g_vertex_set_real32_xyz,	NULL);
	verse_callback_set(verse_send_g_vertex_set_real64_xyz,		cb_g_vertex_set_real64_xyz,	NULL);
	verse_callback_set(verse_send_g_vertex_set_uint32,		cb_g_vertex_set_uint32,		NULL);
	verse_callback_set(verse_send_g_vertex_set_real32,		cb_g_vertex_set_real32,		NULL);
	verse_callback_set(verse_send_g_vertex_set_real64,		cb_g_vertex_set_real64,		NULL);

	verse_callback_set(verse_send_g_polygon_set_corner_uint32,	cb_g_polygon_set_corner_uint32,	NULL);
	verse_callback_set(verse_send_g_polygon_set_corner_real32,	cb_g_polygon_set_corner_real32,	NULL);
	verse_callback_set(verse_send_g_polygon_set_corner_real64,	cb_g_polygon_set_corner_real64,	NULL);

	verse_callback_set(verse_send_g_polygon_set_face_uint8,		cb_g_polygon_set_face_uint8,	NULL);
	verse_callback_set(verse_send_g_polygon_set_face_uint32,	cb_g_polygon_set_face_uint32,	NULL);
	verse_callback_set(verse_send_g_polygon_set_face_real32,	cb_g_polygon_set_face_real32,	NULL);
	verse_callback_set(verse_send_g_polygon_set_face_real64,	cb_g_polygon_set_face_real64,	NULL);

	verse_callback_set(verse_send_g_bone_create,			cb_g_bone_create, NULL);

	verse_callback_set(verse_send_g_crease_set_vertex,		cb_g_crease_set_vertex, NULL);
	verse_callback_set(verse_send_g_crease_set_edge,		cb_g_crease_set_edge, NULL);
}
