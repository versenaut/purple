/*
 * 
*/

#include <string.h>

#include "verse.h"

#include "log.h"

#include "graph.h"

/* ----------------------------------------------------------------------------------------- */

static const VNOParamType	create_type[]   = { VN_O_METHOD_PTYPE_STRING },
				rename_type[]   = { VN_O_METHOD_PTYPE_UINT32 },
				destroy_type[]  = { VN_O_METHOD_PTYPE_UINT32 };
static char			*create_name[]  = { "name" },
				*rename_name[]  = { "graph" },
				*destroy_name[] = { "graph" };

static struct
{
	uint8	mid_create, mid_rename, mid_destroy;
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
		LOG_WARN(("Received unknown graph method creation, %s()", name));
}

/* ----------------------------------------------------------------------------------------- */

void graph_method_receive_call(uint8 id, const void *param)
{
	VNOParam	arg;

	if(id == graph_info.mid_create)
	{
		printf("create, create\n");
		if(verse_method_call_unpack(param, sizeof create_type / sizeof *create_type, &arg, create_type))
			printf(" '%s'\n", arg.vstring);
	}
	else if(id == graph_info.mid_rename)
		;
	else if(id == graph_info.mid_destroy)
		;
	else
		LOG_WARN(("Received call to unknown graph method %u", id));
}
