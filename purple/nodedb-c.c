/*
 * nodedb-c.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 *
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

void nodedb_c_construct(NodeCurve *n)
{
	n->curves = NULL;
}

static void cb_key_default(unsigned int index, void *element, void *user)
{
	NdbCKey	*key = element;

	key->pos = V_REAL64_MAX;
}

/* Comparison function for inserting keys into lists, using list_insert_sorted(). */
static int cb_key_compare(const void *d1, const void *d2)
{
	const NdbCKey	*k1 = d1, *k2 = d2;

	return k1->pos < k2->pos ? -1 : k1->pos > k2->pos;
}

/* Compare data from list, <listdata>, against <data> which should be pointer to pos. Returns -1, 0 or 1. */
static int cb_key_compare_find(const void *listdata, const void *data)
{
	const NdbCKey	*k = listdata;
	real64		pos = *(real64 *) data;

	return k->pos < pos ? -1 : k->pos > pos;
}

static void cb_copy_curve(void *d, const void *s, void *user)
{
	const NdbCCurve	*src = s;
	NdbCCurve	*dst = d;
	NdbCKey		*key;
	unsigned int	i;

	dst->id = src->id;
	strcpy(dst->name, src->name);
	dst->dimensions = src->dimensions;
	dst->keys = dynarr_new_copy(src->keys, NULL, NULL);	/* Keys are trivially copyable. */
	dst->curve = NULL;
	for(i = 0; (key = dynarr_index(dst->keys, i)) != NULL; i++)
		dst->curve = list_insert_sorted(dst->curve, key, cb_key_compare);
}

void nodedb_c_copy(NodeCurve *n, const NodeCurve *src)
{
	n->curves = dynarr_new_copy(src->curves, cb_copy_curve, NULL);
}

void nodedb_c_destruct(NodeCurve *n)
{
	unsigned int	i, num;

	num = dynarr_size(n->curves);
	for(i = 0; i < num; i++)
	{
		NdbCCurve	*c;

		if((c = dynarr_index(n->curves, i)) != NULL && c->name[0] != '\0')
		{
			printf("destroying curve %u\n", i);
			dynarr_destroy(c->keys);
			list_destroy(c->curve);
		}
	}
	dynarr_destroy(n->curves);
}

/* ----------------------------------------------------------------------------------------- */

unsigned int nodedb_c_curve_num(const NodeCurve *node)
{
	unsigned int	i, num;
	const NdbCCurve	*curve;

	if(node == NULL)
		return 0;
	for(i = num = 0; (curve = dynarr_index(node->curves, i)) != NULL; i++)
	{
		if(curve->name[0] == '\0')
			continue;
		num++;
	}
	return num;
}

NdbCCurve * nodedb_c_curve_nth(const NodeCurve *node, unsigned int n)
{
	unsigned int	i;
	NdbCCurve	*curve;

	if(node == NULL)
		return NULL;
	for(i = 0; (curve = dynarr_index(node->curves, i)) != NULL; i++)
	{
		if(curve->name[0] == '\0')
			continue;
		if(n == 0)
			return curve;
		n--;
	}
	return NULL;
}

NdbCCurve * nodedb_c_curve_find(const NodeCurve *node, const char *name)
{
	unsigned int	i;
	NdbCCurve	*curve;

	if(node == NULL || name == NULL || *name == '\0')
		return NULL;
	for(i = 0; ((curve = dynarr_index(node->curves, i)) != NULL); i++)
	{
		if(strcmp(curve->name, name) == 0)
			return curve;
	}
	return NULL;
}

/* ----------------------------------------------------------------------------------------- */

static void cb_curve_default(unsigned int indec, void *element, void *user)
{
	NdbCCurve	*curve = element;

	curve->id = ~0;
	curve->name[0] = '\0';
}

NdbCCurve * nodedb_c_curve_create(NodeCurve *node, VLayerID curve_id, const char *name, uint8 dimensions)
{
	NdbCCurve	*curve;

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
}

uint8 nodedb_c_curve_dimensions_get(const NdbCCurve *curve)
{
	if(curve == NULL)
		return 0;
	return curve->dimensions;
}

