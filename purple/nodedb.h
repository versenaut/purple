/*
 * nodedb.h
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * 
*/

typedef struct
{
	char		name[VN_TAG_NAME_SIZE];
	VNTagType	type;
	VNTag		value;
} NdbTag;

typedef struct
{
	char		name[VN_TAG_GROUP_SIZE];
	DynArr		*tags;
} NdbTagGroup;

/* This is typedef:ed to Node in the public purple.h header. */
struct Node
{
	int		ref;		/* Reference count. When decresed to zero, node is destroyed. */

	VNodeID		id;
	VNodeType	type;
	char		name[32];
	VNodeOwner	owner;
	DynArr		*tag_groups;

	List		*notify;

	union
	{
	Node		*parent;	/* Used in output node to keep track of original. */
	Node		*child;		/* Used in input node to keep track of copy. */
	}		copy;
};

#include "nodedb-b.h"
#include "nodedb-c.h"
#include "nodedb-g.h"
#include "nodedb-m.h"
#include "nodedb-o.h"
#include "nodedb-t.h"

extern void		nodedb_register_callbacks(VNodeID avatar, uint32 mask);

extern Node *		nodedb_lookup(VNodeID node_id);
extern Node *		nodedb_lookup_by_name(const char *name);
extern Node *		nodedb_lookup_with_type(VNodeID node_id, VNodeType type);
extern NodeObject *	nodedb_lookup_object(VNodeID node_id);
extern NodeText *	nodedb_lookup_text(VNodeID node_id);

extern Node *		nodedb_new(VNodeType type);
extern Node *		nodedb_new_copy(const Node *src);
extern void		nodedb_destroy(Node *n);

/* Nodes are reference counted. Users are supposed to call nodedb_new(), then immediately ref() the
 * created node on success (initial count is zero). Calling unref() will decrease count by one, and
 * automatically destroy the node if it went below one.
*/
extern void		nodedb_ref(Node *n);
extern int		nodedb_unref(Node *n);	/* Returns 1 if node was destroyed. */

extern void		nodedb_rename(Node *node, const char *name);
extern VNodeType	nodedb_type_get(const Node *node);

extern NdbTagGroup *	nodedb_tag_group_lookup(const Node *node, const char *name);

typedef enum { NODEDB_OWNERSHIP_ALL, NODEDB_OWNERSHIP_MINE, NODEDB_OWNERSHIP_OTHERS } NodeOwnership;
typedef enum { NODEDB_NOTIFY_CREATE, NODEDB_NOTIFY_STRUCTURE, NODEDB_NOTIFY_DATA, NODEDB_NOTIFY_DESTROY } NodeNotifyEvent;

/* Add a callback that will be called when a change occurs in a node. */
extern void		nodedb_notify_add(NodeOwnership whose, void (*notify)(Node *node, NodeNotifyEvent ev));

/* Add a callback to be called on change of a specific node, rather than a broad category like above. */
extern void *		nodedb_notify_node_add(Node *node,
					       void (*notify)(Node *node, NodeNotifyEvent ev, void *user),
					       void *user);

extern void		nodedb_notify_node_remove(Node *node, void *handle);
