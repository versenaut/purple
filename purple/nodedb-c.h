/*
 * Curve node databasing.
*/

typedef struct
{
	uint16	id;
	char	name[16];
	DynArr	*keys;
} NdbCCurve;

typedef struct
{
	Node	node;
	DynArr	*curves;
} NodeCurve;

extern void		nodedb_c_construct(NodeCurve *n);
extern void		nodedb_c_copy(NodeCurve *n, const NodeCurve *src);
extern void		nodedb_c_destruct(NodeCurve *n);

extern void		nodedb_c_register_callbacks(void);
