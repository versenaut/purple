/*
 * nodedb-m.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Material node databasing.
*/

#include <stddef.h>
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

/* Set node reference in a material fragment. */
static void node_ref_set(NdbMFragment *frag, VNodeID *reffield, const Node *node)
{
	if(frag == NULL || reffield == NULL)
		return;
	if(node != NULL)
		*reffield = node->id;
	else
		*reffield = (VNodeID) ~0;
	frag->node = node;	/* Very little work to do; the importance here is in the formalism. */
	printf("set fragment; frag=%p field=%u node=%p\n", frag, *reffield, frag->node);
}

/* Compare node references in two material fragments, knowing that they have an embedded
 * VNodeID member at <refoff> into the union. Uses direct node pointer if set, else reads
 * out the embedded value.
*/
static int node_ref_equal(const NdbMFragment *a, const NdbMFragment *b, size_t refoff)
{
	VNodeID	ra, rb;

	ra = a->node != NULL ? a->node->id : *(const VNodeID *) (((const unsigned char *) &a->frag) + refoff);
	rb = b->node != NULL ? b->node->id : *(const VNodeID *) (((const unsigned char *) &b->frag) + refoff);

/*	printf("got node refs from %u in %p and %p; %u and %u\n", refoff, a, b, ra, rb);*/
	return ra == rb;
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
			return node_ref_equal(a, b, offsetof(VMatFrag, light.brdf)) &&
				a->frag.light.normal_falloff == b->frag.light.normal_falloff &&
				strcmp(a->frag.light.brdf_r, b->frag.light.brdf_r) == 0 &&
				strcmp(a->frag.light.brdf_g, b->frag.light.brdf_g) == 0 &&
				strcmp(a->frag.light.brdf_b, b->frag.light.brdf_b) == 0;
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
		if(fragment_refs_equal(node, a->frag.texture.mapping, target, b->frag.texture.mapping))
		{
			return node_ref_equal(a, b, offsetof(VMatFrag, texture.bitmap)) &&
				strcmp(a->frag.texture.layer_r, b->frag.texture.layer_r) == 0 &&
				strcmp(a->frag.texture.layer_g, b->frag.texture.layer_g) == 0 &&
				strcmp(a->frag.texture.layer_b, b->frag.texture.layer_b) == 0;
		}
		return 0;
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
		return strcmp(a->frag.output.label, b->frag.output.label) == 0 &&
			fragment_refs_equal(node, a->frag.output.front, target, b->frag.output.front) &&
			fragment_refs_equal(node, a->frag.output.back,  target, b->frag.output.back);
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
	if(f == (VNMFragmentID) ~0)	/* If reference is NULL, it resolves easily enough. */
	{
		*id = f;
		return 1;
	}
	if((frag = nodedb_m_fragment_lookup(source, f)) != NULL)
	{
		if((tfrag = nodedb_m_fragment_find_equal(node, source, frag)) != NULL)
		{
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
		f->node = NULL;		/* Always set after creation, when used (light, texture). */
		f->pending = 0;
		printf("fragment %u.%u created, type=%u\n", node->node.id, fragment_id, f->type);
	}
	return f;
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
	stu_strncpy_accept_null(frag.light.brdf_r, sizeof frag.light.brdf_r, brdf_r);
	stu_strncpy_accept_null(frag.light.brdf_g, sizeof frag.light.brdf_g, brdf_g);
	stu_strncpy_accept_null(frag.light.brdf_b, sizeof frag.light.brdf_b, brdf_b);
	f = nodedb_m_fragment_create(node, ~0, VN_M_FT_LIGHT, &frag);
	node_ref_set(f, &f->frag.light.brdf, brdf);
	return f;
}

NdbMFragment * nodedb_m_fragment_create_reflection(NodeMaterial *node, real64 normal_falloff)
{
	VMatFrag	frag;

	if(node == NULL)
		return NULL;
	frag.reflection.normal_falloff = normal_falloff;
	return nodedb_m_fragment_create(node, ~0, VN_M_FT_REFLECTION, &frag);
}

NdbMFragment * nodedb_m_fragment_create_transparency(NodeMaterial *node, real64 normal_falloff, real64 refraction_index)
{
	VMatFrag	frag;

	if(node == NULL)
		return NULL;
	frag.transparency.normal_falloff = normal_falloff;
	frag.transparency.refraction_index = refraction_index;
	return nodedb_m_fragment_create(node, ~0, VN_M_FT_TRANSPARENCY, &frag);
}

NdbMFragment * nodedb_m_fragment_create_volume(NodeMaterial *node, real64 diffusion,
					       real64 col_r, real64 col_g, real64 col_b, const NdbMFragment *color)
{
	VMatFrag	frag;

	if(node == NULL)
		return NULL;
	frag.volume.diffusion = diffusion;
	frag.volume.col_r = col_r;
	frag.volume.col_g = col_g;
	frag.volume.col_b = col_b;
	link_set(&frag.volume.color, color);
	return nodedb_m_fragment_create(node, ~0, VN_M_FT_VOLUME, &frag);
}

NdbMFragment * nodedb_m_fragment_create_geometry(NodeMaterial *node, const char *layer_r, const char *layer_g,
						 const char *layer_b)
{
	VMatFrag	frag;

	if(node == NULL)
		return NULL;
	stu_strncpy_accept_null(frag.geometry.layer_r, sizeof frag.geometry.layer_r, layer_r);
	stu_strncpy_accept_null(frag.geometry.layer_g, sizeof frag.geometry.layer_g, layer_g);
	stu_strncpy_accept_null(frag.geometry.layer_b, sizeof frag.geometry.layer_b, layer_b);
	return nodedb_m_fragment_create(node, ~0, VN_M_FT_GEOMETRY, &frag);
}

NdbMFragment * nodedb_m_fragment_create_texture(NodeMaterial *node, const Node *bitmap,
							 const char *layer_r, const char *layer_g, const char *layer_b,
							 const NdbMFragment *mapping)
{
	VMatFrag	frag;
	NdbMFragment	*f;

	if(node == NULL)
		return NULL;
	if(bitmap != NULL)
		frag.texture.bitmap = bitmap->id;
	stu_strncpy_accept_null(frag.texture.layer_r, sizeof frag.texture.layer_r, layer_r);
	stu_strncpy_accept_null(frag.texture.layer_g, sizeof frag.texture.layer_g, layer_g);
	stu_strncpy_accept_null(frag.texture.layer_b, sizeof frag.texture.layer_b, layer_b);
	link_set(&frag.texture.mapping, mapping);
	f = nodedb_m_fragment_create(node, ~0, VN_M_FT_TEXTURE, &frag);
	node_ref_set(f, &f->frag.texture.bitmap, bitmap);
	return f;
}

NdbMFragment * nodedb_m_fragment_create_noise(NodeMaterial *node, VNMNoiseType type, const NdbMFragment *mapping)
{
	VMatFrag	frag;

	if(node == NULL)
		return NULL;
	frag.noise.type = type;
	link_set(&frag.noise.mapping, mapping);
	return nodedb_m_fragment_create(0, ~0, VN_M_FT_NOISE, &frag);
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

NdbMFragment * nodedb_m_fragment_create_ramp(NodeMaterial *node, VNMRampType type, uint8 channel,
					     const NdbMFragment *mapping, uint8 point_count, const VNMRampPoint *ramp)
{
	VMatFrag	frag;

	if(node == NULL || point_count > 48)
		return NULL;
	frag.ramp.type = type;
	frag.ramp.channel = channel;
	link_set(&frag.ramp.mapping, mapping);
	frag.ramp.point_count = point_count;
	memcpy(frag.ramp.ramp, ramp, point_count * sizeof *frag.ramp.ramp);
	return nodedb_m_fragment_create(node, ~0, VN_M_FT_RAMP, &frag);
}

NdbMFragment * nodedb_m_fragment_create_animation(NodeMaterial *node, const char *label)
{
	VMatFrag	frag;

	if(node == NULL || label == NULL || *label == '\0')
		return NULL;
	stu_strncpy_accept_null(frag.animation.label, sizeof frag.animation.label, label);
	return nodedb_m_fragment_create(node, ~0, VN_M_FT_ANIMATION, &frag);
}

NdbMFragment * nodedb_m_fragment_create_alternative(NodeMaterial *node, const NdbMFragment *alt_a,
						    const NdbMFragment *alt_b)
{
	VMatFrag	frag;

	if(node == NULL)
		return NULL;
	link_set(&frag.alternative.alt_a, alt_a);
	link_set(&frag.alternative.alt_b, alt_b);
	return nodedb_m_fragment_create(node, ~0, VN_M_FT_ALTERNATIVE, &frag);
}

NdbMFragment * nodedb_m_fragment_create_output(NodeMaterial *node, const char *label,
					       const NdbMFragment *front, const NdbMFragment *back)
{
	VMatFrag	frag;

	if(node == NULL || label == NULL || *label == '\0')
		return NULL;
	stu_strncpy_accept_null(frag.output.label, sizeof frag.output.label, label);
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
