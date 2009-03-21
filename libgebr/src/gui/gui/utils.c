/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
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

void
gtk_tree_model_iter_copy_values(GtkTreeModel * model, GtkTreeIter * iter, GtkTreeIter * source)
{
	GValue	value;
	guint	i, n;

	value = (GValue){0, };
	n = gtk_tree_model_get_n_columns(model);
	for (i = 0; i < n; ++i) {
		gtk_tree_model_get_value(model, source, i, &value);
		if (G_TYPE_CHECK_INSTANCE_TYPE(model, GTK_TYPE_LIST_STORE))
			gtk_list_store_set_value(GTK_LIST_STORE(model), iter, i, &value);
		else if (G_TYPE_CHECK_INSTANCE_TYPE(model, GTK_TYPE_TREE_STORE))
			gtk_tree_store_set_value(GTK_TREE_STORE(model), iter, i, &value);
		else
			return;

		g_value_unset(&value);
	}
}

gboolean
gtk_tree_model_path_to_iter(GtkTreeModel * model, GtkTreePath * tree_path, GtkTreeIter * iter)
{
	gchar *		path_string;
	gboolean	ret;

	path_string = gtk_tree_path_to_string(tree_path);
	ret = gtk_tree_model_get_iter_from_string(model, iter, path_string);
	g_free(path_string);

	return ret;
}

GList *
libgebr_gtk_tree_model_path_to_iter_list(GtkTreeModel * model, GList * path_list)
{
	GList *	iter_list, * i;

	iter_list = NULL;
	for (i = g_list_first(path_list); i != NULL; i = g_list_next(i)) {
		GtkTreeIter	iter;

		gtk_tree_model_path_to_iter(model, (GtkTreePath*)i->data, &iter);
		iter_list = g_list_prepend(iter_list, gtk_tree_iter_copy(&iter));
	}
	iter_list = g_list_reverse(iter_list);

	return iter_list;
}

gboolean
libgebr_gtk_tree_view_get_selected(GtkTreeView * tree_view, GtkTreeIter * iter)
{
	GtkTreeSelection *	tree_selection;

	tree_selection = gtk_tree_view_get_selection(tree_view);
	if (gtk_tree_selection_get_mode(tree_selection) != GTK_SELECTION_MULTIPLE)
		return gtk_tree_selection_get_selected(tree_selection, NULL, iter);
	else {
		GtkTreeModel *	model;
		GList *		list, * first;

		list = gtk_tree_selection_get_selected_rows(
			gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view)), &model);
		first = g_list_first(list);
		if (first == NULL)
			return FALSE;
		gtk_tree_model_path_to_iter(model, (GtkTreePath*)first->data, iter);

		g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
		g_list_free(list);

		return TRUE;
	}
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

void
gtk_tree_view_select_sibling(GtkTreeView * tree_view)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter, next_iter;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
		return;

	next_iter = iter;
	if (gtk_tree_model_iter_next(model, &next_iter)) {
		gtk_tree_selection_select_iter(selection, &next_iter);
		g_signal_emit_by_name(tree_view, "cursor-changed");
	} else {
		GtkTreePath *	path;

		path = gtk_tree_model_get_path(model, &iter);
		if (gtk_tree_path_prev(path)) {
			gtk_tree_selection_select_path(selection, path);
			g_signal_emit_by_name(tree_view, "cursor-changed");
		}
		gtk_tree_path_free(path);
	}
}

#if GTK_CHECK_VERSION(2,12,0)
struct tooltip_data {
	GtkTreeViewTooltipCallback	callback;
	gpointer			user_data;
};

static void
tooltip_weak_ref(struct tooltip_data * tooltip_data, GtkTreeView * tree_view)
{
	g_free(tooltip_data);
}

