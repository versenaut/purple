/*
 * nodedb-m.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
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

unsigned int nodedb_m_fragment_num(const NodeMaterial *node)
{
	unsigned int	i, num;
	NdbMFragment	*frag;

	if(node == NULL)
		return 0;
	for(i = num = 0; (frag = dynarr_index(node->fragments, i)) != NULL; i++)
	{
		if(frag->id == (VNMFragmentID) ~0)
			continue;
		num++;
	}
	return num;
}

NdbMFragment * nodedb_m_fragment_nth(const NodeMaterial *node, unsigned int n)
{
	unsigned int	i;
	NdbMFragment	*frag;

	if(node == NULL)
		return NULL;
	for(i = 0; (frag = dynarr_index(node->fragments, i)) != NULL; i++)
	{
		if(frag->id == (VNMFragmentID) ~0)
			continue;
		if(n == 0)
			return frag;
		n--;
	}
	return NULL;
}

NdbMFragment * nodedb_m_fragment_lookup(const NodeMaterial *node, VNMFragmentID id)
{
	NdbMFragment	*frag;

	if(node == NULL)
		return NULL;
	if((frag = dynarr_index(node->fragments, id)) != NULL)
	{
		if(frag->id == id)
			return frag;
	}
	return NULL;
}

/* ----------------------------------------------------------------------------------------- */

static int fragments_equal(const NodeMaterial *node, const NdbMFragment *a,
			   const NodeMaterial *target, const NdbMFragment *b);

/* Compare two fragment references, or (if valid) the fragments they refer to. Recursive. */
static int fragment_refs_equal(const NodeMaterial *node, VNMFragmentID ref,
			       const NodeMaterial *target, VNMFragmentID tref)
{
	NdbMFragment	*na, *nb;

	if(ref == (VNMFragmentID) ~0 && tref == (VNMFragmentID) ~0)
		return 1;
	na = nodedb_m_fragment_lookup(node, ref);
	nb = nodedb_m_fragment_lookup(target, tref);
	return fragments_equal(node, na, target, nb);
}

