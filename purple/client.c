/*
 * The Verse magic happens mainly in here.
*/

#include <stdio.h>
#include <string.h>

#include "verse.h"
#include "purple.h"

#include "cron.h"
#include "dynarr.h"
#include "list.h"
#include "log.h"
#include "mem.h"
#include "textbuf.h"
#include "timeval.h"
#include "plugins.h"
#include "strutil.h"
#include "value.h"
#include "xmlnode.h"

#include "nodedb.h"

#include "client.h"
#include "graph.h"

/* ----------------------------------------------------------------------------------------- */

#define METHOD_GROUP_CONTROL_NAME	"PurpleGraph"

ClientInfo	client_info = { 0 };

/* ----------------------------------------------------------------------------------------- */

static int cb_plugins_refresh(void *data)
{
	return 0;
}

static int cb_graphs_refresh(void *data)
{
	return 0;
}

static void notify_mine_create(Node *node)
{
	if(node->type == V_NT_TEXT && client_info.meta == ~0)
	{
		printf("It's the meta text node!\n");
		client_info.meta = node->id;
		verse_send_node_name_set(node->id, "PurpleMeta");
		verse_send_t_set_language(node->id, "xml/purple/meta");
		verse_send_t_buffer_create(node->id, ~0, 0, "plugins");
		verse_send_t_buffer_create(node->id, ~0, 0, "graphs");
		verse_send_node_subscribe(node->id);
		verse_send_o_link_set(client_info.avatar, ~0, node->id, "meta", 0);
	}
}

/* Track changes to nodes owned by this client, i.e. the PurpleMeta text node. */
static void cb_node_notify_mine(Node *node, NodeNotifyEvent e)
{
	if(e == NODEDB_NOTIFY_CREATE)
		notify_mine_create(node);
	else if(node->id == client_info.meta)
	{
		NdbTBuffer	*buf;

		if(e == NODEDB_NOTIFY_STRUCTURE)
		{
			if(client_info.plugins.buffer == (uint16) ~0)
			{
				if((buf = nodedb_t_buffer_lookup((NodeText *) node, "plugins")) != NULL)
				{
					char	*text;

					client_info.plugins.buffer = buf->id;
					if((text = plugins_build_xml()) != NULL)
					{
						char	buf[512];
						size_t	len = strlen(text), pos, chunk;

						for(pos = 0; pos < len; pos += chunk)
						{
							chunk = (len - pos) > sizeof buf - 1 ? sizeof buf - 1 : len - pos;
							memcpy(buf, text + pos, chunk);
							buf[chunk] = '\0';
							verse_send_t_text_set(client_info.meta, client_info.plugins.buffer, pos, chunk, buf);
						}
						mem_free(text);
					}
				}
			}
			if(client_info.graphs.buffer == (uint16) ~0)
			{
				if((buf = nodedb_t_buffer_lookup((NodeText *) node, "graphs")) != NULL)
				{
					const char	*header = "<?xml version=\"1.0\" standalone=\"yes\"?>\n\n"
							      "<purple-graphs>\n"
							      "</purple-graphs>\n";

					client_info.graphs.buffer = buf->id;
					verse_send_t_buffer_subscribe(client_info.meta, client_info.graphs.buffer);
					verse_send_t_text_set(client_info.meta, client_info.graphs.buffer, 0, 0, header);
					client_info.graphs.start = strchr(header, '/') - header - 1;
				}
			}
		}
		else if(e == NODEDB_NOTIFY_DATA)
		{
			if(client_info.plugins.cron == 0)
				cron_set(client_info.plugins.cron, 1.0, cb_plugins_refresh, NULL);
			if(client_info.graphs.cron == 0)
				client_info.graphs.cron = cron_add(CRON_ONESHOT, 0.1, cb_graphs_refresh, NULL);
		}
	}
	else if(node->id == client_info.avatar)
	{
		NdbOMethodGroup	*g;

		if(e == NODEDB_NOTIFY_STRUCTURE)
		{
			if(client_info.gid_control == (uint16) ~0 &&
			   (g = nodedb_o_method_group_lookup((NodeObject *) node, METHOD_GROUP_CONTROL_NAME)) != NULL)
			{
				client_info.gid_control = g->id;
				graph_method_send_creates(node->id, g->id);
			}
			else if(client_info.gid_control != (uint16) ~0)
			{
				graph_method_check_created((NodeObject *) node);
			}
		}
	}
}

/* ----------------------------------------------------------------------------------------- */

static void cb_connect_accept(void *user, VNodeID avatar, void *address, void *connection, uint8 *host_id)
{
	if(!client_info.connected)
	{
		client_info.connected  = 1;
		client_info.avatar     = avatar;
		client_info.connection = connection;

		LOG_MSG(("Connected to Verse server, as avatar %u", avatar));
		nodedb_register_callbacks(avatar, (1 << V_NT_OBJECT) | (1 << V_NT_TEXT));
/*		verse_send_node_list(1 << V_NT_OBJECT);*/
		verse_send_node_subscribe(avatar);
		verse_send_node_name_set(avatar, "PurpleEngine");
		verse_send_o_method_group_create(avatar, 0, METHOD_GROUP_CONTROL_NAME);

		verse_send_node_create(~0, V_NT_TEXT, VN_OWNER_MINE);
	}
	else
		LOG_MSG(("Got redundant connect-accept command--ignoring"));
}

static void cb_o_method_call(void *user, VNodeID node_id, uint8 group_id, uint8 method_id, VNodeID sender, void *params)
{
	if(group_id == client_info.gid_control)
		graph_method_receive_call(method_id, params);
	else
		LOG_WARN(("Got method call to unknown group %u, method %u", group_id, method_id));
}

/* ----------------------------------------------------------------------------------------- */

int client_connect(const char *address)
{
	verse_callback_set(verse_send_connect_accept,		cb_connect_accept,		NULL);
	verse_callback_set(verse_send_o_method_call,		cb_o_method_call,		NULL);

	client_info.connected = 0;
	client_info.address = stu_strdup(address);

	return 1;
}

int cb_reconnect(void *data)
{
	if(!client_info.connected)
	{
		if(client_info.connection != NULL)
		{
			verse_session_destroy(client_info.connection);	/* Avoid socket leakage. */
		}
		LOG_MSG(("Connection attempt %d", client_info.conn_count++));
		client_info.connection = verse_send_connect("purple", "purple-password", client_info.address, NULL);
	}
	return 1;
}

/* ----------------------------------------------------------------------------------------- */

void client_init(void)
{
	client_info.avatar = ~0;
	client_info.meta   = ~0;
	cron_add(CRON_PERIODIC, 5.0, cb_reconnect, NULL);
	nodedb_notify_add(NODEDB_OWNERSHIP_MINE, cb_node_notify_mine);
	client_info.gid_control = ~0;
	client_info.plugins.buffer = ~0;
	client_info.graphs.buffer = ~0;
}
