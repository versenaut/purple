/*
 * nodedb.c
 * 
 * Copyright (C) 2004 PDC, KTH. See COPYING for license details.
 * 
 * Main node database file. Relies on co-module nodedb-X to handle nodes of type X.
 * Node database is not very opaque, for performance reasons (synchronizer needs
 * access to everything).
*/

#include <stdio.h>
#include <string.h>

#include "verse.h"
#include "purple.h"

#include "dynarr.h"
#include "hash.h"
#include "list.h"
#include "iter.h"
#include "log.h"
#include "mem.h"
#include "memchunk.h"
#include "strutil.h"

#include "nodedb.h"
#include "nodedb-internal.h"

/* ----------------------------------------------------------------------------------------- */

/* A node holds a list of these. */
typedef struct
{
	void	(*callback)(PNode *node, NodeNotifyEvent ev, void *user);
	void	*user;
} NotifyInfo;

static struct
{
	VNodeID		avatar;
	MemChunk	*chunk_node[V_NT_NUM_TYPES];
	Hash		*nodes;				/* Probably not efficient enough, but a start. */
	Hash		*nodes_mine;			/* Duplicate links to nodes owned by this client. */

	List		*notify_mine;
	MemChunk	*chunk_notify;			/* For allocating NotifyInfos. */
} nodedb_info = { 0 };

/* ----------------------------------------------------------------------------------------- */

void nodedb_register(PNode *n)
{
	hash_insert(nodedb_info.nodes, (void *) n->id, n);
}

PNode * nodedb_lookup(VNodeID node_id)
{
	return hash_lookup(nodedb_info.nodes, (const void *) node_id);
}

PNode * nodedb_lookup_with_type(VNodeID node_id, VNodeType type)
{
	PNode	*n;

	if((n = nodedb_lookup(node_id)) != NULL && n->type == type)
		return n;
	return NULL;
}

static int cb_lookup_name(const void *data, void *user)
{
	if(strcmp(((const PNode *) data)->name, ((void **) user)[0]) == 0)
	{
		((void **) user)[1] = (void *) data;
		return 0;
	}
	return 1;
}

PNode * nodedb_lookup_by_name(const char *name)
{
	void	*data[2] = { (void *) name, NULL };	/* I'm too lazy for a struct. */
	/* FIXME: This is completely inefficent. */
	hash_foreach(nodedb_info.nodes, cb_lookup_name, data);
	return data[1];
}

PNode * nodedb_lookup_by_name_with_type(const char *name, VNodeType type)
{
	PNode	*n;

	if((n = nodedb_lookup_by_name(name)) != NULL && n->type == type)
		return n;
	return NULL;
}

NodeObject * nodedb_lookup_object(VNodeID node_id)
{
	PNode	*n;

	if((n = nodedb_lookup(node_id)) != NULL)
		return n->type == V_NT_OBJECT ? (NodeObject *) n : NULL;
	return NULL;
}

NodeText * nodedb_lookup_text(VNodeID node_id)
{
	PNode	*n;

	if((n = nodedb_lookup(node_id)) != NULL)
		return n->type == V_NT_TEXT ? (NodeText *) n : NULL;
	return NULL;
}

/* ----------------------------------------------------------------------------------------- */

void nodedb_internal_notify_mine_check(PNode *n, NodeNotifyEvent ev)
{
	if(nodedb_info.notify_mine != NULL/* && (n->owner == VN_OWNER_MINE)*/)	/* FIXME: Server needs fixing. */
	{
		const List	*iter;

		for(iter = nodedb_info.notify_mine; iter != NULL; iter = list_next(iter))
			((void (*)(PNode *node, NodeNotifyEvent e)) list_data(iter))(n, ev);
	}
}

void nodedb_internal_notify_node_check(PNode *n, NodeNotifyEvent ev)
{
	const List	*iter;
	const NotifyInfo *ni;

	for(iter = n->notify; iter != NULL; iter = list_next(iter))
	{
		ni = list_data(iter);
		ni->callback(n, ev, ni->user);
	}
}

/* ----------------------------------------------------------------------------------------- */

