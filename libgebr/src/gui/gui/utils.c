/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2008 GeBR core team (http://gebr.sourceforge.net)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

/*
 * Internal functions
 */

struct popup_callback {
	GtkPopupCallback	callback;
	gpointer		user_data;

	GtkWidget *		event_box;
	GtkWidget *		widget;
};

static void
__popup_callback_weak_notify(struct popup_callback * popup_callback, GtkObject * object)
{
	if (popup_callback->event_box != NULL)
		gtk_widget_destroy(popup_callback->event_box);
	g_free(popup_callback);
}

static struct popup_callback *
__popup_callback_init(GObject * object, GtkPopupCallback callback, gpointer user_data, GtkWidget * event_box)
{
	struct popup_callback *	popup_callback;

	popup_callback = g_malloc(sizeof(struct popup_callback));
	*popup_callback = (struct popup_callback){
		.callback = callback,
		.user_data = user_data,
		.event_box = event_box,
		.widget = GTK_WIDGET(object)
	};
	g_object_weak_ref(object, (GWeakNotify)__popup_callback_weak_notify, popup_callback);

	return popup_callback;
}

static gboolean
__gtk_tree_view_on_button_pressed(GtkTreeView * tree_view, GdkEventButton * event, struct popup_callback * popup_callback)
{
	GtkMenu *		menu;
	GtkTreeSelection *	selection;

	if (event->button != 3)
		return FALSE;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
	if (gtk_tree_selection_count_selected_rows(selection) <= 1) {
		GtkTreePath *	path;

		/* Get tree path for row that was clicked */
		if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tree_view),
			(gint)event->x, (gint)event->y, &path, NULL, NULL, NULL)) {
			gtk_tree_selection_unselect_all(selection);
			gtk_tree_selection_select_path(selection, path);
			gtk_tree_path_free(path);
		}
	}

	/* FIXME: call default implementation to do this */
	g_signal_emit_by_name(tree_view, "cursor-changed");
	menu = popup_callback->callback(GTK_WIDGET(tree_view), popup_callback->user_data);
	if (menu == NULL)
		return TRUE;
	gtk_menu_popup(menu, NULL, NULL, NULL, NULL,
		event->button, gdk_event_get_time((GdkEvent*)event));

	return TRUE;
}

static gboolean
__gtk_widget_on_button_pressed(GtkWidget * widget, GdkEventButton * event, struct popup_callback * popup_callback)
{
	GtkMenu *		menu;

	if (event->button != 3)
		return FALSE;

	menu = popup_callback->callback(popup_callback->widget, popup_callback->user_data);
	if (menu == NULL)
		return TRUE;
	gtk_menu_popup(menu, NULL, NULL, NULL, NULL,
		event->button, gdk_event_get_time((GdkEvent*)event));

	return TRUE;
}

static void
__gtk_widget_on_popup_menu(GtkWidget * widget, struct popup_callback * popup_callback)
{
	GtkMenu *	menu;

	menu = popup_callback->callback(widget, popup_callback->user_data);
	if (menu == NULL)
		return;
	gtk_menu_popup(menu, NULL, NULL, NULL, NULL,
		0, gdk_event_get_time(NULL));
}

/*
 * Public functions
 */

gboolean
gtk_list_store_can_move_up(GtkListStore * store, GtkTreeIter * iter)
{
	GtkTreePath *	previous_path;
	gboolean	ret;

	previous_path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), iter);
	ret = gtk_tree_path_prev(previous_path);

	gtk_tree_path_free(previous_path);

	return ret;
}

gboolean
gtk_list_store_can_move_down(GtkListStore * store, GtkTreeIter * iter)
{
	GtkTreeIter	next;

	next = *iter;
	return gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &next);
}

gboolean
gtk_list_store_move_up(GtkListStore * store, GtkTreeIter * iter)
{
	GtkTreeIter 	previous;
	GtkTreePath *	previous_path;

	previous_path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), iter);
	if (gtk_tree_path_prev(previous_path) == FALSE) {
		gtk_tree_path_free(previous_path);
		return FALSE;
	}

	gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &previous, previous_path);
	gtk_list_store_move_before(store, iter, &previous);

	gtk_tree_path_free(previous_path);

	return TRUE;
}

