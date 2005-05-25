/*
 * synchronizer.h
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Output node synchronization pipeline. Maintains a bunch of nodes, compares them to
 * corresponding "host side" nodes, and generates difference-removing commands as is
 * needed.
*/

/* Tie synchronizer into nodedb notification system. */
extern void	sync_init(void);

/* Request that the given node be synchronized, i.e. compared to local
 * copy and any required changes generated and sent to the Verse server.
*/
extern void	sync_node_add(PNode *node);

/* Run the synchronizer, attempting not to spend more than <duration> seconds. */
extern void	sync_update(double slice);