PNode * nodedb_new(VNodeType type)
{
	MemChunk	*ch;

	if((ch = nodedb_info.chunk_node[type]) != NULL)
	{
		PNode	*n = (PNode *) memchunk_alloc(ch);

		n->ref	      = 0;	/* Caller must ref() immediately. */
		n->id         = ~0U;	/* Identify as locally created (non-host-approved) node. */
		n->type       = type;
		n->name[0]    = '\0';
		n->owner      = 0U;
		n->tag_groups = NULL;
		n->notify     = NULL;
		n->creator.port   = NULL;
		n->creator.remote = NULL;

		switch(n->type)
		{
		case V_NT_BITMAP:
			nodedb_b_construct((NodeBitmap *) n);
			break;
		case V_NT_GEOMETRY:
			nodedb_g_construct((NodeGeometry *) n);
			break;
		case V_NT_OBJECT:
			nodedb_o_construct((NodeObject *) n);
			break;
		case V_NT_MATERIAL:
			nodedb_m_construct((NodeMaterial *) n);
			break;
		case V_NT_TEXT:
			nodedb_t_construct((NodeText *) n);
			break;
		case V_NT_CURVE:
			nodedb_c_construct((NodeCurve *) n);
			break;
		case V_NT_AUDIO:
			nodedb_a_construct((NodeAudio *) n);
			break;
		default:
			LOG_WARN(("Missing node-specific init code for type %d", type));
		}
		return n;
	}
	return NULL;
}

static void cb_copy_tag(void *d, const void *s, void *user)
{
	const NdbTag	*src = s;
	NdbTag		*dst = d;

	dst->id = src->id;
	strcpy(dst->name, src->name);
	dst->type = src->type;
	if(NODEDB_TAG_TYPE_SCALAR(dst->type))
		dst->value = src->value;
	else
	{
		if(dst->type == VN_TAG_STRING)
			dst->value.vstring = stu_strdup(src->value.vstring);
		else if(dst->type == VN_TAG_BLOB)
		{
			dst->value.vblob.size = src->value.vblob.size;
			if((dst->value.vblob.blob = mem_alloc(dst->value.vblob.size)) != NULL)
				memcpy(dst->value.vblob.blob, src->value.vblob.blob, dst->value.vblob.size);
		}
		else
			LOG_WARN(("Missing copying code for non-scalar tag type %d", dst->type));
	}
}

static void cb_copy_tag_group(void *d, const void *s, void *user)
{
	const NdbTagGroup	*src = s;
	NdbTagGroup		*dst = d;

	strcpy(dst->name, src->name);
	dst->tags = dynarr_new_copy(src->tags, cb_copy_tag, NULL);
}

static void copy_name(char *out, const char *in)
{
	unsigned int	n, len;

	if(strncmp(in, "copy of ", 8) == 0)
		sprintf(out, "copy 2 of %s", in + 8);
	else if(sscanf(in, "copy %d of %n", &n, &len) > 1)
		sprintf(out, "copy %u of %s", n + 1, in + len);
	else
		sprintf(out, "copy of %s", in);
}

PNode * nodedb_new_copy(const PNode *src)
{
	PNode	*n;

	if(src == NULL)
		return NULL;
	if((n = nodedb_new(src->type)) != NULL)
	{
		n->id = src->id;
		n->id = ~0u;
		copy_name(n->name, src->name);
/*		strcpy(n->name, src->name);*/
		n->owner = src->owner;
		n->tag_groups = dynarr_new_copy(src->tag_groups, cb_copy_tag_group, NULL);
		switch(n->type)
		{
		case V_NT_AUDIO:
			nodedb_a_copy((NodeAudio *) n, (const NodeAudio *) src);
			break;
		case V_NT_BITMAP:
			nodedb_b_copy((NodeBitmap *) n, (const NodeBitmap *) src);
			break;	
		case V_NT_CURVE:
			nodedb_c_copy((NodeCurve *) n, (const NodeCurve *) src);
			break;
		case V_NT_GEOMETRY:
			nodedb_g_copy((NodeGeometry *) n, (const NodeGeometry *) src);
			break;
		case V_NT_MATERIAL:
			nodedb_m_copy((NodeMaterial *) n, (const NodeMaterial *) src);
			break;
		case V_NT_OBJECT:
			nodedb_o_copy((NodeObject *) n, (const NodeObject *) src);
			break;
		case V_NT_TEXT:
			nodedb_t_copy((NodeText *) n, (const NodeText *) src);
			break;
		default:
			LOG_WARN(("Node copy not implemented for type %d\n", n->type));
		}
	}
	return n;
}

