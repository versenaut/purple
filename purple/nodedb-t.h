/*
 * 
*/

typedef struct
{
	Node	node;
	char	language[32];
	DynArr	*buffers;
} NodeText;

typedef struct
{
	uint16	id;
	char	name[16];
	char	content;
} NdbTBuffer;

extern void		nodedb_t_init(NodeText *n);

extern NdbTBuffer *	nodedb_t_buffer_lookup(const NodeText *node, const char *name);

extern void		nodedb_t_register_callbacks(void);
