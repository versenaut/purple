/*
 * nodedb.c
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
#include "hash.h"
#include "list.h"
#include "log.h"
#include "memchunk.h"
#include "strutil.h"
#include "textbuf.h"

#include "nodedb.h"
#include "nodedb-internal.h"

/* ----------------------------------------------------------------------------------------- */

/* A node holds a list of these. */
typedef struct
{
	void	(*callback)(Node *node, NodeNotifyEvent ev, void *user);
	void	*user;
} NotifyInfo;

static struct
{
	VNodeID		avatar;
	MemChunk	*chunk_node[V_NT_NUM_TYPES];
	Hash		*nodes;				/* Probably not efficient enough, but a start. */
	Hash		*nodes_mine;			/* Duplicate links to nodes owned by this client. */

	List		*notify_mine;
	MemChunk	*chunk_notify;			/* For allocating NotifyInfos. */
} nodedb_info = { 0 };

/* ----------------------------------------------------------------------------------------- */

void nodedb_register(Node *n)
{
	hash_insert(nodedb_info.nodes, (void *) n->id, n);
}

Node * nodedb_lookup(VNodeID node_id)
{
	return hash_lookup(nodedb_info.nodes, (const void *) node_id);
}

static int cb_lookup_name(const void *data, void *user)
{
	if(strcmp(((const Node *) data)->name, ((void **) user)[0]) == 0)
	{
		((void **) user)[1] = (void *) data;
		return 0;
	}
	return 1;
}

Node * nodedb_lookup_by_name(const char *name)
{
	void	*data[2] = { (void *) name, NULL };	/* I'm too lazy for a struct. */
	/* FIXME: This is completely inefficent. */
	hash_foreach(nodedb_info.nodes, cb_lookup_name, data);
	return data[1];
}

Node * nodedb_lookup_with_type(VNodeID node_id, VNodeType type)
{
	Node	*n;

	if((n = nodedb_lookup(node_id)) != NULL && n->type == type)
		return n;
	return NULL;
}
			       
NodeObject * nodedb_lookup_object(VNodeID node_id)
{
	Node	*n;

	if((n = nodedb_lookup(node_id)) != NULL)
		return n->type == V_NT_OBJECT ? (NodeObject *) n : NULL;
	return NULL;
}

NodeText * nodedb_lookup_text(VNodeID node_id)
{
	Node	*n;

	if((n = nodedb_lookup(node_id)) != NULL)
		return n->type == V_NT_TEXT ? (NodeText *) n : NULL;
	return NULL;
}

/* ----------------------------------------------------------------------------------------- */

void nodedb_internal_notify_mine_check(Node *n, NodeNotifyEvent ev)
{
	if(nodedb_info.notify_mine != NULL/* && (n->owner == VN_OWNER_MINE)*/)	/* FIXME: Server needs fixing. */
	{
		const List	*iter;

		for(iter = nodedb_info.notify_mine; iter != NULL; iter = list_next(iter))
			((void (*)(Node *node, NodeNotifyEvent e)) list_data(iter))(n, ev);
	}
}

void nodedb_internal_notify_node_check(Node *n, NodeNotifyEvent ev)
{
	const List	*iter;
	const NotifyInfo *ni;

	for(iter = n->notify; iter != NULL; iter = list_next(iter))
	{
		ni = list_data(iter);
		ni->callback(n, ev, ni->user);
	}
}

/* ----------------------------------------------------------------------------------------- */

Node * nodedb_new(VNodeType type)
{
	MemChunk	*ch;

	if((ch = nodedb_info.chunk_node[type]) != NULL)
	{
		Node	*n = (Node *) memchunk_alloc(ch);

		n->ref	      = 0;	/* Caller must ref() immediately. */
		n->id         = ~0U;	/* Identify as locally created (non-host-approved) node. */
		n->type       = type;
		n->name[0]    = '\0';
		n->owner      = 0U;
		n->tag_groups = NULL;
		n->notify     = NULL;

		switch(n->type)
		{
		case V_NT_BITMAP:
			nodedb_b_construct((NodeBitmap *) n);
			break;
		case V_NT_GEOMETRY:
			nodedb_g_construct((NodeGeometry *) n);
			break;
		case V_NT_OBJECT:
			nodedb_o_construct((NodeObject *) n);
			break;
		case V_NT_TEXT:
			nodedb_t_construct((NodeText *) n);
			break;
		case V_NT_CURVE:
			nodedb_c_construct((NodeCurve *) n);
			break;
		default:
			LOG_WARN(("Missing node-specific init code for type %d", type));
		}
		return n;
	}
	return NULL;
}

