/*
 * 
*/

#include <stdio.h>

#include "verse.h"

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

static struct
{
	VNodeID		avatar;
	MemChunk	*chunk_node[V_NT_NUM_TYPES];
	Hash		*nodes;				/* Probably not efficient enough, but a start. */
	Hash		*nodes_mine;			/* Duplicate links to nodes owned by this client. */

	List		*notify_mine;
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
	if(nodedb_info.notify_mine != NULL && (n->owner == nodedb_info.avatar || n->id == nodedb_info.avatar))
	{
		const List	*iter;

		for(iter = nodedb_info.notify_mine; iter != NULL; iter = list_next(iter))
			((void (*)(Node *node, NodeNotifyEvent e)) list_data(iter))(n, ev);
	}
}

/* ----------------------------------------------------------------------------------------- */

static void cb_node_create(void *user, VNodeID node_id, VNodeType type, VNodeID owner_id)
{
	MemChunk	*ch;

	if((ch = nodedb_info.chunk_node[type]) != NULL)
	{
		Node	*n = (Node *) memchunk_alloc(ch);

		n->id = node_id;
		n->type = type;
		n->name[0] = '\0';
		n->owner = owner_id;
		n->tag_groups = NULL;
		switch(n->type)
		{
		case V_NT_OBJECT:
			nodedb_o_init((NodeObject *) n);
			break;
		case V_NT_TEXT:
			nodedb_t_init((NodeText *) n);
			break;
		default:
			LOG_WARN(("Missing node-specific init code for type %d", type));
		}
		hash_insert(nodedb_info.nodes, (const void *) n->id, n);
		LOG_MSG(("Stored node %u, type %d", node_id, type));
		if(n->owner == nodedb_info.avatar)
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

static unsigned int node_hash(const void *p)
{
	return (unsigned int) p;
}

static int node_has_key(const void *key, const void *obj)
{
	return ((const Node *) obj)->id == (uint32) key;
}

void nodedb_register_callbacks(VNodeID avatar, uint32 mask)
{
	int	i;

	nodedb_info.avatar = avatar;

	for(i = 0; i < sizeof nodedb_info.chunk_node / sizeof *nodedb_info.chunk_node; i++)
		nodedb_info.chunk_node[i] = NULL;
	nodedb_info.chunk_node[V_NT_OBJECT] = memchunk_new("chunk-node-object", sizeof (NodeObject), 16);
	nodedb_info.chunk_node[V_NT_TEXT]   = memchunk_new("chunk-node-text", sizeof (NodeText), 16);

	nodedb_info.nodes      = hash_new(node_hash, node_has_key);
	nodedb_info.nodes_mine = hash_new(node_hash, node_has_key);

	verse_callback_set(verse_send_node_create,	cb_node_create,	NULL);
	verse_callback_set(verse_send_node_name_set,	cb_node_name_set, NULL);

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
