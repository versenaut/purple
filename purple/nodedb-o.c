/*
 * Object node databasing.
*/

#include <stdio.h>
#include <string.h>

#include "verse.h"

#include "dynarr.h"
#include "mem.h"
#include "strutil.h"
#include "textbuf.h"

#include "nodedb.h"
#include "nodedb-internal.h"

/* ----------------------------------------------------------------------------------------- */

void nodedb_o_init(NodeObject *n)
{
	n->light[0] = n->light[1] = n->light[2] = 0.0;
	n->links = NULL;
	n->method_groups = NULL;
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

static void cb_o_link_set(void *user, VNodeID node_id, uint16 link_id, uint32 link, const char *name, uint32 target_id)
{
	NodeObject	*n;

	if((n = nodedb_lookup_object(node_id)) != NULL)
	{
		NdbOLink	*l;

		if(n->links == NULL)
			n->links = dynarr_new(sizeof (NdbOLink), 1);
		if((l = dynarr_set(n->links, link_id, NULL)) != NULL)
		{
			l->id = link_id;
			l->link = link;
			stu_strncpy(l->name, sizeof l->name, name);
			l->target = target_id;
			NOTIFY(n, DATA);
		}
	}
}

static void cb_o_light_set(void *user, VNodeID node_id, real64 r, real64 g, real64 b)
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

static void cb_o_method_group_create(void *user, VNodeID node_id, uint16 group_id, const char *name)
{
	NodeObject	*n;

	if((n = nodedb_lookup_object(node_id)) != NULL)
	{
		NdbOMethodGroup	*g;

		if(n->method_groups == NULL)
			n->method_groups = dynarr_new(sizeof (NdbOMethodGroup), 1);
		if((g = dynarr_set(n->method_groups, group_id, NULL)) != NULL)
		{
			g->id = group_id;
			stu_strncpy(g->name, sizeof g->name, name);
			g->methods = NULL;
			verse_send_o_method_group_subscribe(node_id, group_id);
			NOTIFY(n, STRUCTURE);
		}
	}
}

static void cb_o_method_group_destroy(void *user, VNodeID node_id, uint16 group_id, const char *name)
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

static void cb_o_method_create(void *user, VNodeID node_id, uint16 group_id, uint8 method_id, const char *name,
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
				g->methods = dynarr_new(sizeof (NdbOMethod), 4);
			if((m = dynarr_set(g->methods, method_id, NULL)) != NULL)
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
				NOTIFY(n, STRUCTURE);
			}
		}
	}
}

/* ----------------------------------------------------------------------------------------- */

void nodedb_o_register_callbacks(void)
{
	verse_callback_set(verse_send_o_link_set,		cb_o_link_set, NULL);
	verse_callback_set(verse_send_o_light_set,		cb_o_light_set, NULL);
	verse_callback_set(verse_send_o_method_group_create,	cb_o_method_group_create, NULL);
	verse_callback_set(verse_send_o_method_group_destroy,	cb_o_method_group_destroy, NULL);
	verse_callback_set(verse_send_o_method_create,		cb_o_method_create, NULL);
}