/*
 * Internal definitions for nodedb-X implementations. The price paid for splitting it
 * into several C files.
*/

#define	NOTIFY(n, e)	nodedb_internal_notify_mine_check((Node *) (n), NODEDB_NOTIFY_ ## e);

extern void	nodedb_internal_notify_mine_check(Node *n, NodeNotifyEvent ev);
