/*
 * Object node databasing.
*/

typedef struct
{
	Node	node;
	real64	xform[3][3];
	real64	light[3];
	DynArr	*links;
	DynArr	*method_groups;
} NodeObject;

typedef struct
{
	uint16	id;
	VNodeID	link;
	char	name[16];
	uint32	target;
} NdbOLink;

typedef struct
{
	uint16	id;
	char	name[16];
	DynArr	*methods;
} NdbOMethodGroup;

extern void		nodedb_o_init(NodeObject *n);

extern void		nodedb_o_register_callbacks(void);