gboolean
gtk_list_store_move_down(GtkListStore * store, GtkTreeIter * iter)
{
	GtkTreeIter	next;

	next = *iter;
	if (gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &next) == FALSE)
		return FALSE;

	gtk_list_store_move_after(store, iter, &next);

	return TRUE;
}

gulong
gtk_list_store_get_iter_index(GtkListStore * list_store, GtkTreeIter * iter)
{
	gchar *	node;
	gulong	index;

	node = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(list_store), iter);
	index = (gulong)atol(node);
	g_free(node);

	return index;
}

gboolean
gtk_tree_store_can_move_up(GtkTreeStore * store, GtkTreeIter * iter)
{
	GtkTreePath *	previous_path;
	gboolean	ret;

	previous_path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), iter);
	ret = gtk_tree_path_prev(previous_path);

	gtk_tree_path_free(previous_path);

	return ret;
}

gboolean
gtk_tree_store_can_move_down(GtkTreeStore * store, GtkTreeIter * iter)
{
	GtkTreeIter	next;

	next = *iter;
	return gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &next);
}

gboolean
gtk_tree_store_move_up(GtkTreeStore * store, GtkTreeIter * iter)
{
	GtkTreeIter 	previous;
	GtkTreePath *	previous_path;

	previous_path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), iter);
	if (gtk_tree_path_prev(previous_path) == FALSE) {
		gtk_tree_path_free(previous_path);
		return FALSE;
	}

	gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &previous, previous_path);
	gtk_tree_store_move_before(store, iter, &previous);

	gtk_tree_path_free(previous_path);

	return TRUE;
}

gboolean
gtk_tree_store_move_down(GtkTreeStore * store, GtkTreeIter * iter)
{
	GtkTreeIter	next;

	next = *iter;
	if (gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &next) == FALSE)
		return FALSE;

	gtk_tree_store_move_after(store, iter, &next);

	return TRUE;
}

gboolean
gtk_widget_set_popup_callback(GtkWidget * widget, GtkPopupCallback callback, gpointer user_data)
{
	struct popup_callback *	popup_callback;

	if (GTK_WIDGET_FLAGS(widget) & GTK_NO_WINDOW)
		return FALSE;

	popup_callback = __popup_callback_init(G_OBJECT(widget), callback, user_data, NULL);
	GTK_WIDGET_SET_FLAGS(widget, GDK_BUTTON_PRESS);
	g_signal_connect(widget, "button-press-event",
		(GCallback)__gtk_widget_on_button_pressed, popup_callback);
	g_signal_connect(widget, "popup-menu",
		(GCallback)__gtk_widget_on_popup_menu, popup_callback);

	return TRUE;
}

void
gtk_tree_view_set_popup_callback(GtkTreeView * tree_view, GtkPopupCallback callback, gpointer user_data)
{
	struct popup_callback *	popup_callback;

	popup_callback = __popup_callback_init(G_OBJECT(tree_view), callback, user_data, NULL);
	g_signal_connect(tree_view, "button-press-event",
		(GCallback)__gtk_tree_view_on_button_pressed, popup_callback);
	g_signal_connect(tree_view, "popup-menu",
		(GCallback)__gtk_widget_on_popup_menu, popup_callback);
}

#if GTK_CHECK_VERSION(2,12,0)
struct tooltip_data {
	GtkTreePath *	path;
	int		column;
	void		(*gtk_tooltip_set)(GtkTooltip * tooltip, const gchar * string);
	const gchar *	string;
};

static void
tooltip_weak_ref(struct tooltip_data * tooltip_data, GtkTreeView * tree_view)
{
	gtk_tree_path_free(tooltip_data->path);
	g_free(tooltip_data);
}

