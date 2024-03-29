/*
 * synchronizer.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Main node synchronization pipeline implementation. Basically, maintain lists of nodes that
 * are known to have changed in some way, and compare each node to its corresponding "input
 * version" held in the nodedb. Then, if differences are found, send Verse commands to make
 * them go away. :)
*/

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "verse.h"

#include "dynarr.h"
#include "diff.h"
#include "list.h"
#include "log.h"
#include "mem.h"
#include "plugins.h"
#include "strutil.h"
#include "textbuf.h"
#include "nodedb.h"
#include "value.h"

#include "graph.h"

#include "synchronizer.h"

/* ----------------------------------------------------------------------------------------- */

static struct
{
	List	*queue_create;
	List	*queue_create_pend;
	List	*queue_sync;
} sync_info;

/* ----------------------------------------------------------------------------------------- */

static void cb_notify(PNode *node, NodeNotifyEvent ev)
{
	List	*iter, *next;

	if(ev != NODEDB_NOTIFY_CREATE)
		return;
	for(iter = sync_info.queue_create_pend; iter != NULL; iter = next)
	{
		PNode	*n;

		next = list_next(iter);
		n = list_data(iter);
		if(n->type == node->type)
		{
			LOG_MSG(("Node at %p has host-side ID %u, we can now sync it", n, node->id));
			sync_info.queue_create_pend = list_unlink(sync_info.queue_create_pend, iter);
			list_destroy(iter);
			n->id = node->id;	/* To-sync copy now has known ID. Excellent. */
			sync_info.queue_sync = list_prepend(sync_info.queue_sync, n);	/* Re-add to other queue. */
			if(n->creator.port != NULL)
			{
				n->creator.remote = node;	/* Fill in the remote version field. */
				graph_port_output_create_notify(n);
			}
			break;
		}
	}
}

void sync_init(void)
{
	nodedb_notify_add(NODEDB_OWNERSHIP_MINE, cb_notify);
}

/* ----------------------------------------------------------------------------------------- */

static int sync_head_tags(const PNode *n, const PNode *target)
{
	unsigned int	i, sync = 1;
	NdbTagGroup	*g, *tg;

	for(i = 0; (g = dynarr_index(n->tag_groups, i)) != NULL; i++)
	{
		if(g->name[0] == '\0')
			continue;
		if((tg = nodedb_tag_group_find(target, g->name)) != NULL)
		{
			unsigned int	j;
			NdbTag		*tag, *ttag;

			for(j = 0; (tag = dynarr_index(g->tags, j)) != NULL; j++)
			{
				if(tag->name[0] == '\0')
					continue;
				ttag = nodedb_tag_group_tag_find(tg, tag->name);
				if(ttag != NULL)
				{
					/* Tag exists, see if fields match. */
					if(tag->type != ttag->type || !nodedb_tag_values_equal(tag, ttag))
					{
						verse_send_tag_create(target->id, tg->id, ttag->id,
								      ttag->name, tag->type, &tag->value);
						sync = 0;
					}
				}
				else
				{
					verse_send_tag_create(target->id, tg->id, ~0, tag->name, tag->type, &tag->value);
					sync = 0;
				}
			}
		}
		else
		{
			printf("sync creating tag group %s in %u\n", g->name, target->id);
			verse_send_tag_group_create(target->id, ~0, g->name);
			sync = 0;
		}
	}
	return sync;
}

/* Synchronize node "head" data, i.e. name and tags. */
static int sync_head(const PNode *n, const PNode *target)
{
	int	sync = 1;

	/* Compare names, if set. */
	if(n->name[0] != '\0' && strcmp(n->name, target->name) != 0)
	{
		verse_send_node_name_set(target->id, n->name);
		sync = 0;
	}
	sync &= sync_head_tags(n, target);
	return sync;
}

/* ----------------------------------------------------------------------------------------- */

static int object_link_exists(const NodeObject *n, VNodeID link, const char *label, uint32 target_id)
{
	unsigned int	i;
	const NdbOLink	*l;

	for(i = 0; (l = dynarr_index(n->links, i)) != NULL; i++)
	{
		if(l->link == link && l->target_id == target_id && strcmp(l->label, label) == 0)
			return 1;
	}
	return 0;
}

static uint16 object_link_find_same(const NodeObject *n, const char *label, uint32 target_id)
{
	unsigned int	i;
	const NdbOLink	*l;

	for(i = 0; (l = dynarr_index(n->links, i)) != NULL; i++)
	{
		if(l->label[0] == '\0')
			continue;
		if((target_id == ~0u || target_id == l->target_id) && strcmp(label, l->label) == 0)
			return i;
	}
	return (uint16) ~0u;
}

