/*
 * The Vrse magic happens mainly in here.
*/

#include <stdio.h>
#include <string.h>

#include "verse.h"

#include "cron.h"
#include "graph.h"
#include "log.h"
#include "mem.h"
#include "textbuf.h"
#include "timeval.h"
#include "plugins.h"
#include "strutil.h"
#include "xmlnode.h"

#include "client.h"

/* ----------------------------------------------------------------------------------------- */

#define METHOD_GROUP_CONTROL_NAME	"PurpleGraph"

ClientInfo	client_info = { 0 };

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
	if(owner_id == client_info.avatar && type == V_NT_TEXT && client_info.meta == 0)
	{
		printf("It's the meta text node!\n");
		client_info.meta = node_id;
		verse_send_t_set_language(client_info.meta, "xml/purple/meta");
		verse_send_t_buffer_create(client_info.meta, ~0, 0, "plugins");
		verse_send_t_buffer_create(client_info.meta, ~1, 0, "graphs");
		verse_send_node_subscribe(client_info.meta);
		verse_send_o_link_set(client_info.meta, ~0, client_info.meta, "meta", 0);
	}
}

static void cb_o_method_group_create(void *user, VNodeID node_id, uint8 group_id, const char *name)
{
	LOG_MSG(("Node %u has method group %u, \"%s\"", node_id, group_id, name));
	if(node_id == client_info.avatar && strcmp(name, METHOD_GROUP_CONTROL_NAME) == 0)
	{
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

static int cb_plugins_refresh(void *data)
{
	printf("Verse plugins: '%s'\n", textbuf_text(client_info.plugins.text));
	client_info.plugins.cron = 0;
	
	{
		XmlNode	*root;

		if((root = xmlnode_new(textbuf_text(client_info.plugins.text))) != NULL)
		{
			printf("it parsed:\n");
			xmlnode_print_outline(root);
			xmlnode_destroy(root);
		}
	}
	return 0;
}

static int cb_graphs_refresh(void *data)
{
	printf("Verse graphs: '%s'\n", textbuf_text(client_info.graphs.text));
	client_info.graphs.cron = 0;

	return 0;
}

static void cb_t_buffer_create(void *user, VNodeID node_id, VNMBufferID buffer_id, uint16 index, const char *name)
{
	printf("Text node %u has buffer named \"%s\", id=%u\n", node_id, name, buffer_id);
	if(node_id == client_info.meta && strcmp(name, "plugins") == 0 && client_info.plugins.buffer == 0)
	{
		char	*text;

		printf(" It's the plugins buffer\n");
		client_info.plugins.buffer = buffer_id;
		verse_send_t_buffer_subscribe(node_id, buffer_id);
		if(client_info.plugins.cron == 0)
			client_info.plugins.cron = cron_add(CRON_PERIODIC, 1.0E9, cb_plugins_refresh, NULL);

		/* Send initial XML. */
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
		client_info.plugins.text = textbuf_new(2048);
	}
	else if(node_id == client_info.meta && strcmp(name, "graphs") == 0)
	{
		const char	*header = "<?xml version=\"1.0\" standalone=\"yes\"?>\n\n"
				      "<purple-graphs>\n"
				      "</purple-graphs>\n";

		client_info.graphs.buffer = buffer_id;
		verse_send_t_buffer_subscribe(client_info.meta, client_info.graphs.buffer);
		client_info.graphs.text = textbuf_new(2048);
		verse_send_t_text_set(client_info.meta, client_info.graphs.buffer, 0, 0, header);
		client_info.graphs.start = strchr(header, '/') - header - 1;
	}
	else
		printf(" Unknown, ignoring\n");
}

static void cb_t_text_set(void *user, VNodeID node_id, VNMBufferID buffer_id, uint32 pos, uint32 len, const char *text)
{
/*	if(node_id != client_info.meta)
		return;
*/
	if(node_id == client_info.meta && buffer_id == client_info.plugins.buffer && client_info.plugins.text != NULL)
	{
		textbuf_delete(client_info.plugins.text, pos, len);
		textbuf_insert(client_info.plugins.text, pos, text);
		if(client_info.plugins.cron != 0)
			cron_set(client_info.plugins.cron, 1.0, cb_plugins_refresh, NULL);
	}
	else if(node_id == client_info.meta && buffer_id == client_info.graphs.buffer && client_info.graphs.text != NULL)
	{
		textbuf_delete(client_info.graphs.text, pos, len);
		textbuf_insert(client_info.graphs.text, pos, text);
		if(client_info.graphs.cron == 0)
			client_info.graphs.cron = cron_add(CRON_ONESHOT, 0.1, cb_graphs_refresh, NULL);
	}
	else
		printf("Unknown text (at %u in %u.%u): \"%s\"\n", pos, node_id, buffer_id, text);
}

/* ----------------------------------------------------------------------------------------- */

int client_connect(const char *address)
{
	verse_callback_set(verse_send_connect_accept,		cb_connect_accept,		NULL);
	verse_callback_set(verse_send_node_create,		cb_node_create,			NULL);
	verse_callback_set(verse_send_o_method_group_create,	cb_o_method_group_create,	NULL);
	verse_callback_set(verse_send_o_method_create,		cb_o_method_create,		NULL);
	verse_callback_set(verse_send_o_method_call,		cb_o_method_call,		NULL);

	verse_callback_set(verse_send_t_buffer_create,		cb_t_buffer_create,		NULL);
	verse_callback_set(verse_send_t_text_set,		cb_t_text_set,			NULL);

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
		client_info.connection = verse_send_connect("purple", "purple-password", client_info.address);
	}
	return 0;
}

/* ----------------------------------------------------------------------------------------- */

void client_init(void)
{
	cron_add(CRON_PERIODIC, 5.0, cb_reconnect, NULL);
}
