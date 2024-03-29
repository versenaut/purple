/*
 * nodedb-o.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Object node databasing.
*/

#include <stdio.h>
#include <string.h>

#include "verse.h"
#include "purple.h"

#include "dynarr.h"
#include "list.h"
#include "log.h"
#include "mem.h"
#include "strutil.h"
#include "textbuf.h"

#include "nodedb.h"
#include "nodedb-internal.h"

/* ----------------------------------------------------------------------------------------- */

void nodedb_o_construct(NodeObject *n)
{
	n->pos[0] = n->pos[1] = n->pos[2] = 0.0;
	n->rot[0] = n->rot[1] = n->rot[2] = 0.0;
	n->rot[3] = 1.0;
	n->scale[0] = n->scale[1] = n->scale[2] = 1.0;
	n->light[0] = n->light[1] = n->light[2] = 0.0;
	n->links	 = NULL;
	n->links_local   = NULL;
	n->method_groups = NULL;
}

/* A helper function that initializes a freshly allocated method from a bunch of parameters. Equally
 * useful in create()-callback and when copying an existing method.
*/
static void method_set(NdbOMethod *m, uint16 method_id, const char *name, uint8 param_count,
		       const VNOParamType *param_type, const char *param_name[])
{
	unsigned int	i;
	char		*put;
	size_t		size;

	m->id = method_id;
	stu_strncpy(m->name, sizeof m->name, name);

	size = param_count * (sizeof *m->param_type + sizeof *m->param_name);
	for(i = 0; i < param_count; i++)
		size += strlen(param_name[i]) + 1;
	m->param_type = mem_alloc(size);
	memcpy(m->param_type, param_type, param_count * sizeof *m->param_type);
	m->param_name = (char **) (m->param_type + param_count);
	put = (char *) (m->param_name + param_count);
	for(i = 0; i < param_count; i++)
	{
		m->param_name[i] = put;
		strcpy(put, param_name[i]);
		put += strlen(param_name[i]) + 1;
	}
}

/* Copy a method. Simple, just set the fresh memory using the old one for parameters. */
static void cb_copy_method(void *d, const void *s, UNUSED(void *user))
{
	const NdbOMethod	*src = s;

	method_set(d, src->id, src->name, src->param_count, src->param_type, (const char **) src->param_name);
}

/* Copy a method group, including (of course) all its methods. */
static void cb_copy_method_group(void *d, const void *s, UNUSED(void *user))
{
	const NdbOMethodGroup	*src = s;
	NdbOMethodGroup		*dst = d;

	dst->id = src->id;
	strcpy(dst->name, src->name);
	dst->methods = dynarr_new_copy(src->methods, cb_copy_method, NULL);
}

void nodedb_o_copy(NodeObject *n, const NodeObject *src)
{
	memcpy(n->pos, src->pos, sizeof n->pos);
	memcpy(n->rot, src->rot, sizeof n->rot);
	memcpy(n->scale, src->scale, sizeof n->scale);
	memcpy(n->light, src->light, sizeof n->light);

	n->links = dynarr_new_copy(src->links, NULL, NULL);	/* Link data structure is monolithic. */
	if(n->links_local)	/* FIXME: Do we need to copy local links? */
		LOG_WARN(("Local links not copied"));
	n->method_groups = dynarr_new_copy(src->method_groups, cb_copy_method_group, NULL);
}

void nodedb_o_set(NodeObject *n, const NodeObject *src)
{
	/* FIXME: This could be quicker. */
	nodedb_o_destruct(n);
	nodedb_o_copy(n, src);
}

