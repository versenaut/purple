/*
 * A quick hack to visualize bitmap contents. Handy while playing with Purple
 * plug-ins that manipulate bitmaps, such as filters/effects.
 * 
 * This program requires a version of GTK+ 2.0 that is recent enough to include
 * the GtkComboBox widget. This means version 2.4.0 or greater, I think.
*/

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define	GTK_DISABLE_DEPRECATED	1

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "verse.h"

typedef struct {
	uint16		id;
	VNodeID		parent;
	char		name[32];
	VNBLayerType	type;
	void		*data;
	size_t		size;
} BitmapLayer;

typedef struct {
	VNodeID		id;
	gchar		name[32];
	uint16		width, height, depth;
	GList		*layers;
	GtkWidget	*scwin;
	GtkWidget	*image;		/* Image visualizing contents of this node. */
	GdkPixbuf	*pix;		/* Pixbuf holding image data, at zoom=1. */
	GtkWidget	*info;		/* Label showing info. */
	GtkWidget	*pbar;		/* Progress bar. */
	gint		page_num;
	guint		evt_refresh;
	gboolean	subscribed;
	gboolean	full;
	guint		tile_count;
	gdouble		zoom;
} NodeBitmap;

typedef struct {
	VSession	*session;
	GList		*nodes;
	GtkWidget	*window;

	GtkWidget	*combo_nodes, *subscribe;
	GtkWidget	*notebook;

	VNodeID		cur_node;
} MainInfo;

/* ----------------------------------------------------------------------------------------- */

static NodeBitmap * node_lookup(const MainInfo *min, VNodeID node_id)
{
	const GList	*iter;

	for(iter = min->nodes; iter != NULL; iter = g_list_next(iter))
	{
		if(((NodeBitmap *) iter->data)->id == node_id)
			return iter->data;
	}
	return NULL;
}

static void node_select(MainInfo *min, const char *name)
{
	const GList	*iter;
	NodeBitmap	*node;

	min->cur_node = ~0;
	for(iter = min->nodes; iter != NULL; iter = g_list_next(iter))
	{
		if(strcmp(((const NodeBitmap *) iter->data)->name, name) == 0)
		{
			min->cur_node = ((const NodeBitmap *) iter->data)->id;
			break;
		}
	}
	if((node = node_lookup(min, min->cur_node)) != NULL)
		gtk_widget_set_sensitive(min->subscribe, !node->subscribed);
}

static BitmapLayer * layer_lookup(const MainInfo *min, VNodeID node_id, VLayerID layer_id)
{
	NodeBitmap	*node;
	const GList	*iter;

	if((node = node_lookup(min, node_id)) == NULL)
		return NULL;
	for(iter = node->layers; iter != NULL; iter = g_list_next(iter))
	{
		if(((const BitmapLayer *) iter->data)->id == layer_id)
			return iter->data;
	}
	return NULL;
}

static NodeBitmap * node_bitmap_new(MainInfo *min, VNodeID node_id)
{
	NodeBitmap	*node;

	if((node = node_lookup(min, node_id)) != NULL)
		return node;
	node = g_malloc(sizeof *node);
	node->id = node_id;
	node->name[0] = '\0';
	node->width = node->height = node->depth = 0;
	node->layers = NULL;
	node->evt_refresh = 0u;
	node->scwin = NULL;
	node->image = NULL;
	node->pix = NULL;
	node->info = NULL;
	min->nodes = g_list_append(min->nodes, node);

	node->full = FALSE;
	node->tile_count = 0;
	node->subscribed = FALSE;

	node->zoom = 1.0;

	return node;
}

