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
			break;
		}
	}
}

void sync_init(void)
{
	nodedb_notify_add(NODEDB_OWNERSHIP_MINE, cb_notify);
}

/* ----------------------------------------------------------------------------------------- */

static int sync_geometry_layer(const NodeGeometry *node, const NdbGLayer *layer, size_t size,
				const NodeGeometry *target, const NdbGLayer *tlayer, size_t tsize)
{
	const unsigned char	*data, *tdata;
	size_t		i, esize;
	int		send = 0;

	esize = tlayer->type == VN_G_LAYER_VERTEX_XYZ ? (3 * sizeof (real64)) : (4 * sizeof (uint32));

	data  = dynarr_index(layer->data, 0);
	tdata = dynarr_index(tlayer->data, 0);
	for(i = 0; i < size; i++)
	{
		if(i >= tsize)					/* If data is not even in target, we must send it. */
			send = TRUE;
		else
			send = memcmp(data, tdata, esize) != 0;	/* If it fits, compare to see if send needed. */
		if(send)
		{
			switch(tlayer->type)
			{
			case VN_G_LAYER_VERTEX_XYZ:
				verse_send_g_vertex_set_real64_xyz(target->node.id, tlayer->id, i,
							      ((real64 *) data)[0],
							      ((real64 *) data)[1], 
							      ((real64 *) data)[2]);
				break;
			case VN_G_LAYER_POLYGON_CORNER_UINT32:
				verse_send_g_polygon_set_corner_uint32(target->node.id, tlayer->id, i,
								       ((uint32 *) data)[0],
								       ((uint32 *) data)[1],
								       ((uint32 *) data)[2],
								       ((uint32 *) data)[3]);
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
			{
				size_t	len, tlen;

				/* Vertex data must be identical for this to pass; order is significant. */
				len  = dynarr_size(layer->data);
				tlen = dynarr_size(tlayer->data);
				sync &= !sync_geometry_layer(n, layer, len, target, tlayer, tlen);
			}
		}
		else
		{
			verse_send_g_layer_create(target->node.id, ~0, layer->name, layer->type, layer->def_uint, layer->def_real);
			printf(" no, we need to create it\n");
			sync = 0;
		}
	}
	return sync;
}

static int sync_node(Node *n)
{
	Node	*target;

	if((target = nodedb_lookup(n->id)) == NULL)
	{
		LOG_WARN(("Couldn't look up existing (target) node for %u--aborting sync", n->id));
		return 0;
	}
	if(n->type == V_NT_GEOMETRY)
		return sync_geometry((NodeGeometry *) n, (NodeGeometry *) target);
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
	nodedb_ref(node);	/* We've added a refernce to the node. */
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
