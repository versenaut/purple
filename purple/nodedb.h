/*
 * 
*/

extern void		nodedb_register_callbacks(VNodeID avatar, uint32 mask);

extern Node *		nodedb_lookup(VNodeID node_id);
extern NodeText *	nodedb_lookup_text(VNodeID node_id);	/* Convenient. */
