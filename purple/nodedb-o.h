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

typedef struct
{
	uint8		id;
	char		name[16];
	size_t		param_count;
	VNOParamType	*param_type;
	char		**param_name;
} NdbOMethod;

extern void		nodedb_o_construct(NodeObject *n);
extern void		nodedb_o_destruct(NodeObject *n);

extern NdbOMethodGroup*	nodedb_o_method_group_lookup(NodeObject *n, const char *name);
extern const NdbOMethod*nodedb_o_method_lookup(const NdbOMethodGroup *group, const char *name);
extern const NdbOMethod*nodedb_o_method_lookup_id(const NdbOMethodGroup *group, uint8 id);

extern void		nodedb_o_register_callbacks(void);