static void put_layer(GdkPixbuf *dst, const BitmapLayer *layer, const NodeBitmap *node)
{
	const unsigned char	*get;
	guchar			*pix, *put;
	int			c, x, y, stride, j;

	stride = gdk_pixbuf_get_rowstride(dst);
	pix = gdk_pixbuf_get_pixels(dst);
	get = layer->data;
	if(strcmp(layer->name, "color_r") == 0)
		c = 0;
	else if(strcmp(layer->name, "color_g") == 0)
		c = 1;
	else if(strcmp(layer->name, "color_b") == 0)
		c = 2;
	else if(strcmp(layer->name, "alpha") == 0)	/* Software-blend using alpha against checkered background. Nice. */
	{
		register gfloat	alpha, ialpha;
		int	bg;

		for(y = 0; y < node->height; y++)
		{
			put = pix + y * stride;
			for(x = 0; x < node->width; x++)
			{
				alpha  = *get++ / 255.0;
				ialpha = 1.0f - alpha;
				bg = (y & 16) ^ (x & 16);	/* Set bg to 0 or 1 in checker-board pattern, 16 pixels square. */
				bg = bg ? 200 : 150;		/* Convert to suitable grayscale component. */
				for(j = 0; j < 3; j++, put++)
					*put = (alpha * *put) + ialpha * bg;
			}
		}
		return;
	}
	if(layer->type == VN_B_LAYER_UINT1)	/* One-bit per pixel requires unpacking the bits. */
	{
		for(y = 0; y < node->height; y++)
		{
			uint8	v, b, m;

			put = pix + y * stride + c;
			for(x = 0; x < node->width;)
			{
				v = *get++;
				for(b = 0, m = 0x80; b < 8 && x < node->width; b++, x++, put += 3, m >>= 1)
					*put = (v & m) ? 0xff : 0x00;
			}
		}
	}
	else if(layer->type == VN_B_LAYER_UINT8)	/* 8bpp images can just be copied, but layers make it tricky. */
	{
		uint8	*get8 = layer->data;

		for(y = 0; y < node->height; y++)
		{
			put = pix + y * stride + c;
			for(x = 0; x < node->width; x++, put += 3)
				*put = *get8++;
		}
	}
	else if(layer->type == VN_B_LAYER_UINT16)
	{
		uint16	*get16 = layer->data;
		for(y = 0; y < node->height; y++)
		{
			put = pix + y * stride + c;
			for(x = 0; x < node->width; x++, put += 3)
				*put = (*get16++) >> 8;		/* Just keep the most significant byte. */
		}
	}
}

/* Set the visible image, by scaling from source pixbuf into a new one, which is then set on the image widget. */
static void set_image(GtkWidget *image, const GdkPixbuf *src, gdouble zoom, size_t w, size_t h)
{
	GdkPixbuf	*scaled;

	scaled = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, zoom * w, zoom * h);
	gdk_pixbuf_scale(src, scaled, 0, 0, zoom * w, zoom * h, 0.0, 0.0, zoom, zoom, GDK_INTERP_NEAREST);
	gtk_image_set_from_pixbuf(GTK_IMAGE(image), scaled);
	g_object_unref(scaled);	/* Drop the reference, we're done with the scaled image. */
}

static gboolean cb_refresh_timeout(gpointer user)
{
	NodeBitmap	*node = user;
	const GList	*iter;

	if(node->image != NULL)
	{
		GList		*l = NULL;

		gdk_pixbuf_fill(node->pix, 0u);
		for(iter = node->layers; iter != NULL; iter = g_list_next(iter))
		{
			if(strcmp(((BitmapLayer *) iter->data)->name, "alpha") == 0)
				l = g_list_append(l, iter->data);
			else
				l = g_list_prepend(l, iter->data);
		}
		for(iter = l; iter != NULL; iter = g_list_next(iter))
			put_layer(node->pix, iter->data, node);
		g_list_free(l);
		set_image(node->image, node->pix, node->zoom, node->width, node->height);
	}
	node->evt_refresh = 0u;
	return FALSE;
}

static void node_bitmap_queue_refresh(NodeBitmap *node)
{
	if(node->evt_refresh == 0)
		node->evt_refresh = g_timeout_add(500, cb_refresh_timeout, node);
}

static void node_name_set(MainInfo *min, VNodeID node_id, const char *name)
{
	NodeBitmap	*node;

	if((node = node_lookup(min, node_id)) == NULL)
		return;
	g_snprintf(node->name, sizeof node->name, "[%u] %s", node_id, name);
}

static size_t type_bits(VNBLayerType type)
{
	switch(type)
	{
	case VN_B_LAYER_UINT1:	return 1;
	case VN_B_LAYER_UINT8:	return 8;
	case VN_B_LAYER_UINT16:	return 16;
	case VN_B_LAYER_REAL32:	return 32;
	case VN_B_LAYER_REAL64:	return 64;
	}
	printf("unknown layer type %d\n", type);
	return 0;
}

