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

extern void		nodedb_t_construct(NodeText *n);
extern void		nodedb_t_copy(NodeText *n, const NodeText *src);
extern void		nodedb_t_destruct(NodeText *n);

extern const char *	nodedb_t_language_get(const NodeText *node);
extern size_t		nodedb_t_buffer_get_count(const NodeText *node);
extern NdbTBuffer *	nodedb_t_buffer_get_nth(const NodeText *node, unsigned int n);
extern NdbTBuffer *	nodedb_t_buffer_get_named(const NodeText *node, const char *name);

extern void		nodedb_t_register_callbacks(void);