static int sync_object(NodeObject *n, const NodeObject *target)
{
	int		sync = 1, i, j;
	List		*iter, *next;
	NdbOLink	*link, *tlink;

	/* Synchronize transform. Just basic "current value"-support, for now. */
	if(memcmp(n->pos, target->pos, sizeof target->pos) != 0)
	{
		verse_send_o_transform_pos_real64(target->node.id, 0u, 0u, n->pos, NULL, NULL, NULL, 0.0);
		sync = 0;
	}
	if(memcmp(n->rot, target->rot, sizeof target->rot) != 0)
	{
		VNQuat64	rot;

		rot.x = n->rot[0];
		rot.y = n->rot[1];
		rot.z = n->rot[2];
		rot.w = n->rot[3];
		verse_send_o_transform_rot_real64(target->node.id, 0u, 0u, &rot, NULL, NULL, NULL, 0.0);
		sync = 0;
	}
	/* Synchronize light source information. */
	if(n->light[0] != target->light[0] || n->light[1] != target->light[1] || n->light[2] != target->light[2])
	{
		verse_send_o_light_set(target->node.id, n->light[0], n->light[1], n->light[2]);
		sync = 0;
	}
	/* Synchronize true links. */
	for(i = 0; (link = dynarr_index(n->links, i)) != NULL; i++)
	{
		if(link->id == (uint16) ~0u)
			continue;
		if(link->deleted)	/* Don't send links that we know have been deleted, locally. */
			continue;
		if(!object_link_exists(target, link->link, link->label, link->target_id))
		{
/*			printf("sending true link set, %u->%u\n", target->node.id, link->link);*/
			verse_send_o_link_set(target->node.id, (uint16) ~0u, link->link, link->label, link->target_id);
			sync = 0;
		}
	}
	/* Look through local (non-synced) links. */
	for(iter = n->links_local; iter != NULL; iter = next)
	{
		NdbOLinkLocal	*link = list_data(iter);

		next = list_next(iter);
		if(link->link->id == (uint16) ~0)	/* Can't sync until link target gets server-side ID. */
			sync = 0;
		else
		{
			uint16	id;
	
/*			printf("local node has been resolved. now see if link it replaces anything\n");*/
			if((id = object_link_find_same(target, link->label, link->target_id)) != (uint16) ~0u)
			{
/*				printf(" replacing link %u with the local one\n", id);*/
				verse_send_o_link_set(target->node.id, id, link->link->id, link->label, link->target_id);
				sync = 0;
			}
			if(!object_link_exists(target, link->link->id, link->label, link->target_id))	/* Only set if no equivalent link exists. */
			{
/*				printf(" sending local link set, %u->%u\n", target->node.id, link->link->id);*/
				verse_send_o_link_set(target->node.id, ~0, link->link->id, link->label, link->target_id);
				sync = 0;
			}
			n->links_local = list_unlink(n->links_local, iter);
			mem_free(link);
			list_destroy(iter);
		}
	}
	/* Look for "real" links that have been marked 'deleted', and delete any link with the same label. */
	for(i = 0; (link = dynarr_index(n->links, i)) != NULL; i++)
	{
		if(link->id == (uint16) ~0u)
			continue;
		if(link->deleted)
		{
/*			printf(" link %u with ID=%u is marked as deleted; this means any link with the same label in the target needs to go\n", i, link->id);*/
			for(j = 0; (tlink = dynarr_index(target->links, j)) != NULL; j++)
			{
				if(tlink->id == (uint16) ~0u)
					continue;
				if(strcmp(tlink->label, link->label) == 0)
				{
/*					printf(" got match in link %u.%u, pointing at %u\n", target->node.id, tlink->id, tlink->link);*/
					verse_send_o_link_destroy(target->node.id, tlink->id);
					sync = 0;
/*					printf("  delete sent\n");*/
				}
			}
			link->id = (uint16) ~0u;	/* Then mark the link as gone, so we don't send destroys twice. */
		}
	}
	/* FIXME: We should probably clean out any redundant (non-existant in local data) links, here. */
	return sync;
}

/* ----------------------------------------------------------------------------------------- */

static int vertex_deleted(const real64 *data)
{
	return fabs(data[0]) > 1e300;
}

static int polygon_deleted(const uint32 *data)
{
	return data != NULL && *data == ~0u;
}