static void cb_copy_tag_group(void *d, const void *s, void *user)
{
	const NdbTagGroup	*src = s;
	NdbTagGroup		*dst = d;

	strcpy(dst->name, src->name);
	dst->tags = dynarr_new_copy(src->tags, NULL, NULL);	/* Tags are memcpy():able, let dynarr do it. */
}

Node * nodedb_new_copy(const Node *src)
{
	Node	*n;

	if(src == NULL)
		return NULL;
	if((n = nodedb_new(src->type)) != NULL)
	{
		n->id = src->id;
		n->type = src->type;
		strcpy(n->name, src->name);
		n->owner = src->owner;
		n->tag_groups = dynarr_new_copy(src->tag_groups, cb_copy_tag_group, NULL);
		switch(n->type)
		{
		case V_NT_BITMAP:
			nodedb_b_copy((NodeBitmap *) n, (const NodeBitmap *) src);
			break;
		case V_NT_GEOMETRY:
			nodedb_g_copy((NodeGeometry *) n, (const NodeGeometry *) src);
			break;
		case V_NT_OBJECT:
			nodedb_o_copy((NodeObject *) n, (const NodeObject *) src);
			break;
		case V_NT_TEXT:
			nodedb_t_copy((NodeText *) n, (const NodeText *) src);
			break;
		case V_NT_CURVE:
			nodedb_c_copy((NodeCurve *) n, (const NodeCurve *) src);
			break;
		default:
			LOG_WARN(("Node copy not implemented for type %d\n", n->type));
		}
	}
	return n;
}

void nodedb_destroy(Node *n)
{
	MemChunk	*ch;

	if(n == NULL)
		return;

	if((ch = nodedb_info.chunk_node[n->type]) != NULL)
	{
		unsigned int	i, num;

		switch(n->type)
		{
		case V_NT_BITMAP:
			nodedb_b_destruct((NodeBitmap *) n);
			break;
		case V_NT_GEOMETRY:
			nodedb_g_destruct((NodeGeometry *) n);
			break;
		case V_NT_OBJECT:
			nodedb_o_destruct((NodeObject *) n);
			break;
		case V_NT_TEXT:
			nodedb_t_destruct((NodeText *) n);
			break;
		case V_NT_CURVE:
			nodedb_c_destruct((NodeCurve *) n);
			break;
		default:
			LOG_WARN(("Node destruction not implemented for type %d\n", n->type));
		}
		num = dynarr_size(n->tag_groups);
		for(i = 0; i < num; i++)
		{
			NdbTagGroup	*tg;

			if((tg = dynarr_index(n->tag_groups, i)) != NULL && tg->name[0] != '\0')
				dynarr_destroy(tg->tags);
		}
		dynarr_destroy(n->tag_groups);
		memchunk_free(ch, n);
	}
}

/* ----------------------------------------------------------------------------------------- */

void nodedb_ref(Node *node)
{
	if(node != NULL)
		node->ref++;
}

int nodedb_unref(Node *node)
{
	if(node != NULL)
	{
/*		printf("unreffing %p (%u)\n", node, node->id);*/
		node->ref--;
		if(node->ref <= 0)
		{
			LOG_MSG(("Node %u (%s) has zero references, destroying", node->id, node->name));
			nodedb_destroy(node);
			return 1;
		}
/*		else
			printf(" not destroyed, count=%d\n", node->ref);
*/	}
	return 0;
}

void nodedb_rename(Node *node, const char *name)
{
	if(node == NULL || name == NULL)
		return;
	stu_strncpy(node->name, sizeof node->name, name);
}

VNodeType nodedb_type_get(const Node *node)
{
	if(node != NULL)
		return node->type;
	return V_NT_NUM_TYPES;
}

