/*
 * The Verse magic happens mainly in here.
*/

#include <stdio.h>
#include <string.h>

#include "verse.h"

#include "cron.h"
#include "graph.h"
#include "log.h"
#include "timeval.h"
#include "strutil.h"

#include "client.h"

/* ----------------------------------------------------------------------------------------- */

#define METHOD_GROUP_CONTROL_NAME	"PurpleControl"

static struct
{
	int		connected;
	int		conn_count;
	char		*address;
	VSession	*connection;

	VNodeID		avatar;
	VNodeID		plugins;

	uint16		gid_control;
} client_info = { 0 };

/* ----------------------------------------------------------------------------------------- */

static void cb_connect_accept(void *user, uint32 avatar, void *address, void *connection)
{
	if(!client_info.connected)
	{
		client_info.connected  = 1;
		client_info.avatar     = avatar;
		client_info.connection = connection;

		LOG_MSG(("Connected to Verse server, as avatar %u", avatar));
		verse_send_node_subscribe(avatar);
		verse_send_node_name_set(avatar, "Purple Engine");
		verse_send_o_method_group_create(avatar, 0, METHOD_GROUP_CONTROL_NAME);

		verse_send_node_list(1 << V_NT_TEXT);
		verse_send_node_create(0, V_NT_TEXT, client_info.avatar);
	}
	else
		LOG_MSG(("Got redundant connect-accept command--ignoring"));
}

static void cb_node_create(void *user, VNodeID node_id, uint8 type, VNodeID owner_id)
{
	LOG_MSG(("There is a node of type %d called %u", type, node_id));
	if(owner_id == client_info.avatar && type == V_NT_TEXT && client_info.plugins == 0)
	{
		printf("It's the plugins text node!\n");
		client_info.plugins = node_id;
		verse_send_t_set_language(client_info.plugins, "xml/purple/methods");
	}
}

static void cb_o_method_group_create(void *user, VNodeID node_id, uint8 group_id, const char *name)
{
	LOG_MSG(("Node %u has method group %u, \"%s\"", node_id, group_id, name));
	if(node_id == client_info.avatar && strcmp(name, METHOD_GROUP_CONTROL_NAME) == 0)
	{
		const VNOParamType	create_type[]   = { VN_O_METHOD_PTYPE_STRING },
					destroy_type[]  = { VN_O_METHOD_PTYPE_UINT32 };
		const char		*create_name[]  = { "name" },
					*destroy_name[] = { "graph" };

		client_info.gid_control = group_id;
		verse_send_o_method_group_subscribe(client_info.avatar, group_id);
		graph_method_send_creates(client_info.avatar, client_info.gid_control);
	}
}

static void cb_o_method_create(void *user, VNodeID node_id, uint8 group_id, uint8 method_id, const char *name, uint8 param_count, const VNOParamType *types, const char **names)
{
	unsigned int	i;

	printf("param names in method %u.%u %s(): ", group_id, method_id, name);
	for(i = 0; i < param_count; i++)
		printf(" %s (%d)", names[i], strlen(names[i]));
	printf("\n");
	graph_method_receive_create(method_id, name);
}

static void cb_o_method_call(void *user, VNodeID node_id, uint8 group_id, uint8 method_id, VNodeID sender, void *params)
{
	if(group_id == client_info.gid_control)
		graph_method_receive_call(method_id, params);
	else
		LOG_WARN(("Got method call to unknown group %u, method %u", group_id, method_id));
}

int client_connect(const char *address)
{
	verse_callback_set(verse_send_connect_accept,		cb_connect_accept,		NULL);
	verse_callback_set(verse_send_node_create,		cb_node_create,			NULL);
	verse_callback_set(verse_send_o_method_group_create,	cb_o_method_group_create,	NULL);
	verse_callback_set(verse_send_o_method_create,		cb_o_method_create,		NULL);
	verse_callback_set(verse_send_o_method_call,		cb_o_method_call,		NULL);

	client_info.connected = 0;
	client_info.address = stu_strdup(address);

	return 1;
}

void cb_reconnect(void)
{
	if(!client_info.connected)
	{
		if(client_info.connection != NULL)
		{
			verse_session_destroy(client_info.connection);	/* Avoid socket leakage. */
		}
		LOG_MSG(("Connection attempt %d", client_info.conn_count++));
		client_info.connection = verse_send_connect("purple", "purple-password", client_info.address);
	}
}

void client_init(void)
{
	cron_add(CRON_PERIODIC, 5.0, cb_reconnect, NULL);
}