/* Set node <dst>'s contents to be the same as <src>'s, or just return a copy if <dst> is NULL. */
PNode * nodedb_set(PNode *dst, const PNode *src)
{
	if(src == NULL)
		return NULL;
	if(dst == NULL)
		return nodedb_new_copy(src);
	if(dst->type != src->type)
	{
		LOG_WARN(("Type mismatch in nodedb_set() (%d vs %d), can't set()", dst->type, src->type));
		return NULL;
	}
	switch(src->type)
	{
	case V_NT_AUDIO:
		nodedb_a_set((NodeAudio *) dst, (NodeAudio *) src);
		break;
	case V_NT_BITMAP:
		nodedb_b_set((NodeBitmap *) dst, (NodeBitmap *) src);
		break;
	case V_NT_CURVE:
		nodedb_c_set((NodeCurve *) dst, (NodeCurve *) src);
		break;
	case V_NT_GEOMETRY:
		nodedb_g_set((NodeGeometry *) dst, (NodeGeometry *) src);
		break;
	case V_NT_MATERIAL:
		nodedb_m_set((NodeMaterial *) dst, (NodeMaterial *) src);
		break;
	case V_NT_OBJECT:
		nodedb_o_set((NodeObject *) dst, (NodeObject *) src);
		break;
	case V_NT_TEXT:
		nodedb_t_set((NodeText *) dst, (NodeText *) src);
		break;
	default:
		LOG_ERR(("Can't set() node of type %d", src->type));
		return NULL;
	}
	return dst;
}

void nodedb_destroy(PNode *n)
{
	MemChunk	*ch;

	if(n == NULL)
		return;

	if((ch = nodedb_info.chunk_node[n->type]) != NULL)
	{
		unsigned int	i, num;

		switch(n->type)
		{
		case V_NT_BITMAP:
			nodedb_b_destruct((NodeBitmap *) n);
			break;
		case V_NT_GEOMETRY:
			nodedb_g_destruct((NodeGeometry *) n);
			break;
		case V_NT_OBJECT:
			nodedb_o_destruct((NodeObject *) n);
			break;
		case V_NT_TEXT:
			nodedb_t_destruct((NodeText *) n);
			break;
		case V_NT_CURVE:
			nodedb_c_destruct((NodeCurve *) n);
			break;
		default:
			LOG_WARN(("Node destruction not implemented for type %d\n", n->type));
		}
		num = dynarr_size(n->tag_groups);
		for(i = 0; i < num; i++)
		{
			NdbTagGroup	*tg;

			if((tg = dynarr_index(n->tag_groups, i)) != NULL && tg->name[0] != '\0')
				dynarr_destroy(tg->tags);
		}
		dynarr_destroy(n->tag_groups);
		memchunk_free(ch, n);
	}
}

/* ----------------------------------------------------------------------------------------- */

void nodedb_ref(PNode *node)
{
	if(node != NULL)
		node->ref++;
}

int nodedb_unref(PNode *node)
{
	if(node != NULL)
	{
		node->ref--;
		if(node->ref <= 0)
		{
			LOG_MSG(("Node %u (%s) type %d at %p has zero references, destroying", node->id, node->name,
				 node->type, node));
			nodedb_destroy(node);
			return 1;
		}
/*		else
			printf(" not destroyed, count=%d\n", node->ref);
*/	}
	return 0;
}

void nodedb_rename(PNode *node, const char *name)
{
	if(node == NULL || name == NULL)
		return;
	stu_strncpy(node->name, sizeof node->name, name);
}

VNodeType nodedb_type_get(const PNode *node)
{
	if(node != NULL)
		return node->type;
	return V_NT_NUM_TYPES;
}