static int fragments_equal(const NodeMaterial *node, const NdbMFragment *a,
			   const NodeMaterial *target, const NdbMFragment *b)
{
	if(node == NULL || a == NULL || b == NULL || a->type != b->type)
		return 0;
	switch(a->type)
	{
	case VN_M_FT_COLOR:
		return memcmp(&a->frag.color, &b->frag.color, sizeof a->frag.color) == 0;
	case VN_M_FT_LIGHT:
		if(a->frag.light.type == b->frag.light.type)
		{
			if(a->frag.light.normal_falloff != b->frag.light.normal_falloff)
				return 0;
			if(strcmp(a->frag.light.brdf_r, b->frag.light.brdf_r) != 0)
				return 0;
			if(strcmp(a->frag.light.brdf_g, b->frag.light.brdf_g) != 0)
				return 0;
			if(strcmp(a->frag.light.brdf_b, b->frag.light.brdf_b) != 0)
				return 0;
			/* Compare the 'brdf' node reference. Here, it gets tricky. */
		}
		return 0;
	case VN_M_FT_REFLECTION:
		return a->frag.reflection.normal_falloff == b->frag.reflection.normal_falloff;
	case VN_M_FT_TRANSPARENCY:
		return a->frag.transparency.normal_falloff == b->frag.transparency.normal_falloff &&
			a->frag.transparency.refraction_index == b->frag.transparency.refraction_index;
	case VN_M_FT_VOLUME:
		if(a->frag.volume.diffusion == b->frag.volume.diffusion &&
		   a->frag.volume.col_r == b->frag.volume.col_r &&
		   a->frag.volume.col_g == b->frag.volume.col_g &&
		   a->frag.volume.col_b == b->frag.volume.col_b)
			return fragment_refs_equal(node, a->frag.volume.color, target, b->frag.volume.color);
		return 0;
	case VN_M_FT_GEOMETRY:
		return strcmp(a->frag.geometry.layer_r, b->frag.geometry.layer_r) == 0 &&
		       strcmp(a->frag.geometry.layer_g, b->frag.geometry.layer_g) == 0 &&
		       strcmp(a->frag.geometry.layer_b, b->frag.geometry.layer_b) == 0;
	case VN_M_FT_TEXTURE:
		return 0;	/* Hard! */
	case VN_M_FT_NOISE:
		return a->frag.noise.type == b->frag.noise.type &&
			fragment_refs_equal(node, a->frag.noise.mapping, target, b->frag.noise.mapping);
	case VN_M_FT_BLENDER:
		return a->frag.blender.type == b->frag.blender.type &&
			fragment_refs_equal(node, a->frag.blender.data_a, target, b->frag.blender.data_a) &&
			fragment_refs_equal(node, a->frag.blender.data_b, target, b->frag.blender.data_b) &&
			fragment_refs_equal(node, a->frag.blender.control, target, b->frag.blender.control);
	case VN_M_FT_MATRIX:
		return memcmp(a->frag.matrix.matrix, b->frag.matrix.matrix, sizeof a->frag.matrix.matrix) == 0 &&
			fragment_refs_equal(node, a->frag.matrix.data, target, b->frag.matrix.data);
	case VN_M_FT_RAMP:
		if(a->frag.ramp.type == b->frag.ramp.type && a->frag.ramp.channel == b->frag.ramp.channel)
		{
			if(a->frag.ramp.point_count == b->frag.ramp.point_count)
			{
				if(memcmp(a->frag.ramp.ramp, b->frag.ramp.ramp,
					  a->frag.ramp.point_count * sizeof a->frag.ramp.ramp) == 0)
					return fragment_refs_equal(node, a->frag.ramp.mapping, target, b->frag.ramp.mapping);
			}
		}
		return 0;
	case VN_M_FT_ANIMATION:
		return strcmp(a->frag.animation.label, b->frag.animation.label) == 0;
	case VN_M_FT_ALTERNATIVE:
		return fragment_refs_equal(node, a->frag.alternative.alt_a,
					   target, b->frag.alternative.alt_a) &&
			fragment_refs_equal(node, a->frag.alternative.alt_b,
					    target, b->frag.alternative.alt_b);
	case VN_M_FT_OUTPUT:
		printf("comparing output fragments\n");
		return strcmp(a->frag.output.label, b->frag.output.label) == 0 &&
				fragment_refs_equal(node, a->frag.output.front,
					   target, b->frag.output.front) &&
				fragment_refs_equal(node, a->frag.output.back,
					    target, b->frag.output.back);

	}
	return 0;
}

/* Find pointer to fragment in <node> that is "equal" to <f> in <source>. Somewhat complicated. */
const NdbMFragment * nodedb_m_fragment_find_equal(const NodeMaterial *node,
						  const NodeMaterial *source, const NdbMFragment *f)
{
	unsigned int		i;
	const NdbMFragment	*there;

	for(i = 0; (there = dynarr_index(node->fragments, i)) != NULL; i++)
	{
		if(there->id == (VNMFragmentID) ~0)
			continue;
		if(fragments_equal(source, f, node, there))
			return there;
	}
	return NULL;
}

int nodedb_m_fragment_resolve(VNMFragmentID *id, const NodeMaterial *node,
			      const NodeMaterial *source, VNMFragmentID f)
{
	const NdbMFragment	*frag, *tfrag;

	if(id == NULL || node == NULL || source == NULL)
		return 0;
	printf("resolving %u\n", f);
	if(f == (VNMFragmentID) ~0)	/* If reference is NULL, it resolves easily enough. */
	{
		*id = f;
		return 1;
	}
	if((frag = nodedb_m_fragment_lookup(source, f)) != NULL)
	{
		if((tfrag = nodedb_m_fragment_find_equal(node, source, frag)) != NULL)
		{
			printf("found equal fragment type %d\n", tfrag->type);
			*id = tfrag->id;
			return 1;
		}
	}
	return 0;
}

/* ----------------------------------------------------------------------------------------- */

