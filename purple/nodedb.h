/*
 * nodedb.h
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * 
*/

typedef struct
{
	uint16		id;
	char		name[VN_TAG_NAME_SIZE];
	VNTagType	type;
	VNTag		value;
} NdbTag;

typedef struct
{
	uint16		id;
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
};

#include "nodedb-a.h"
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

#define	NODEDB_TAG_TYPE_SCALAR(t)	(((t) != VN_TAG_STRING) && ((t) != VN_TAG_BLOB))

extern unsigned int	nodedb_tag_group_num(const Node *node);
extern NdbTagGroup *	nodedb_tag_group_nth(const Node *node, unsigned int n);
extern NdbTagGroup *	nodedb_tag_group_find(const Node *node, const char *name);

extern NdbTagGroup *	nodedb_tag_group_create(Node *node, uint16 group_id, const char *name);
extern NdbTagGroup *	nodedb_tag_group_lookup(const Node *node, const char *name);
extern void		nodedb_tag_group_destroy(NdbTagGroup *group);

extern unsigned int	nodedb_tag_group_tag_num(const NdbTagGroup *group);
extern NdbTag *		nodedb_tag_group_tag_nth(const NdbTagGroup *group, unsigned int n);
extern NdbTag *		nodedb_tag_group_tag_find(const NdbTagGroup *group, const char *name);
extern void		nodedb_tag_create(NdbTagGroup *group, uint16 tag_id, const char *name, VNTagType type, const VNTag *value);
extern void		nodedb_tag_destroy(NdbTagGroup *group, NdbTag *tag);
extern void		nodedb_tag_destroy_all(NdbTagGroup *group);
extern const char *	nodedb_tag_get_name(const NdbTag *tag);
extern void		nodedb_tag_value_clear(NdbTag *tag);
extern void		nodedb_tag_value_set(NdbTag *tag, VNTagType type, const VNTag *value);
extern int		nodedb_tag_values_equal(const NdbTag *t1, const NdbTag *t2);

typedef enum { NODEDB_OWNERSHIP_ALL, NODEDB_OWNERSHIP_MINE, NODEDB_OWNERSHIP_OTHERS } NodeOwnership;
typedef enum { NODEDB_NOTIFY_CREATE, NODEDB_NOTIFY_STRUCTURE, NODEDB_NOTIFY_DATA, NODEDB_NOTIFY_DESTROY } NodeNotifyEvent;

/* Add a callback that will be called when a change occurs in a node. */
extern void		nodedb_notify_add(NodeOwnership whose, void (*notify)(Node *node, NodeNotifyEvent ev));

/* Add a callback to be called on change of a specific node, rather than a broad category like above. */
extern void *		nodedb_notify_node_add(Node *node,
					       void (*notify)(Node *node, NodeNotifyEvent ev, void *user),
					       void *user);

extern void		nodedb_notify_node_remove(Node *node, void *handle);