static gboolean
on_tooltip_query(GtkTreeView * tree_view, gint x, gint y, gboolean keyboard_mode,
	GtkTooltip * tooltip, struct tooltip_data * tooltip_data)
{
	GtkTreePath *		path;
	GtkTreeViewColumn *	column;

	gtk_tree_view_convert_widget_to_bin_window_coords(tree_view, x, y, &x, &y);
	if (!gtk_tree_view_get_path_at_pos(tree_view, x, y, &path, &column, NULL, NULL) ||
	gtk_tree_path_compare(tooltip_data->path, path) ||
	column != gtk_tree_view_get_column(tree_view, tooltip_data->column)) {
		gtk_tree_path_free(path);
		return FALSE;
	}

	tooltip_data->gtk_tooltip_set(tooltip, tooltip_data->string);
	if (tooltip_data->column != -1)
		gtk_tree_view_set_tooltip_row(tree_view, tooltip, path);
	else
		gtk_tree_view_set_tooltip_cell(tree_view, tooltip, path, column, NULL);

	gtk_tree_path_free(path);

	return TRUE;
}

void
gtk_tree_view_set_tooltip_text(GtkTreeView * tree_view, GtkTreeIter * iter, int column, const char * text)
{
	struct tooltip_data *	tooltip_data;

	if (column < -1)
		return;
	if (!g_signal_handler_find(tree_view, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, iter->user_data3) ||
	((struct tooltip_data *)iter->user_data3)->column != column) {
		tooltip_data = g_malloc(sizeof(struct tooltip_data));
		*tooltip_data = (struct tooltip_data) {
			.path = gtk_tree_model_get_path(gtk_tree_view_get_model(tree_view), iter),
			.column = column,
			.gtk_tooltip_set = gtk_tooltip_set_text,
			.string = text,
		};
		iter->user_data3 = tooltip_data;

		g_object_set(G_OBJECT(tree_view), "has-tooltip", TRUE, NULL);
		g_signal_connect(tree_view, "query-tooltip",
			(GCallback)on_tooltip_query, tooltip_data);
		g_object_weak_ref(G_OBJECT(tree_view), (GWeakNotify)tooltip_weak_ref, tooltip_data);
	} else {
		tooltip_data = iter->user_data3;
		if (text == NULL || !strlen(text)) {
			tooltip_weak_ref(tooltip_data, tree_view);
			g_object_weak_unref(G_OBJECT(tree_view), (GWeakNotify)tooltip_weak_ref, tooltip_data);
		} else
			tooltip_data->string = text; 
	}
}
#else
void
gtk_tree_view_set_tooltip_text(GtkTreeView * tree_view, GtkTreeIter * iter, int column, const char * text) {}
#endif

struct reorderable_data {
	GeoXmlSequence *		inserted;
	GtkTreeIter			inserted_iter;
	gint				geoxml_sequence_pointer_column;
	GtkTreeModelReorderedCallback	callback;
	gpointer			user_data;
};

static void
gtk_tree_model_set_geoxml_sequence_moveable_weak_ref(struct reorderable_data * data, GtkTreeView * tree_view)
{
	g_free(data);
}

static void
on_gtk_tree_model_row_inserted(GtkTreeModel * tree_model, GtkTreePath * path, GtkTreeIter * iter, struct reorderable_data * data)
{
	gtk_tree_model_get(tree_model, iter, data->geoxml_sequence_pointer_column, &data->inserted, -1);
	data->inserted_iter = *iter;
}

static void
on_gtk_tree_model_row_changed(GtkTreeModel * tree_model, GtkTreePath * path, GtkTreeIter * iter, struct reorderable_data * data)
{
	if (data->inserted_iter.stamp == iter->stamp)
		gtk_tree_model_get(tree_model, iter, data->geoxml_sequence_pointer_column, &data->inserted, -1);
}

static void
on_gtk_tree_model_row_deleted(GtkTreeModel * tree_model, GtkTreePath * path, struct reorderable_data * data)
{
	if (data->inserted == NULL)
		return;
	gint			index;
	gint			path_index;

	index = geoxml_sequence_get_index(data->inserted);
	path_index = gtk_tree_path_get_indices(path)[gtk_tree_path_get_depth(path)-1];
	if (index == path_index || (index < path_index && index == path_index-1)) {
		GtkTreeIter		iter;
		GeoXmlSequence *	before;

		iter = data->inserted_iter;
		if (gtk_tree_model_iter_next(tree_model, &iter))
			gtk_tree_model_get(tree_model, &iter,
				data->geoxml_sequence_pointer_column, &before, -1);
		else
			before = NULL;
		geoxml_sequence_move_before(data->inserted, before);

		if (data->callback != NULL)
			data->callback(tree_model, data->inserted, before, data->user_data);
		data->inserted = NULL;
	}
}

