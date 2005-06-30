/*
 * A quick hack to visualize text buffer contents. Handy while developing Purple, with its
 * various XML-based interfaces.
 * 
 * This program requires a version of GTK+ 2.0 that is recent enough to include the GtkComboBox
 * widget. This means version 2.4.0 or greater, I think.
*/

#include <stdio.h>
#include <string.h>

#define	GTK_DISABLE_DEPRECATED	1

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "verse.h"

typedef struct {
	uint16		id;
	VNodeID		parent;
	char		name[32];
	GtkWidget	*text;		/* GtkTextView visualizing the buffer contents. */
	GtkWidget	*info;		/* Label showing size. */
	GtkWidget	*pulse;		/* Progressbar showing network activity/buffer changes. */
	guint		pulse_clear;	/* gtk_timeout handle for clearing the pulser. */
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
	buf->info = NULL;
	buf->pulse = NULL;
	buf->page_num = -1;
	node->buffers = g_list_append(node->buffers, buf);

	return buf;
}

/* ----------------------------------------------------------------------------------------- */

static void combo_box_clear(GtkComboBox *combo)
{
	GtkTreeModel	*tm = gtk_combo_box_get_model(combo);
	GtkTreeIter	iter;
	gboolean	valid;
	gint		rows = 0;

	valid = gtk_tree_model_get_iter_first(tm, &iter);
	while(valid)
	{
		rows ++;
		valid = gtk_tree_model_iter_next(tm, &iter);
	}
	while(rows > 0)
		gtk_combo_box_remove_text(combo, --rows);
}

static void combo_nodes_refresh(MainInfo *min)
{
	GList	*iter;

	combo_box_clear(GTK_COMBO_BOX(min->combo_nodes));
	for(iter = min->nodes; iter != NULL; iter = g_list_next(iter))
		gtk_combo_box_append_text(GTK_COMBO_BOX(min->combo_nodes), ((NodeText *) iter->data)->name);
}

static void combo_buffers_refresh(MainInfo *min)
{
	NodeText	*node;
	GList		*iter;

	if((node = node_lookup(min, min->cur_node)) == NULL)
		return;
	combo_box_clear(GTK_COMBO_BOX(min->combo_buffers));
	for(iter = node->buffers; iter != NULL; iter = g_list_next(iter))
		gtk_combo_box_append_text(GTK_COMBO_BOX(min->combo_buffers), ((TextBuffer *) iter->data)->name);
}

/* ----------------------------------------------------------------------------------------- */

static void cb_connect_accept(void *user, uint32 avatar, void *address, void *connection, uint8 *host_id)
{
	printf("Connected\n");
	verse_send_node_index_subscribe(1 << V_NT_TEXT);
}

static void cb_node_create(void *user, VNodeID node_id, VNodeType type, VNodeID owner)
{
	verse_send_node_subscribe(node_id);
	node_text_new(user, node_id);
}

/*
static void cb_get_time(void *user, uint32 time)
{
	printf("time: %u\n", time);
}
*/

static void cb_ping(void *user, const char *address, const char *text)
{
	printf("ping from '%s': '%s'\n", address, text);
}

static void cb_node_name_set(void *user, VNodeID node_id, const char *name)
{
	node_name_set(user, node_id, name);
	combo_nodes_refresh(user);
}

static void cb_node_t_buffer_create(void *user, VNodeID node_id, uint16 buffer_id, const char *name)
{
	node_text_buffer_new(user, node_id, buffer_id, name);
	if(((MainInfo *) user)->cur_node == node_id)
		combo_buffers_refresh(user);
}

static void evt_node_changed(GtkWidget *wid, gpointer user)
{
	MainInfo	*min = user;
	GtkTreeIter	iter;

	if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(wid), &iter))
	{
		GtkTreeModel	*tm;
		gchar 		*v;
		VNodeID		cur = min->cur_node;

		tm = gtk_combo_box_get_model(GTK_COMBO_BOX(wid));
		gtk_tree_model_get(tm, &iter, 0, &v, -1);

		cur = min->cur_node;
		node_select(min, v);
		if(min->cur_node != cur)
			combo_buffers_refresh(min);
	}
}

