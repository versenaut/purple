/*
 * A quick hack to visualize text buffer contents. Handy while developing Purple, with its
 * various XML-based interfaces. Written against GTK+ 1.2.10, because I didn't have the time
 * and energy to get 2.4.x up on my work machine. Oops.
*/

#include <stdio.h>
#include <string.h>

#define	GTK_DISABLE_DEPRECATED	1

#include <gtk/gtk.h>

#include "verse.h"

typedef struct {
	uint16		id;
	VNodeID		parent;
	char		name[32];
	GtkWidget	*text;
	gint		page_num;	/* In GtkNotebook. */
} TextBuffer;

typedef struct {
	VNodeID		id;
	gchar		name[32];
	GList		*buffers;
} NodeText;

typedef struct {
	VSession	*session;
	GList		*nodes;
	GtkWidget	*window;

	GtkWidget	*combo_nodes, *combo_buffers, *subscribe;
	GtkWidget	*notebook;

	VNodeID		cur_node;
	uint16		cur_buffer;
} MainInfo;

/* ----------------------------------------------------------------------------------------- */

static NodeText * node_lookup(const MainInfo *min, VNodeID node_id)
{
	const GList	*iter;

	for(iter = min->nodes; iter != NULL; iter = g_list_next(iter))
	{
		if(((NodeText *) iter->data)->id == node_id)
			return iter->data;
	}
	return NULL;
}

static void node_select(MainInfo *min, const char *name)
{
	const GList	*iter;

	for(iter = min->nodes; iter != NULL; iter = g_list_next(iter))
	{
		if(strcmp(((const NodeText *) iter->data)->name, name) == 0)
		{
			min->cur_node = ((const NodeText *) iter->data)->id;
			return;
		}
	}
}

static TextBuffer * buffer_select(MainInfo *min, const char *name)
{
	NodeText	*node;
	const GList	*iter;

	if((node = node_lookup(min, min->cur_node)) == NULL)
		return NULL;
	for(iter = node->buffers; iter != NULL; iter = g_list_next(iter))
	{
		if(strcmp(((const TextBuffer *) iter->data)->name, name) == 0)
		{
			min->cur_buffer = ((const TextBuffer *) iter->data)->id;
			return iter->data;
		}
	}
	return NULL;
}

static TextBuffer * buffer_lookup(const MainInfo *min, VNodeID node_id, uint16 buffer_id)
{
	NodeText	*node;
	const GList	*iter;

	if((node = node_lookup(min, node_id)) == NULL)
		return NULL;
	for(iter = node->buffers; iter != NULL; iter = g_list_next(iter))
	{
		if(((const TextBuffer *) iter->data)->id == buffer_id)
			return iter->data;
	}
	return NULL;
}

static NodeText * node_text_new(MainInfo *min, VNodeID node_id)
{
	NodeText	*node;

	if((node = node_lookup(min, node_id)) != NULL)
		return node;
	node = g_malloc(sizeof *node);
	node->id = node_id;
	node->name[0] = '\0';
	node->buffers = NULL;
	min->nodes = g_list_append(min->nodes, node);

	return node;
}

static void node_name_set(MainInfo *min, VNodeID node_id, const char *name)
{
	NodeText	*node;

	if((node = node_lookup(min, node_id)) == NULL)
		return;
	g_snprintf(node->name, sizeof node->name, "[%u] %s", node_id, name);
}

static TextBuffer * node_text_buffer_new(MainInfo *min, VNodeID node_id, uint16 buffer_id, const char *name)
{
	NodeText	*node;
	TextBuffer	*buf;

	if((node = node_lookup(min, node_id)) == NULL)
		return NULL;
	buf = g_malloc(sizeof *buf);
	buf->id = buffer_id;
	buf->parent = node_id;
	g_snprintf(buf->name, sizeof buf->name, "[%u] %s", buffer_id, name);
	buf->text = NULL;
	buf->page_num = -1;
	node->buffers = g_list_append(node->buffers, buf);

	return buf;
}

/* ----------------------------------------------------------------------------------------- */

static void combo_nodes_refresh(MainInfo *min)
{
	GList	*iter, *names = NULL;

	for(iter = min->nodes; iter != NULL; iter = g_list_next(iter))
		names = g_list_append(names, ((NodeText *) iter->data)->name);
	gtk_combo_set_popdown_strings(GTK_COMBO(min->combo_nodes), names);
}