unsigned int nodedb_c_curve_key_num(const NdbCCurve *curve)
{
	if(curve == NULL)
		return 0;
	return list_length(curve->curve);
}

NdbCKey * nodedb_c_curve_key_nth(const NdbCCurve *curve, unsigned int n)
{
	List	*iter;

	if(curve == NULL)
		return NULL;
	for(iter = curve->curve; iter != NULL; iter = list_next(iter), n--)
	{
		if(n == 0)
			return list_data(iter);
	}
	return NULL;
}

NdbCKey * nodedb_c_curve_key_find(const NdbCCurve *curve, real64 pos)
{
	const List	*iter;
	NdbCKey		*key;

	if(curve == NULL)
		return NULL;
	for(iter = curve->curve; iter != NULL; iter = list_next(iter))
	{
		key = list_data(iter);
		if(key->pos == pos)
			return key;
		if(key->pos > pos)	/* List is sorted, so we can exit a bit quicker. */
			return NULL;
	}
	return NULL;
}

int nodedb_c_curve_key_equal(const NdbCCurve *curve, const NdbCKey *k1, const NdbCKey *k2)
{
	int	i;

	if(curve == NULL || k1 == NULL || k2 == NULL)
		return 0;
	if(k1->pos != k2->pos)
		return 0;
	for(i = 0; i < curve->dimensions; i++)
	{
		if(k1->value[i] != k2->value[i])
			return 0;
		if(k1->pre.pos[i] != k2->pre.pos[i])
			return 0;
		if(k1->pre.value[i] != k2->pre.value[i])
			return 0;
		if(k1->post.pos[i] != k2->post.pos[i])
			return 0;
		if(k1->post.value[i] != k2->post.value[i])
			return 0;
	}
	return 1;
}

NdbCKey * nodedb_c_key_create(NdbCCurve *curve, uint32 key_id,
				     real64 pos, const real64 *value,
				     const uint32 *pre_pos, const real64 *pre_value,
				     const uint32 *post_pos, const real64 *post_value)
{
	NdbCKey	*key;
	int	insert = 0, sort = 0, i;

	if(curve->keys == NULL)
	{
		curve->keys = dynarr_new(sizeof *key, 8);
		dynarr_set_default_func(curve->keys, cb_key_default, NULL);
	}
	if(curve->keys == NULL)
		return NULL;
	if(key_id == ~0)
	{
		/* If creating a new key, make sure it's not clobbering an existing position. Search. */
		List	*old;

		if((old = list_find_sorted(curve->curve, &pos, cb_key_compare_find)) != NULL)
			key = list_data(old);	/* Just re-use the same slot, since we're not changing position. */
		else
		{
			key = dynarr_append(curve->keys, NULL, NULL);
			insert = 1;
		}
	}
	else
	{
		key = dynarr_set(curve->keys, key_id, NULL);
		sort = 1;	/* We might re-sort. Turned off if new pos equals old, below. */
	}
	if(key == NULL)
		return NULL;
	key->id = key_id;
	if(sort)
		sort = key->pos != pos;	/* Only resort if new position differs from the one we're reusing. */
	key->pos = pos;
	for(i = 0; i < curve->dimensions; i++)
	{
		key->value[i]      = value[i];
		key->pre.pos[i]    = pre_pos[i];
		key->pre.value[i]  = pre_value[i];
		key->post.pos[i]   = post_pos[i];
		key->post.value[i] = post_value[i];
	}
	if(insert)
	{
		printf("inserting key in list\n");
		curve->curve = list_insert_sorted(curve->curve, key, cb_key_compare);
	}
	else if(sort)
	{
		List	*nl = NULL, *iter;

		printf("resorting list\n");
		/* This is considered a rare event, so we can be a bit expensive. */
		nl = list_insert_sorted(nl, key, cb_key_compare);	/* Get the new one in there. */
		for(iter = curve->curve; iter != NULL; iter = list_next(iter))
			nl = list_insert_sorted(nl, list_data(iter), cb_key_compare);
		list_destroy(curve->curve);
		curve->curve = nl;
		printf("key list resorted\n");
	}
	return key;
}

