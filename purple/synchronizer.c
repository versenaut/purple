/*
 * Main node synchronization pipeline implementation. Basically, maintain lists of nodes that
 * are known to have changed in some way, and compare each node to its corresponding "input
 * version" held in the nodedb. Then, if differences are found, send Verse commands to make
 * them go away. :)
*/

#include <stdio.h>
#include <string.h>

#include "verse.h"

#include "dynarr.h"
#include "list.h"
#include "log.h"
#include "mem.h"
#include "plugins.h"
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
			printf("node at %p has host-side ID %u\n", n, n->id);
			break;
		}
	}
}

void sync_init(void)
{
	nodedb_notify_add(NODEDB_OWNERSHIP_MINE, cb_notify);
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
		if(link->link->id == ~0)
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
		if((tlayer = nodedb_g_layer_lookup(target, layer->name)) != NULL)
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
	/* Step two: see if crease information has changed. */
	sync &= sync_geometry_creases(n, target);

	return sync;
}

/* ----------------------------------------------------------------------------------------- */

static int sync_bitmap_dimensions(const NodeBitmap *n, const NodeBitmap *target)
{
	if(n->width != target->width || n->height != target->height || n->depth != target->depth)
	{
		printf("setting dimensions of node %u\n", target->node.id);
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

static int sync_bitmap(NodeBitmap *n, const NodeBitmap *target)
{
	unsigned int	i, sync = 1;
	const NdbBLayer	*layer, *tlayer;

	if(!sync_bitmap_dimensions(n, target))
		return 0;
	for(i = 0; ((layer = dynarr_index(n->layers, i)) != NULL); i++)
	{
		if(layer->name[0] == '\0')
			continue;
		if((tlayer = nodedb_b_layer_lookup(target, layer->name)) != NULL)
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
	return sync;
}

/* ----------------------------------------------------------------------------------------- */

static int sync_node(Node *n)
{
	Node	*target;

	if((target = nodedb_lookup(n->id)) == NULL)
	{
		LOG_WARN(("Couldn't look up existing (target) node for %u--aborting sync", n->id));
		return 0;
	}
	switch(n->type)
	{
	case V_NT_OBJECT:
		return sync_object((NodeObject *) n, (NodeObject *) target);
	case V_NT_GEOMETRY:
		return sync_geometry((NodeGeometry *) n, (NodeGeometry *) target);
	case V_NT_BITMAP:
		return sync_bitmap((NodeBitmap *) n, (NodeBitmap *) target);
	default:
		printf("Can't sync node of type %d\n", n->type);
	}
	return 0;
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
