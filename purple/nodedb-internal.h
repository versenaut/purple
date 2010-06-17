/*
 * nodedb-internal.h
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Internal definitions for nodedb-X implementations. The price paid for splitting it
 * into several C files.
*/

#define	NOTIFY(n, e)	do { \
				nodedb_internal_notify_mine_check((PNode *) (n), NODEDB_NOTIFY_ ## e); \
				nodedb_internal_notify_node_check((PNode *) (n), NODEDB_NOTIFY_ ## e); \
			} while(0)

extern void	nodedb_internal_notify_mine_check(PNode *n, NodeNotifyEvent ev);
extern void	nodedb_internal_notify_node_check(PNode *n, NodeNotifyEvent ev);

#if defined __GNUC__
#define	UNUSED(a)	__attribute__((unused)) a
#else
#define UNUSED(a) 	a
#endif
