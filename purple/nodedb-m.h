/*
 * Material node databasing.
*/

typedef struct
{
	VNMFragmentID	id;
	VNMFragmentType	type;
	VMatFrag	frag;
} NdbMFragment;

typedef struct
{
	Node	node;
	DynArr	*fragments;
} NodeMaterial;

extern void		nodedb_m_construct(NodeMaterial *n);
extern void		nodedb_m_copy(NodeMaterial *n, const NodeMaterial *src);
extern void		nodedb_m_destruct(NodeMaterial *n);

extern void		nodedb_m_register_callbacks(void);