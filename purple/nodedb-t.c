/*
 *
*/

#include "verse.h"

#include "dynarr.h"
#include "strutil.h"

#include "nodedb.h"
#include "nodedb-internal.h"

/* ----------------------------------------------------------------------------------------- */

void nodedb_t_init(NodeText *n)
{
	n->buffers = dynarr_new(sizeof (NdbTBuffer), 2);
}

NdbTBuffer * nodedb_t_buffer_lookup(const NodeText *node, const char *name)
{
	unsigned int	i;
	NdbTBuffer	*b;

	if(node == NULL || name == NULL || *name == '\0')
		return NULL;
	for(i = 0; i < dynarr_size(node->buffers); i++)
	{
		if((b = dynarr_index(node->buffers, i)) != NULL)
		{
			if(strcmp(b->name, name) == 0)
				return b;
		}
	}
	return NULL;
}

/* ----------------------------------------------------------------------------------------- */

static void cb_t_buffer_create(void *user, VNodeID node_id, uint16 buffer_id, uint16 index, const char *name)
{
	NodeText	*n;

	if((n = nodedb_lookup_text(node_id)) != NULL)
	{
		NdbTBuffer	*tb;

		if((tb = dynarr_set(n->buffers, buffer_id, NULL)) != NULL)
		{
			tb->id = buffer_id;
			stu_strncpy(tb->name, sizeof tb->name, name);
			printf("text buffer %u.%u %s created\n", node_id, buffer_id, name);
			NOTIFY(n, STRUCTURE);
		}
	}
}

/* ----------------------------------------------------------------------------------------- */

void nodedb_t_register_callbacks(void)
{
	verse_callback_set(verse_send_t_buffer_create,	cb_t_buffer_create, NULL);
}
