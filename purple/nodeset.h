/*
 * NodeSets store nodes in ports. They are a bit asymmetrical: output nodes go in
 * (as a node is output through a port), input nodes come out (as a port is queried
 * using the input API). This is only a change in const-ness, however.
*/

typedef struct NodeSet	NodeSet;

/* Add a node to the nodeset. If the nodeset is NULL, a new set is created. */
extern NodeSet *	nodeset_add(NodeSet *ns, PONode *node);

extern void		nodeset_clear(NodeSet *ns);

extern PINode *		nodeset_retrieve(const NodeSet *ns);

/* Interpret a NodeSet as one of the basic types. This is used in input-accessing. Store
 * the interpretation in the provided PValue, so the nodeset itself need not take on
 * responsibility for caching values. The <dest> is often the cache part of a port.
*/
extern boolean		nodeset_get_boolean(const NodeSet *ns, PValue *dest);
extern boolean		nodeset_get_int32(const NodeSet *ns, PValue *dest);
extern boolean		nodeset_get_uint32(const NodeSet *ns, PValue *dest);
extern boolean		nodeset_get_real32(const NodeSet *ns, PValue *dest);
extern boolean		nodeset_get_real32_vec2(const NodeSet *ns, PValue *dest);
extern boolean		nodeset_get_real32_vec3(const NodeSet *ns, PValue *dest);
extern boolean		nodeset_get_real32_vec4(const NodeSet *ns, PValue *dest);
extern boolean		nodeset_get_real32_mat16(const NodeSet *ns, PValue *dest);
extern boolean		nodeset_get_real64(const NodeSet *ns, PValue *dest);
extern boolean		nodeset_get_real64_vec2(const NodeSet *ns, PValue *dest);
extern boolean		nodeset_get_real64_vec3(const NodeSet *ns, PValue *dest);
extern boolean		nodeset_get_real64_vec4(const NodeSet *ns, PValue *dest);
extern boolean		nodeset_get_real64_mat16(const NodeSet *ns, PValue *dest);
extern boolean		nodeset_get_string(const NodeSet *ns, PValue *dest);