static void evt_buffer_changed(GtkWidget *wid, gpointer user)
{
	MainInfo	*min = user;
	GtkTreeIter	iter;

	if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(wid), &iter))
	{
		GtkTreeModel	*tm;
		gchar		*bufname;
		TextBuffer	*buf;
	
		if((tm = gtk_combo_box_get_model(GTK_COMBO_BOX(wid))) == NULL)
			return;
		gtk_tree_model_get(tm, &iter, 0, &bufname, -1);
		if(bufname != NULL && (buf = buffer_select(min, bufname)) != NULL)
			gtk_widget_set_sensitive(min->subscribe, buf->text == NULL);
		else
			gtk_widget_set_sensitive(min->subscribe, FALSE);
	}
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
	TextBuffer	*buf = g_object_get_data(G_OBJECT(wid), "buf");

	if(buf != NULL)
	{
		verse_send_t_buffer_unsubscribe(buf->parent, buf->id);
		gtk_notebook_remove_page(GTK_NOTEBOOK(min->notebook), buf->page_num);
		buf->text = NULL;
		buf->info = NULL;
		buf->pulse = NULL;

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
		GtkWidget	*vbox, *scwin, *hbox, *label, *cross;

		if(buf->text != NULL)
			return;
		vbox = gtk_vbox_new(FALSE, 0);
		scwin = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		buf->text = gtk_text_view_new();
/*		gtk_text_set_editable(GTK_TEXT(buf->text), FALSE);*/
		gtk_container_add(GTK_CONTAINER(scwin), buf->text);
		gtk_box_pack_start(GTK_BOX(vbox), scwin, TRUE, TRUE, 0);
		hbox = gtk_hbox_new(FALSE, 0);
		buf->info = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(buf->info), 0.0f, 0.5f);
		gtk_box_pack_start(GTK_BOX(hbox), buf->info, TRUE, TRUE, 0);
		buf->pulse = gtk_progress_bar_new();
		gtk_box_pack_start(GTK_BOX(hbox), buf->pulse, FALSE, FALSE, 0);		

		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
		nptr = strchr(buf->name, ' ') + 1;
		g_snprintf(ltext, sizeof ltext, "[%u.%u] %s", min->cur_node, min->cur_buffer, nptr);
		hbox = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new(ltext);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
		cross = gtk_button_new_with_label("X");
		gtk_button_set_relief(GTK_BUTTON(cross), GTK_RELIEF_NONE);
		g_object_set_data(G_OBJECT(cross), "buf", buf);
		g_signal_connect(G_OBJECT(cross), "clicked", G_CALLBACK(evt_buffer_close_clicked), min);
		gtk_box_pack_start(GTK_BOX(hbox), cross, FALSE, FALSE, 0);
		gtk_widget_show_all(hbox);
		gtk_notebook_append_page(GTK_NOTEBOOK(min->notebook), vbox, hbox);
		buf->page_num = gtk_notebook_page_num(GTK_NOTEBOOK(min->notebook), vbox);
		gtk_widget_show_all(vbox);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(min->notebook), buf->page_num);
		verse_send_t_buffer_subscribe(min->cur_node, min->cur_buffer);
		subscribe_set_sensitive(min);
	}
}

/* Clear the progress pulser/progress bar, a while after last (?) change has arrived. */
static gboolean cb_pulse_timeout(gpointer user)
{
	TextBuffer	*buf = user;

	buf->pulse_clear = 0;
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(buf->pulse), 0.0);
	return FALSE;
}

static void cb_t_text_set(void *user, VNodeID node_id, uint16 buffer_id, uint32 pos, uint32 len, const char *text)
{
	TextBuffer	*buf;
	GtkTextBuffer	*textbuf;
	GtkTextIter	start, end;
	gchar		info[32];

	if((buf = buffer_lookup(user, node_id, buffer_id)) == NULL)
		return;
	if(buf->text == NULL)
	{
		printf("unsubscribing from text buffer %u\n", buffer_id);
		verse_send_t_buffer_unsubscribe(node_id, buffer_id);
		return;
	}
	if((textbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(buf->text))) == NULL)
		return;
	gtk_text_buffer_get_iter_at_offset(textbuf, &start, pos);
	gtk_text_buffer_get_iter_at_offset(textbuf, &end, pos + len);
	gtk_text_buffer_delete(textbuf, &start, &end);
	gtk_text_buffer_insert(textbuf, &start, text, -1);

	g_snprintf(info, sizeof info, "%u bytes, %u lines",
		   gtk_text_buffer_get_char_count(textbuf),
		   gtk_text_buffer_get_line_count(textbuf));
	gtk_label_set_text(GTK_LABEL(buf->info), info);

	gtk_progress_bar_pulse(GTK_PROGRESS_BAR(buf->pulse));
	if(buf->pulse_clear != 0)
		g_source_remove(buf->pulse_clear);
	buf->pulse_clear = g_timeout_add(35, cb_pulse_timeout, buf);
}