static gboolean
on_tooltip_query(GtkTreeView * tree_view, gint x, gint y, gboolean keyboard_mode,
	GtkTooltip * tooltip, struct tooltip_data * tooltip_data)
{
	GtkTreePath *		path;
	GtkTreeViewColumn *	column;
	gchar *			path_string;
	GtkTreeIter		iter;

#if GTK_CHECK_VERSION(2,12,0)
	gtk_tree_view_convert_widget_to_bin_window_coords(tree_view, x, y, &x, &y);
#else
	gtk_tree_view_widget_to_tree_coords(tree_view, x, y, &x, &y);
#endif
	if (!gtk_tree_view_get_path_at_pos(tree_view, x, y, &path, &column, NULL, NULL)) {
		gtk_tree_path_free(path);
		return FALSE;
	}

	/* get iter */
	path_string = gtk_tree_path_to_string(path);
	gtk_tree_model_get_iter_from_string(gtk_tree_view_get_model(tree_view), &iter, path_string);

	/* frees */
	gtk_tree_path_free(path);
	g_free(path_string);

	return tooltip_data->callback(tree_view, tooltip, &iter, column, tooltip_data->user_data);
}

void
gtk_tree_view_set_tooltip_callback(GtkTreeView * tree_view, GtkTreeViewTooltipCallback callback, gpointer user_data)
{
	gulong			signal_id;
	struct tooltip_data *	tooltip_data;

	if ((signal_id = g_signal_handler_find(tree_view, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, on_tooltip_query, NULL))) {
		g_object_set(G_OBJECT(tree_view), "has-tooltip", FALSE, NULL);
		g_signal_handler_disconnect(tree_view, signal_id);
	}
	if (callback == NULL)
		return;

	tooltip_data = g_malloc(sizeof(struct tooltip_data));
	*tooltip_data = (struct tooltip_data) {
		.callback = callback,
		.user_data = user_data,
	};

	g_object_set(G_OBJECT(tree_view), "has-tooltip", TRUE, NULL);
	g_signal_connect(tree_view, "query-tooltip",
		(GCallback)on_tooltip_query, tooltip_data);
	g_object_weak_ref(G_OBJECT(tree_view), (GWeakNotify)tooltip_weak_ref, tooltip_data);
}
#endif

struct reorderable_data {
	gint				geoxml_sequence_pointer_column;
	GtkTreeViewMoveSequenceCallback	callback;
	gpointer			user_data;
};

static void
gtk_tree_view_set_geoxml_sequence_moveable_weak_ref(struct reorderable_data * data, GtkTreeView * tree_view)
{
	g_free(data);
}

gboolean
gtk_tree_view_reorder_callback(GtkTreeView * tree_view, GtkTreeIter * iter, GtkTreeIter * position,
	GtkTreeViewDropPosition drop_position, struct reorderable_data * data)
{
	GtkTreeModel *		tree_model;
	GeoXmlSequence *	sequence;
	GeoXmlSequence *	position_sequence;

	tree_model = gtk_tree_view_get_model(tree_view);
	gtk_tree_model_get(tree_model, iter,
		data->geoxml_sequence_pointer_column, &sequence, -1);
	gtk_tree_model_get(tree_model, position,
		data->geoxml_sequence_pointer_column, &position_sequence, -1);

	if (drop_position == GTK_TREE_VIEW_DROP_AFTER) {
		geoxml_sequence_move_after(sequence, position_sequence);
		gtk_list_store_move_after(GTK_LIST_STORE(tree_model), iter, position);

		geoxml_sequence_next(&position_sequence);
	} else {
		geoxml_sequence_move_before(sequence, position_sequence);
		gtk_list_store_move_before(GTK_LIST_STORE(tree_model), iter, position);
	}

	if (data->callback != NULL)
		data->callback(tree_model, sequence, position_sequence, data->user_data);

	return TRUE;
}

void
gtk_tree_view_set_geoxml_sequence_moveable(GtkTreeView * tree_view, gint geoxml_sequence_pointer_column,
	GtkTreeViewMoveSequenceCallback callback, gpointer user_data)
{
	struct reorderable_data * data;

	data = g_malloc(sizeof(struct reorderable_data));
	*data = (struct reorderable_data) {
		.geoxml_sequence_pointer_column = geoxml_sequence_pointer_column,
		.callback = callback,
		.user_data = user_data,
	};

	gtk_tree_view_set_reorder_callback(tree_view,
		(GtkTreeViewReorderCallback)gtk_tree_view_reorder_callback, NULL, data);

	g_object_weak_ref(G_OBJECT(tree_view),
		(GWeakNotify)gtk_tree_view_set_geoxml_sequence_moveable_weak_ref, data);
}