void nodedb_o_destruct(NodeObject *n)
{
	List		*iter;
	unsigned int	i;
	NdbOMethodGroup	*g;

	dynarr_destroy(n->links);
	n->links = NULL;
	for(iter = n->links_local; iter != NULL; iter = list_next(iter))
		mem_free(list_data(iter));
	list_destroy(n->links_local);
	n->links_local = NULL;
	for(i = 0; i < dynarr_size(n->method_groups); i++)
	{
		if((g = dynarr_index(n->method_groups, i)) && g->name[0] != '\0')
		{
			unsigned int	j;
			NdbOMethod	*m;

			for(j = 0; j < dynarr_size(g->methods); j++)
			{
				if((m = dynarr_index(g->methods, j)) && m->name[0] != '\0')
				{
					mem_free(m->param_type);
				}
			}
			dynarr_destroy(g->methods);
		}
	}
	dynarr_destroy(n->method_groups);
	n->method_groups = NULL;
}

void nodedb_o_pos_set(NodeObject *n, const real64 *pos)
{
	if(n != NULL && pos != NULL)
		memcpy(n->pos, pos, sizeof n->pos);
}

void nodedb_o_pos_get(const NodeObject *n, real64 *pos)
{
	if(n != NULL && pos != NULL)
		memcpy(pos, n->pos, sizeof n->pos);
}

void nodedb_o_rot_set(NodeObject *n, const real64 *rot)
{
	if(n != NULL && rot != NULL)
		memcpy(n->rot, rot, sizeof n->rot);
}

void nodedb_o_rot_get(const NodeObject *n, real64 *rot)
{
	if(n != NULL && rot != NULL)
		memcpy(rot, n->rot, sizeof n->rot);
}

void nodedb_o_light_set(NodeObject *n, real64 red, real64 green, real64 blue)
{
	if(n == NULL)
		return;
	n->light[0] = red;
	n->light[1] = green;
	n->light[2] = blue;
}

void nodedb_o_light_get(const NodeObject *n, real64 *red, real64 *green, real64 *blue)
{
	if(n == NULL)
		return;
	if(red != NULL)
		*red = n->light[0];
	if(green != NULL)
		*green = n->light[1];
	if(blue != NULL)
		*blue = n->light[2];
}

#if 0
static void link_dump(const NodeObject *n)
{
	unsigned int	i;
	const NdbOLink	*rl;
	const List	*iter;

	printf("Links in node %u (\"%s\") (remote=%p):\n", n->node.id, n->node.name, n->node.creator.remote);
	printf(" remote:\n");
	for(i = 0; (rl = dynarr_index(n->links, i)) != NULL; i++)
	{
		if(rl->id == (uint16) ~0u)
			continue;
		printf("  id=%u node=%u label=\"%s\" target_id=%u deleted=%s\n", rl->id, rl->link, rl->label, rl->target_id, rl->deleted ? "TRUE" : "FALSE");
	}
	printf(" local:\n");
	for(iter = n->links_local; iter != NULL; iter = list_next(iter))
	{
		const NdbOLinkLocal	*ll = list_data(iter);

		printf("  node=\"%s\" label=\"%s\" target_id=%u\n", ll->link->name, ll->label, ll->target_id);
	}
}
#endif

void nodedb_o_link_set(NodeObject *n, uint16 link_id, VNodeID link, const char *label, uint32 target_id)
{
	NdbOLink	*l;
	unsigned int	i;

	if(n == NULL || n->node.type != V_NT_OBJECT)
		return;
	if(n->links == NULL)
		n->links = dynarr_new(sizeof (NdbOLink), 1);

	/* Look for existing link with the same label and target_id combo. */
	for(i = 0; (l = dynarr_index(n->links, i)) != NULL; i++)
	{
		if(l->label[0] == '\0')
			continue;
		if(l->target_id == target_id && strcmp(l->label, label) == 0)
		{
			l->link = link;
			return;
		}
	}
	l = dynarr_set(n->links, link_id, NULL);
	l->id = link_id;
	l->link = link;
	stu_strncpy(l->label, sizeof l->label, label);
	l->target_id = target_id;
	l->deleted = FALSE;
	printf("stored link set %u->%u, ID %u, label '%s' target %u\n", n->node.id, link, link_id, label, target_id);
}

