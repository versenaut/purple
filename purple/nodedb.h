/*
 * 
*/

extern void		nodedb_register_callbacks(VNodeID avatar, uint32 mask);

extern Node *		nodedb_lookup(VNodeID node_id);
extern NodeText *	nodedb_lookup_text(VNodeID node_id);	/* Convenient. */

extern Node *		nodedb_lookup_mine_by_name(const char *name);

typedef enum { NODEDB_OWNERSHIP_ALL, NODEDB_OWNERSHIP_MINE, NODEDB_OWNERSHIP_OTHERS } NodeOwnership;

/* Add a callback that will be called when a change occurs in a node. */
extern void		nodedb_notify_add(NodeOwnership whose, void (*notify)(Node *node));