/* ----------------------------------------------------------------------------------------- */

static void cb_default_tag_group(unsigned int index, void *element, void *user)
{
	NdbTagGroup	*g = element;

	g->name[0] = '\0';
	g->tags    = NULL;
}

unsigned int nodedb_tag_group_num(const PNode *node)
{
	unsigned int		i, num;
	const NdbTagGroup	*tg;

	if(node == NULL)
		return 0;
	for(i = num = 0; (tg = dynarr_index(node->tag_groups, i)) != NULL; i++)
	{
		if(tg->name[0] == '\0')
			continue;
		num++;
	}
	return num;
}

NdbTagGroup * nodedb_tag_group_nth(const PNode *node, unsigned int n)
{
	unsigned int	i;
	NdbTagGroup	*tg;

	if(node == NULL)
		return NULL;
	for(i = 0; (tg = dynarr_index(node->tag_groups, i)) != NULL; i++, n--)
	{
		if(tg->name[0] == '\0')
			continue;
		if(n == 0)
			return tg;
	}
	return NULL;
}

NdbTagGroup * nodedb_tag_group_find(const PNode *node, const char *name)
{
	unsigned int	i;
	NdbTagGroup	*tg;

	if(node == NULL || name == NULL || *name == '\0')
		return NULL;
	for(i = 0; (tg = dynarr_index(node->tag_groups, i)) != NULL; i++)
	{
		if(tg->name[0] == '\0')
			continue;
	
		if(strcmp(tg->name, name) == 0)
			return tg;
	}
	return NULL;
}

NdbTagGroup * nodedb_tag_group_create(PNode *node, uint16 group_id, const char *name)
{
	NdbTagGroup	*group;

	if(node == NULL || name == NULL || *name == '\0')
		return NULL;
	if(node->tag_groups == NULL)
	{
		node->tag_groups = dynarr_new(sizeof *group, 2);
		dynarr_set_default_func(node->tag_groups, cb_default_tag_group, NULL);
	}
	if(group_id == (uint16) ~0)
		group = dynarr_append(node->tag_groups, NULL, NULL);
	else
		group = dynarr_set(node->tag_groups, group_id, NULL);
	if(group != NULL)
	{
		group->id = group_id;
		stu_strncpy(group->name, sizeof group->name, name);
		group->tags = NULL;
	}
	return group;
}

void nodedb_tag_group_destroy(NdbTagGroup *group)
{
	if(group == NULL)
		return;
	nodedb_tag_destroy_all(group);
	printf("destroying tag group '%s', id=%u\n", group->name, group->id);
	group->name[0] = '\0';
	group->id = -1;
}

static void cb_default_tag(unsigned int index, void *element, void *user)
{
	NdbTag	*tag = element;

	tag->name[0] = '\0';
	tag->type = -1;
}

unsigned int nodedb_tag_group_tag_num(const NdbTagGroup *group)
{
	unsigned int	i, num;
	const NdbTag	*tag;

	if(group == NULL)
		return 0;
	for(i = num = 0; (tag = dynarr_index(group->tags, i)) != NULL; i++)
	{
		if(tag->name[0] == '\0')
			continue;
		num++;
	}
	return num;
}

NdbTag * nodedb_tag_group_tag_nth(const NdbTagGroup *group, unsigned int n)
{
	unsigned int	i;
	NdbTag		*tag;

	if(group == NULL)
		return NULL;
	for(i = 0; (tag = dynarr_index(group->tags, i)) != NULL; i++)
	{
		if(tag->name[0] == '\0')
			continue;
		if(n == 0)
			return tag;
		n--;
	}
	return NULL;
}

NdbTag * nodedb_tag_group_tag_find(const NdbTagGroup *group, const char *name)
{
	unsigned int	i;
	NdbTag		*tag;

	if(group == NULL || name == NULL || *name == '\0')
		return NULL;
	for(i = 0; (tag = dynarr_index(group->tags, i)) != NULL; i++)
	{
		if(tag->name[0] == '\0')
			continue;
		if(strcmp(tag->name, name) == 0)
			return tag;
	}
	return NULL;
}