void nodedb_o_link_destroy(NodeObject *n, uint16 link_id)
{
	NdbOLink	*lnk;

	if(n == NULL || n->node.type != V_NT_OBJECT)
		return;

	if((lnk = dynarr_index(n->links, link_id)) == NULL)
		return;
	lnk->id = (uint16) ~0u;	/* This marks the link as gone. */
	lnk->link = ~0u;
	lnk->label[0] = '\0';
	printf("link %u.%u destroyed\n", n->node.id, link_id);
}

void nodedb_o_link_set_local(NodeObject *n, PINode *link, const char *label, uint32 target_id)
{
	NdbOLinkLocal	*l;

	if(n == NULL || n->node.type != V_NT_OBJECT)
		return;
	printf("in link_set_local(), target=%p label=\"%s\" id=%u\n", link, label, target_id);
	/* Check if equivalent link already exists, and if so don't add it. Saves synchronizer some work. */
	if(link->id != ~0u)
	{
		unsigned int	i;
		NdbOLink	*l;

		for(i = 0; (l = dynarr_index(n->links, i)) != NULL; i++)
		{
			printf(" checking, is %u == %u?\n", l->link, link->id);
			if(l->target_id == target_id && strcmp(l->label, label) == 0)
			{
				printf("link from %u to %u with label \"%s\" exists on host, setting to %u\n", n->node.id,
				       l->link, label, link->id);
				l->link = link->id;
				printf("link from %u to %u is known on host side, ignoring\n", n->node.id, link->id);
				return;
			}
		}
	}
	else if(link->id == (VNodeID) ~0)	/* Don't duplicate local links. */
	{
		const List	*iter;

		for(iter = n->links_local; iter != NULL; iter = list_next(iter))
		{
			NdbOLinkLocal	*l = list_data(iter);

			if(l->target_id == target_id && strcmp(l->label, label) == 0)
			{
				printf("local link from %u to %u with label \"%s\" already exists, setting to %u\n", n->node.id,
				       l->link->id, l->label, link->id);
				l->link = (PONode *) link;
				return;
			}
		}
	}
	printf("no existing link detected, creating new local\n");
	l = mem_alloc(sizeof *l);
	l->link = (PONode *) link;
	stu_strncpy(l->label, sizeof l->label, label);
	l->target_id = target_id;
	n->links_local = list_prepend(n->links_local, l);
	printf("'local' link set to node %u\n", link->id);
}

void nodedb_o_link_set_local_single(NodeObject *n, PINode *link, const char *label)
{
	unsigned int	i;
	const List	*iter;
	NdbOLink	*l;
	NdbOLinkLocal	*lnk;

	if(n == NULL || link == NULL || label == NULL || *label == '\0')
		return;

	/* Check if there already is a pending local link with the same label. */
	for(iter = n->links_local; iter != NULL; iter = list_next(iter))
	{
		lnk = list_data(iter);
		if(strcmp(lnk->label, label) == 0)
		{
/*			printf(" found local link with label \"%s\", replacing\n", label);*/
			lnk->link = (PONode *) link;
			return;
		}
	}
	/* Check for existing "real" links. */
	for(i = 0; (l = dynarr_index(n->links, i)) != NULL; i++)
	{
		if(l->label[0] == '\0')
			continue;
		if(strcmp(l->label, label) == 0)
		{
			l->deleted = TRUE;	/* Mark link as deleted, so synchronizer can do its thing. Then keep looking. */
		}
	}
	lnk = mem_alloc(sizeof *l);
	lnk->link = (PONode *) link;
	stu_strncpy(lnk->label, sizeof l->label, label);
	lnk->target_id = ~0u;
	n->links_local = list_prepend(n->links_local, lnk);
}

PONode * nodedb_o_link_get_local(const NodeObject *n, const char *label, uint32 *target_id)
{
	const List	*iter;

	if(n == NULL || label == NULL)
		return 0;
	for(iter = n->links_local; iter != NULL; iter = list_next(iter))
	{
		NdbOLinkLocal	*l = list_data(iter);

		if(strcmp(l->label, label) == 0)
		{
			if(target_id != NULL)
				*target_id = l->target_id;
			return l->link;
		}
	}
	return NULL;
}

