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

/* ----------------------------------------------------------------------------------------- */

void nodedb_o_register_callbacks(void)
{
}