static int sync_geometry_layer(const NodeGeometry *node, const NdbGLayer *layer,
				const NodeGeometry *target, const NdbGLayer *tlayer)
{
	const uint8	*data, *tdata;
	size_t		i, size, tsize, esize;
	int		send = 0;

/*	printf("synchronizing geometry layer '%s' against '%s'\n", layer->name, tlayer->name);*/

	/* Basically break the dynarr abstraction, for speed. */
	esize = dynarr_get_elem_size(layer->data);
	size  = dynarr_size(layer->data);
	tsize = dynarr_size(tlayer->data);
	data  = dynarr_index(layer->data, 0);
	tdata = dynarr_index(tlayer->data, 0);
/*	printf(" local geometry size: %u at %p, remote is %u at %p\n", size, data, tsize, tdata);*/
	for(i = 0; i < size; i++)
	{
		if(i >= tsize)					/* If data is not even in target, we must send it. */
			send = 1;
		else
		{
			send = memcmp(data, tdata, esize) != 0;	/* If it fits, compare to see if send needed. */
			/* If vertex layer, do more intelligent comparison for deleted vertices. */
			if(tlayer->id == 0 && tlayer->type == VN_G_LAYER_VERTEX_XYZ && tdata != NULL)
			{
				if(vertex_deleted((real64 *) data))
				{
					if(vertex_deleted((real64 *) tdata))
						send = 0;
				}
			}
			else if(tlayer->id == 1 && tlayer->type == VN_G_LAYER_POLYGON_CORNER_UINT32 && tdata != NULL)	/* And for polygon layers, too. */
			{
/*				printf("base polygon %u: local delete is %d, remote is %d, send=%d (local=%p remote=%p)\n",
				       i, polygon_deleted((uint32 *) data), polygon_deleted((uint32 *) tdata), send, data, tdata);
*/				if(polygon_deleted((uint32 *) data) && !polygon_deleted((uint32 *) tdata))
				{
/*					printf("  deleting polygon %u.%u\n", target->node.id, i);*/
					verse_send_g_polygon_delete(target->node.id, i);
					send = 0;
				}
			}
		}
/*		if(send)
			printf(" send %s %u in %u is %d\n", layer->name, i, node->node.id, send);
*/		if(send)
		{
			switch(tlayer->type)
			{
			case VN_G_LAYER_VERTEX_XYZ:
				verse_send_g_vertex_set_xyz_real64(target->node.id, tlayer->id, i,
							      ((const real64 *) data)[0],
							      ((const real64 *) data)[1], 
							      ((const real64 *) data)[2]);
				break;
			case VN_G_LAYER_VERTEX_UINT32:
				verse_send_g_vertex_set_uint32(target->node.id, tlayer->id, i, ((const uint32 *) data)[0]);
				break;
			case VN_G_LAYER_VERTEX_REAL:
				verse_send_g_vertex_set_real64(target->node.id, tlayer->id, i, ((const real64 *) data)[0]);
				break;
			case VN_G_LAYER_POLYGON_CORNER_UINT32:
				verse_send_g_polygon_set_corner_uint32(target->node.id, tlayer->id, i,
								       ((const uint32 *) data)[0],
								       ((const uint32 *) data)[1],
								       ((const uint32 *) data)[2],
								       ((const uint32 *) data)[3]);
				break;
			case VN_G_LAYER_POLYGON_CORNER_REAL:
				verse_send_g_polygon_set_corner_real64(target->node.id, tlayer->id, i,
								       ((const real64 *) data)[0],
								       ((const real64 *) data)[1],
								       ((const real64 *) data)[2],
								       ((const real64 *) data)[3]);
				break;
			case VN_G_LAYER_POLYGON_FACE_UINT8:
				verse_send_g_polygon_set_face_uint8(target->node.id, tlayer->id, i,
								       ((const uint8 *) data)[0]);
				break;
			case VN_G_LAYER_POLYGON_FACE_UINT32:
				verse_send_g_polygon_set_face_uint32(target->node.id, tlayer->id, i,
								       ((const uint32 *) data)[0]);
				break;
			case VN_G_LAYER_POLYGON_FACE_REAL:
				verse_send_g_polygon_set_face_real64(target->node.id, tlayer->id, i,
								       ((const real64 *) data)[0]);
				break;
			default:
				;
			}
		}
		data  += esize;
		tdata += esize;
	}
	if(size < tsize)	/* We have less data than the target, so delete remainder. */
	{
		send = 1;
		printf("** Target is too large, deleting -------------------------------------------------------\n");
/*		if(layer->type >= VN_G_LAYER_VERTEX_XYZ && layer->type < VN_G_LAYER_POLYGON_CORNER_UINT32)
		{
			for(i = size; i < tsize; i++)
				verse_send_g_vertex_delete_real64(target->node.id, i);
		}
*/		if(tlayer->id == 1)
		{
			printf("deleting excess polygons\n");
			for(i = size; i < tsize; i++)
				verse_send_g_polygon_delete(target->node.id, i);
		}
	}
	return send;
}

static int sync_geometry_bones(const NodeGeometry *n, const NodeGeometry *target)
{
#if 0
	unsigned int	i, sync = 1;
	NdbGBone	*b, *tb;

	for(i = 0; (b = dynarr_index(n->bones, i)) != NULL; i++)
	{
		if(b->id == (uint16) ~0u)
			continue;
		printf("looking at bone %u\n", b->id);
		if((tb = nodedb_g_bone_find_equal(target, n, b)) == NULL)
		{
			uint16	parent;

			printf("bone %u is not in target, we need to create it\n", b->id);
			if(b->pending)
			{
				printf(" we already have, so just hang on\n");
				sync = 0;
				continue;
			}
			if(b->parent == (uint16) ~0u || nodedb_g_bone_resolve(&parent, target, n, b->parent))
			{
				VNQuat64	rot;

				rot.x = b->rot[0];
				rot.y = b->rot[1];
				rot.z = b->rot[2];
				rot.w = b->rot[3];
				if(b->parent == (uint16) ~0u)
				{
					printf(" it's a root bone, so just create it\n");
					verse_send_g_bone_create(target->node.id, (uint16) ~0u, b->weight, b->reference, (uint16) ~0u,
								 b->pos[0], b->pos[1], b->pos[2], b->pos_curve,
								 &rot, b->rot_curve);
					b->pending = 1;
				}
				else
				{
					printf(" the parent exists, as %u, creating child then\n", parent);
					verse_send_g_bone_create(target->node.id, (uint16) ~0u, b->weight, b->reference, parent,
								 b->pos[0], b->pos[1], b->pos[2], b->pos_curve,
								 &rot, b->rot_curve);
					b->pending = 1;
				}
			}
			else
				printf(" the parent %u neither, hoping for it to show up soon ...\n", b->parent);
			sync = 0;
		}
		else
			printf(" already in target at %p, id=%u\n", tb, tb->id);
	}
	if(sync == 1)
	{
		/* If everything is in sync, nothing is pending, so clear the flags. */
		for(i = 0; (b = dynarr_index(n->bones, i)) != NULL; i++)
		{
			if(b->id == (uint16) ~0u)
				continue;
			b->pending = 0;
		}
		/* Do a reverse sweep, checking for bones in remote node that are not
		 * defined in Purple. If so, destroy them.
		*/
		for(i = 0; (b = dynarr_index(target->bones, i)) != NULL; i++)
		{
			if(b->id == (uint16) ~0u)
				continue;
			if(nodedb_g_bone_find_equal(n, target, b) == NULL)
			{
				verse_send_g_bone_destroy(target->node.id, b->id);
				printf(">> sync destroying target-only bone %u\n", b->id);
			}
		}
	}
	return sync;
#endif
	return 1;
}