static void layer_resize(BitmapLayer *layer,
			 uint16 width, uint16 height, uint16 depth,
			 uint16 old_width, uint16 old_height, uint16 old_depth)
{
	void	*nd;

	if(layer->type == VN_B_LAYER_UINT1)
	{
		layer->size = ((width + 7) / 8) * height * depth;
	}
	layer->size = (width * height * depth * type_bits(layer->type)) / 8;
	printf("allocating %u bytes for %ux%ux%u type %d layer\n", layer->size, width, height, depth, layer->type);
	nd = g_malloc(layer->size);
	if(nd != NULL)
	{
		if(depth != 1 || old_depth != 1)
			memset(nd, 0xaa, layer->size);
		else if(layer->type != VN_B_LAYER_UINT1)
		{
			uint32	y, cx, cy, ps = type_bits(layer->type) / 8;

			cx = MIN(width, old_width);
			cy = MIN(height, old_height);
/*			printf("width:  new=%u old=%u common=%u\n", width, old_width, cx);
			printf("height: new=%u old=%u common=%u\n", height, old_height, cy);
*/			memset(nd, 0xaa, layer->size);
			for(y = 0; y < cy; y++)
			{
				memcpy((uint8 *) nd + width * ps * y, layer->data + old_width * ps * y, cx * ps);
			}
		}
/*		printf("layer %s resized to %ux%ux%u, data at %p\n", layer->name, width, height, depth, layer->data);*/
		g_free(layer->data);
		layer->data = nd;
	}
}

static void node_bitmap_refresh_info(const NodeBitmap *node)
{
	if(node->info != NULL)
	{
		const GList	*iter;
		size_t		size, ln;
		gchar		ltext[64];

		for(iter = node->layers, ln = size = 0; iter != NULL; iter = g_list_next(iter), ln++)
			size += ((BitmapLayer *) iter->data)->size;
		g_snprintf(ltext, sizeof ltext, "%ux%ux%u pixels in %u layers; %u bytes total", node->width, node->height, node->depth, ln, size);
		gtk_label_set_text(GTK_LABEL(node->info), ltext);
	}
}

static BitmapLayer * node_bitmap_layer_new(MainInfo *min, VNodeID node_id, VLayerID layer_id, const char *name, VNBLayerType type)
{
	NodeBitmap	*node;
	BitmapLayer	*layer;

	if((node = node_lookup(min, node_id)) == NULL)
		return NULL;
	layer = g_malloc(sizeof *layer);
	layer->id = layer_id;
	layer->parent = node_id;
	g_snprintf(layer->name, sizeof layer->name, "%s", name);
	layer->type = type;
	layer->data = NULL;

	layer_resize(layer, node->width, node->height, node->depth, 0, 0, 0);

	node->layers = g_list_append(node->layers, layer);

	printf("created layer %u (%s.%s), type %d\n", layer->id, node->name, layer->name, layer->type);

	return layer;
}

static void node_bitmap_layer_destroy(MainInfo *min, VNodeID node_id, VLayerID layer_id)
{
	NodeBitmap	*node;
	BitmapLayer	*layer;

	if((node = node_lookup(min, node_id)) == NULL)
		return;
	if((layer = layer_lookup(min, node_id, layer_id)) == NULL)
		return;
	g_free(layer->data);
	node->layers = g_list_remove(node->layers, layer);
	node_bitmap_queue_refresh(node);
}

static void node_bitmap_show(NodeBitmap *node)
{
	if(node->scwin != NULL)
	{
		GdkPixbuf	*pb;

		pb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, node->width, node->height);
		if(pb != NULL)
		{
			if(node->image != NULL)
				gtk_widget_destroy(node->image);
			node->image = gtk_image_new_from_pixbuf(pb);
			gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(node->scwin), node->image);
			gtk_widget_show(node->image);
		}
	}
}

