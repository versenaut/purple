/*
 * 
*/

#include "verse.h"

#include "purple.h"

#include "dynarr.h"
#include "list.h"
#include "textbuf.h"

#include "nodedb.h"

/* ----------------------------------------------------------------------------------------- */

const char * p_node_name_get(const Node *node)
{
	return node != NULL ? node->name : NULL;
}

void p_node_name_set(PONode *node, const char *name)
{
	nodedb_rename(node, name);
}
