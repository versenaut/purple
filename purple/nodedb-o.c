/*
 * Object node databasing.
*/

#include <stdio.h>
#include <string.h>

#include "verse.h"

#include "dynarr.h"
#include "strutil.h"
#include "textbuf.h"

#include "nodedb.h"
#include "nodedb-internal.h"

/* ----------------------------------------------------------------------------------------- */

void nodedb_o_init(NodeObject *n)
{
	n->light[0] = n->light[1] = n->light[2] = 0.0;
	n->links = NULL;
	n->method_groups = NULL;
}

/* ----------------------------------------------------------------------------------------- */

static void cb_o_link_set(void *user, VNodeID node_id, uint16 link_id, uint32 link, const char *name, uint32 target_id)
{
	NodeObject	*n;

	if((n = nodedb_lookup_object(node_id)) != NULL)
	{
		NdbOLink	*l;

		if(n->links == NULL)
			n->links = dynarr_new(sizeof (NdbOLink), 1);
		if((l = dynarr_set(n->links, link_id, NULL)) != NULL)
		{
			l->id = link_id;
			l->link = link;
			stu_strncpy(l->name, sizeof l->name, name);
			l->target = target_id;
			NOTIFY(n, DATA);
		}
	}
}

static void cb_o_light_set(void *user, VNodeID node_id, real64 r, real64 g, real64 b)
{
	NodeObject	*n;

	if((n = nodedb_lookup_object(node_id)) != NULL)
	{
		n->light[0] = r;
		n->light[1] = g;
		n->light[2] = b;
		NOTIFY(n, DATA);
	}
}


static void cb_o_method_group_create(void *user, VNodeID node_id, uint16 group_id, const char *name)
{
	NodeObject	*n;

	if((n = nodedb_lookup_object(node_id)) != NULL)
	{
		NdbOMethodGroup	*g;

		if(n->method_groups == NULL)
			n->method_groups = dynarr_new(sizeof (NdbOMethodGroup), 1);
		if((g = dynarr_set(n->method_groups, group_id, NULL)) != NULL)
		{
			g->id = group_id;
			stu_strncpy(g->name, sizeof name, name);
			g->methods = NULL;
		}
	}
}

/* ----------------------------------------------------------------------------------------- */

void nodedb_o_register_callbacks(void)
{
	verse_callback_set(verse_send_o_link_set,		cb_o_link_set, NULL);
	verse_callback_set(verse_send_o_light_set,		cb_o_light_set, NULL);
	verse_callback_set(verse_send_o_method_group_create,	cb_o_method_group_create, NULL);
}
