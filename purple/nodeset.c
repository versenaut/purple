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

/* ----------------------------------------------------------------------------------------- */

boolean nodeset_get_boolean(const NodeSet *ns, PValue *dest)
{
	return FALSE;
}

boolean nodeset_get_int32(const NodeSet *ns, PValue *dest)
{
	return FALSE;
}

boolean nodeset_get_uint32(const NodeSet *ns, PValue *dest)
{
	return FALSE;
}

boolean nodeset_get_real32(const NodeSet *ns, PValue *dest)
{
	return FALSE;
}

boolean nodeset_get_real32_vec2(const NodeSet *ns, PValue *dest)
{
	return FALSE;
}

boolean nodeset_get_real32_vec3(const NodeSet *ns, PValue *dest)
{
	return FALSE;
}

boolean nodeset_get_real32_vec4(const NodeSet *ns, PValue *dest)
{
	return FALSE;
}

boolean nodeset_get_real32_mat16(const NodeSet *ns, PValue *dest)
{
	return FALSE;
}

boolean nodeset_get_real64(const NodeSet *ns, PValue *dest)
{
	return FALSE;
}

boolean nodeset_get_real64_vec2(const NodeSet *ns, PValue *dest)
{
	return FALSE;
}

boolean nodeset_get_real64_vec3(const NodeSet *ns, PValue *dest)
{
	return FALSE;
}

boolean nodeset_get_real64_vec4(const NodeSet *ns, PValue *dest)
{
	return FALSE;
}

boolean nodeset_get_real64_mat16(const NodeSet *ns, PValue *dest)
{
	return FALSE;
}

/* String interpretation: name of first node. */
boolean nodeset_get_string(const NodeSet *ns, PValue *dest)
{
	PINode	*n;

	if(ns == NULL || dest == NULL)
		return FALSE;
	if((n = nodeset_retrieve(ns)) != NULL)
	{
		value_set(dest, P_VALUE_STRING, n);
		return TRUE;
	}
	return FALSE;
}
