/*
 * 
*/

#include <stdio.h>

#include "verse.h"

#include "hash.h"
#include "log.h"
#include "memchunk.h"
#include "strutil.h"

#include "node.h"
#include "nodedb.h"

/* ----------------------------------------------------------------------------------------- */

static struct
{
	VNodeID		avatar;
	MemChunk	*chunk_node[V_NT_NUM_TYPES];
	Hash		*nodes;				/* Probably not efficient enough, but a start. */
} nodedb_info;

/* ----------------------------------------------------------------------------------------- */

void nodedb_register(Node *n)
{
	hash_insert(nodedb_info.nodes, (void *) n->id, n);
}

Node * nodedb_lookup(VNodeID node_id)
{
	return hash_lookup(nodedb_info.nodes, (const void *) node_id);
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
		hash_insert(nodedb_info.nodes, (const void *) n->id, n);
		LOG_MSG(("Stored node %u, type %d", node_id, type));
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
	nodedb_info.chunk_node[V_NT_TEXT] = memchunk_new("chunk-node-text", sizeof (NodeText), 16);
	printf("chunk for type %d is %p\n", V_NT_TEXT, nodedb_info.chunk_node[V_NT_TEXT]);

	nodedb_info.nodes = hash_new(node_hash, node_has_key);

	verse_callback_set(verse_send_node_create,	cb_node_create,	NULL);
	verse_callback_set(verse_send_node_name_set,	cb_node_name_set, NULL);

 	verse_send_node_list(0);
 	verse_send_node_list(mask);
}
