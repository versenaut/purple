/*
 * NodeSets are used to hold collections of Verse node references, for value passing.
*/

#include <stdarg.h>

#include "purple.h"

#include "dynarr.h"
#include "dynstr.h"
#include "list.h"
#include "memchunk.h"
#include "textbuf.h"
#include "value.h"

#include "nodedb.h"

#include "nodeset.h"

/* ----------------------------------------------------------------------------------------- */

struct NodeSet
{
	List	*nodes;		/* Ta-da. */
};

static MemChunk	*chunk_nodeset = NULL;

/* ----------------------------------------------------------------------------------------- */

static NodeSet * nodeset_new(void)
{
	if(chunk_nodeset == NULL)
		chunk_nodeset = memchunk_new("nodeset", sizeof (NodeSet), 16);
	if(chunk_nodeset != NULL)
	{
		NodeSet	*ns;

		ns = memchunk_alloc(chunk_nodeset);
		ns->nodes = NULL;
		return ns;
	}
	return NULL;
}

NodeSet * nodeset_add(NodeSet *ns, PONode *n)
{
	if(ns == NULL)
		ns = nodeset_new();
	if(ns != NULL)
	{
		ns->nodes = list_prepend(ns->nodes, n);
	}
	return ns;
}

void nodeset_clear(NodeSet *ns)
{
	List	*iter;

	if(ns == NULL)
		return;
	for(iter = ns->nodes; iter != NULL; iter = list_next(iter))
		nodedb_destroy((Node *) list_data(iter));
	list_destroy(ns->nodes);
	ns->nodes = NULL;
}

PINode * nodeset_retrieve(const NodeSet *ns)
{
	if(ns == NULL)
		return NULL;
	return list_data(ns->nodes);
}

const char * nodeset_get_string(const NodeSet *ns)
{
	PINode	*n;

	if(ns == NULL)
		return "";
	if((n = nodeset_retrieve(ns)) != NULL)
		return p_node_name_get(n);
	return NULL;
}