const char * nodedb_tag_get_name(const NdbTag *tag)
{
	if(tag != NULL)
		return tag->name;
	return NULL;
}

void nodedb_tag_create(NdbTagGroup *group, uint16 tag_id, const char *name, VNTagType type, const VNTag *value)
{
	NdbTag	*tag;

	if(group == NULL || name == NULL || *name == '\0' || value == NULL)
		return;
	if(group->tags == NULL)
	{
		group->tags = dynarr_new(sizeof *tag, 4);
		dynarr_set_default_func(group->tags, cb_default_tag, NULL);
	}
	if(tag_id == (uint16) ~0)
		tag = dynarr_append(group->tags, NULL, NULL);
	else
		tag = dynarr_set(group->tags, tag_id, NULL);
	if(tag != NULL)
	{
		tag->id = tag_id;
		stu_strncpy(tag->name, sizeof tag->name, name);
		nodedb_tag_value_set(tag, type, value);
	}
}

void nodedb_tag_destroy(NdbTagGroup *group, NdbTag *tag)
{
	if(group == NULL || tag == NULL)
		return;
	nodedb_tag_value_clear(tag);
	tag->name[0] = '\0';
	tag->id = -1;
}

void nodedb_tag_destroy_all(NdbTagGroup *group)
{
	unsigned int	i;
	NdbTag		*tag;

	if(group == NULL)
		return;
	for(i = 0; (tag = dynarr_index(group->tags, i)) != NULL; i++)
	{
		if(tag->name[0] == '\0')
			continue;
		nodedb_tag_value_clear(tag);
	}
	dynarr_destroy(group->tags);
	group->tags = NULL;
}

void nodedb_tag_value_clear(NdbTag *tag)
{
	/* Throw out any old non-scalar value. */
	if(!NODEDB_TAG_TYPE_SCALAR(tag->type))
	{
		if(tag->type == VN_TAG_STRING)
			mem_free(tag->value.vstring);
		else if(tag->type == VN_TAG_BLOB)
			mem_free(tag->value.vblob.blob);
	}
	tag->type = -1;
}

void nodedb_tag_value_set(NdbTag *tag, VNTagType type, const VNTag *value)
{
	if(tag == NULL || value == NULL)
		return;
	nodedb_tag_value_clear(tag);
	tag->type  = type;
	if(NODEDB_TAG_TYPE_SCALAR(type))
		tag->value = *value;
	else
	{
		if(type == VN_TAG_STRING)
			tag->value.vstring = stu_strdup(value->vstring);
		else if(type == VN_TAG_BLOB)
		{
			tag->value.vblob.size = value->vblob.size;
			tag->value.vblob.blob = mem_alloc(tag->value.vblob.size);
			if(tag->value.vblob.blob != NULL)
				memcpy(tag->value.vblob.blob, value->vblob.blob, tag->value.vblob.size);
		}
	}
}

/* Compare two tags. Does *not* check the name, only the type and actual values. Returns boolean equality. */
int nodedb_tag_values_equal(const NdbTag *t1, const NdbTag *t2)
{
	const VNTag	*tag1, *tag2;

	if(t1 == NULL || t2 == NULL)
		return 0;
	if(t1->type != t2->type)
		return 0;
	tag1 = &t1->value;
	tag2 = &t2->value;
	switch(t1->type)
	{
	case VN_TAG_BOOLEAN:
		return tag1->vboolean == tag2->vboolean;
	case VN_TAG_UINT32:
		return tag1->vuint32 == tag2->vuint32;
	case VN_TAG_REAL64:
		return tag1->vreal64 == tag2->vreal64;
	case VN_TAG_STRING:
		if(tag1->vstring == NULL || tag2->vstring == NULL)
			return 0;
		return strcmp(tag1->vstring, tag2->vstring) == 0;
	case VN_TAG_REAL64_VEC3:
		return memcmp(tag1->vreal64_vec3, tag2->vreal64_vec3, sizeof tag1->vreal64_vec3) == 0;
	case VN_TAG_LINK:
		return tag1->vlink == tag2->vlink;
	case VN_TAG_ANIMATION:
		return memcmp(&tag1->vanimation, &tag2->vanimation, sizeof tag1->vanimation) == 0;
	case VN_TAG_BLOB:
		if(tag1->vblob.size != tag2->vblob.size)
			return 0;
		if(tag1->vblob.blob == NULL || tag2->vblob.blob == NULL)
			return 0;
		return memcmp(tag1->vblob.blob, tag2->vblob.blob, tag1->vblob.size) == 0;
	default:
		LOG_WARN(("Missing code for comparing tags of type %d", t1->type));
		return 0;
	}
	return 0;
}