static int sync_geometry_creases(const NodeGeometry *n, const NodeGeometry *target)
{
	int	sync = 1;

	if(target->crease_vertex.def != n->crease_vertex.def || strcmp(target->crease_vertex.layer, n->crease_vertex.layer) != 0)
	{
		verse_send_g_crease_set_vertex(target->node.id, n->crease_vertex.layer, n->crease_vertex.def);
		sync = 0;
	}
	if(target->crease_edge.def != n->crease_edge.def || strcmp(target->crease_edge.layer, n->crease_edge.layer) != 0)
	{
		printf("edge crease mismatch. have: '%s', %u -- want '%s',%u\n",
		       target->crease_edge.layer, target->crease_edge.def,
		       n->crease_edge.layer, n->crease_edge.def);
		verse_send_g_crease_set_edge(target->node.id, n->crease_edge.layer, n->crease_edge.def);
		printf(" sent '%s',%u\n", n->crease_edge.layer, n->crease_edge.def);
		sync = 0;
	}
	return sync;
}

static int sync_geometry(const NodeGeometry *n, const NodeGeometry *target)
{
	unsigned int	i, sync = 1;
	const NdbGLayer	*layer, *tlayer;

	/* Step one: see if desired layers exist in target, else create them. */
	for(i = 0; ((layer = dynarr_index(n->layers, i)) != NULL); i++)
	{
		if(layer->name[0] == '\0')
			continue;
/*		printf("does target have '%s'?\n", layer->name);*/
		if((tlayer = nodedb_g_layer_find(target, layer->name)) != NULL)
		{
/*			printf(" yes\n");*/
			if(layer->type != tlayer->type)
				printf("  but the type is wrong :/\n");
			else if(layer->def_uint != tlayer->def_uint)
				printf("  but the default integer is wrong\n");
			else if(layer->def_real != tlayer->def_real)
				printf("  but the default real is wrong\n");
			else	/* "Envelope" is fine, inspect contents. */
				sync &= !sync_geometry_layer(n, layer, target, tlayer);
		}
		else
		{
			verse_send_g_layer_create(target->node.id, ~0, layer->name, layer->type, layer->def_uint, layer->def_real);
/*			printf(" no, sync sending create of layer %s in %u\n", layer->name, target->node.id);*/
			sync = 0;
		}
	}
	/* Step two: see if target has layers that have been destroyed. */
	for(i = 0; (layer = dynarr_index(target->layers, i)) != NULL; i++)
	{
		if(layer->name[0] == '\0')
			continue;
		if(nodedb_g_layer_find(n, layer->name) == 0)
		{
			printf("sync destroying target-only geometry layer %u.%u (%s)\n", target->node.id, layer->id, layer->name);
			verse_send_g_layer_destroy(target->node.id, layer->id);
		}
	}
	/* Step three: handle any bones. */
	sync &= sync_geometry_bones(n, target);

	/* Step four: see if crease information has changed. */
	sync &= sync_geometry_creases(n, target);

	return sync;
}

/* ----------------------------------------------------------------------------------------- */

#if 0
/* Convenience function to print a material, concisely just listing fragments and links. */
static void print_material(const NodeMaterial *n)
{
	unsigned int	i;
	const NdbMFragment	*f;
	static const char	*tname[] = { "color", "light", "reflection", "transparency",
						"volume", "geometry", "texture", "noise",
						"blender", "matrix", "ramp", "animation",
						"alternative", "output" 
	};

	for(i = 0; (f = dynarr_index(n->fragments, i)) != NULL; i++)
	{
		if(f->id == (VNMFragmentID) ~0)
			continue;
		printf(" %u=%s", f->id, tname[f->type]);
		switch(f->type)
		{
		case VN_M_FT_LIGHT:
			printf("(N%u)", f->frag.light.brdf);
			break;
		case VN_M_FT_VOLUME:
			printf("(%u)", f->frag.volume.color);
			break;
		case VN_M_FT_TEXTURE:
			printf("(N%u,%u)", f->frag.texture.bitmap, f->frag.texture.mapping);
			break;
		case VN_M_FT_NOISE:
			printf("(%u)", f->frag.noise.mapping);
			break;
		case VN_M_FT_BLENDER:
			printf("(%u,%u,%u)", f->frag.blender.data_a, f->frag.blender.data_b, f->frag.blender.control);
			break;
		case VN_M_FT_MATRIX:
			printf("(%u)", f->frag.matrix.data);
			break;
		case VN_M_FT_RAMP:
			printf("(%u)", f->frag.ramp.mapping);
			break;
		case VN_M_FT_ALTERNATIVE:
			printf("(%u,%u)", f->frag.alternative.alt_a, f->frag.alternative.alt_b);
			break;
		case VN_M_FT_OUTPUT:
			printf("(%u,%u)", f->frag.output.front, f->frag.output.back);
			break;
		default:
			;	/* "Scalar" fragment. */
		}
	}
	printf("\n");
}
#endif		/* 0 */

