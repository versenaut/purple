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
	NdbMFragment	*f;

	if(node == NULL || fragment == NULL)
		return NULL;
	if(node->fragments == NULL)
	{
		node->fragments = dynarr_new(sizeof (NdbMFragment), 4);
		dynarr_set_default_func(node->fragments, cb_fragment_default, NULL);
	}
	if(fragment_id == (VLayerID) ~0)
		f = dynarr_append(node->fragments, NULL, NULL);
	else
		f = dynarr_set(node->fragments, fragment_id, NULL);
	if(f != NULL)
	{
		f->id   = fragment_id;
		f->type = type;
		f->frag = *fragment;
		printf("fragment %u.%u created, type=%u\n", node->node.id, fragment_id, f->type);
	}
	return f;
}

/* ----------------------------------------------------------------------------------------- */

static void cb_m_fragment_create(void *user, VNodeID node_id, VNMFragmentID fragment_id, VNMFragmentType type, VMatFrag *fragment)
{
	NodeMaterial	*node;

	if((node = (NodeMaterial *) nodedb_lookup_with_type(node_id, V_NT_MATERIAL)) != NULL)
	{
		NdbMFragment	*frag;
		int		old = 0;

		if((frag = dynarr_index(node->fragments, fragment_id)) != NULL)
			old = 1;
		if((frag = nodedb_m_fragment_create(node, fragment_id, type, fragment)) != NULL)
		{
			if(old)
				NOTIFY(node, DATA);
			else
				NOTIFY(node, STRUCTURE);
		}
	}
}

static void cb_m_fragment_destroy(void *user, VNodeID node_id, VNMFragmentID fragment_id)
{
	NodeMaterial	*node;

	if((node = (NodeMaterial *) nodedb_lookup_with_type(node_id, V_NT_MATERIAL)) != NULL)
	{
		NdbMFragment	*frag;

		if((frag = dynarr_index(node->fragments, fragment_id)) != NULL)
		{
			frag->id   = ~0;
			frag->type = -1;
			NOTIFY(node, STRUCTURE);
		}
	}
}

/* ----------------------------------------------------------------------------------------- */

void nodedb_m_register_callbacks(void)
{
	verse_callback_set(verse_send_m_fragment_create,	cb_m_fragment_create, NULL);
	verse_callback_set(verse_send_m_fragment_destroy,	cb_m_fragment_destroy, NULL);
}
