/*
 * nodedb-t.h
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Text node databasing module.
*/

typedef struct
{
	uint16	id;
	char	name[16];
	TextBuf	*text;
} NdbTBuffer;

typedef struct
{
	Node	node;
	char	language[32];
	DynArr	*buffers;
} NodeText;

extern void		nodedb_t_construct(NodeText *n);
extern void		nodedb_t_copy(NodeText *n, const NodeText *src);
extern void		nodedb_t_destruct(NodeText *n);

extern const char *	nodedb_t_language_get(const NodeText *node);
extern size_t		nodedb_t_buffer_get_count(const NodeText *node);
extern NdbTBuffer *	nodedb_t_buffer_get_nth(const NodeText *node, unsigned int n);
extern NdbTBuffer *	nodedb_t_buffer_get_named(const NodeText *node, const char *name);

extern NdbTBuffer *	nodedb_t_buffer_create(NodeText *node, VLayerID buffer_id, const char *name);
extern char *		nodedb_t_buffer_read_line(NdbTBuffer *buffer, unsigned int line, char *put, size_t putmax);
extern void		nodedb_t_buffer_insert(NdbTBuffer *buffer, size_t pos, const char *text);
extern void		nodedb_t_buffer_append(NdbTBuffer *buffer, const char *text);
extern void		nodedb_t_buffer_delete(NdbTBuffer *buffer, size_t pos, size_t length);
extern void		nodedb_t_buffer_clear(NdbTBuffer *buffer);

extern void		nodedb_t_register_callbacks(void);
