/*
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

	key->id = ~0;
}

static int cb_key_compare(const void *d1, const void *d2)
{
	const NdbCKey	*k1 = d1, *k2 = d2;

	return k1->pos < k2->pos ? -1 : k1->pos > k2->pos;
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
		}
	}
	dynarr_destroy(n->curves);
}

/* ----------------------------------------------------------------------------------------- */

static void cb_c_curve_create(void *user, VNodeID node_id, VLayerID curve_id, const char *name, uint8 dimensions)
{
	NodeCurve	*n;

	if((n = (NodeCurve *) nodedb_lookup_with_type(node_id, V_NT_CURVE)) != NULL)
	{
		NdbCCurve	*c;

		if(n->curves == NULL)
			n->curves = dynarr_new(sizeof (NdbCKey), 4);
		if((c = dynarr_set(n->curves, curve_id, NULL)) != NULL)
		{
			c->id = curve_id;
			stu_strncpy(c->name, sizeof c->name, name);
			c->dimensions = dimensions;
			c->keys = NULL;
			printf("Curve curve %u.%u %s created, dim=%u\n", node_id, curve_id, name, c->dimensions);
			verse_send_c_curve_subscribe(node_id, curve_id);
			NOTIFY(n, STRUCTURE);
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
			c->id = 0;
			c->name[0] = '\0';
			dynarr_destroy(c->keys);
			c->keys = NULL;
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
				NdbCKey	*key;

				if(c->keys == NULL)
				{
					c->keys = dynarr_new(sizeof *key, 8);
					dynarr_set_default_func(c->keys, cb_key_default, NULL);
				}
				if((key = dynarr_set(c->keys, key_id, NULL)) != NULL)
				{
					int	i, ins = key->id == ~0;

					key->id  = key_id;
					key->pos = pos;
					for(i = 0; i < dimensions; i++)
					{
						key->value[i]      = value[i];
						key->pre.pos[i]    = pre_pos[i];
						key->pre.value[i]  = pre_value[i];
						key->post.pos[i]   = post_pos[i];
						key->post.value[i] = post_value[i];
					}
					if(ins)
						c->curve = list_insert_sorted(c->curve, key, cb_key_compare);
/*					{
						const List	*iter;

						printf("Curve: ");
						for(iter = c->curve; iter != NULL; iter = list_next(iter))
						{
							const NdbCKey	*k = list_data(iter);
							printf(" %g (%u)", k->pos, k->id);
						}
						printf("\n");
					}
*//*					printf("setting curve %u.%u key=%u\n", node_id, curve_id, key_id);
					printf("  pre:");
					for(i = 0; i < dimensions; i++)
					{
						printf(" (%u,%g)", pre_pos[i], pre_value[i]);
					}
					printf("\n");
					printf("  now: (%g,[", pos);
					for(i = 0; i < dimensions; i++)
						printf(" %g", value[i]);
					printf(" ])\n");
					printf(" post: ");
					for(i = 0; i < dimensions; i++)
						printf(" (%u,%g)", post_pos[i], post_value[i]);
					printf("\n");
*/					if(ins)
						NOTIFY(n, STRUCTURE);
					else
						NOTIFY(n, DATA);
				}
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
				k->id = ~0;
				c->curve = list_remove(c->curve, k);
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
