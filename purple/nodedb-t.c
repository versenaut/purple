/*
 * 
*/

#include "verse.h"

#include "dynarr.h"
#include "strutil.h"

#include "nodedb.h"
#include "nodedb-internal.h"

/* ----------------------------------------------------------------------------------------- */

static void cb_t_buffer_create(void *user, VNodeID node_id, uint16 buffer_id, const char *name)
{
	NodeText	*n;

	if((n = nodedb_lookup_text(node_id)) != NULL)
	{
		NdbTBuffer	*tb;

		if((tb = dynarr_set(n->buffers, buffer_id, NULL)) != NULL)
		{
			stu_strncpy(tb->name, sizeof tb->name, name);
		}
		NOTIFY(n, STRUCTURE);
	}
}

/* ----------------------------------------------------------------------------------------- */

void nodedb_t_register_callbacks(void)
{
	verse_callback_set(verse_send_t_buffer_create,	cb_t_buffer_create, NULL);
}
