/*
 * Material node databasing.
*/

#include <stdio.h>
#include <string.h>

#include "verse.h"
#include "purple.h"

#include "dynarr.h"
#include "list.h"
#include "log.h"
#include "strutil.h"
#include "textbuf.h"

#include "nodedb.h"
#include "nodedb-internal.h"

/* ----------------------------------------------------------------------------------------- */

void nodedb_m_construct(NodeMaterial *n)
{
	n->fragments = NULL;
}

void nodedb_m_copy(NodeMaterial *n, const NodeMaterial *src)
{
	n->fragments = dynarr_new_copy(src->fragments, NULL, NULL);
}

void nodedb_m_destruct(NodeMaterial *n)
{
	dynarr_destroy(n->fragments);
}

/* ----------------------------------------------------------------------------------------- */

static void cb_fragment_default(unsigned int indec, void *element, void *user)
{
	NdbMFragment	*frag = element;

	frag->id   = ~0;
	frag->type = -1;
}

NdbMFragment * nodedb_m_fragment_create(NodeMaterial *node, VNMFragmentID fragment_id, VNMFragmentType type, const VMatFrag *fragment)
{
/*	NdbCCurve	*curve;

	if(node == NULL || name == NULL || dimensions > 4)
		return NULL;
	if(node->curves == NULL)
	{
		node->curves = dynarr_new(sizeof (NdbCCurve), 4);
		dynarr_set_default_func(node->curves, cb_curve_default, NULL);
	}
	if(curve_id == (VLayerID) ~0)
		curve = dynarr_append(node->curves, NULL, NULL);
	else
		curve = dynarr_set(node->curves, curve_id, NULL);
	if(curve != NULL)
	{
		curve->id = curve_id;
		stu_strncpy(curve->name, sizeof curve->name, name);
		curve->dimensions = dimensions;
		curve->keys = NULL;
		printf("Curve curve %u.%u %s created, dim=%u\n", node->node.id, curve_id, name, curve->dimensions);
	}
	return curve;
*/
	return NULL;
}

/* ----------------------------------------------------------------------------------------- */

static void cb_m_fragment_create(void *user, VNodeID node_id, VNMFragmentID fragment_id, VNMFragmentType type, VMatFrag *fragment)
{
	NodeMaterial	*n;

	if((n = (NodeMaterial *) nodedb_lookup_with_type(node_id, V_NT_MATERIAL)) != NULL)
	{
	}
}

static void cb_m_fragment_destroy(void *user, VNodeID node_id, VNMFragmentID fragment_id)
{
	NodeMaterial	*n;

	if((n = (NodeMaterial *) nodedb_lookup_with_type(node_id, V_NT_MATERIAL)) != NULL)
	{
	}
}

/* ----------------------------------------------------------------------------------------- */

void nodedb_m_register_callbacks(void)
{
	verse_callback_set(verse_send_m_fragment_create,	cb_m_fragment_create, NULL);
	verse_callback_set(verse_send_m_fragment_destroy,	cb_m_fragment_destroy, NULL);
}