/* ----------------------------------------------------------------------------------------- */

NdbTagGroup * nodedb_tag_group_lookup(const Node *node, const char *name)
{
	unsigned int	i;
	NdbTagGroup	*g;

	if(node == NULL || name == NULL || *name == '\0')
		return NULL;
	for(i = 0; (g = dynarr_index(node->tag_groups, i)) != NULL; i++)
	{
		if(*g->name == '\0')
			continue;
		if(strcmp(g->name, name) == 0)
			return g;
	}
	return NULL;
}

/* ----------------------------------------------------------------------------------------- */

static void cb_node_create(void *user, VNodeID node_id, VNodeType type, VNodeOwner ownership)
{
	Node	*n;

	if((n = nodedb_new(type)) != NULL)
	{
		n->id    = node_id;	/* Set ID to actual host-assigned value. */
		n->owner = ownership;
		hash_insert(nodedb_info.nodes, (const void *) n->id, n);
		nodedb_ref(n);
		LOG_MSG(("Stored node %u, type %d", node_id, type));
		if(n->owner == VN_OWNER_MINE)
		{
			hash_insert(nodedb_info.nodes_mine, (const void *) n->id, n);
			NOTIFY(n, CREATE);
		}
		verse_send_node_subscribe(node_id);
	}
	else
		LOG_WARN(("Can't handle creation of node type %d--not implemented", type));
}

static void cb_node_name_set(void *user, VNodeID node_id, const char *name)
{
	Node	*n;

	if((n = nodedb_lookup(node_id)) != NULL)
	{
		stu_strncpy(n->name, sizeof n->name, name);
		LOG_MSG(("Name of %u set to \"%s\"", n->id, n->name));
		NOTIFY(n, DATA);
	}
	else
		LOG_WARN(("Couldn't set name of node %u, not found in database", node_id));
}

/* ----------------------------------------------------------------------------------------- */

static void cb_tag_group_create(void *user, VNodeID node_id, uint16 group_id, const char *name)
{
	Node	*n;

	if((n = nodedb_lookup(node_id)) != NULL)
	{
		NdbTagGroup	*tg;

		if(n->tag_groups == NULL)
			n->tag_groups = dynarr_new(sizeof *tg, 2);
		if((tg = dynarr_set(n->tag_groups, group_id, NULL)) != NULL)
		{
			stu_strncpy(tg->name, sizeof tg->name, name);
			tg->tags = NULL;
			NOTIFY(n, STRUCTURE);
		}
	}
}

static void cb_tag_group_destroy(void *user, VNodeID node_id, uint16 group_id)
{
	Node	*n;

	if((n = nodedb_lookup(node_id)) != NULL)
	{
		NdbTagGroup	*tg;

		if((tg = dynarr_index(n->tag_groups, group_id)) != NULL)
		{
			tg->name[0] = '\0';
			dynarr_destroy(tg->tags);
			tg->tags = NULL;
			NOTIFY(n, STRUCTURE);
		}
	}
}

static void cb_tag_create(void *user, VNodeID node_id, uint16 group_id, uint16 tag_id, const char *name, VNTagType type, const VNTag *value)
{
	Node		*n;
	NdbTagGroup	*tg;
	NdbTag		*tag;

	if((n = nodedb_lookup(node_id)) == NULL)
		return;
	if((tg = dynarr_index(n->tag_groups, group_id)) == NULL || tg->name[0] == '\0')
		return;
	if(tg->tags == NULL)
		tg->tags = dynarr_new(sizeof *tag, 4);
	if((tag = dynarr_set(tg->tags, tag_id, NULL)) != NULL)
	{
		stu_strncpy(tag->name, sizeof tag->name, name);
		tag->type  = type;
		tag->value = *value;
	}
}

static void cb_tag_destroy(void *user, VNodeID node_id, uint16 group_id, uint16 tag_id)
{
	Node		*n;
	NdbTagGroup	*tg;
	NdbTag		*tag;

	if((n = nodedb_lookup(node_id)) == NULL)
		return;
	if((tg = dynarr_index(n->tag_groups, group_id)) == NULL || tg->name[0] == '\0')
		return;
	if((tag = dynarr_index(tg->tags, tag_id)) != NULL)
	{
		tag->name[0] = '\0';
		tag->type = -1;
	}
}

