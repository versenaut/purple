/*
 * Curve node databasing.
*/

#define	P_CURVE_DIM_MAX	4

typedef struct
{
	uint32	pos[P_CURVE_DIM_MAX];
	real64	value[P_CURVE_DIM_MAX];
} NdbCKeyWing;

typedef struct
{
	uint32		id;
	real64		pos;
	real64		value[P_CURVE_DIM_MAX];
	NdbCKeyWing	pre, post;
} NdbCKey;

typedef struct
{
	VLayerID	id;
	char		name[16];
	uint8		dimensions;
	DynArr		*keys;		/* Array of Keys, actual storage, arranged by ID/index. */
	List		*curve;		/* List of Keys, ordered by pos. */
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