PINode * nodedb_o_link_get(const NodeObject *n, const char *label, uint32 *target_id)
{
	const NdbOLink	*link;
	unsigned int	i;

	for(i = 0; (link = dynarr_index(n->links, i)) != NULL; i++)
	{
		if(link->label != NULL && strcmp(link->label, label) == 0)
		{
			if(target_id != NULL)
				*target_id = link->target_id;
			return nodedb_lookup(link->link);
		}
	}
	return NULL;
}

NdbOMethodGroup * nodedb_o_method_group_lookup(NodeObject *node, const char *name)
{
	unsigned int	i;
	NdbOMethodGroup	*g;

	if(node == NULL || name == NULL || *name == '\0')
		return NULL;
	for(i = 0; (g = dynarr_index(node->method_groups, i)) != NULL; i++)
	{
		if(strcmp(g->name, name) == 0)
			return g;
	}
	return NULL;
}

const NdbOMethod * nodedb_o_method_lookup(const NdbOMethodGroup *group, const char *name)
{
	unsigned int	i;
	NdbOMethod	*m;

	if(group == NULL || name == NULL || *name == '\0')
		return NULL;
	for(i = 0; (m = dynarr_index(group->methods, i)) != NULL; i++)
	{
		if(strcmp(m->name, name) == 0)
			return m;
	}
	return NULL;
}

const NdbOMethod * nodedb_o_method_lookup_id(const NdbOMethodGroup *group, uint8 id)
{
	const NdbOMethod	*m;

	if(group == NULL)
		return NULL;
	if((m = dynarr_index(group->methods, id)) == NULL)
		return NULL;
	return m;
}

/* ----------------------------------------------------------------------------------------- */

static void cb_o_transform_rot_real64(UNUSED(void *user), VNodeID node_id, UNUSED(uint32 time_s), UNUSED(uint32 time_f),
				      const VNQuat64 *rot, UNUSED(const VNQuat64 *speed), UNUSED(const VNQuat64 *accelerate),
				      UNUSED(const VNQuat64 *drag_normal), UNUSED(real64 drag))
{
	NodeObject	*n;

	if((n = nodedb_lookup_object(node_id)) != NULL && rot != NULL)
	{
		real64	r[4];

		r[0] = rot->x;
		r[1] = rot->y;
		r[2] = rot->z;
		r[3] = rot->w;
		nodedb_o_rot_set(n, r);
		NOTIFY(n, DATA);
	}
}

static void cb_o_transform_pos_real64(UNUSED(void *user), VNodeID node_id, UNUSED(uint32 time_s), UNUSED(uint32 time_f),
				      const real64 *pos, UNUSED(const real64 *speed), UNUSED(const real64 *accelerate),
				      UNUSED(const real64 *drag_normal), UNUSED(real64 drag))
{
	NodeObject	*n;

	if((n = nodedb_lookup_object(node_id)) != NULL && pos != NULL)
	{
		nodedb_o_pos_set(n, pos);
		NOTIFY(n, DATA);
	}
}

static void cb_o_link_set(UNUSED(void *user), VNodeID node_id, uint16 link_id, VNodeID link, const char *label, uint32 target_id)
{
	NodeObject	*n;

	if((n = nodedb_lookup_object(node_id)) != NULL)
	{
		nodedb_o_link_set(n, link_id, link, label, target_id);
		NOTIFY(n, DATA);
	}
}

static void cb_o_link_destroy(UNUSED(void *user), VNodeID node_id, uint16 link_id)
{
	NodeObject	*n;

	if((n = nodedb_lookup_object(node_id)) != NULL)
	{
		nodedb_o_link_destroy(n, link_id);
		NOTIFY(n, DATA);
	}
}

static void cb_o_light_set(UNUSED(void *user), VNodeID node_id, real64 r, real64 g, real64 b)
{
	NodeObject	*n;

	if((n = nodedb_lookup_object(node_id)) != NULL)
	{
		n->light[0] = r;
		n->light[1] = g;
		n->light[2] = b;
		NOTIFY(n, DATA);
	}
}

