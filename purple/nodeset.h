/*
 * NodeSets store nodes in ports. They are a bit asymmetrical: output nodes go in
 * (as a node is output through a port), input nodes come out (as a port is queried
 * using the input API). This is only a change in const-ness, however.
*/

typedef struct NodeSet	NodeSet;

/* Add a node to the nodeset. If the nodeset is NULL, a new set is created. */
extern NodeSet *	nodeset_add(NodeSet *ns, PONode *node);

extern void		nodeset_clear(NodeSet *ns);

extern PINode *		nodeset_retreive(const NodeSet *ns);

extern const char *	nodeset_get_string(const NodeSet *ns);