static void node_dimensions_set(NodeBitmap *node, uint16 width, uint16 height, uint16 depth)
{
	const GList	*iter;

	printf("dimensions set to %ux%ux%u\n", width, height, depth);
	for(iter = node->layers; iter != NULL; iter = g_list_next(iter))
		layer_resize((BitmapLayer *) iter->data, width, height, depth, node->width, node->height, node->depth);
	node->width  = width;
	node->height = height;
	node->depth  = depth;
	if(node->pix != NULL)
		g_object_unref(G_OBJECT(node->pix));
	node->pix = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, node->width, node->height);
	node_bitmap_show(node);
	node_bitmap_queue_refresh(node);
	node_bitmap_refresh_info(node);
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
		gtk_combo_box_append_text(GTK_COMBO_BOX(min->combo_nodes), ((NodeBitmap *) iter->data)->name);
}

/* ----------------------------------------------------------------------------------------- */

static void cb_connect_accept(void *user, uint32 avatar, void *address, void *connection, uint8 *host_id)
{
	printf("Connected\n");
	verse_send_node_index_subscribe(1 << V_NT_BITMAP);
}

static void cb_node_create(void *user, VNodeID node_id, VNodeType type, VNodeID owner)
{
	verse_send_node_subscribe(node_id);
	node_bitmap_new(user, node_id);
}

static void cb_ping(void *user, const char *address, const char *text)
{
	printf("ping from '%s': '%s'\n", address, text);
}

static void cb_node_name_set(void *user, VNodeID node_id, const char *name)
{
	node_name_set(user, node_id, name);
	combo_nodes_refresh(user);
}

static void cb_b_dimensions_set(void *user, VNodeID node_id, uint16 width, uint16 height, uint16 depth)
{
	NodeBitmap	*node;

	if((node = node_lookup(user, node_id)) == NULL)
		return;
	node_dimensions_set(node, width, height, depth);
}

static void cb_b_layer_create(void *user, VNodeID node_id, VLayerID layer_id, const char *name, VNBLayerType type)
{
	node_bitmap_layer_new(user, node_id, layer_id, name, type);
}

static void cb_b_layer_destroy(void *user, VNodeID node_id, VLayerID layer_id)
{
	node_bitmap_layer_destroy(user, node_id, layer_id);
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
	}
}

static void subscribe_set_sensitive(MainInfo *min)
{
	NodeBitmap	*node;

	if((node = node_lookup(min, min->cur_node)) == NULL)
		return;
	gtk_widget_set_sensitive(min->subscribe, !node->subscribed);
}

static void evt_node_close_clicked(GtkWidget *wid, gpointer user)
{
	MainInfo	*min = user;
	NodeBitmap	*node = g_object_get_data(G_OBJECT(wid), "node");

	if(node != NULL)
	{
		verse_send_node_unsubscribe(node->id);
		gtk_notebook_remove_page(GTK_NOTEBOOK(min->notebook), node->page_num);
		node->subscribed = FALSE;
		subscribe_set_sensitive(min);
/*		gtk_image_destroy(node->image);*/
		node->image = 0;
	}
}

static void evt_zoom_changed(GtkWidget *wid, gpointer user)
{
	NodeBitmap	*node = user;

	node->zoom = gtk_range_get_value(GTK_RANGE(wid));
	set_image(node->image, node->pix, node->zoom, node->width, node->height);
}

