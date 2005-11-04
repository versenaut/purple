/*
 * nodedb-g.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * 
*/

#include <stdio.h>
#include <string.h>

#include "verse.h"
#include "purple.h"

#include "dynarr.h"
#include "list.h"
#include "iter.h"
#include "log.h"
#include "mem.h"
#include "memchunk.h"
#include "strutil.h"
#include "textbuf.h"

#include "nodedb.h"
#include "nodedb-internal.h"

/* ----------------------------------------------------------------------------------------- */

static MemChunk	*the_chunk_bone = NULL;

/* ----------------------------------------------------------------------------------------- */

void nodedb_g_construct(NodeGeometry *n)
{
	n->num_vertex = n->num_polygon = 0;
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
	dst->def  = src->def;
	dynarr_set_default(dst->data, &dst->def);
	dst->node = user;
}

void nodedb_g_copy(NodeGeometry *n, const NodeGeometry *src)
{
	n->num_vertex  = src->num_vertex;
	n->num_polygon = src->num_polygon;
	if(src->layers != NULL)
		n->layers = dynarr_new_copy(src->layers, cb_copy_layer, n);
	if(src->bones != NULL)
		n->bones = idtree_new_copy(src->bones, NULL, NULL);
	n->crease_vertex = src->crease_vertex;
	n->crease_edge   = src->crease_edge;
}

void nodedb_g_set(NodeGeometry *n, const NodeGeometry *src)
{
	/* FIXME: This is very costly. */
	nodedb_g_destruct(n);
	nodedb_g_copy(n, src);
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
		n->layers = NULL;
	}
	if(n->bones != NULL)
	{
		idtree_destroy(n->bones);	/* Bones contain no pointers. */
		n->bones = NULL;
	}
}

/* ----------------------------------------------------------------------------------------- */

