/*
 *
*/

#include <stdio.h>
#include <string.h>

#include "verse.h"
#include "purple.h"

#include "dynarr.h"
#include "list.h"
#include "strutil.h"
#include "textbuf.h"

#include "nodedb.h"
#include "nodedb-internal.h"

/* ----------------------------------------------------------------------------------------- */

void nodedb_c_construct(NodeCurve *n)
{
	n->curves = NULL;
}

static void cb_copy_curve(void *d, const void *s, void *user)
{
/*	const NdbTBuffer	*src = s;
	NdbTBuffer		*dst = d;

	dst->id = src->id;
	strcpy(dst->name, src->name);
	dst->text = textbuf_new(textbuf_length(src->text));
	textbuf_insert(dst->text, 0, textbuf_text(src->text));
*/}

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

static void cb_c_curve_create(void *user, VNodeID node_id, uint16 curve_id, const char *name)
{
	NodeCurve	*n;

	if((n = (NodeCurve *) nodedb_lookup_with_type(node_id, V_NT_CURVE)) != NULL)
	{
		NdbCCurve	*c;

		if((c = dynarr_set(n->curves, curve_id, NULL)) != NULL)
		{
			c->id = curve_id;
			stu_strncpy(c->name, sizeof c->name, name);
			c->keys = NULL;
			printf("Curve curve %u.%u %s created\n", node_id, curve_id, name);
			NOTIFY(n, STRUCTURE);
		}
	}
}

static void cb_c_curve_destroy(void *user, VNodeID node_id, uint16 curve_id)
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

/* ----------------------------------------------------------------------------------------- */

void nodedb_c_register_callbacks(void)
{
	verse_callback_set(verse_send_c_curve_create,	cb_c_curve_create, NULL);
	verse_callback_set(verse_send_c_curve_destroy,	cb_c_curve_destroy, NULL);
}