/* ----------------------------------------------------------------------------------------- */

static void cb_node_create(void *user, VNodeID node_id, VNodeType type, VNodeOwner ownership)
{
	PNode	*n;

	if((n = nodedb_new(type)) != NULL)
	{
		n->id    = node_id;	/* Set ID to actual host-assigned value. */
		n->owner = ownership;
		hash_insert(nodedb_info.nodes, (const void *) n->id, n);
		nodedb_ref(n);
		LOG_MSG(("Stored node %u, type %d", node_id, type));
		if(n->owner == VN_OWNER_MINE)
		{
			hash_insert(nodedb_info.nodes_mine, (const void *) n->id, n);
			NOTIFY(n, CREATE);
		}
		verse_send_node_subscribe(node_id);
	}
	else
		LOG_WARN(("Can't handle creation of node type %d--not implemented", type));
}

static void cb_node_name_set(void *user, VNodeID node_id, const char *name)
{
	PNode	*n;

	if((n = nodedb_lookup(node_id)) != NULL)
	{
		stu_strncpy(n->name, sizeof n->name, name);
		LOG_MSG(("Name of %u set to \"%s\"", n->id, n->name));
		NOTIFY(n, NAME);
	}
	else
		LOG_WARN(("Couldn't set name of node %u, not found in database", node_id));
}

/* ----------------------------------------------------------------------------------------- */

static void cb_tag_group_create(void *user, VNodeID node_id, uint16 group_id, const char *name)
{
	PNode	*n;

	if((n = nodedb_lookup(node_id)) != NULL)
	{
		if(nodedb_tag_group_create(n, group_id, name) != NULL)
		{
			verse_send_tag_group_subscribe(node_id, group_id);
			NOTIFY(n, STRUCTURE);
		}
	}
}

static void cb_tag_group_destroy(void *user, VNodeID node_id, uint16 group_id)
{
	PNode	*n;

	if((n = nodedb_lookup(node_id)) != NULL)
	{
		NdbTagGroup	*tg;

		if((tg = dynarr_index(n->tag_groups, group_id)) != NULL)
		{
			tg->name[0] = '\0';
			dynarr_destroy(tg->tags);
			tg->tags = NULL;
			NOTIFY(n, STRUCTURE);
		}
	}
}

static void cb_tag_create(void *user, VNodeID node_id, uint16 group_id, uint16 tag_id, const char *name, VNTagType type, const VNTag *value)
{
	PNode		*n;
	NdbTagGroup	*tg;

	if((n = nodedb_lookup(node_id)) == NULL)
		return;
	if((tg = dynarr_index(n->tag_groups, group_id)) == NULL || tg->name[0] == '\0')
		return;
	nodedb_tag_create(tg, tag_id, name, type, value);
	NOTIFY(n, DATA);
}

static void cb_tag_destroy(void *user, VNodeID node_id, uint16 group_id, uint16 tag_id)
{
	PNode		*n;
	NdbTagGroup	*tg;
	NdbTag		*tag;

	if((n = nodedb_lookup(node_id)) == NULL)
		return;
	if((tg = dynarr_index(n->tag_groups, group_id)) == NULL || tg->name[0] == '\0')
		return;
	if((tag = dynarr_index(tg->tags, tag_id)) != NULL)
	{
		nodedb_tag_value_clear(tag);
		tag->name[0] = '\0';
		NOTIFY(n, DATA);
	}
}

/* ----------------------------------------------------------------------------------------- */

static unsigned int node_hash(const void *p)
{
	return (unsigned int) p;
}

