/*
 * 
*/

/* Request that the given node be synchronized, i.e. compared to local
 * copy and any required changes generated and sent to the Verse server.
*/
extern void	sync_node_add(const Node *node);

/* Run the synchronizer, attempting not to spend more than <duration> seconds. */
extern void	sync_update(double slice);