unsigned int nodedb_g_layer_num(const NodeGeometry *node)
{
	unsigned int	i, num;
	NdbGLayer	*layer;

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

NdbGLayer * nodedb_g_layer_nth(const NodeGeometry *node, unsigned int index)
{
	unsigned int	i;
	NdbGLayer	*layer;

	if(node == NULL)
		return 0;
	for(i = 0; (layer = dynarr_index(node->layers, i)) != NULL; i++, index--)
	{
		if(layer->name[0] == '\0')
			continue;
		if(index == 0)
			return layer;
	}
	return NULL;
}

NdbGLayer * nodedb_g_layer_find(const NodeGeometry *node, const char *name)
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

size_t nodedb_g_layer_get_size(const NdbGLayer *layer)
{
	if(layer != NULL)
		return dynarr_size(layer->data);
	return 0;
}

const char * nodedb_g_layer_get_name(const NdbGLayer *layer)
{
	if(layer != NULL)
		return layer->name;
	return NULL;
}

static size_t layer_element_size(VNGLayerType type)
{
	switch(type)
	{
	case VN_G_LAYER_VERTEX_XYZ:		return 3 * sizeof (real64);
	case VN_G_LAYER_VERTEX_UINT32:		return sizeof (uint32);
	case VN_G_LAYER_VERTEX_REAL:		return sizeof (real64);
	case VN_G_LAYER_POLYGON_CORNER_UINT32:	return 4 * sizeof (uint32);
	case VN_G_LAYER_POLYGON_CORNER_REAL:	return 4 * sizeof (real64);
	case VN_G_LAYER_POLYGON_FACE_UINT8:	return sizeof (uint8);
	case VN_G_LAYER_POLYGON_FACE_UINT32:	return sizeof (uint32);
	case VN_G_LAYER_POLYGON_FACE_REAL:	return sizeof (real64);
	}
	return 0;
}

#define	VERTEX_LAYER(l)		((l)->type >= VN_G_LAYER_VERTEX_XYZ && ((l)->type < VN_G_LAYER_POLYGON_CORNER_UINT32))
#define	POLYGON_LAYER(l)	((l)->type >= VN_G_LAYER_POLYGON_CORNER_UINT32 && ((l)->type <= VN_G_LAYER_POLYGON_FACE_REAL))

static void nodedb_g_layer_allocate(NodeGeometry *node, NdbGLayer *layer)
{
	if(node == NULL || layer == NULL || layer->data != NULL)
		return;
	layer->data = dynarr_new(layer_element_size(layer->type), 16);
	dynarr_set_default(layer->data, &layer->def);

	if(VERTEX_LAYER(layer) && node->num_vertex > 0)
		dynarr_grow(layer->data, node->num_vertex - 1, 1);
	else if(POLYGON_LAYER(layer) && node->num_polygon > 0)
		dynarr_grow(layer->data, node->num_polygon - 1, 1);
}

void nodedb_g_layer_set_default(NdbGLayer *layer, uint32 def_uint, real64 def_real)
{
	switch(layer->type)
	{
	case VN_G_LAYER_VERTEX_XYZ:
		layer->def.v_xyz[0] = layer->def.v_xyz[1] = layer->def.v_xyz[2] = def_real;
		break;
	case VN_G_LAYER_VERTEX_UINT32:
		layer->def.v_uint32 = def_uint;
		break;
	case VN_G_LAYER_VERTEX_REAL:
		layer->def.v_real = def_real;
		break;
	case VN_G_LAYER_POLYGON_CORNER_UINT32:
		layer->def.p_corner_uint32[0] = layer->def.p_corner_uint32[1] = layer->def.p_corner_uint32[2] = layer->def.p_corner_uint32[3] = def_uint;
		break;
	case VN_G_LAYER_POLYGON_CORNER_REAL:
		layer->def.p_corner_real[0] = layer->def.p_corner_real[1] = layer->def.p_corner_real[2] = layer->def.p_corner_real[3] = def_real;
		break;
	case VN_G_LAYER_POLYGON_FACE_UINT8:
		layer->def.p_face_uint8 = def_uint;
		break;
	case VN_G_LAYER_POLYGON_FACE_UINT32:
		layer->def.p_face_uint32 = def_uint;
		break;
	case VN_G_LAYER_POLYGON_FACE_REAL:
		layer->def.p_face_real = def_real;
		break;
	}
	layer->def_uint = def_uint;
	layer->def_real = def_real;
}

static void cb_def_layer(unsigned int index, void *element, void *user)
{
	NdbGLayer	*layer = element;

	layer->name[0] = '\0';
	layer->data = NULL;
}

NdbGLayer * nodedb_g_layer_create(NodeGeometry *node, VLayerID layer_id, const char *name, VNGLayerType type, uint32 def_uint, real64 def_real)
{
	NdbGLayer	*layer;

	if(node->layers == NULL)
	{
		node->layers = dynarr_new(sizeof *layer, 2);
		dynarr_set_default_func(node->layers, cb_def_layer, NULL);
	}
	if(node->layers == NULL)
		return NULL;
	if(layer_id == (VLayerID) ~0)
		layer = dynarr_append(node->layers, NULL, NULL);
	else
	{
		printf("creating layer %u (\"%s\") in node %u, type %u, def_uint=%u def_real=%g\n", layer_id, name, node->node.id, type, def_uint, def_real);
		if((layer = dynarr_index(node->layers, layer_id)) != NULL)
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
		layer = dynarr_set(node->layers, layer_id, NULL);
	}
	layer->id = layer_id;
	stu_strncpy(layer->name, sizeof layer->name, name);
	layer->type = type;
	layer->node = node;
	nodedb_g_layer_set_default(layer, def_uint, def_real);
	nodedb_g_layer_allocate(node, layer);
	printf("done, layer %s created\n", name);

	return layer;
}

void nodedb_g_layer_destroy(NodeGeometry *node, NdbGLayer *layer)
{
	if(node == NULL || layer == NULL)
		return;
	layer->name[0] = '\0';
	if(layer->data != NULL)
		dynarr_destroy(layer->data);
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
	if((layer = nodedb_g_layer_find(node, name)) != NULL)
	{
		if(layer->type == type)
		{
			layer->id = layer_id;
			nodedb_g_layer_set_default(layer, def_uint, def_real);
		}
		else
			fprintf(stderr, "Missing code here, geo layer needs to be reborn\n");	/* FIXME */
	}
	else
	{	
		nodedb_g_layer_create(node, layer_id, name, type, def_uint, def_real);
		if((layer = nodedb_g_layer_find(node, name)) != NULL)
		{
			layer->id = layer_id;
			stu_strncpy(layer->name, sizeof layer->name, name);
			layer->type = type;
			layer->data = NULL;
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
	nodedb_g_layer_destroy(node, layer);
	NOTIFY(node, STRUCTURE);
}

/* ----------------------------------------------------------------------------------------- */

void nodedb_g_vertex_set_selected(NodeGeometry *node, uint32 vertex_id, real64 selection)
{
	NdbGLayer	*sel;

	if(node == NULL)
		return;
	if((sel = nodedb_g_layer_find(node, "selection")) == NULL)
	{
		sel = nodedb_g_layer_create(node, (uint16) ~0u, "selection", VN_G_LAYER_VERTEX_REAL, 0u, 47.11);
	}
	nodedb_g_vertex_set_real(sel, vertex_id, selection);
	printf("set selection of vertex %u.%u to %g\n", node->node.id, vertex_id, selection);
}

real64 nodedb_g_vertex_get_selected(const NodeGeometry *node, uint32 vertex_id)
{
	NdbGLayer	*sel;

	if(node == NULL)
		return 0.0;
	if((sel = nodedb_g_layer_find(node, "selection")) == NULL)
		return 0.0;
	return nodedb_g_vertex_get_real(sel, vertex_id);
}

/* ----------------------------------------------------------------------------------------- */

static void resize_vertex(NodeGeometry *node, uint32 size)
{
	unsigned int	i;
	NdbGLayer	*layer;

	for(i = 2; (layer = dynarr_index(node->layers, i)) != NULL; i++)
	{
		if(layer->name[0] == '\0')
			continue;
		if(layer->type >= VN_G_LAYER_POLYGON_CORNER_UINT32)	/* Really skip polygon layers. */
			continue;
		if(layer->data == NULL)
			layer->data = dynarr_new(layer_element_size(layer->type), 16);
		if(dynarr_size(layer->data) < size)
			dynarr_grow(layer->data, size - 1, 1);
	}
	node->num_vertex = size;
}

void nodedb_g_vertex_set_xyz(NdbGLayer *layer, uint32 vertex_id, real64 x, real64 y, real64 z)
{
	real64	*vtx;

	if(layer == NULL)
		return;
	if(layer->id != 0 && vertex_id >= layer->node->num_vertex)
		return;
	if(layer->data == NULL)
		layer->data = dynarr_new(3 * sizeof *vtx, 16);	/* FIXME: Should care about defaults. */\
	if((vtx = dynarr_set(layer->data, vertex_id, NULL)) != NULL)
	{
		vtx[0] = x;
		vtx[1] = y;
		vtx[2] = z;
		if(vertex_id >= layer->node->num_vertex)
			resize_vertex(layer->node, vertex_id + 1);
	}
}

void nodedb_g_vertex_delete(NdbGLayer *layer, uint32 vertex_id)
{
	if(layer == NULL)
		return;
	if(layer->data == NULL)
		return;
	if(vertex_id < dynarr_size(layer->data))
	{
		real64	*vtx = dynarr_index(layer->data, vertex_id);

		if(vtx != NULL)
			vtx[0] = vtx[1] = vtx[2] = V_REAL64_MAX;
	}
}

void nodedb_g_vertex_get_xyz(const NdbGLayer *layer, uint32 vertex_id, real64 *x, real64 *y, real64 *z)
{
	real64	*vtx;

	if(layer == NULL || layer->type != VN_G_LAYER_VERTEX_XYZ || layer->data == NULL || vertex_id >= layer->node->num_vertex)
		return;
	if((vtx = dynarr_index(layer->data, vertex_id)) != NULL)
	{
		*x = vtx[0];
		*y = vtx[1];
		*z = vtx[2];
	}
	else
		*x = *y = *z = 0.0;
}

void nodedb_g_vertex_set_uint32(NdbGLayer *layer, uint32 vertex_id, uint32 value)
{
	if(layer == NULL)
		return;
	dynarr_set(layer->data, vertex_id, &value);
}

uint32 nodedb_g_vertex_get_uint32(const NdbGLayer *layer, uint32 vertex_id)
{
	if(layer == NULL)
		return 0u;
	if(layer->data == NULL)
		return 0;
	return *(uint32 *) dynarr_index(layer->data, vertex_id);
}

void nodedb_g_vertex_set_real(NdbGLayer *layer, uint32 vertex_id, real64 value)
{
	if(layer == NULL || layer->type != VN_G_LAYER_VERTEX_REAL)
		return;
	dynarr_set(layer->data, vertex_id, &value);
}

real64 nodedb_g_vertex_get_real(const NdbGLayer *layer, uint32 vertex_id)
{
	if(layer == NULL || layer->data == NULL)
		return 0.0;
	return *(real64 *) dynarr_index(layer->data, vertex_id);
}

/* Macro to define a vertex XYZ handler function. */
#define	VERTEX_XYZ(t)	\
	static void cb_g_vertex_set_xyz_ ##t(void *user, VNodeID node_id, VLayerID layer_id, uint32 vertex_id,\
						       t x, t y, t z)\
	{\
		NodeGeometry	*node;\
		NdbGLayer	*layer;\
		t		*vtx;\
		if((node = (NodeGeometry *) nodedb_lookup_with_type(node_id, V_NT_GEOMETRY)) == NULL)\
			return;\
		if((layer = nodedb_g_layer_lookup_id(node, layer_id)) == NULL || layer->name[0] == '\0')\
			return;\
		nodedb_g_vertex_set_xyz(layer, vertex_id, x, y, z);\
		NOTIFY(node, DATA);\
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
		nodedb_g_vertex_set_ ##t(layer, vertex_id, value);\
		NOTIFY(node, DATA);\
	}

VERTEX_SCALAR(uint32)

static void cb_g_vertex_set_real32(void *user, VNodeID node_id, VLayerID layer_id, uint32 vertex_id, real32 value)
{
	NodeGeometry	*node;
	NdbGLayer	*layer;
	
	if((node = (NodeGeometry *) nodedb_lookup_with_type(node_id, V_NT_GEOMETRY)) == NULL)
		return;
	if((layer = nodedb_g_layer_lookup_id(node, layer_id)) == NULL || layer->name[0] == '0')
		return;
	nodedb_g_vertex_set_real(layer, vertex_id, value);
	NOTIFY(node, DATA);
}

static void cb_g_vertex_set_real64(void *user, VNodeID node_id, VLayerID layer_id, uint32 vertex_id, real64 value)
{
	NodeGeometry	*node;
	NdbGLayer	*layer;
	
	if((node = (NodeGeometry *) nodedb_lookup_with_type(node_id, V_NT_GEOMETRY)) == NULL)
		return;
	if((layer = nodedb_g_layer_lookup_id(node, layer_id)) == NULL || layer->name[0] == '0')
		return;
	nodedb_g_vertex_set_real(layer, vertex_id, value);
	NOTIFY(node, DATA);
}

/* Macro to define polygon corner value handler functions (api set/get, and callback set). */
#define POLYGON_CORNER(t)	\
	void nodedb_g_polygon_set_corner_ ##t(NdbGLayer *layer, uint32 polygon_id, t v0, t v1, t v2, t v3)\
	{\
		t	*v;\
		\
		if(layer->data == NULL)\
			layer->data = dynarr_new(4 * sizeof *v, 16);	/* FIXME: Should care about defaults. */\
		if((v = dynarr_set(layer->data, polygon_id, NULL)) != NULL)\
		{\
			v[0] = v0;\
			v[1] = v1;\
			v[2] = v2;\
			v[3] = v3;\
		}\
	}\
	\
	void nodedb_g_polygon_get_corner_ ##t(const NdbGLayer *layer, uint32 polygon_id, t *v0, t *v1, t *v2, t *v3)\
	{\
		const t	*v;\
		\
		if(layer->data == NULL)\
			return;\
		if((v = dynarr_index(layer->data, polygon_id)) != NULL)\
		{\
			if(v0)\
				*v0 = v[0];\
			if(v1)\
				*v1 = v[1];\
			if(v2)\
				*v2 = v[2];\
			if(v3)\
				*v3 = v[3];\
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

static void cb_g_polygon_delete(void *user, VNodeID node_id, uint32 polygon_id)
{
	NodeGeometry	*node;
	NdbGLayer	*layer;
	uint32		*p;

	if((node = (NodeGeometry *) nodedb_lookup_with_type(node_id, V_NT_GEOMETRY)) == NULL)
		return;
	if((layer = nodedb_g_layer_lookup_id(node, 1)) == NULL || layer->name[0] == '\0')
		return;
	if(layer->data == NULL)
		return;
	if((p = dynarr_set(layer->data, polygon_id, NULL)) != NULL)
	{
		p[0] = p[1] = p[2] = p[3] = ~0u;
		NOTIFY(node, DATA);
	}
}

/* Macro to define a polygon face value handler function. */
#define POLYGON_FACE(t)	\
	void nodedb_g_polygon_set_face_ ##t(NdbGLayer *layer, uint32 polygon_id, t value)\
	{\
		t	*v;\
		\
		if(layer->data == NULL)\
			layer->data = dynarr_new(sizeof *v, 16);	/* FIXME: Should care about defaults. */\
		dynarr_set(layer->data, polygon_id, &value);\
	}\
	\
	t nodedb_g_polygon_get_face_ ##t(const NdbGLayer *layer, uint32 polygon_id)\
	{\
		const t	*v;\
		\
		if((v = dynarr_index(layer->data, polygon_id)) != NULL)\
			return *v;\
		return 0;\
	}\
	\
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

static void cb_g_vertex_delete(void *user, VNodeID node_id, uint32 vertex_id)
{
	NodeGeometry	*node;
	NdbGLayer	*layer;

	if((node = (NodeGeometry *) nodedb_lookup_with_type(node_id, V_NT_GEOMETRY)) == NULL)
		return;
	if((layer = nodedb_g_layer_lookup_id(node, 0)) == NULL || layer->name[0] == '\0')
		return;
	nodedb_g_vertex_delete(layer, vertex_id);
}

/* ----------------------------------------------------------------------------------------- */

unsigned int nodedb_g_bone_num(const NodeGeometry *node)
{
	if(node == NULL)
		return 0;
	return idtree_size(node->bones);
}

NdbGBone * nodedb_g_bone_nth(const NodeGeometry *node, unsigned int n)
{
	return NULL;
/*	unsigned int	i;
	NdbGBone	*bone;

	if(node == NULL)
		return 0;
	for(i = 0; (bone = dynarr_index(node->bones, i)) != NULL; i++)
	{
		if(bone->id == (uint16) ~0u)
			continue;
		if(i == n)
			return bone;
	}
	return NULL;
*/}

void nodedb_g_bone_iter(const NodeGeometry *node, PIter *iter)
{
	iter_init_dynarr_uint16_ffff(iter, ((NodeGeometry *) node)->bones, offsetof(NdbGBone, id));
}

NdbGBone * nodedb_g_bone_lookup(const NodeGeometry *node, uint16 id)
{
	NdbGBone	*bone;

	if(node == NULL || id == (uint16) ~0u)
		return NULL;
	return idtree_get(node->bones, id);
}

static int bones_equal(const NodeGeometry *node, const NdbGBone *a,
		       const NodeGeometry *target, const NdbGBone *b);

/* Compare two bone references, and (if valid) the bones they refer to. Recursive. */
static int bone_refs_equal(const NodeGeometry *node, uint16 ref,
		       const NodeGeometry *target, uint16 tref)
{
	NdbGBone	*ba, *bb;

	if(ref == (uint16) ~0u && tref == (uint16) ~0u)
		return 1;
	ba = nodedb_g_bone_lookup(node, ref);
	bb = nodedb_g_bone_lookup(target, tref);
	return bones_equal(node, ba, target, bb);
}

/* Compare two bones, in the contexts of their respective nodes. We try to do
 * all the cheap comparisons first, before recursing on the parent link in each.
*/
static int bones_equal(const NodeGeometry *node, const NdbGBone *a,
		       const NodeGeometry *target, const NdbGBone *b)
{
	if(node == NULL || a == NULL || target == NULL || b == NULL)
		return 0;
	/* The position and rotation are simple enough to compare. */
	if(memcmp(a->pos, b->pos, sizeof a->pos) != 0)
		return 0;
	if(memcmp(a->rot, b->rot, sizeof a->rot) != 0)
		return 0;
	/* I'm not sure here, perhaps we should compare the actual layer contents,
	 * rather than just relying on the names being equal. If so, this is not
	 * at all a lightweight test, of course.
	*/
	if(strcmp(a->weight, b->weight) != 0)
		return 0;
	if(strcmp(a->reference, b->reference) != 0)
		return 0;
	/* FIXME: This could, I guess, also compare the actual curves. That's be
	 * kind of expensive and complicated though, so for now let's not do that.
	*/
	if(strcmp(a->pos_curve, b->pos_curve) != 0)
		return 0;
	if(strcmp(a->rot_curve, b->rot_curve) != 0)
		return 0;
	/* Only the parent left to compare, at this point. Recurse. */
	return bone_refs_equal(node, a->parent, target, b->parent);
}

static void cb_bone_default(unsigned int index, void *element, void *user)
{
	NdbGBone	*bone = element;

	bone->id = ~0;
}

/* Find pointer to bone in <n> that is equal to <bone> in <source>. */
const NdbGBone * nodedb_g_bone_find_equal(const NodeGeometry *n, const NodeGeometry *source, const NdbGBone *bone)
{
	unsigned int	i;
	const NdbGBone	*there;

	for(i = 0; (there = dynarr_index(n->bones, i)) != NULL; i++)
	{
		if(there->id == (uint16) ~0u)
			continue;
		if(bones_equal(source, bone, n, there))
			return there;
	}
	return NULL;
}

int nodedb_g_bone_resolve(uint16 *id, const NodeGeometry *n, const NodeGeometry *source, uint16 b)
{
	const NdbGBone	*bone, *tbone;	/* Heh. */

	if(id == NULL || n == NULL || source == NULL)
		return 0;
	if(b == (uint16) ~0u)
	{
		*id = (uint16) ~0u;
		return 1;
	}
	if((bone = nodedb_g_bone_lookup(source, b)) != NULL)
	{
		if((tbone = nodedb_g_bone_find_equal(n, source, bone)) != NULL)
		{
			*id = tbone->id;
			return 1;
		}
	}
	return 0;
}

NdbGBone * nodedb_g_bone_create(NodeGeometry *n, uint16 id, const char *weight, const char *reference,
				uint16 parent,
				real64 px, real64 py, real64 pz, const char *pos_curve,
				real64 rx, real64 ry, real64 rz, real64 rw, const char *rot_curve)
{
	NdbGBone	*bone;

	if(n == NULL)
		return NULL;
	if(n->bones == NULL)
	{
		n->bones = dynarr_new(sizeof (NdbGBone), 128);	/* FIXME: This will break if realloc() causes bones to move! */
		dynarr_set_default_func(n->bones, cb_bone_default, NULL);
	}
	if(id == (uint16) ~0)
	{
		unsigned int	index;

		bone = dynarr_append(n->bones, NULL, &index);
		id = index;
	}
	else
		bone = dynarr_set(n->bones, id, NULL);
	if(bone != NULL)
	{
		bone->id     = id;
		bone->parent = parent;

		stu_strncpy(bone->weight, sizeof bone->weight, weight);
		stu_strncpy(bone->reference, sizeof bone->reference, reference);
		bone->pos[0] = px;
		bone->pos[1] = py;
		bone->pos[2] = pz;
		stu_strncpy(bone->pos_curve, sizeof bone->pos_curve, pos_curve);
		bone->rot[0] = rx;
		bone->rot[1] = ry;
		bone->rot[2] = rz;
		bone->rot[3] = rw;
		stu_strncpy(bone->rot_curve, sizeof bone->rot_curve, rot_curve);
		printf("****bone at %p created; id=%u weight='%s' reference='%s' parent=%u pcurve='%s' rcurve='%s'\n", bone, id,
		       bone->weight, bone->reference, bone->parent, bone->pos_curve, bone->rot_curve);
		bone->pending = 0;
	}
	return bone;
}

void nodedb_g_bone_destroy(NodeGeometry *n, NdbGBone *bone)
{
	if(n == NULL || bone == NULL)
		return;
	bone->id = (uint16) ~0u;	/* All it takes, really. */
}

uint16 nodedb_g_bone_get_id(const NdbGBone *bone)
{
	return bone != NULL ? bone->id : (uint16) ~0u;
}

const char * nodedb_g_bone_get_weight(const NdbGBone *bone)
{
	if(bone == NULL)
		return NULL;
	return bone->weight;
}

const char * nodedb_g_bone_get_reference(const NdbGBone *bone)
{
	if(bone == NULL)
		return NULL;
	return bone->reference;
}

uint16 nodedb_g_bone_get_parent(const NdbGBone *bone)
{
	if(bone == NULL)
		return (uint16) ~0u;
	return bone->parent;
}

void nodedb_g_bone_get_pos(const NdbGBone *bone, real64 *pos_x, real64 *pos_y, real64 *pos_z)
{
	if(bone == NULL)
		return;
	if(pos_x != NULL)
		*pos_x = bone->pos[0];
	if(pos_y != NULL)
		*pos_y = bone->pos[1];
	if(pos_z != NULL)
		*pos_z = bone->pos[2];
}

void nodedb_g_bone_get_rot(const NdbGBone *bone, real64 *rot_x, real64 *rot_y, real64 *rot_z, real64 *rot_w)
{
	if(bone == NULL)
		return;
	if(rot_x != NULL)
		*rot_x = bone->rot[0];
	if(rot_y != NULL)
		*rot_y = bone->rot[1];
	if(rot_z != NULL)
		*rot_z = bone->rot[2];
	if(rot_w != NULL)
		*rot_z = bone->rot[3];
}

const char * nodedb_g_bone_get_pos_curve(const NdbGBone *bone)
{
	if(bone == NULL)
		return NULL;
	return bone->pos_curve;
}

const char * nodedb_g_bone_get_rot_curve(const NdbGBone *bone)
{
	if(bone == NULL)
		return NULL;
	return bone->rot_curve;
}

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
			     uint32 parent_id, real64 pos_x, real64 pos_y, real64 pos_z, const char *pos_curve,
			     const VNQuat64 *rot, const char *rot_curve)
{
	NodeGeometry	*node;
	int		old;

	if((node = (NodeGeometry *) nodedb_lookup_with_type(node_id, V_NT_GEOMETRY)) == NULL)
		return;
	old = nodedb_g_bone_lookup(node, node_id) != NULL;
	nodedb_g_bone_create(node, bone_id, weight, reference, parent_id, pos_x, pos_y, pos_z, pos_curve, rot->x, rot->y, rot->z, rot->w, rot_curve);
	if(old)
		NOTIFY(node, DATA);
	else
		NOTIFY(node, STRUCTURE);
}

static void cb_g_bone_destroy(void *user, VNodeID node_id, uint16 bone_id)
{
	NodeGeometry	*node;

	if((node = (NodeGeometry *) nodedb_lookup_with_type(node_id, V_NT_GEOMETRY)) != NULL)
	{
		NdbGBone	*bone;

		if((bone = nodedb_g_bone_lookup(node, bone_id)) != NULL)
		{
			nodedb_g_bone_destroy(node, bone);
			NOTIFY(node, STRUCTURE);
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

	verse_callback_set(verse_send_g_vertex_set_xyz_real32,		cb_g_vertex_set_xyz_real32,	NULL);
	verse_callback_set(verse_send_g_vertex_set_xyz_real64,		cb_g_vertex_set_xyz_real64,	NULL);
	verse_callback_set(verse_send_g_vertex_delete_real32,		cb_g_vertex_delete,		NULL);
	verse_callback_set(verse_send_g_vertex_delete_real64,		cb_g_vertex_delete,		NULL);
	verse_callback_set(verse_send_g_vertex_set_uint32,		cb_g_vertex_set_uint32,		NULL);
	verse_callback_set(verse_send_g_vertex_set_real32,		cb_g_vertex_set_real32,		NULL);
	verse_callback_set(verse_send_g_vertex_set_real64,		cb_g_vertex_set_real64,		NULL);

	verse_callback_set(verse_send_g_polygon_set_corner_uint32,	cb_g_polygon_set_corner_uint32,	NULL);
	verse_callback_set(verse_send_g_polygon_delete,			cb_g_polygon_delete,		NULL);
	verse_callback_set(verse_send_g_polygon_set_corner_real32,	cb_g_polygon_set_corner_real32,	NULL);
	verse_callback_set(verse_send_g_polygon_set_corner_real64,	cb_g_polygon_set_corner_real64,	NULL);

	verse_callback_set(verse_send_g_polygon_set_face_uint8,		cb_g_polygon_set_face_uint8,	NULL);
	verse_callback_set(verse_send_g_polygon_set_face_uint32,	cb_g_polygon_set_face_uint32,	NULL);
	verse_callback_set(verse_send_g_polygon_set_face_real32,	cb_g_polygon_set_face_real32,	NULL);
	verse_callback_set(verse_send_g_polygon_set_face_real64,	cb_g_polygon_set_face_real64,	NULL);

	verse_callback_set(verse_send_g_bone_create,			cb_g_bone_create, NULL);
	verse_callback_set(verse_send_g_bone_destroy,			cb_g_bone_destroy, NULL);

	verse_callback_set(verse_send_g_crease_set_vertex,		cb_g_crease_set_vertex, NULL);
	verse_callback_set(verse_send_g_crease_set_edge,		cb_g_crease_set_edge, NULL);

	the_chunk_bone = memchunk_new("NdbGBone", sizeof (NdbGBone), 8);
}
