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

#include "synchronizer.h"

/* ----------------------------------------------------------------------------------------- */

static struct
{
	List	*queue_create;
	List	*queue_create_pend;
	List	*queue_sync;
} sync_info;

/* ----------------------------------------------------------------------------------------- */

static void cb_notify(Node *node, NodeNotifyEvent ev)
{
	List	*iter, *next;

	if(ev != NODEDB_NOTIFY_CREATE)
		return;
	for(iter = sync_info.queue_create_pend; iter != NULL; iter = next)
	{
		Node	*n;

		next = list_next(iter);
		n = list_data(iter);
		if(n->type == node->type)
		{
			sync_info.queue_create_pend = list_unlink(sync_info.queue_create_pend, iter);
			list_destroy(iter);
			n->id = node->id;	/* To-sync copy now has known ID. Excellent. */
			sync_info.queue_sync = list_prepend(sync_info.queue_sync, n);	/* Re-add to other queue. */
			LOG_MSG(("Node at %p has host-side ID %u", n, n->id));
			break;
		}
	}
}

void sync_init(void)
{
	nodedb_notify_add(NODEDB_OWNERSHIP_MINE, cb_notify);
}

/* ----------------------------------------------------------------------------------------- */

static int sync_head_tags(const Node *n, const Node *target)
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
static int sync_head(const Node *n, const Node *target)
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

static int sync_object(NodeObject *n, const NodeObject *target)
{
	int	sync = 1;
	List	*iter, *next;

	/* Synchronize light source information. */
	if(n->light[0] != target->light[0] || n->light[1] != target->light[1] || n->light[2] != target->light[2])
	{
		verse_send_o_light_set(target->node.id, n->light[0], n->light[1], n->light[2]);
		sync = 0;
	}
	/* Look through local (non-synced) links. */
/*	printf("syncing %u links in object %u\n", list_length(n->links_local), n->node.id);*/
	for(iter = n->links_local; iter != NULL; iter = next)
	{
		NdbOLinkLocal	*link = list_data(iter);

		next = list_next(iter);
		if(link->link->id == (uint16) ~0)
		{
/*			printf("can't sync link '%s', target not known yet\n", link->label);*/
			sync = 0;
		}
		else
		{
/*			printf("can sync link '%s', target is %u\n", link->label, link->link->id);*/
			if(!object_link_exists(target, link->link->id, link->label, link->target_id))
				verse_send_o_link_set(target->node.id, ~0, link->link->id, link->label, link->target_id);
/*			else
				printf("link exists, doing nothing\n");
*/			n->links_local = list_unlink(n->links_local, iter);
			mem_free(link);
			list_destroy(iter);
		}
	}
	return sync;
}

/* ----------------------------------------------------------------------------------------- */

static int sync_geometry_layer(const NodeGeometry *node, const NdbGLayer *layer,
				const NodeGeometry *target, const NdbGLayer *tlayer)
{
	const uint8	*data, *tdata;
	size_t		i, size, tsize, esize;
	int		send = 0;

	/* Basically break the dynarr abstraction, for speed. */
	esize = dynarr_get_elem_size(layer->data);
	size  = dynarr_size(layer->data);
	tsize = dynarr_size(tlayer->data);
	data  = dynarr_index(layer->data, 0);
	tdata = dynarr_index(tlayer->data, 0);
	for(i = 0; i < size; i++)
	{
		if(i >= tsize)					/* If data is not even in target, we must send it. */
			send = TRUE;
		else
			send = memcmp(data, tdata, esize) != 0;	/* If it fits, compare to see if send needed. */
/*		if(send)
			printf(" send %s %u in %u is %d\n", layer->name, i, node->node.id, send);
*/		if(send)
		{
			switch(tlayer->type)
			{
			case VN_G_LAYER_VERTEX_XYZ:
				verse_send_g_vertex_set_real64_xyz(target->node.id, tlayer->id, i,
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
		printf(" target is too large, deleting\n");
		if(layer->type >= VN_G_LAYER_VERTEX_XYZ && layer->type < VN_G_LAYER_POLYGON_CORNER_UINT32)
		{
			for(i = size; i < tsize; i++)
				verse_send_g_vertex_delete_real64(target->node.id, i);
		}
		else
		{
			for(i = size; i < tsize; i++)
				verse_send_g_polygon_delete(target->node.id, i);
		}
	}
	return send;
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
			printf(" sync sending create of layer %s in %u\n", layer->name, target->node.id);
			sync = 0;
		}
	}
	/* Step two: see if target has layers that have been destroyed. */

	/* Step three: see if crease information has changed. */
	sync &= sync_geometry_creases(n, target);

	return sync;
}

/* ----------------------------------------------------------------------------------------- */

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
				if(nodedb_m_fragment_resolve(&tmp.volume.color, target, n, f->frag.volume.color))
				{
					tmp.volume.diffusion = f->frag.volume.diffusion;
					tmp.volume.col_r = f->frag.volume.col_r;
					tmp.volume.col_g = f->frag.volume.col_g;
					tmp.volume.col_b = f->frag.volume.col_b;
					send = 1;
				}
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

					for(line = 0;
					    line < tile.out.height;
					    line++, p1 += tile.out.mod_row, p2 += ttile.out.mod_row)
					{
						if(memcmp(p1, p2, tile.out.width) != 0)
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
						memcpy(put, tile.out.ptr, tile.out.width);
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
	/* FIXME: Scan for destroyed layers, too. */
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
			verse_send_t_buffer_create(target->node.id, ~0, 0, buffer->name);
			sync = 0;
		}
	}
	/* FIXME: Scan for destroyed buffers, too. */
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
						     key->pre.value, key->pre.pos,
						     key->value, key->pos,
						     key->post.value, key->post.pos);
				sync = 0;
			}
		}
		else
		{
			printf("sync sending create of key at %g\n", key->pos);
			verse_send_c_key_set(target->node.id, tcurve->id, ~0, curve->dimensions,
					     key->pre.value, key->pre.pos,
					     key->value, key->pos,
					     key->post.value, key->post.pos);
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
	/* FIXME: Scan for destroyed curves, too. */
	return sync;
}

/* ----------------------------------------------------------------------------------------- */

static int sync_node(Node *n)
{
	Node	*target;
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
	default:
		printf("Can't sync node of type %d\n", n->type);
	}
	return sync;
}

/* ----------------------------------------------------------------------------------------- */

void sync_node_add(Node *node)
{
	if(node == NULL)
		return;
	if(node->id == ~0)	/* Locally created? */
		sync_info.queue_create = list_prepend(sync_info.queue_create, (void *) node);
	else
		sync_info.queue_sync = list_prepend(sync_info.queue_sync, (void *) node);
	nodedb_ref(node);	/* We've added a reference to the node. */
	printf("node at %p (%u) added to synchronizing system (ref=%d)\n", node, node->id, node->ref);
}

void sync_update(double slice)
{
	List	*iter, *next;
	Node	*n;

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
	for(iter = sync_info.queue_sync; iter != NULL; iter = next)
	{
		Node	*n = list_data(iter);

		next = list_next(iter);

		if(sync_node(n))
		{
			sync_info.queue_sync = list_unlink(sync_info.queue_sync, iter);
			printf("removing node %u from sync queue, it's in sync\n", n->id);
			nodedb_unref(n);
			list_destroy(iter);
		}
	}
}