static void evt_subscribe_clicked(GtkWidget *wid, gpointer user)
{
	MainInfo	*min = user;
	NodeBitmap	*node;

	if((node = node_lookup(min, min->cur_node)) != NULL)
	{
		gchar		ltext[64], *nptr;
		GtkWidget	*vbox, *hbox, *label, *cross, *paned, *zs;
		const GList	*iter;

		vbox = gtk_vbox_new(FALSE, 0);
		paned = gtk_hpaned_new();
		node->scwin = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(node->scwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		node->image = NULL;
		gtk_paned_pack1(GTK_PANED(paned), node->scwin, TRUE, TRUE);
		zs = gtk_vscale_new_with_range(0.1, 20.0, 0.5);
		gtk_scale_set_draw_value(GTK_SCALE(zs), TRUE);
		gtk_scale_set_value_pos(GTK_SCALE(zs), GTK_POS_TOP);
		gtk_scale_set_digits(GTK_SCALE(zs), 1);
		gtk_range_set_value(GTK_RANGE(zs), 1.0);
		gtk_range_set_inverted(GTK_RANGE(zs), TRUE);
		g_signal_connect(G_OBJECT(zs), "value_changed", G_CALLBACK(evt_zoom_changed), node);
		gtk_widget_set_size_request(zs, 32, -1);
		gtk_paned_pack2(GTK_PANED(paned), zs, FALSE, FALSE);
		gtk_box_pack_start(GTK_BOX(vbox), paned, TRUE, TRUE, 0);
		hbox = gtk_hbox_new(FALSE, 0);
		node->info = gtk_label_new("");
		node_bitmap_refresh_info(node);
		gtk_misc_set_alignment(GTK_MISC(node->info), 0.0f, 0.5f);
		gtk_box_pack_start(GTK_BOX(hbox), node->info, TRUE, TRUE, 0);
		node->pbar = gtk_progress_bar_new();
		gtk_box_pack_start(GTK_BOX(hbox), node->pbar, FALSE, FALSE, 0);

		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
		nptr = strchr(node->name, ' ') + 1;
		g_snprintf(ltext, sizeof ltext, "[%u] %s", min->cur_node, nptr);
		hbox = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new(ltext);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
		cross = gtk_button_new_with_label("X");
		g_object_set_data(G_OBJECT(cross), "node", node);
		gtk_button_set_relief(GTK_BUTTON(cross), GTK_RELIEF_NONE);
		g_signal_connect(G_OBJECT(cross), "clicked", G_CALLBACK(evt_node_close_clicked), min);
		gtk_box_pack_start(GTK_BOX(hbox), cross, FALSE, FALSE, 0);
		gtk_widget_show_all(hbox);
		gtk_notebook_append_page(GTK_NOTEBOOK(min->notebook), vbox, hbox);
		node->page_num = gtk_notebook_page_num(GTK_NOTEBOOK(min->notebook), vbox);
		gtk_widget_show_all(vbox);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(min->notebook), node->page_num);
		subscribe_set_sensitive(min);

		for(iter = node->layers; iter != NULL; iter = g_list_next(iter))
			verse_send_b_layer_subscribe(node->id, ((BitmapLayer *) iter->data)->id, 0);
		node->full = FALSE;
		node_bitmap_show(node);
		node->subscribed = TRUE;
		node->full = FALSE;
		node->tile_count = 0;
		subscribe_set_sensitive(min);
	}
}

static struct {
	int		count;
	struct timeval	first, last;
	double		dt;
} bench;

static double time_between(const struct timeval *t1, const struct timeval *t2)
{
	return t2->tv_sec - t1->tv_sec + 1E-6 * (t2->tv_usec - t1->tv_usec);
}

static void cb_b_tile_set(void *user, VNodeID node_id, VLayerID layer_id, uint16 tile_x, uint16 tile_y, uint16 tile_z, VNBLayerType type, const VNBTile *tile)
{
	NodeBitmap	*node;
	BitmapLayer	*layer;
	int		x, y, wt, ht, tw, th;
	guint		tnum;
	size_t		sheet;

	if((node = node_lookup(user, node_id)) == NULL)
		return;
	if((layer = layer_lookup(user, node_id, layer_id)) == NULL)
		return;

	wt = (node->width  + VN_B_TILE_SIZE - 1) / VN_B_TILE_SIZE;
	ht = (node->height + VN_B_TILE_SIZE - 1) / VN_B_TILE_SIZE;
	tw = (tile_x == wt - 1) && (node->width  % VN_B_TILE_SIZE) != 0 ? node->width  % VN_B_TILE_SIZE : VN_B_TILE_SIZE;
	th = (tile_y == ht - 1) && (node->height % VN_B_TILE_SIZE) != 0 ? node->height % VN_B_TILE_SIZE : VN_B_TILE_SIZE;
	sheet = (type_bits(layer->type) * node->width * node->height + 7) / 8;
	switch(layer->type)
	{
	case VN_B_LAYER_UINT1:
		{
			size_t	rs = (node->width + 7) / 8;
			uint8	*put;

			put = layer->data + tile_z * node->height * rs + tile_y * VN_B_TILE_SIZE * rs + tile_x;
			for(y = 0; y < th; y++, put += (node->width + 7) / 8)
				*put = tile->vuint1[y];
		}
		break;
	case VN_B_LAYER_UINT8:
		{
			uint8	*put = layer->data + tile_z * sheet + tile_y * VN_B_TILE_SIZE * node->width + tile_x * VN_B_TILE_SIZE;

			for(y = 0; y < th; y++, put += node->width)
			{
				for(x = 0; x < tw; x++)
					put[x] = tile->vuint8[y * VN_B_TILE_SIZE + x];
			}
		}
		break;
	case VN_B_LAYER_UINT16:
		{
			uint16	*put = (uint16 *) layer->data + tile_z * sheet + tile_y * VN_B_TILE_SIZE * node->width + tile_x * VN_B_TILE_SIZE;

			for(y = 0; y < th; y++, put += node->width)
			{
				for(x = 0; x < tw; x++)
					put[x] = tile->vuint16[y * VN_B_TILE_SIZE + x];
			}
		}
		break;
	default:
		fprintf(stderr, "vbv: Layer type %d (not uint) not yet supported\n", layer->type);
		return;
	}
	node->tile_count++;
	tnum = g_list_length(node->layers) * wt * ht * node->depth;

	if(bench.count == 0)
	{
		gettimeofday(&bench.first, NULL);
	}
	bench.count++;
	gettimeofday(&bench.last, NULL);
	bench.dt = time_between(&bench.first, &bench.last);
/*	printf("count=%d, dt=%g -> %.1f tiles/second, %d tiles total\n", bench.count, bench.dt, bench.count / bench.dt, tnum);*/

	if(node->tile_count == tnum)
	{
		printf("Image complete, got %u tiles in %g seconds, %.3g MB/s\n",
		       node->tile_count, bench.dt, (node->tile_count * VN_B_TILE_SIZE * VN_B_TILE_SIZE) / (1024.0 * 1024.0 * bench.dt));
		node_bitmap_queue_refresh(node);
		bench.count = 0;
	}

	if(node->tile_count <= tnum)
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(node->pbar), (gdouble) node->tile_count / tnum);
	else
		gtk_progress_bar_pulse(GTK_PROGRESS_BAR(node->pbar));