struct reorder_data {
	GtkTreeIter			iter;
	GtkTreeIter			position;
	GtkTreeViewDropPosition		drop_position;
	GtkTreeViewReorderCallback	callback;
	GtkTreeViewReorderCallback	can_callback;
	gpointer			user_data;
};

static void
gtk_tree_view_reorder_weak_ref(struct reorder_data * data, GtkTreeView * tree_view)
{
	g_free(data);
}

static void
on_gtk_tree_view_drag_begin(GtkTreeView * tree_view, GdkDragContext * drag_context, struct reorder_data * data)
{
	GtkTreeSelection *	selection;

	selection = gtk_tree_view_get_selection(tree_view);
	gtk_tree_selection_get_selected(selection, NULL, &data->iter);
}

static gboolean
on_gtk_tree_view_drag_motion(GtkTreeView * tree_view, GdkDragContext * drag_context, gint x, gint y,
	guint time, struct reorder_data * data)
{
	GtkWidgetClass *	widget_class;
	GtkTreePath *		tree_path;

	/* to draw drop indicator */
	widget_class = GTK_WIDGET_GET_CLASS(GTK_WIDGET(tree_view));
	if (!widget_class->drag_motion(GTK_WIDGET(tree_view), drag_context, x, y, time))
		return TRUE;

#if GTK_CHECK_VERSION(2,12,0)
	gtk_tree_view_convert_widget_to_bin_window_coords(tree_view, x, y, &x, &y);
#else
	gtk_tree_view_widget_to_tree_coords(tree_view, x, y, &x, &y);
#endif
	gtk_tree_view_get_drag_dest_row(tree_view, &tree_path, &data->drop_position);
	gtk_tree_model_path_to_iter(gtk_tree_view_get_model(tree_view), tree_path, &data->position);

	if (data->can_callback == NULL || data->can_callback(tree_view, &data->iter, &data->position,
	data->drop_position, data->user_data))
		gdk_drag_status(drag_context, GDK_ACTION_MOVE, time);
	else
		gdk_drag_status(drag_context, 0, time);

	/* frees */
	gtk_tree_path_free(tree_path);

	return TRUE;
}

gboolean
on_gtk_tree_view_drag_drop(GtkTreeView * tree_view, GdkDragContext * drag_context, gint x, gint y,
	guint time, struct reorder_data * data)
{
	data->callback(tree_view, &data->iter, &data->position, data->drop_position, data->user_data);
	gtk_drag_finish(drag_context, TRUE, FALSE, time);

	return TRUE;
}

void
gtk_tree_view_set_reorder_callback(GtkTreeView * tree_view, GtkTreeViewReorderCallback callback,
	GtkTreeViewReorderCallback can_callback, gpointer user_data)
{
	const static GtkTargetEntry	target_entries [] = {
		{"reorder", GTK_TARGET_SAME_WIDGET, 1}
	};
	struct reorder_data *		data;

	if (tree_view == NULL || callback == NULL)
		return;

	data = g_malloc(sizeof(struct reorder_data));
	*data = (struct reorder_data) {
		.callback = callback,
		.can_callback = can_callback,
		.user_data = user_data,
	};

	gtk_tree_view_enable_model_drag_source(tree_view, GDK_MODIFIER_MASK, target_entries, 1 , GDK_ACTION_MOVE);
	gtk_tree_view_enable_model_drag_dest(tree_view, target_entries, 1 , GDK_ACTION_MOVE);

	g_signal_connect(tree_view, "drag-begin",
		(GCallback)on_gtk_tree_view_drag_begin, data);
	g_signal_connect(tree_view, "drag-drop",
		(GCallback)on_gtk_tree_view_drag_drop, data);
	g_signal_connect(tree_view, "drag-motion",
		(GCallback)on_gtk_tree_view_drag_motion, data);

	g_object_weak_ref(G_OBJECT(tree_view),
		(GWeakNotify)gtk_tree_view_reorder_weak_ref, data);
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