static void cb_fragment_default(unsigned int indec, void *element, void *user)
{
	NdbMFragment	*frag = element;

	frag->id = ~0;
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
	{
		unsigned int	index;

		/* Locally created fragments do not have ID = ~0, they have their array index. */
		f = dynarr_append(node->fragments, NULL, &index);
		fragment_id = index;
	}
	else
		f = dynarr_set(node->fragments, fragment_id, NULL);
	if(f != NULL)
	{
		f->id   = fragment_id;
		f->type = type;
		f->frag = *fragment;
		f->node = NULL;
		printf("fragment %u.%u created, type=%u\n", node->node.id, fragment_id, f->type);
	}
	return f;
}

static void nodedb_m_node_ref_set(NdbMFragment *frag, const Node *node)
{
	if(frag == NULL || node == NULL)
		return;
	frag->node = node;	/* Very little work to do; the importance here is in the formalism. */
}

/* ----------------------------------------------------------------------------------------- */

static void link_set(VNMFragmentID *ptr, const NdbMFragment *fragment)
{
	*ptr = fragment != NULL ? fragment->id : ~0;
}

NdbMFragment * nodedb_m_fragment_create_color(NodeMaterial *node, real64 red, real64 green, real64 blue)
{
	VMatFrag	frag;

	frag.color.red   = red;
	frag.color.green = green;
	frag.color.blue  = blue;
	return nodedb_m_fragment_create(node, ~0, VN_M_FT_COLOR, &frag);
}

NdbMFragment * nodedb_m_fragment_create_light(NodeMaterial *node, VNMLightType type,
					      real64 normal_falloff, const Node *brdf,
					      const char *brdf_r, const char *brdf_g, const char *brdf_b)
{
	VMatFrag	frag;
	NdbMFragment	*f;

	if(node == NULL)
		return NULL;
	frag.light.type = type;
	frag.light.normal_falloff = normal_falloff;
	stu_strncpy(frag.light.brdf_r, sizeof frag.light.brdf_r, brdf_r);
	stu_strncpy(frag.light.brdf_g, sizeof frag.light.brdf_g, brdf_g);
	stu_strncpy(frag.light.brdf_b, sizeof frag.light.brdf_b, brdf_b);
	f = nodedb_m_fragment_create(node, ~0, VN_M_FT_LIGHT, &frag);
	nodedb_m_node_ref_set(f, brdf);
	return f;
}

NdbMFragment * nodedb_m_fragment_create_blender(NodeMaterial *node, VNMBlendType type,
						const PNMFragment *data_a, const PNMFragment *data_b, const PNMFragment *ctrl)
{
	VMatFrag	frag;

	if(node == NULL)
		return NULL;
	frag.blender.type = type;
	link_set(&frag.blender.data_a, data_a);
	link_set(&frag.blender.data_b, data_b);
	link_set(&frag.blender.control, ctrl);
	return nodedb_m_fragment_create(node, ~0, VN_M_FT_BLENDER, &frag);
}

NdbMFragment * nodedb_m_fragment_create_matrix(NodeMaterial *node, const real64 *matrix, const PNMFragment *data)
{
	VMatFrag	frag;

	if(node == NULL || matrix == NULL)
		return NULL;
	memcpy(frag.matrix.matrix, matrix, sizeof frag.matrix.matrix);
	link_set(&frag.matrix.data, data);
	return nodedb_m_fragment_create(node, ~0, VN_M_FT_MATRIX, &frag);
}

NdbMFragment * nodedb_m_fragment_create_output(NodeMaterial *node, const char *label,
					       const NdbMFragment *front, const NdbMFragment *back)
{
	VMatFrag	frag;

	if(node == NULL || label == NULL || *label == '\0')
		return NULL;
	stu_strncpy(frag.output.label, sizeof frag.output.label, label);
	link_set(&frag.output.front, front);
	link_set(&frag.output.back,  back);
	return nodedb_m_fragment_create(node, ~0, VN_M_FT_OUTPUT, &frag);
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
