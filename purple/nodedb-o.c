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
	n->method_groups = dynarr_new(sizeof (NdbOMethodGroup), 1);
}

/* ----------------------------------------------------------------------------------------- */

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

/* ----------------------------------------------------------------------------------------- */

void nodedb_o_register_callbacks(void)
{
	verse_callback_set(verse_send_o_light_set, cb_o_light_set, NULL);
}