void
gtk_tree_model_set_geoxml_sequence_moveable(GtkTreeModel * tree_model, GtkTreeView * tree_view,
	gint geoxml_sequence_pointer_column, GtkTreeModelReorderedCallback callback, gpointer user_data)
{
	struct reorderable_data * data;

	data = g_malloc(sizeof(struct reorderable_data));
	*data = (struct reorderable_data) {
		.inserted = NULL,
		.geoxml_sequence_pointer_column = geoxml_sequence_pointer_column,
		.callback = callback,
		.user_data = user_data,
	};
	gtk_tree_view_set_reorderable(tree_view, TRUE);
	g_signal_connect(tree_model, "row-inserted",
		(GCallback)on_gtk_tree_model_row_inserted, data);
	g_signal_connect(tree_model, "row-changed",
		(GCallback)on_gtk_tree_model_row_changed, data);
	g_signal_connect(tree_model, "row-deleted",
		(GCallback)on_gtk_tree_model_row_deleted, data);
	g_object_weak_ref(G_OBJECT(tree_model),
		(GWeakNotify)gtk_tree_model_set_geoxml_sequence_moveable_weak_ref, data);
}

/*
 * Function: confirm_action_dialog
 * Show an action confirmation dialog with formated _message_
 */
gboolean
confirm_action_dialog(const gchar * title, const gchar * message, ...)
{
	GtkWidget *	dialog;

	gchar *		string;
	va_list		argp;
	gboolean	confirmed;

	va_start(argp, message);
	string = g_strdup_vprintf(message, argp);
	va_end(argp);

	dialog = gtk_message_dialog_new(NULL,
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_QUESTION,
		GTK_BUTTONS_YES_NO,
		string);
	gtk_window_set_title(GTK_WINDOW(dialog), title);
	confirmed = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES ? TRUE : FALSE;

	gtk_widget_destroy(dialog);
	g_free(string);

	return confirmed;
}

/*
 * Function: set_tooltip
 * Set tooltip all across the code.
 */
void
set_tooltip(GtkWidget * widget, const gchar * tip)
{
#if GTK_CHECK_VERSION(2,12,0)
	g_object_set(G_OBJECT(widget), "tooltip-text",  tip, NULL);
#else
	static GtkTooltips * tips = NULL;
	if (tips == NULL)
		tips = gtk_tooltips_new();
	gtk_tooltips_set_tip(tips, widget, tip, NULL);
#endif
}

GtkWidget *
gtk_container_add_depth_hbox(GtkWidget * container)
{
	GtkWidget *	depth_hbox;
	GtkWidget *	depth_widget;

	depth_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(container), depth_hbox);
	gtk_widget_show(depth_hbox);

	depth_widget = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(depth_hbox), depth_widget, FALSE, TRUE, 0);
	gtk_widget_set_size_request(depth_widget, 25, -1);
	gtk_widget_show(depth_widget);

	return depth_hbox;
}

void
gtk_expander_hacked_visible(GtkWidget * expander, GtkWidget * label_widget)
{
	g_signal_handlers_unblock_matched(G_OBJECT(label_widget),
		G_SIGNAL_MATCH_FUNC,
		0, 0, NULL,
		(GCallback)gtk_expander_hacked_idle,
		NULL);
}

gboolean
gtk_expander_hacked_idle(GtkWidget * label_widget, GdkEventExpose *event, GtkWidget * expander)
{
	g_signal_handlers_block_matched(G_OBJECT(label_widget),
		G_SIGNAL_MATCH_FUNC,
		0, 0, NULL,
		(GCallback)gtk_expander_hacked_idle,
		NULL);
	g_object_ref (G_OBJECT (label_widget));
	gtk_expander_set_label_widget (GTK_EXPANDER (expander), NULL);
	gtk_expander_set_label_widget (GTK_EXPANDER (expander), label_widget);
	g_object_unref (G_OBJECT (label_widget));

	return TRUE;
}
