/*
 * 
*/

#include "verse.h"

#include "dynarr.h"
#include "list.h"
#include "plugins.h"
#include "textbuf.h"
#include "nodedb.h"

#include "synchronizer.h"

/* ----------------------------------------------------------------------------------------- */

static struct
{
	List	*queue_create;
	List	*queue_sync;
} sync_info;

/* ----------------------------------------------------------------------------------------- */

void sync_node_add(const Node *node)
{
	if(node == NULL)
		return;
	if(node->id == ~0)	/* Locally created? */
		sync_info.queue_create = list_prepend(sync_info.queue_create, (void *) node);
	else
		sync_info.queue_sync = list_prepend(sync_info.queue_sync, (void *) node);
}

void sync_update(double slice)
{
}