static int sync_material(const NodeMaterial *n, const NodeMaterial *target)
{
	unsigned int	i, sync = 1, send;
	NdbMFragment	*f;

/*	printf("syncing \n");
	print_material(n);
	printf("against \n");
	print_material(target);
*/
	for(i = 0; (f = dynarr_index(n->fragments, i)) != NULL; i++)
	{
		if(f->id == (VNMFragmentID) ~0)
			continue;
/*		printf("There's a source fragment type %d, can we find it in destination?\n", f->type);*/
		if(nodedb_m_fragment_find_equal(target, n, f) == NULL)
		{
			VMatFrag	tmp;

			if(f->pending)
			{
/*				printf(" no, but fragment create has already been issued and is pending, just wait\n");*/
				sync = 0;	/* If creation is pending, we are by definition not in sync yet. */
				continue;
			}
/*			printf(" no, attempting create\n");*/
			send = 0;
			switch(f->type)
			{
			case VN_M_FT_COLOR:
				tmp.color = f->frag.color;
				send = 1;
				break;
			case VN_M_FT_LIGHT:
				if((f->node == NULL || f->node->id != (VNodeID) ~0) ||
				   f->frag.light.brdf == (VNodeID) ~0)
				{
					tmp.light.type = f->frag.light.type;
					tmp.light.normal_falloff = f->frag.light.normal_falloff;
					tmp.light.brdf = f->node != NULL ? f->node->id : (VNodeID) ~0;
					strcpy(tmp.light.brdf_r, f->frag.light.brdf_r);
					strcpy(tmp.light.brdf_g, f->frag.light.brdf_g);
					strcpy(tmp.light.brdf_b, f->frag.light.brdf_b);
					send = 1;
				}
				else if(f->node != NULL && f->node->id == (VNodeID) ~0)
					sync = 0;
				break;
			case VN_M_FT_REFLECTION:
				tmp.reflection = f->frag.reflection;
				send = 1;
				break;
			case VN_M_FT_TRANSPARENCY:
				tmp.transparency = f->frag.transparency;
				send = 1;
				break;
			case VN_M_FT_VOLUME:
				tmp.volume.diffusion = f->frag.volume.diffusion;
				tmp.volume.col_r = f->frag.volume.col_r;
				tmp.volume.col_g = f->frag.volume.col_g;
				tmp.volume.col_b = f->frag.volume.col_b;
				send = 1;
				break;
			case VN_M_FT_VIEW:
				send = 1;
				break;
			case VN_M_FT_GEOMETRY:
				tmp.geometry = f->frag.geometry;
				send = 1;
				break;
			case VN_M_FT_TEXTURE:
				if((f->node == NULL || (f->node->id != (VNodeID) ~0)) &&
				   nodedb_m_fragment_resolve(&tmp.texture.mapping, target, n, f->frag.texture.mapping))
				{
					tmp.texture.bitmap = f->node->id;
					strcpy(tmp.texture.layer_r, f->frag.texture.layer_r);
					strcpy(tmp.texture.layer_g, f->frag.texture.layer_g);
					strcpy(tmp.texture.layer_b, f->frag.texture.layer_b);
					send = 1;
				}
				else if(f->node != NULL && f->node->id == (VNodeID) ~0)
					sync = 0;
				break;
			case VN_M_FT_NOISE:
				if(nodedb_m_fragment_resolve(&tmp.noise.mapping, target, n, f->frag.noise.mapping))
				{
					tmp.noise.type = f->frag.noise.type;
					send = 1;
				}
				break;
			case VN_M_FT_BLENDER:
				if(nodedb_m_fragment_resolve(&tmp.blender.data_a, target, n, f->frag.blender.data_a)
				   && nodedb_m_fragment_resolve(&tmp.blender.data_b, target, n, f->frag.blender.data_b)
				   && nodedb_m_fragment_resolve(&tmp.blender.control, target, n, f->frag.blender.control))
				{
					tmp.blender.type = f->frag.blender.type;
					send = 1;
				}
				break;
			case VN_M_FT_CLAMP:
				if(nodedb_m_fragment_resolve(&tmp.clamp.data, target, n, f->frag.clamp.data))
				{
					tmp.clamp.min   = f->frag.clamp.min;
					tmp.clamp.red   = f->frag.clamp.red;
					tmp.clamp.green = f->frag.clamp.green;
					tmp.clamp.blue  = f->frag.clamp.blue;
					send = 1;
				}
				break;
			case VN_M_FT_MATRIX:
				if(nodedb_m_fragment_resolve(&tmp.matrix.data, target, n, f->frag.matrix.data))
				{
					memcpy(tmp.matrix.matrix, f->frag.matrix.matrix, sizeof tmp.matrix.matrix);
					send = 1;
				}
				break;
			case VN_M_FT_RAMP:
				if(nodedb_m_fragment_resolve(&tmp.ramp.mapping, target, n, f->frag.ramp.mapping))
				{
					tmp.ramp.type = f->frag.ramp.type;
					tmp.ramp.channel = f->frag.ramp.channel;
					tmp.ramp.point_count = f->frag.ramp.point_count;
					memcpy(tmp.ramp.ramp, f->frag.ramp.ramp, tmp.ramp.point_count * sizeof *f->frag.ramp.ramp);
					send = 1;
				}
				break;
			case VN_M_FT_ANIMATION:
				tmp.animation = f->frag.animation;
				send = 1;
				break;
			case VN_M_FT_ALTERNATIVE:
				send = nodedb_m_fragment_resolve(&tmp.alternative.alt_a, target, n, f->frag.alternative.alt_a)
					&& nodedb_m_fragment_resolve(&tmp.alternative.alt_b, target, n, f->frag.alternative.alt_b);
				break;
			case VN_M_FT_OUTPUT:
				if(nodedb_m_fragment_resolve(&tmp.output.front, target, n, f->frag.output.front) &&
				   nodedb_m_fragment_resolve(&tmp.output.back,  target, n, f->frag.output.back))
				{
					strcpy(tmp.output.label, f->frag.output.label);
					send = 1;
				}
				break;
			}
			if(send)
			{
/*				printf("SYNC>>> sync sending create of type %d mat frag in %u\n", f->type, target->node.id);*/
				verse_send_m_fragment_create(target->node.id, ~0, f->type, &tmp);
				f->pending = 1;
				sync = 0;
			}
			break;
		}
	}
	if(sync == 1)	/* Once sync is (re)established, clear all pending flags. */
	{
		for(i = 0; (f = dynarr_index(n->fragments, i)) != NULL; i++)
		{
			if(f->id == (VNMFragmentID) ~0)
				continue;
			f->pending = 0;
		}

		/* Do a "reverse sync" sweep; checking if there are fragments in the server-side
		 * node that are not in fact present in the Purple local one. If so, destroy them.
		*/
		for(i = 0; (f = dynarr_index(target->fragments, i)) != NULL; i++)
		{
			if(f->id == (VNMFragmentID) ~0)
				continue;
			if(!nodedb_m_fragment_find_equal(n, target, f))
			{
				verse_send_m_fragment_destroy(target->node.id, f->id);
				printf(">> sync destroying material fragment %u.%u\n", target->node.id, f->id);
			}
		}
	}
	return sync;
}

