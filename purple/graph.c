/*
 * 
*/

#include <stdio.h>
#include <string.h>

#include "verse.h"

#include "hash.h"
#include "log.h"
#include "textbuf.h"

#include "client.h"

#include "graph.h"

/* ----------------------------------------------------------------------------------------- */

typedef struct
{
	uint32	id;
	char	name[32];

	VNodeID	node;
	uint16	buffer;
} Graph;

static const VNOParamType	create_type[]   = { VN_O_METHOD_PTYPE_NODE, VN_O_METHOD_PTYPE_LAYER,
							VN_O_METHOD_PTYPE_STRING },
				rename_type[]   = { VN_O_METHOD_PTYPE_UINT32 },
				destroy_type[]  = { VN_O_METHOD_PTYPE_UINT32 };
static char			*create_name[]  = { "node", "buffer", "name" },
				*rename_name[]  = { "graph" },
				*destroy_name[] = { "graph" };

static struct
{
	uint8	mid_create, mid_rename, mid_destroy;
	Hash	*graphs;
} graph_info;

/* ----------------------------------------------------------------------------------------- */

void graph_method_send_creates(uint32 avatar, uint8 group_id)
{
	verse_send_o_method_create(avatar, group_id, -1, "graph_create",
				   sizeof create_type / sizeof *create_type, create_type, create_name);
	verse_send_o_method_create(avatar, group_id, -2, "graph_rename", 
				   sizeof rename_type / sizeof *rename_type, rename_type, rename_name);
	verse_send_o_method_create(avatar, group_id, -3, "graph_destroy", 
				   sizeof destroy_type / sizeof *destroy_type, destroy_type, destroy_name);
}

/* ----------------------------------------------------------------------------------------- */

void graph_method_receive_create(uint8 id, const char *name)
{
	if(strcmp(name, "graph_create") == 0)
		graph_info.mid_create = id;
	else if(strcmp(name, "graph_rename") == 0)
		graph_info.mid_rename = id;
	else if(strcmp(name, "graph_destroy") == 0)
		graph_info.mid_destroy = id;
	else
		LOG_WARN(("Received unknown (non graph-related?) method creation, %s()", name));
}

/* ----------------------------------------------------------------------------------------- */

static void graph_create(VNodeID node_id, uint16 buffer_id, const char *name)
{
	printf("Create graph named '%s' in node %u, buffer %u\n", name, node_id, buffer_id);
}

/* ----------------------------------------------------------------------------------------- */

void graph_method_receive_call(uint8 id, const void *param)
{
	VNOParam	arg[sizeof create_type / sizeof *create_type];

	if(id == graph_info.mid_create)
	{
		printf("create, create\n");
		if(verse_method_call_unpack(param, sizeof create_type / sizeof *create_type, create_type, arg))
			graph_create(arg[0].vnode, arg[1].vlayer, arg[2].vstring);
	}
	else if(id == graph_info.mid_rename)
		;
	else if(id == graph_info.mid_destroy)
		;
	else
		LOG_WARN(("Received call to unknown graph method %u", id));
}
