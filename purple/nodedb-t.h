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
	TextBuf	*text;
} NdbTBuffer;

extern void		nodedb_t_init(NodeText *n);

extern NdbTBuffer *	nodedb_t_buffer_lookup(const NodeText *node, const char *name);
extern NdbTBuffer *	nodedb_t_buffer_lookup_id(const NodeText *node, uint16 buffer_id);

extern void		nodedb_t_register_callbacks(void);