static gboolean evt_window_keypress(GtkWidget *win, GdkEventKey *evt, gpointer user)
{
	MainInfo	*min = user;

	if(evt->state & GDK_CONTROL_MASK)
	{
		if(evt->keyval == GDK_q)
			gtk_main_quit();
		else if(evt->keyval >= GDK_1 && evt->keyval <= GDK_9)
			gtk_notebook_set_current_page(GTK_NOTEBOOK(min->notebook), evt->keyval - GDK_1);
	}
	return TRUE;
}

static void evt_ping(GtkWidget *win, gpointer user)
{
	printf("Pinging server\n");
	verse_send_ping("localhost", "foobar");
}

static void evt_window_delete(GtkWidget *win, GdkEvent *evt, gpointer user)
{
	gtk_main_quit();
}

static gboolean evt_timeout(gpointer user)
{
	verse_callback_update(1000);
	return TRUE;
}

int main(int argc, char *argv[])
{
	const char	*ip = "localhost";
	MainInfo	min;
	GtkWidget	*vbox, *hbox, *label, *btn;
	int		i;

	min.nodes = NULL;

	gtk_init(&argc, &argv);
	min.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT(min.window), "key_press_event", G_CALLBACK(evt_window_keypress), &min);
	g_signal_connect(G_OBJECT(min.window), "delete_event", G_CALLBACK(evt_window_delete), &min);
	gtk_window_set_title(GTK_WINDOW(min.window), "Verse Text Viewer");
	gtk_widget_set_size_request(min.window, 640, 480);

	vbox = gtk_vbox_new(FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("Nodes:");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	min.combo_nodes = gtk_combo_box_new_text();
	g_signal_connect(G_OBJECT(min.combo_nodes), "changed", G_CALLBACK(evt_node_changed), &min);
	gtk_box_pack_start(GTK_BOX(hbox), min.combo_nodes, FALSE, FALSE, 0);

	label = gtk_label_new("Buffers:");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	min.combo_buffers = gtk_combo_box_new_text();
	g_signal_connect(G_OBJECT(min.combo_buffers), "changed", GTK_SIGNAL_FUNC(evt_buffer_changed), &min);
	gtk_box_pack_start(GTK_BOX(hbox), min.combo_buffers, FALSE, FALSE, 0);

	min.subscribe = gtk_button_new_with_label("Subscribe");
	g_signal_connect(G_OBJECT(min.subscribe), "clicked", G_CALLBACK(evt_subscribe_clicked), &min);
	gtk_box_pack_start(GTK_BOX(hbox), min.subscribe, FALSE, FALSE, 0);

	btn = gtk_button_new_with_label("Quit");
	g_signal_connect(G_OBJECT(btn), "clicked", G_CALLBACK(evt_window_delete), &min);
	gtk_box_pack_end(GTK_BOX(hbox), btn, FALSE, FALSE, 0);

/*	btn = gtk_button_new_with_label("Get Time");
	g_signal_connect(G_OBJECT(btn), "clicked", G_CALLBACK(evt_get_time), &min);
	gtk_box_pack_end(GTK_BOX(hbox), btn, FALSE, FALSE, 0);
*/
	btn = gtk_button_new_with_label("Ping");
	g_signal_connect(G_OBJECT(btn), "clicked", G_CALLBACK(evt_ping), &min);
	gtk_box_pack_end(GTK_BOX(hbox), btn, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	min.notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vbox), min.notebook, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(min.window), vbox);

	gtk_widget_show_all(min.window);

	g_timeout_add(50, (GSourceFunc) evt_timeout, &min);

	verse_callback_set(verse_send_connect_accept,	cb_connect_accept, &min);
	verse_callback_set(verse_send_node_create,	cb_node_create,	&min);
/*	verse_callback_set(verse_send_get_time,		cb_get_time, &min);*/
	verse_callback_set(verse_send_ping,		cb_ping, &min);
	verse_callback_set(verse_send_node_name_set,	cb_node_name_set, &min);
	verse_callback_set(verse_send_t_buffer_create,	cb_node_t_buffer_create, &min);
	verse_callback_set(verse_send_t_text_set,	cb_t_text_set, &min);

	for(i = 1; argv[i] != NULL; i++)
	{
		if(strncmp(argv[i], "-ip=", 4) == 0)
			ip = argv[i] + 4;
	}

	min.session = verse_send_connect("vtv", "secret", ip, NULL);
	gtk_main();

	return 0;
}