static void combo_buffers_refresh(MainInfo *min)
{
	NodeText	*node;
	GList		*iter, *names = NULL;

	if((node = node_lookup(min, min->cur_node)) == NULL)
		return;
	for(iter = node->buffers; iter != NULL; iter = g_list_next(iter))
		names = g_list_append(names, ((TextBuffer *) iter->data)->name);
	if(names != NULL)
		gtk_combo_set_popdown_strings(GTK_COMBO(min->combo_buffers), names);
	else
	{
		gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(min->combo_buffers)->entry), "");
		gtk_list_clear_items(GTK_LIST(GTK_COMBO(min->combo_buffers)->list), 0, 1 << 30);
	}
}

/* ----------------------------------------------------------------------------------------- */

static void cb_connect_accept(void *user, uint32 avatar, void *address, void *connection)
{
	printf("Connected\n");
	verse_send_node_list(1 << V_NT_TEXT);
}

static void cb_node_create(void *user, VNodeID node_id, VNodeType type, VNodeID owner)
{
	verse_send_node_subscribe(node_id);
	node_text_new(user, node_id);
}

static void cb_node_name_set(void *user, VNodeID node_id, const char *name)
{
	node_name_set(user, node_id, name);
	combo_nodes_refresh(user);
}

static void cb_node_t_buffer_create(void *user, VNodeID node_id, uint16 buffer_id, uint16 index, const char *name)
{
	node_text_buffer_new(user, node_id, buffer_id, name);
	if(((MainInfo *) user)->cur_node == node_id)
		combo_buffers_refresh(user);
}

static void evt_node_select(GtkWidget *wid, GtkWidget *child, gpointer user)
{
	MainInfo	*min = user;
	GtkWidget	*cwid;

	cwid = GTK_BIN(child)->child;
	if(GTK_IS_LABEL(cwid))
	{
		gchar	*text;

		gtk_label_get(GTK_LABEL(cwid), &text);
		if(text)
		{
			VNodeID	cur = min->cur_node;

			node_select(min, text);
			if(min->cur_node != cur)
				combo_buffers_refresh(min);
		}
	}
}

static void evt_buffer_select(GtkWidget *wid, GtkWidget *child, gpointer user)
{
	MainInfo	*min = user;
	GtkWidget	*cwid;
	gchar		*text;
	TextBuffer	*buf;

	if((cwid = GTK_BIN(child)->child) == NULL || !GTK_IS_LABEL(cwid))
		return;
	gtk_label_get(GTK_LABEL(cwid), &text);
	if(text != NULL && (buf = buffer_select(min, text)) != NULL)
		gtk_widget_set_sensitive(min->subscribe, buf->text == NULL);
	else
		gtk_widget_set_sensitive(min->subscribe, FALSE);
}

static void subscribe_set_sensitive(MainInfo *min)
{
	TextBuffer	*buf;

	if((buf = buffer_lookup(min, min->cur_node, min->cur_buffer)) == NULL)
		return;
	gtk_widget_set_sensitive(min->subscribe, buf->text == NULL);
}

static void evt_buffer_close_clicked(GtkWidget *wid, gpointer user)
{
	MainInfo	*min = user;
	TextBuffer	*buf = gtk_object_get_data(GTK_OBJECT(wid), "buf");

	if(buf != NULL)
	{
		verse_send_t_buffer_unsubscribe(buf->parent, buf->id);
		gtk_notebook_remove_page(GTK_NOTEBOOK(min->notebook), buf->page_num);
		buf->text = NULL;

		subscribe_set_sensitive(min);
	}
}