/* ----------------------------------------------------------------------------------------- */

static int sync_bitmap_dimensions(const NodeBitmap *n, const NodeBitmap *target)
{
	if(n->width != target->width || n->height != target->height || n->depth != target->depth)
	{
		verse_send_b_dimensions_set(target->node.id, n->width, n->height, n->depth);
		return 0;
	}
	return 1;
}

static int sync_bitmap_layer(const NodeBitmap *n, const NdbBLayer *layer,
			     const NodeBitmap *target, const NdbBLayer *tlayer)
{
	uint16		x, y, z, hit, wit, line, send;
	NdbBTileDesc	tile, ttile;
	int		sync = 1;

	if(layer->type != tlayer->type)
		return 1;		/* FIXME: This obviously isn't the proper thing to do. */

/*	printf("syncing '%s' layers at %p and %p\n", layer->name, layer, tlayer);*/
	wit = (n->width  + VN_B_TILE_SIZE - 1) / VN_B_TILE_SIZE;
	hit = (n->height + VN_B_TILE_SIZE - 1) / VN_B_TILE_SIZE;
	for(z = 0; z < n->depth; z++)
	{
		tile.in.z = ttile.in.z = z;
		for(y = 0; y < hit; y++)
		{
			tile.in.y = ttile.in.y = y;
			for(x = 0; x < wit; x++)
			{
				tile.in.x = ttile.in.x = x;
				nodedb_b_tile_describe(n, layer, &tile);
				nodedb_b_tile_describe(target, tlayer, &ttile);
				send = 0;
				if(ttile.out.ptr == NULL)
					send = 1;
				else
				{
					const uint8	*p1 = tile.out.ptr, *p2 = ttile.out.ptr;

					if(layer->type == VN_B_LAYER_UINT1)
					{
						tile.out.width = (tile.out.width + 7) / 8;
						ttile.out.width = (ttile.out.width + 7) / 8;
					}
					for(line = 0;
					    line < tile.out.height;
					    line++, p1 += tile.out.mod_row, p2 += ttile.out.mod_row)
					{
						if(memcmp(p1, p2, tile.out.width_bytes) != 0)
						{
							send = 1;
							break;
						}
					}
				}
				if(send)
				{
					VNBTile	out;
					uint8	*put = (uint8 *) &out;

					for(line = 0;
					    line < tile.out.height;
					    line++, put += tile.out.mod_tile, tile.out.ptr += tile.out.mod_row)
						memcpy(put, tile.out.ptr, tile.out.width_bytes);
					verse_send_b_tile_set(target->node.id, tlayer->id, x, y, z, tlayer->type, &out);
					sync = 0;
				}
			}
		}
	}
	return sync;
}

static int sync_bitmap(const NodeBitmap *n, const NodeBitmap *target)
{
	unsigned int	i, sync = 1;
	const NdbBLayer	*layer, *tlayer;

	if(!sync_bitmap_dimensions(n, target))
		return 0;
	for(i = 0; ((layer = dynarr_index(n->layers, i)) != NULL); i++)
	{
		if(layer->name[0] == '\0')
			continue;
		if((tlayer = nodedb_b_layer_find(target, layer->name)) != NULL)
		{
			if(layer->type != tlayer->type)
				printf(" bitmap layer type mismatch\n");
			else
				sync &= sync_bitmap_layer(n, layer, target, tlayer);
		}
		else
		{
			verse_send_b_layer_create(target->node.id, ~0, layer->name, layer->type);
			printf(" sync sending create of bitmap layer %s in %u\n", layer->name, target->node.id);
			sync = 0;
		}
	}
	for(i = 0; (layer = dynarr_index(target->layers, i)) != NULL; i++)
	{
		if(layer->name[0] == '\0')
			continue;
		if(nodedb_b_layer_find(n, layer->name) == NULL)
		{
			printf("sync destroying target-only bitmap layer %u.%u (%s)\n", target->node.id, layer->id, layer->name);
			verse_send_b_layer_destroy(target->node.id, layer->id);
		}
	}
	return sync;
}

/* ----------------------------------------------------------------------------------------- */