/* ----------------------------------------------------------------------------------------- */

static unsigned int node_hash(const void *p)
{
	return (unsigned int) p;
}

static int node_key_eq(const void *key1, const void *key2)
{
	return key1 == key2;
}

void nodedb_register_callbacks(VNodeID avatar, uint32 mask)
{
	int	i;

	nodedb_info.avatar = avatar;

	for(i = 0; i < sizeof nodedb_info.chunk_node / sizeof *nodedb_info.chunk_node; i++)
		nodedb_info.chunk_node[i] = NULL;
	nodedb_info.chunk_node[V_NT_BITMAP]   = memchunk_new("chunk-node-bitmap", sizeof (NodeBitmap), 16);
	nodedb_info.chunk_node[V_NT_GEOMETRY] = memchunk_new("chunk-node-geometry", sizeof (NodeGeometry), 16);
	nodedb_info.chunk_node[V_NT_OBJECT]   = memchunk_new("chunk-node-object", sizeof (NodeObject), 16);
	nodedb_info.chunk_node[V_NT_TEXT]     = memchunk_new("chunk-node-text",   sizeof (NodeText), 16);
	nodedb_info.chunk_node[V_NT_CURVE]    = memchunk_new("chunk-node-curve",  sizeof (NodeCurve), 16);
	nodedb_info.chunk_node[V_NT_MATERIAL] = memchunk_new("chunk-node-material", sizeof (NodeMaterial), 16);

	nodedb_info.nodes      = hash_new(node_hash, node_key_eq);
	nodedb_info.nodes_mine = hash_new(node_hash, node_key_eq);

	nodedb_info.chunk_notify = memchunk_new("chunk-node-notify", sizeof (NotifyInfo), 16);

	verse_callback_set(verse_send_node_create,		cb_node_create,	NULL);
	verse_callback_set(verse_send_node_name_set,		cb_node_name_set, NULL);
	verse_callback_set(verse_send_tag_group_create,		cb_tag_group_create, NULL);
	verse_callback_set(verse_send_tag_group_destroy,	cb_tag_group_destroy, NULL);
	verse_callback_set(verse_send_tag_create,		cb_tag_create, NULL);
	verse_callback_set(verse_send_tag_destroy,		cb_tag_destroy, NULL);

	nodedb_b_register_callbacks();
	nodedb_c_register_callbacks();
	nodedb_g_register_callbacks();
	nodedb_m_register_callbacks();
	nodedb_o_register_callbacks();
	nodedb_t_register_callbacks();

 	verse_send_node_list(mask);
}

void nodedb_notify_add(NodeOwnership whose, void (*notify)(Node *node, NodeNotifyEvent e))
{
	if(whose == NODEDB_OWNERSHIP_MINE)
	{
		nodedb_info.notify_mine = list_append(nodedb_info.notify_mine, notify);
	}
	else
		LOG_ERR(("Notification for non-MINE ownership not implemented"));
}

void * nodedb_notify_node_add(Node *node, void (*notify)(Node *node, NodeNotifyEvent e, void *user), void *user)
{
	List		*iter;
	NotifyInfo	*ni;

	if(node == NULL || notify == NULL)
		return NULL;
	/* Check if node already has notification using this function & data. */
	for(iter = node->notify; iter != NULL; iter = list_next(iter))
	{
		ni = list_data(iter);
		if(ni->callback == notify && ni->user == user)
			return iter;	/* It does, so return same handle. */
	}
	ni = memchunk_alloc(nodedb_info.chunk_notify);
	ni->callback = notify;
	ni->user = user;
	iter = node->notify = list_prepend(node->notify, ni);

	return iter;		/* Opaque "handle" is simply list pointer. Simplifies remove. */
}

void nodedb_notify_node_remove(Node *node, void *handle)
{
	List		*item;
	NotifyInfo	*ni;

	if(node == NULL || handle == NULL)
		return;
	item = handle;
	node->notify = list_unlink(node->notify, item);
	ni = list_data(item);
	memchunk_free(nodedb_info.chunk_notify, ni);
	list_destroy(item);
}