static void evt_subscribe_clicked(GtkWidget *wid, gpointer user)
{
	MainInfo	*min = user;
	TextBuffer	*buf;

	if((buf = buffer_lookup(min, min->cur_node, min->cur_buffer)) != NULL)
	{
		gchar		ltext[64], *nptr;
		GtkWidget	*scwin, *hbox, *label, *cross;

		if(buf->text != NULL)
			return;
		scwin = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		buf->text = gtk_text_new(gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scwin)),
					 gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scwin)));
		gtk_text_set_editable(GTK_TEXT(buf->text), FALSE);
		gtk_container_add(GTK_CONTAINER(scwin), buf->text);
		nptr = strchr(buf->name, ' ') + 1;
		g_snprintf(ltext, sizeof ltext, "[%u.%u] %s", min->cur_node, min->cur_buffer, nptr);
		hbox = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new(ltext);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
		cross = gtk_button_new_with_label("X");
		gtk_button_set_relief(GTK_BUTTON(cross), GTK_RELIEF_NONE);
		gtk_object_set_data(GTK_OBJECT(cross), "buf", buf);
		gtk_signal_connect(GTK_OBJECT(cross), "clicked", GTK_SIGNAL_FUNC(evt_buffer_close_clicked), min);
		gtk_box_pack_start(GTK_BOX(hbox), cross, FALSE, FALSE, 0);
		gtk_widget_show_all(hbox);
		gtk_notebook_append_page(GTK_NOTEBOOK(min->notebook), scwin, hbox);
		buf->page_num = gtk_notebook_page_num(GTK_NOTEBOOK(min->notebook), scwin);
		gtk_widget_show_all(scwin);
		gtk_notebook_set_page(GTK_NOTEBOOK(min->notebook), buf->page_num);
		verse_send_t_buffer_subscribe(min->cur_node, min->cur_buffer);
		subscribe_set_sensitive(min);
	}
}

static void cb_t_text_set(void *user, VNodeID node_id, uint16 buffer_id, uint32 pos, uint32 len, const char *text)
{
	TextBuffer	*buf;

	if((buf = buffer_lookup(user, node_id, buffer_id)) == NULL)
		return;
	if(buf->text == NULL)
	{
		verse_send_t_buffer_unsubscribe(node_id, buffer_id);
		return;
	}
	gtk_text_set_point(GTK_TEXT(buf->text), pos);
	gtk_text_forward_delete(GTK_TEXT(buf->text), len);
	gtk_text_insert(GTK_TEXT(buf->text), NULL, NULL, NULL, text, strlen(text));
}

static void evt_window_delete(GtkWidget *win, GdkEvent *evt, gpointer user)
{
	gtk_main_quit();
}

static gint evt_timeout(gpointer user)
{
	verse_callback_update(1000);
	return TRUE;
}

int main(int argc, char *argv[])
{
	MainInfo	min;
	GtkWidget	*vbox, *hbox, *label;

	min.nodes = NULL;

	gtk_init(&argc, &argv);
	min.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_signal_connect(GTK_OBJECT(min.window), "delete_event", GTK_SIGNAL_FUNC(evt_window_delete), &min);
	gtk_window_set_title(GTK_WINDOW(min.window), "Verse Text Viewer");
	gtk_widget_set_usize(min.window, 640, 480);

	vbox = gtk_vbox_new(FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("Nodes:");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	min.combo_nodes = gtk_combo_new();
	gtk_combo_set_value_in_list(GTK_COMBO(min.combo_nodes), TRUE, TRUE);
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(min.combo_nodes)->list), "select_child", GTK_SIGNAL_FUNC(evt_node_select), &min);
	gtk_box_pack_start(GTK_BOX(hbox), min.combo_nodes, FALSE, FALSE, 0);

	label = gtk_label_new("Buffers:");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	min.combo_buffers = gtk_combo_new();
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(min.combo_buffers)->list), "select_child", GTK_SIGNAL_FUNC(evt_buffer_select), &min);
	gtk_box_pack_start(GTK_BOX(hbox), min.combo_buffers, FALSE, FALSE, 0);

	min.subscribe = gtk_button_new_with_label("Subscribe");
	gtk_signal_connect(GTK_OBJECT(min.subscribe), "clicked", GTK_SIGNAL_FUNC(evt_subscribe_clicked), &min);
	gtk_box_pack_start(GTK_BOX(hbox), min.subscribe, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	min.notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vbox), min.notebook, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(min.window), vbox);

	gtk_widget_show_all(min.window);

	gtk_timeout_add(50, (GtkFunction) evt_timeout, &min);

	verse_callback_set(verse_send_connect_accept,	cb_connect_accept, &min);
	verse_callback_set(verse_send_node_create,	cb_node_create,	&min);
	verse_callback_set(verse_send_node_name_set,	cb_node_name_set, &min);
	verse_callback_set(verse_send_t_buffer_create,	cb_node_t_buffer_create, &min);
	verse_callback_set(verse_send_t_text_set,	cb_t_text_set, &min);

	min.session = verse_send_connect("vtv", "secret", "localhost");
	gtk_main();

	return 0;
}
