/*
 * Object node databasing.
*/

typedef struct
{
	Node	node;
	real64	xform[3][3];
	real64	light[3];
	DynArr	*links;
	List	*links_local;
	DynArr	*method_groups;
} NodeObject;

typedef struct
{
	uint16	id;
	VNodeID	link;
	char	label[16];
	uint32	target_id;
} NdbOLink;

/* Used for local links, before syncing. */
typedef struct {
	Node	*link;
	char	label[16];
	uint32	target_id;
} NdbOLinkLocal;

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
extern void		nodedb_o_copy(NodeObject *n, const NodeObject *src);
extern void		nodedb_o_destruct(NodeObject *n);

extern void		nodedb_o_link_set(NodeObject *n, uint16 link_id, VNodeID link, const char *label, uint32 target_id);
extern void		nodedb_o_link_set_local(NodeObject *n, const PONode *link, const char *label, uint32 target_id);
extern PONode *		nodedb_o_link_get_local(const NodeObject *n, const char *label, uint32 target_id);
extern PINode *		nodedb_o_link_get(const NodeObject *n, const char *label, uint32 target_id);

extern NdbOMethodGroup*	nodedb_o_method_group_lookup(NodeObject *n, const char *name);
extern const NdbOMethod*nodedb_o_method_lookup(const NdbOMethodGroup *group, const char *name);
extern const NdbOMethod*nodedb_o_method_lookup_id(const NdbOMethodGroup *group, uint8 id);

extern void		nodedb_o_register_callbacks(void);