/*	node_bitmap_queue_refresh(node);*/
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
	gtk_window_set_title(GTK_WINDOW(min.window), "Verse Bitmap Viewer");
	gtk_widget_set_size_request(min.window, 640, 480);

	vbox = gtk_vbox_new(FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	label = gtk_label_new("Nodes:");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	min.combo_nodes = gtk_combo_box_new_text();
	g_signal_connect(G_OBJECT(min.combo_nodes), "changed", G_CALLBACK(evt_node_changed), &min);
	gtk_box_pack_start(GTK_BOX(hbox), min.combo_nodes, FALSE, FALSE, 0);

	min.subscribe = gtk_button_new_with_label("Subscribe");
	g_signal_connect(G_OBJECT(min.subscribe), "clicked", G_CALLBACK(evt_subscribe_clicked), &min);
	gtk_box_pack_start(GTK_BOX(hbox), min.subscribe, FALSE, FALSE, 0);

	btn = gtk_button_new_with_label("Quit");
	g_signal_connect(G_OBJECT(btn), "clicked", G_CALLBACK(evt_window_delete), &min);
	gtk_box_pack_end(GTK_BOX(hbox), btn, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	min.notebook = gtk_notebook_new();
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(min.notebook), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), min.notebook, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(min.window), vbox);

	gtk_widget_show_all(min.window);

	g_timeout_add(20, (GSourceFunc) evt_timeout, &min);

	verse_callback_set(verse_send_connect_accept,	cb_connect_accept, &min);
	verse_callback_set(verse_send_node_create,	cb_node_create,	&min);
	verse_callback_set(verse_send_ping,		cb_ping, &min);
	verse_callback_set(verse_send_node_name_set,	cb_node_name_set, &min);
	verse_callback_set(verse_send_b_dimensions_set,	cb_b_dimensions_set, &min);
	verse_callback_set(verse_send_b_layer_create,	cb_b_layer_create, &min);
	verse_callback_set(verse_send_b_layer_destroy,	cb_b_layer_destroy, &min);
	verse_callback_set(verse_send_b_tile_set,	cb_b_tile_set, &min);

	for(i = 1; argv[i] != NULL; i++)
	{
		if(strncmp(argv[i], "-ip=", 4) == 0)
			ip = argv[i] + 4;
	}

	bench.count = 0;

	min.session = verse_send_connect("vbv", "secret", ip, NULL);
	gtk_main();
	verse_send_connect_terminate(ip, "User quit");

	return 0;
}