static void cb_o_method_group_create(UNUSED(void *user), VNodeID node_id, uint16 group_id, const char *name)
{
	NodeObject	*n;

	if((n = nodedb_lookup_object(node_id)) != NULL)
	{
		NdbOMethodGroup	*g;

		if(n->method_groups == NULL)
			n->method_groups = dynarr_new(sizeof (NdbOMethodGroup), 1);
		if((g = dynarr_index(n->method_groups, group_id)) != NULL && strcmp(g->name, name) == 0)
			;
		else if((g = dynarr_set(n->method_groups, group_id, NULL)) != NULL)
		{
			g->id = group_id;
			stu_strncpy(g->name, sizeof g->name, name);
			g->methods = NULL;
			verse_send_o_method_group_subscribe(node_id, group_id);
			NOTIFY(n, STRUCTURE);
		}
	}
}

static void cb_o_method_group_destroy(UNUSED(void *user), VNodeID node_id, uint16 group_id, UNUSED(const char *name))
{
	NodeObject	*n;

	if((n = nodedb_lookup_object(node_id)) != NULL)
	{
		NdbOMethodGroup	*g;

		if((g = dynarr_index(n->method_groups, group_id)) != NULL)
		{
			unsigned int	i;
			NdbOMethod	*m;

			g->id = 0;
			g->name[0] = '\0';
			for(i = 0; (m = dynarr_index(g->methods, i)) != NULL; i++)
			{
				if(m->name[i] == '\0')
					continue;
				mem_free(m->param_type);
			}
			dynarr_destroy(g->methods);
			NOTIFY(n, STRUCTURE);
		}
	}
}

static void cb_method_default(UNUSED(unsigned int index), void *element, UNUSED(void *user))
{
	NdbOMethod	*m = element;

	m->name[0] = '\0';
}

static void cb_o_method_create(UNUSED(void *user), VNodeID node_id, uint16 group_id, uint8 method_id, const char *name,
			       uint8 param_count, const VNOParamType *param_type, const char *param_name[])
{
	NodeObject	*n;

	if((n = nodedb_lookup_object(node_id)) != NULL)
	{
		NdbOMethodGroup	*g;

		if((g = dynarr_index(n->method_groups, group_id)) != NULL)
		{
			NdbOMethod	*m;

			if(g->methods == NULL)
			{
				g->methods = dynarr_new(sizeof (NdbOMethod), 4);
				dynarr_set_default_func(g->methods, cb_method_default, NULL);
			}
			if((m = dynarr_set(g->methods, method_id, NULL)) != NULL)
			{
				method_set(m, method_id, name, param_count, param_type, param_name);
				NOTIFY(n, STRUCTURE);
			}
		}
	}
}

/* ----------------------------------------------------------------------------------------- */

void nodedb_o_hide_set(NodeObject *n, boolean hidden)
{
	if(n == NULL)
		return;
	n->hidden = hidden;
}

boolean nodedb_o_hide_get(const NodeObject *n)
{
	return n != NULL ? n->hidden : FALSE;
}

/* ----------------------------------------------------------------------------------------- */

void nodedb_o_register_callbacks(void)
{
	verse_callback_set(verse_send_o_transform_pos_real64,	cb_o_transform_pos_real64, NULL);
	verse_callback_set(verse_send_o_transform_rot_real64,	cb_o_transform_rot_real64, NULL);
	verse_callback_set(verse_send_o_link_set,		cb_o_link_set, NULL);
	verse_callback_set(verse_send_o_link_destroy,		cb_o_link_destroy, NULL);
	verse_callback_set(verse_send_o_light_set,		cb_o_light_set, NULL);
	verse_callback_set(verse_send_o_method_group_create,	cb_o_method_group_create, NULL);
	verse_callback_set(verse_send_o_method_group_destroy,	cb_o_method_group_destroy, NULL);
	verse_callback_set(verse_send_o_method_create,		cb_o_method_create, NULL);
}