static int node_key_eq(const void *key1, const void *key2)
{
	return key1 == key2;
}

void nodedb_register_callbacks(VNodeID avatar, uint32 mask)
{
	unsigned int	i;

	nodedb_info.avatar = avatar;

	for(i = 0; i < sizeof nodedb_info.chunk_node / sizeof *nodedb_info.chunk_node; i++)
		nodedb_info.chunk_node[i] = NULL;
	nodedb_info.chunk_node[V_NT_OBJECT]   = memchunk_new("chunk-node-object", sizeof (NodeObject), 16);
	nodedb_info.chunk_node[V_NT_GEOMETRY] = memchunk_new("chunk-node-geometry", sizeof (NodeGeometry), 16);
	nodedb_info.chunk_node[V_NT_MATERIAL] = memchunk_new("chunk-node-material", sizeof (NodeMaterial), 16);
	nodedb_info.chunk_node[V_NT_BITMAP]   = memchunk_new("chunk-node-bitmap", sizeof (NodeBitmap), 16);
	nodedb_info.chunk_node[V_NT_TEXT]     = memchunk_new("chunk-node-text",   sizeof (NodeText), 16);
	nodedb_info.chunk_node[V_NT_CURVE]    = memchunk_new("chunk-node-curve",  sizeof (NodeCurve), 16);
	nodedb_info.chunk_node[V_NT_AUDIO]    = memchunk_new("chunk-node-audio",  sizeof (NodeAudio), 16);

	nodedb_info.nodes      = hash_new(node_hash, node_key_eq);
	nodedb_info.nodes_mine = hash_new(node_hash, node_key_eq);

	nodedb_info.chunk_notify = memchunk_new("chunk-node-notify", sizeof (NotifyInfo), 16);

	verse_callback_set(verse_send_node_create,		cb_node_create,	NULL);
	verse_callback_set(verse_send_node_name_set,		cb_node_name_set, NULL);
	verse_callback_set(verse_send_tag_group_create,		cb_tag_group_create, NULL);
	verse_callback_set(verse_send_tag_group_destroy,	cb_tag_group_destroy, NULL);
	verse_callback_set(verse_send_tag_create,		cb_tag_create, NULL);
	verse_callback_set(verse_send_tag_destroy,		cb_tag_destroy, NULL);

	nodedb_o_register_callbacks();
	nodedb_g_register_callbacks();
	nodedb_m_register_callbacks();
	nodedb_b_register_callbacks();
	nodedb_t_register_callbacks();
	nodedb_c_register_callbacks();
	nodedb_a_register_callbacks();

 	verse_send_node_index_subscribe(mask);
}

void nodedb_notify_add(NodeOwnership whose, void (*notify)(PNode *node, NodeNotifyEvent e))
{
	if(whose == NODEDB_OWNERSHIP_MINE)
	{
		nodedb_info.notify_mine = list_append(nodedb_info.notify_mine, notify);
	}
	else
		LOG_ERR(("Notification for non-MINE ownership not implemented"));
}

void * nodedb_notify_node_add(PNode *node, void (*notify)(PNode *node, NodeNotifyEvent e, void *user), void *user)
{
	List		*iter;
	NotifyInfo	*ni;

	if(node == NULL || notify == NULL)
		return NULL;
	/* Check if node already has notification using this function & data. */
	for(iter = node->notify; iter != NULL; iter = list_next(iter))
	{
		ni = list_data(iter);
		if(ni->callback == notify && ni->user == user)
			return iter;	/* It does, so return same handle. */
	}
	ni = memchunk_alloc(nodedb_info.chunk_notify);
	ni->callback = notify;
	ni->user = user;
	iter = node->notify = list_prepend(node->notify, ni);

	return iter;		/* Opaque "handle" is simply list pointer. Simplifies remove. */
}

void nodedb_notify_node_remove(PNode *node, void *handle)
{
	List		*item;
	NotifyInfo	*ni;

	if(node == NULL || handle == NULL)
		return;
	item = handle;
	node->notify = list_unlink(node->notify, item);
	ni = list_data(item);
	memchunk_free(nodedb_info.chunk_notify, ni);
	list_destroy(item);
}