void nodedb_c_key_destroy(NdbCCurve *curve, NdbCKey *key)
{
	if(curve == NULL || key == NULL)
		return;
	curve->curve = list_remove(curve->curve, key);
	key->pos = V_REAL64_MAX;
}

void nodedb_c_curve_destroy(NodeCurve *node, NdbCCurve *curve)
{
	if(node == NULL || curve == NULL)
		return;
	list_destroy(curve->curve);
	dynarr_destroy(curve->keys);
	curve->name[0] = '\0';
	curve->id = -1;
}

/* ----------------------------------------------------------------------------------------- */

static void cb_c_curve_create(void *user, VNodeID node_id, VLayerID curve_id, const char *name, uint8 dimensions)
{
	NodeCurve	*node;

	if((node = (NodeCurve *) nodedb_lookup_with_type(node_id, V_NT_CURVE)) != NULL)
	{
		NdbCCurve	*curve;

		if((curve = dynarr_index(node->curves, curve_id)) != NULL && curve->name[0] != '\0' && strcmp(curve->name, name) != 0)
		{
			LOG_WARN(("Layer already exists--unhandled case"));
			return;
		}
		if((curve = nodedb_c_curve_create(node, curve_id, name, dimensions)) != NULL)
		{
			verse_send_c_curve_subscribe(node_id, curve_id);
			NOTIFY(node, STRUCTURE);
		}
	}
	else
		LOG_WARN(("Can't create curve in unknown curve node %u", node_id));
}

static void cb_c_curve_destroy(void *user, VNodeID node_id, VLayerID curve_id)
{
	NodeCurve	*n;

	if((n = (NodeCurve *) nodedb_lookup_with_type(node_id, V_NT_TEXT)) != NULL)
	{
		NdbCCurve	*c;

		if((c = dynarr_index(n->curves, curve_id)) != NULL)
		{
			nodedb_c_curve_destroy(n, c);
			NOTIFY(n, STRUCTURE);
		}
	}
}

static void cb_c_key_set(void *user, VNodeID node_id, VLayerID curve_id, uint32 key_id, uint8 dimensions,
			 real64 *pre_value, uint32 *pre_pos, real64 *value, real64 pos, real64 *post_value, uint32 *post_pos)
{
	NodeCurve	*n;

	if((n = (NodeCurve *) nodedb_lookup_with_type(node_id, V_NT_CURVE)) != NULL)
	{
		NdbCCurve	*c;

		if((c = dynarr_index(n->curves, curve_id)) != NULL)
		{
			if(c->dimensions == dimensions)
			{
				nodedb_c_key_create(c, key_id, pos, value, pre_pos, pre_value, post_pos, post_value);
				NOTIFY(n, DATA);
			}
			else
				LOG_WARN(("Got key_set with wrong dimensions, %u is not %u", dimensions, c->dimensions));
		}
		else
			LOG_WARN(("Got c_key_set in unknown curve %u.%u\n", node_id, curve_id));
	}
}

static void cb_c_key_destroy(void *user, VNodeID node_id, VLayerID curve_id, uint32 key_id)
{
	NodeCurve	*n;

	if((n = (NodeCurve *) nodedb_lookup_with_type(node_id, V_NT_CURVE)) != NULL)
	{
		NdbCCurve	*c;

		if((c = dynarr_index(n->curves, curve_id)) != NULL)
		{
			NdbCKey	*k;

			if((k = dynarr_index(c->keys, key_id)) != NULL)
			{
				nodedb_c_key_destroy(c, k);
				NOTIFY(n, STRUCTURE);
			}
		}
	}
}

/* ----------------------------------------------------------------------------------------- */

void nodedb_c_register_callbacks(void)
{
	verse_callback_set(verse_send_c_curve_create,	cb_c_curve_create, NULL);
	verse_callback_set(verse_send_c_curve_destroy,	cb_c_curve_destroy, NULL);
	verse_callback_set(verse_send_c_key_set,	cb_c_key_set, NULL);
	verse_callback_set(verse_send_c_key_destroy,	cb_c_key_destroy, NULL);
}
