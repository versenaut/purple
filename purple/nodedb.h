/*
 * 
*/

typedef struct
{
	char		name[VN_TAG_NAME_SIZE];
	VNTagType	type;
	VNTag		value;
} Tag;

typedef struct
{
	char		name[VN_TAG_GROUP_SIZE];
	DynArr		*tags;
} TagGroup;

typedef struct
{
	VNodeID		id;
	VNodeType	type;
	char		name[32];
	VNodeID		owner;
	DynArr		*tag_groups;
} Node;

#include "nodedb-o.h"
#include "nodedb-t.h"

extern void		nodedb_register_callbacks(VNodeID avatar, uint32 mask);

extern Node *		nodedb_lookup(VNodeID node_id);
extern Node *		nodedb_lookup_by_name(const char *name);
extern NodeObject *	nodedb_lookup_object(VNodeID node_id);	/* Convenient. */
extern NodeText *	nodedb_lookup_text(VNodeID node_id);

extern Node *		nodedb_lookup_mine_by_name(const char *name);

typedef enum { NODEDB_OWNERSHIP_ALL, NODEDB_OWNERSHIP_MINE, NODEDB_OWNERSHIP_OTHERS } NodeOwnership;
typedef enum { NODEDB_NOTIFY_CREATE, NODEDB_NOTIFY_STRUCTURE, NODEDB_NOTIFY_DATA, NODEDB_NOTIFY_DESTROY } NodeNotifyEvent;

/* Add a callback that will be called when a change occurs in a node. */
extern void		nodedb_notify_add(NodeOwnership whose, void (*notify)(Node *node, NodeNotifyEvent ev));