static int sync_text_buffer(const NodeText *n, const NdbTBuffer *buffer,
			    const NodeText *target, const NdbTBuffer *tbuffer)
{
	int		sync = 1, d;
	DynArr		*edit;
	DiffEdit	*ed;
	const char	*text, *ttext;
	size_t		len, tlen;

	text  = textbuf_text(buffer->text);
	len   = textbuf_length(buffer->text);
	ttext = textbuf_text(tbuffer->text);
	tlen  = textbuf_length(tbuffer->text);
	if(len == tlen && strcmp(text, ttext) == 0)	/* Avoid allocating memory and running diff if equal. */
		return sync;

	edit  = dynarr_new(sizeof (*ed), 8);

/*	printf("  text: '%s' (%u)\n", text, len);
	printf("target: '%s' (%u)\n", ttext, tlen);
*/	d = diff_compare_simple(ttext, tlen, text, len, edit);
/*	printf("Edit distance: %d\n", d);*/
	if(d > 0)
	{
		unsigned int	i, pos = 0;

		for(i = 0; (ed = dynarr_index(edit, i)) != NULL; i++)
		{
			if(ed->op == DIFF_MATCH)
			{
				pos = ed->off + ed->len;
			}
			else if(ed->op == DIFF_DELETE)
			{
				verse_send_t_text_set(target->node.id, tbuffer->id, ed->off, ed->len, NULL);
				pos = ed->off;
			}
			else if(ed->op == DIFF_INSERT)	/* Split inserts into something Verse can handle. */
			{
				char	temp[1024];
				size_t	off, chunk;

				for(off = ed->off, len = ed->len; len > 0; off += chunk, len -= chunk)
				{
					chunk = len > sizeof temp - 1 ? sizeof temp - 1 : len;
					stu_strncpy(temp, chunk + 1, text + off);
					temp[chunk] = '\0';
					verse_send_t_text_set(target->node.id, tbuffer->id, pos, 0, temp);
					pos += chunk - 1;
				}
			}
		}
		sync = 0;
	}
	dynarr_destroy(edit);
	return sync;
}

/* Alter <target> so it becomes copy of <n>. */
static int sync_text(const NodeText *n, const NodeText *target)
{
	unsigned int		i, sync = 1;
	const NdbTBuffer	*buffer, *tbuffer;

	for(i = 0; (buffer = dynarr_index(n->buffers, i)) != NULL; i++)
	{
		if(buffer->name[0] == '\0')
			continue;
		if((tbuffer = nodedb_t_buffer_find(target, buffer->name)) != NULL)
			sync &= sync_text_buffer(n, buffer, target, tbuffer);
		else
		{
			printf(" sync sending create of text buffer '%s' in %u\n", buffer->name, target->node.id);
			verse_send_t_buffer_create(target->node.id, ~0, buffer->name);
			sync = 0;
		}
	}
	/* Do a "reverse sync" sweep, i.e. check target for buffers not in local node, and delete them. */
	for(i = 0; (buffer = dynarr_index(target->buffers, i)) != NULL; i++)
	{
		if(buffer->name[0] == '\0')
			continue;
		if(nodedb_t_buffer_find(n, buffer->name) == NULL)
			verse_send_t_buffer_destroy(target->node.id, buffer->id);
	}
	return sync;
}

/* ----------------------------------------------------------------------------------------- */

static int sync_curve_curve(const NodeCurve *n, const NdbCCurve *curve,
			    const NodeCurve *target, const NdbCCurve *tcurve)
{
	unsigned int	i, sync = 1;
	const NdbCKey	*key, *tkey;

	for(i = 0; (key = dynarr_index(curve->keys, i)) != NULL; i++)
	{
		if(key->pos == V_REAL64_MAX)
			continue;
		if((tkey = nodedb_c_curve_key_find(tcurve, key->pos)) != NULL)
		{
			if(!nodedb_c_curve_key_equal(tcurve, key, tkey))
			{
				verse_send_c_key_set(target->node.id, tcurve->id, tkey->id, curve->dimensions,
						     (real64 *) key->pre.value, (uint32 *) key->pre.pos,
						     (real64 *) key->value, key->pos,
						     (real64 *) key->post.value, (uint32 *) key->post.pos);
				sync = 0;
			}
		}
		else
		{
			printf("sync sending create of key at %g\n", key->pos);
			verse_send_c_key_set(target->node.id, tcurve->id, ~0, curve->dimensions,
					     (real64 *) key->pre.value, (uint32 *) key->pre.pos,
					     (real64 *) key->value, key->pos,
					     (real64 *) key->post.value, (uint32 *) key->post.pos);
			sync = 0;
		}
	}
	/* FIXME: Scan for destroyed keys, too. */
	return sync;
}

/* Synchronize curve node <target> into copy of <n>. */
static int sync_curve(const NodeCurve *n, const NodeCurve *target)
{
	unsigned int	i, sync = 1;
	const NdbCCurve	*curve, *tcurve;

	for(i = 0; (curve = dynarr_index(n->curves, i)) != NULL; i++)
	{
		if(curve->name[0] == '\0')
			continue;
		if((tcurve = nodedb_c_curve_find(target, curve->name)) != NULL)
			sync &= sync_curve_curve(n, curve, target, tcurve);
		else
		{
			printf("sync sending create of curve '%s' in %u\n", curve->name, target->node.id);
			verse_send_c_curve_create(target->node.id, ~0, curve->name, curve->dimensions);
			sync = 0;
		}
	}
	for(i = 0; (curve = dynarr_index(target->curves, i)) != NULL; i++)
	{
		if(curve->name[0] == '\0')
			continue;
		if(nodedb_c_curve_find(n, curve->name) == NULL)
		{
			printf("sync destroying target-only curve %u.%u (%s)\n", target->node.id, curve->id, curve->name);
			verse_send_c_curve_destroy(target->node.id, curve->id);
		}
	}
	return sync;
}

/* ----------------------------------------------------------------------------------------- */

static int sync_audio_buffer(const NodeAudio *n, const NdbABuffer *buffer,
			    const NodeAudio *target, const NdbABuffer *tbuffer)
{
	unsigned int	sync = 1, index;
	BinTreeIter	iter;
	const NdbABlk	*blk, *tblk;

	printf("syncing audio buffer %s\n", buffer->name);
	for(bintree_iter_init(buffer->blocks, &iter); bintree_iter_valid(iter); bintree_iter_next(&iter))
	{
		index = (unsigned int) bintree_iter_key(iter);
		blk = bintree_iter_element(iter);
		if((tblk = bintree_lookup(tbuffer->blocks, (void *) index)) != NULL)
		{
			if(!nodedb_a_blocks_equal(buffer->type, blk, tblk))
			{
/*				printf(" Block %u differs, sending new version\n", index);*/
				verse_send_a_block_set(target->node.id, tbuffer->id, index, tbuffer->type, blk->data);
				sync = 0;
			}
		}
		else
		{
/*			printf(" Target node missing block %u, setting it\n", index);*/
			verse_send_a_block_set(target->node.id, tbuffer->id, index, tbuffer->type, blk->data);
		}
	}
	printf(" block(s) sent\n");
	return sync;
}

static int sync_audio(const NodeAudio *n, const NodeAudio *target)
{
	unsigned int	i, sync = 1;
	const NdbABuffer	*buffer, *tbuffer;

	for(i = 0; (buffer = dynarr_index(n->buffers, i)) != NULL; i++)
	{
		if(buffer->name[0] == '\0')
			continue;
		if((tbuffer = nodedb_a_buffer_find(target, buffer->name)) != NULL)
		{
			printf("buffer: type=%d freq=%g  target: type=%d freq=%g\n",
			       buffer->type, buffer->frequency,
			       tbuffer->type, tbuffer->frequency);
			if(buffer->type == tbuffer->type && buffer->frequency == tbuffer->frequency)
				sync &= sync_audio_buffer(n, buffer, target, tbuffer);
			else
				printf("can't sync mismatched (type/freq) audio buffers!\n");	/* FIXME: Do it. */
		}
		else
		{
			printf("sync sending create of buffer '%s' in %u\n", buffer->name, target->node.id);
			verse_send_a_buffer_create(target->node.id, ~0, buffer->name, buffer->type, buffer->frequency);
			sync = 0;
		}
	}
	return sync;
}

/* ----------------------------------------------------------------------------------------- */

static int sync_node(PNode *n)
{
	PNode	*target;
	int	sync = 1;

	if((target = nodedb_lookup(n->id)) == NULL)
	{
		LOG_WARN(("Couldn't look up existing (target) node for %u--aborting sync", n->id));
		return 0;
	}
	/* First sync node-head data, such as name and tags. */
	sync &= sync_head(n, target);
	switch(n->type)
	{
	case V_NT_OBJECT:
		sync &= sync_object((NodeObject *) n, (NodeObject *) target);
		break;
	case V_NT_GEOMETRY:
		sync &= sync_geometry((NodeGeometry *) n, (NodeGeometry *) target);
		break;
	case V_NT_MATERIAL:
		sync &= sync_material((NodeMaterial *) n, (NodeMaterial *) target);
		break;
	case V_NT_BITMAP:
		sync &= sync_bitmap((NodeBitmap *) n, (NodeBitmap *) target);
		break;
	case V_NT_TEXT:
		sync &= sync_text((NodeText *) n, (NodeText *) target);
		break;
	case V_NT_CURVE:
		sync &= sync_curve((NodeCurve *) n, (NodeCurve *) target);
		break;
	case V_NT_AUDIO:
		sync &= sync_audio((NodeAudio *) n, (NodeAudio *) target);
		break;
	default:
		printf("Can't sync node of type %d\n", n->type);
	}
	return sync;
}

/* ----------------------------------------------------------------------------------------- */

void sync_node_add(PNode *node)
{
	if(node == NULL)
		return;
	if(node->sync.busy)
	{
/*		printf("not adding node %u to synchronizer; it's already being synchronized\n", node->id);*/
		return;
	}
	timeval_jurassic(&node->sync.last_send);
	if(node->id == (VNodeID) ~0)	/* Locally created? */
		sync_info.queue_create = list_prepend(sync_info.queue_create, (void *) node);
	else
		sync_info.queue_sync = list_prepend(sync_info.queue_sync, (void *) node);
	nodedb_ref(node);	/* We've added a reference to the node. */
	node->sync.busy = 1;
}

void sync_update(double slice)
{
	List	*iter, *next;
	PNode	*n;
	TimeVal	now;

	/* Create nodes that need to be created. */
	for(iter = sync_info.queue_create; iter != NULL; iter = next)
	{
		next = list_next(iter);
		n = list_data(iter);
		verse_send_node_create(~0, n->type, 0);
		/* Move node from "to create" to "pending" list; no change in refcount. */
		sync_info.queue_create = list_unlink(sync_info.queue_create, iter);
		list_destroy(iter);
		sync_info.queue_create_pend = list_prepend(sync_info.queue_create_pend, n);
	}

	/* Synchronize existing nodes. */
	timeval_now(&now);
	for(iter = sync_info.queue_sync; iter != NULL; iter = next)
	{
		PNode	*n = list_data(iter);

		next = list_next(iter);

		if(timeval_elapsed(&n->sync.last_send, &now) < 0.1)
			continue;
		n->sync.last_send = now;
		if(sync_node(n))
		{
			sync_info.queue_sync = list_unlink(sync_info.queue_sync, iter);
			printf("removing node %u from sync queue, it's in sync\n", n->id);
			nodedb_unref(n);
			list_destroy(iter);
			n->sync.busy = 0;
		}
	}
}
