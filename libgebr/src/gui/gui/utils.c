/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
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

#include <gdk/gdkkeysyms.h>

#include "utils.h"

/*
 * Internal functions
 */

struct popup_callback {
	GebrGuiGtkPopupCallback callback;
	gpointer user_data;

	GtkWidget *event_box;
	GtkWidget *widget;
};

struct reorderable_data {
	gint gebr_geoxml_sequence_pointer_column;
	GebrGuiGtkTreeViewMoveSequenceCallback callback;
	gpointer user_data;
};

struct reorder_data {
	GtkTreeIter iter;
	GtkTreeIter position;
	GtkTreeViewDropPosition drop_position;
	GebrGuiGtkTreeViewReorderCallback callback;
	GebrGuiGtkTreeViewReorderCallback can_callback;
	gpointer user_data;
};

static void __popup_callback_weak_notify(struct popup_callback *popup_callback, GtkObject * object)
{
	if (popup_callback->event_box != NULL)
		gtk_widget_destroy(popup_callback->event_box);
	g_free(popup_callback);
}

static struct popup_callback *__popup_callback_init(GObject * object, GebrGuiGtkPopupCallback callback,
						    gpointer user_data, GtkWidget * event_box)
{
	struct popup_callback *popup_callback;

	popup_callback = g_malloc(sizeof(struct popup_callback));
	*popup_callback = (struct popup_callback) {
		.callback = callback,.user_data = user_data,.event_box = event_box,.widget = GTK_WIDGET(object)
	};
	g_object_weak_ref(object, (GWeakNotify) __popup_callback_weak_notify, popup_callback);

	return popup_callback;
}

static gboolean
__gtk_tree_view_on_button_pressed(GtkTreeView * tree_view, GdkEventButton * event,
				  struct popup_callback *popup_callback)
{
	GtkMenu *menu;
	GtkTreeSelection *selection;

	if (event->button != 3)
		return FALSE;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
	if (gtk_tree_selection_count_selected_rows(selection) <= 1) {
		GtkTreePath *path;

		/* Get tree path for row that was clicked */
		if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tree_view),
						  (gint) event->x, (gint) event->y, &path, NULL, NULL, NULL)) {
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
	gtk_menu_popup(menu, NULL, NULL, NULL, NULL, event->button, gdk_event_get_time((GdkEvent *) event));

	return TRUE;
}

static gboolean
__gtk_widget_on_button_pressed(GtkWidget * widget, GdkEventButton * event, struct popup_callback *popup_callback)
{
	GtkMenu *menu;

	if (event->button != 3)
		return FALSE;

	menu = popup_callback->callback(popup_callback->widget, popup_callback->user_data);
	if (menu == NULL)
		return TRUE;
	gtk_menu_popup(menu, NULL, NULL, NULL, NULL, event->button, gdk_event_get_time((GdkEvent *) event));

	return TRUE;
}

static void __gtk_widget_on_popup_menu(GtkWidget * widget, struct popup_callback *popup_callback)
{
	GtkMenu *menu;

	menu = popup_callback->callback(widget, popup_callback->user_data);
	if (menu == NULL)
		return;
	gtk_menu_popup(menu, NULL, NULL, NULL, NULL, 0, gdk_event_get_time(NULL));
}

static gboolean widget_return_on_key_press_event(GtkWidget * widget, GdkEventKey * event, GtkDialog * dialog)
{
	gint response;

	switch (event->keyval) {
	case GDK_Return:
		response = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "__widget_return_dialog_response"));
		gtk_dialog_response(dialog, response);
		break;
	default:
		break;
	}

	return FALSE;
}

static GList *gebr_gui_gtk_tree_model_path_to_iter_list(GtkTreeModel * model, GList * path_list)
{
	GList *iter_list, *i;

	iter_list = NULL;
	for (i = g_list_first(path_list); i != NULL; i = g_list_next(i)) {
		GtkTreeIter iter;

		gtk_tree_model_get_iter(model, &iter, (GtkTreePath *) i->data);
		iter_list = g_list_prepend(iter_list, gtk_tree_iter_copy(&iter));
	}
	iter_list = g_list_reverse(iter_list);

	return iter_list;
}

static void on_gtk_tree_view_row_collapsed(GtkTreeView * tree_view)
{
	g_signal_emit_by_name(tree_view, "cursor-changed");
}

static gboolean on_gtk_tree_view_key_press(GtkTreeView * tree_view, GdkEventKey * event)
{
	if (g_unichar_isalpha(gdk_keyval_to_unicode(event->keyval)))
		gtk_tree_view_expand_all(tree_view);
	return FALSE;
}

static void
gebr_gui_gtk_tree_view_set_gebr_geoxml_sequence_moveable_weak_ref(struct reorderable_data *data,
								     GtkTreeView * tree_view)
{
	g_free(data);
}

static void gtk_tree_view_reorder_weak_ref(struct reorder_data *data, GtkTreeView * tree_view)
{
	g_free(data);
}

static void
on_gtk_tree_view_drag_begin(GtkTreeView * tree_view, GdkDragContext * drag_context, struct reorder_data *data)
{
	gebr_gui_gtk_tree_view_get_selected(tree_view, &data->iter);
}

static gboolean
on_gtk_tree_view_drag_motion(GtkTreeView * tree_view, GdkDragContext * drag_context, gint x, gint y,
			     guint time, struct reorder_data *data)
{
	GtkWidgetClass *widget_class;
	GtkTreePath *tree_path;

	/* to draw drop indicator */
	widget_class = GTK_WIDGET_GET_CLASS(GTK_WIDGET(tree_view));
	if (!widget_class->drag_motion(GTK_WIDGET(tree_view), drag_context, x, y, time))
		return TRUE;

	gtk_tree_view_convert_widget_to_bin_window_coords(tree_view, x, y, &x, &y);
	gtk_tree_view_get_drag_dest_row(tree_view, &tree_path, &data->drop_position);
	gtk_tree_model_get_iter(gtk_tree_view_get_model(tree_view), &data->position, tree_path);

	if (data->can_callback == NULL || data->can_callback(tree_view, &data->iter, &data->position,
							     data->drop_position, data->user_data))
		gdk_drag_status(drag_context, GDK_ACTION_MOVE, time);
	else
		gdk_drag_status(drag_context, 0, time);

	/* frees */
	gtk_tree_path_free(tree_path);

	return TRUE;
}

static gboolean gebr_gui_message_dialog_vararg(GtkMessageType type, GtkButtonsType buttons,
					       const gchar * title, const gchar * message, va_list args)
{
	GtkWidget *dialog;

	gint ret;
	gchar *string;
	gchar * escaped;
	gboolean confirmed;

	string = g_strdup_vprintf(message, args);
	escaped = title? g_markup_printf_escaped("<b>%s</b>", title) : g_markup_printf_escaped("%s", string);
	dialog = gtk_message_dialog_new_with_markup(NULL,
						    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						    type, buttons, "%s", escaped);
	if (title) {
		gtk_window_set_title(GTK_WINDOW(dialog), title);
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", string);
	}

	ret = gtk_dialog_run(GTK_DIALOG(dialog));
	confirmed = (ret == GTK_RESPONSE_YES || ret == GTK_RESPONSE_OK) ? TRUE : FALSE;

	gtk_widget_destroy(dialog);
	g_free(escaped);
	g_free(string);

	return confirmed;
}

/*
 * Public functions
 */

void gebr_gui_gtk_dialog_set_response_on_widget_return(GtkDialog * dialog, gint response, GtkWidget * widget)
{
	g_object_set_data(G_OBJECT(widget), "__widget_return_dialog_response", GINT_TO_POINTER(response));
	gtk_widget_set_events(GTK_WIDGET(widget), GDK_KEY_PRESS_MASK);
	g_signal_connect(GTK_OBJECT(widget), "key-press-event", G_CALLBACK(widget_return_on_key_press_event), dialog);
}

gboolean gebr_gui_gtk_list_store_can_move_up(GtkListStore * store, GtkTreeIter * iter)
{
	GtkTreePath *previous_path;
	gboolean ret;

	previous_path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), iter);
	ret = gtk_tree_path_prev(previous_path);

	gtk_tree_path_free(previous_path);

	return ret;
}

gboolean gebr_gui_gtk_list_store_can_move_down(GtkListStore * store, GtkTreeIter * iter)
{
	GtkTreeIter next;

	next = *iter;
	return gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &next);
}

gboolean gebr_gui_gtk_list_store_move_up(GtkListStore * store, GtkTreeIter * iter)
{
	GtkTreeIter previous;
	GtkTreePath *previous_path;

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

gboolean gebr_gui_gtk_list_store_move_down(GtkListStore * store, GtkTreeIter * iter)
{
	GtkTreeIter next;

	next = *iter;
	if (gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &next) == FALSE)
		return FALSE;

	gtk_list_store_move_after(store, iter, &next);

	return TRUE;
}

gulong gebr_gui_gtk_list_store_get_iter_index(GtkListStore * list_store, GtkTreeIter * iter)
{
	gchar *node;
	gulong index;

	node = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(list_store), iter);
	index = (gulong) atol(node);
	g_free(node);

	return index;
}

gboolean gebr_gui_gtk_tree_store_can_move_up(GtkTreeStore * store, GtkTreeIter * iter)
{
	GtkTreePath *previous_path;
	gboolean ret;

	previous_path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), iter);
	ret = gtk_tree_path_prev(previous_path);

	gtk_tree_path_free(previous_path);

	return ret;
}

gboolean gebr_gui_gtk_tree_store_can_move_down(GtkTreeStore * store, GtkTreeIter * iter)
{
	GtkTreeIter next;

	next = *iter;
	return gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &next);
}

gboolean gebr_gui_gtk_tree_store_move_up(GtkTreeStore * store, GtkTreeIter * iter)
{
	GtkTreeIter previous;
	GtkTreePath *previous_path;

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

gboolean gebr_gui_gtk_tree_store_move_down(GtkTreeStore * store, GtkTreeIter * iter)
{
	GtkTreeIter next;

	next = *iter;
	if (gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &next) == FALSE)
		return FALSE;

	gtk_tree_store_move_after(store, iter, &next);

	return TRUE;
}

gboolean gebr_gui_gtk_tree_store_reparent(GtkTreeStore * store, GtkTreeIter * iter, GtkTreeIter * parent)
{
	GtkTreeIter new;

	if (gtk_tree_model_iter_parent(GTK_TREE_MODEL(store), &new, iter))
		if (gebr_gui_gtk_tree_iter_equal_to(&new, parent))
			return FALSE;

	gtk_tree_store_append(store, &new, parent);
	gebr_gui_gtk_tree_model_iter_copy_values(GTK_TREE_MODEL(store), &new, iter);
	gtk_tree_store_remove(store, iter);
	*iter = new;

	return TRUE;
}

gboolean gebr_gui_gtk_tree_model_iter_equal_to(GtkTreeModel * model, GtkTreeIter * iter1, GtkTreeIter * iter2)
{
	gchar * path1;
	gchar * path2;
	gboolean ret;
	path1 = gtk_tree_model_get_string_from_iter(model, iter1);
	path2 = gtk_tree_model_get_string_from_iter(model, iter2);
	ret = strcmp(path1, path2) == 0;
	g_free(path1);
	g_free(path2);
	return ret;
}

void gebr_gui_gtk_tree_model_iter_copy_values(GtkTreeModel * model, GtkTreeIter * iter, GtkTreeIter * source)
{
	GValue value;
	guint i, n;

	value = (GValue) {
	0,};
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

void gebr_gui_gtk_tree_view_scroll_to_iter_cell(GtkTreeView * tree_view, GtkTreeIter * iter)
{
	GtkTreePath *tree_path;

	tree_path = gtk_tree_model_get_path(gtk_tree_view_get_model(tree_view), iter);
	gtk_tree_view_scroll_to_cell(tree_view, tree_path, NULL, FALSE, 0, 0);
	gtk_tree_path_free(tree_path);
}

GList *gebr_gui_gtk_tree_view_get_selected_iters(GtkTreeView * tree_view)
{
	GtkTreeModel *model;
	GList *path_list, *list;

	path_list = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(tree_view), &model);
	list = gebr_gui_gtk_tree_model_path_to_iter_list(model, path_list);

	g_list_foreach(path_list, (GFunc) gtk_tree_path_free, NULL);
	g_list_free(path_list);

	return list;
}

void gebr_gui_gtk_tree_view_turn_to_single_selection(GtkTreeView * tree_view)
{
	GList *selected_iters, *i;
	GtkTreeSelection *tree_selection;

	tree_selection = gtk_tree_view_get_selection(tree_view);
	selected_iters = gebr_gui_gtk_tree_view_get_selected_iters(tree_view);
	i = g_list_first(selected_iters);
	i = g_list_next(i);
	for (; i != NULL; i = g_list_next(i))
		gtk_tree_selection_unselect_iter(tree_selection, (GtkTreeIter *) i->data);

	g_list_foreach(selected_iters, (GFunc) gtk_tree_iter_free, NULL);
	g_list_free(selected_iters);
}

gboolean gebr_gui_gtk_tree_view_get_selected(GtkTreeView * tree_view, GtkTreeIter * iter)
{
	GtkTreeSelection *tree_selection;

	tree_selection = gtk_tree_view_get_selection(tree_view);
	if (gtk_tree_selection_get_mode(tree_selection) != GTK_SELECTION_MULTIPLE)
		return gtk_tree_selection_get_selected(tree_selection, NULL, iter);
	else {
		GtkTreeModel *model;
		GList *list, *first;
		gboolean ret = TRUE;

		list =
		    gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view)), &model);
		first = g_list_first(list);
		if (first == NULL || first->data == NULL)
			ret = FALSE;
		else if (iter != NULL)
			gtk_tree_model_get_iter(model, iter, (GtkTreePath *) first->data);

		g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free(list);

		return ret;
	}
}

gboolean gebr_gui_gtk_tree_view_get_last_selected(GtkTreeView * tree_view, GtkTreeIter * iter)
{
	GList *list, *last;
	gboolean ret = TRUE;

	list = gebr_gui_gtk_tree_view_get_selected_iters(tree_view);
	if ((last = g_list_last(list)) == NULL) {
		ret = FALSE;
		goto out;
	}
	*iter = *(GtkTreeIter*)g_list_last(list)->data;

out:	g_list_foreach(list, (GFunc) gtk_tree_iter_free, NULL);
	g_list_free(list);

	return ret;
}

void gebr_gui_gtk_tree_view_select_iter(GtkTreeView * tree_view, GtkTreeIter * iter)
{
	GtkTreeSelection *tree_selection;

	tree_selection = gtk_tree_view_get_selection(tree_view);
	gtk_tree_selection_unselect_all(tree_selection);
	if (iter == NULL)
		return;
	gebr_gui_gtk_tree_view_expand_to_iter(tree_view, iter);

	GtkTreePath * tree_path;
	tree_path = gtk_tree_model_get_path(gtk_tree_view_get_model(tree_view), iter);
	gtk_tree_view_set_cursor(tree_view, tree_path, gtk_tree_view_get_column(tree_view, 0), FALSE);
	gtk_tree_path_free(tree_path);

	gebr_gui_gtk_tree_view_scroll_to_iter_cell(tree_view, iter);
}

void gebr_gui_gtk_tree_view_expand_to_iter(GtkTreeView * view, GtkTreeIter * iter)
{
	GtkTreePath *path;
	path = gtk_tree_model_get_path(gtk_tree_view_get_model(view), iter);
	gtk_tree_view_expand_to_path(view, path);
	gtk_tree_path_free(path);
}

GtkTreeViewColumn *gebr_gui_gtk_tree_view_get_column_from_renderer(GtkTreeView * tree_view, GtkCellRenderer * renderer)
{
	GList *column_list;
	GtkTreeViewColumn *column = NULL;

	column_list = gtk_tree_view_get_columns(tree_view);
	for (GList * i = column_list; i != NULL; i = g_list_next(i)) {
		GtkTreeViewColumn *i_column = (GtkTreeViewColumn *) i->data;
		GList *renderer_list;

		renderer_list = gtk_tree_view_column_get_cell_renderers(i_column);
		for (GList * j = renderer_list; j != NULL; j = g_list_next(j)) {
			GtkCellRenderer *i_renderer = (GtkCellRenderer *) j->data;

			if (i_renderer == renderer) {
				column = i_column;
				goto out;
			}
		}
		g_list_free(renderer_list);
	}

 out:	g_list_free(column_list);

	return column;
}

GtkTreeViewColumn *gebr_gui_gtk_tree_view_get_next_column(GtkTreeView * tree_view, GtkTreeViewColumn * column)
{
	GList *column_list;
	GtkTreeViewColumn *next_column = NULL;

	column_list = gtk_tree_view_get_columns(tree_view);
	for (GList * i = column_list; i != NULL; i = g_list_next(i)) {
		GtkTreeViewColumn *i_column = (GtkTreeViewColumn *) i->data;

		if (i_column == column) {
			if ((i = g_list_next(i)) != NULL)
				next_column = (GtkTreeViewColumn *) i->data;
			break;
		}
	}

	g_list_free(column_list);

	return next_column;
}

void
gebr_gui_gtk_tree_view_set_cursor(GtkTreeView * tree_view, GtkTreeIter * iter,
				  GtkTreeViewColumn * column, gboolean start_editing)
{
	GtkTreePath *tree_path;

	tree_path = gtk_tree_model_get_path(gtk_tree_view_get_model(tree_view), iter);
	gtk_tree_view_set_cursor(tree_view, tree_path, column, start_editing);
	gtk_tree_path_free(tree_path);
}

void gebr_gui_gtk_tree_view_change_cursor_on_row_collapsed(GtkTreeView * tree_view)
{
	g_signal_connect(tree_view, "row-collapsed", G_CALLBACK(on_gtk_tree_view_row_collapsed), NULL);
}

void gebr_gui_gtk_tree_view_fancy_search(GtkTreeView * tree_view, gint column)
{
	gtk_tree_view_set_enable_search(tree_view, TRUE);
	gtk_tree_view_set_search_column(tree_view, column);
	g_signal_connect(tree_view, "key-press-event", G_CALLBACK(on_gtk_tree_view_key_press), NULL);
}

GtkCellRenderer *gebr_gui_gtk_tree_view_column_get_first_renderer_with_mode(GtkTreeViewColumn * column,
									    GtkCellRendererMode mode)
{
	GList *renderer_list;
	GtkCellRenderer *renderer = NULL;

	renderer_list = gtk_tree_view_column_get_cell_renderers(column);
	for (GList * j = renderer_list; j != NULL; j = g_list_next(j)) {
		GtkCellRenderer *i_renderer = (GtkCellRenderer *) j->data;
		GtkCellRendererMode i_mode;

		g_object_get(i_renderer, "mode", &i_mode, NULL);

		if (i_mode == mode) {
			renderer = i_renderer;
			break;
		}
	}
	g_free(renderer_list);

	return renderer;
}

void gebr_gui_gtk_tree_model_foreach_recursive(GtkTreeModel *tree_model, GtkTreeModelForeachFunc func, gpointer
					       user_data)
{
	GtkTreeIter iter;
	GtkTreeIter parent;
	gboolean coming_back = FALSE;
	gboolean valid;

	valid = gtk_tree_model_get_iter_first(tree_model, &iter);
	while (valid == TRUE) {
		GtkTreeIter user_iter;

		if (coming_back == FALSE)
			while (valid == TRUE && (parent = iter, 1) &&
			       (valid = gtk_tree_model_iter_children(tree_model, &iter, &parent), 1));
		user_iter = iter = parent;
		if (!(valid = gtk_tree_model_iter_next(tree_model, &iter))) {
			if ((coming_back = gtk_tree_model_iter_parent(tree_model, &iter, &parent))) {
				valid = TRUE;
			       	parent = iter;
			} else {
				valid = gtk_tree_model_iter_next(tree_model, &parent);
				iter = parent;
			}
		} else {
			parent = iter;
			coming_back = FALSE;
		} 

		GtkTreePath *tree_path;
		tree_path = gtk_tree_model_get_path(tree_model, &user_iter);
		if ((*func)(tree_model, tree_path, &user_iter, user_data)) {
			gtk_tree_path_free(tree_path);
			return;
		}
		gtk_tree_path_free(tree_path);
	}
}

/*
void gebr_gui_gtk_text_view_set_range_tooltip(GtkTextView * text_view, GtkTextIter * ini, GtkTextIter * end,
					      const gchar * tooltip)
{
	GtkTextTag * tag;
	GtkTextBuffer * buffer;
	buffer = gtk_text_view_get_buffer(text_view);
}
*/

gboolean
gebr_gui_gtk_widget_set_popup_callback(GtkWidget * widget, GebrGuiGtkPopupCallback callback, gpointer user_data)
{
	struct popup_callback *popup_callback;

	if (GTK_WIDGET_FLAGS(widget) & GTK_NO_WINDOW)
		return FALSE;

	popup_callback = __popup_callback_init(G_OBJECT(widget), callback, user_data, NULL);
	GTK_WIDGET_SET_FLAGS(widget, GDK_BUTTON_PRESS);
	g_signal_connect(widget, "button-press-event", G_CALLBACK(__gtk_widget_on_button_pressed), popup_callback);
	g_signal_connect(widget, "popup-menu", G_CALLBACK(__gtk_widget_on_popup_menu), popup_callback);

	return TRUE;
}

void gebr_gui_gtk_widget_drop_down_menu(GtkWidget * widget, GtkMenu * menu)
{
	gint x, y;
	gint xb, yb, hb;

	gdk_window_get_origin(widget->window, &x, &y);
	xb = widget->allocation.x;
	yb = widget->allocation.y;
	hb = widget->allocation.height;

	void popup_position(GtkMenu *m, gint *xp, gint *yp, gboolean *push_in, gpointer d) {
		*xp = x + xb;
		*yp = y + yb + hb;
		*push_in = TRUE;
	}

	gtk_widget_show_all(GTK_WIDGET(menu));
	gtk_menu_popup(menu, NULL, NULL, popup_position, NULL, 0, gtk_get_current_event_time());
}

struct GebrGuiDropDownData {
	GebrGuiDropDownFunc dropdown;
	gpointer user_data;
};

static gboolean on_widget_press_drop_down(GtkWidget * widget, GdkEventButton * event, struct GebrGuiDropDownData * data)
{
	GtkMenu *menu;
	menu = data->dropdown(widget, data->user_data);
	gebr_gui_gtk_widget_drop_down_menu(widget, menu);
	return FALSE;
}

void gebr_gui_gtk_widget_set_drop_down_menu_on_click(GtkWidget * widget, GebrGuiDropDownFunc dropdown,
						     gpointer user_data)
{
	struct GebrGuiDropDownData *data;
	data = g_new(struct GebrGuiDropDownData, 1);
	data->dropdown = dropdown;
	data->user_data = user_data;
	g_signal_connect(widget, "button-press-event", G_CALLBACK(on_widget_press_drop_down), data);
	g_object_weak_ref(G_OBJECT(widget), (GWeakNotify)g_free, data);
}

void
gebr_gui_gtk_tree_view_set_popup_callback(GtkTreeView * tree_view, GebrGuiGtkPopupCallback callback, gpointer user_data)
{
	struct popup_callback *popup_callback;

	popup_callback = __popup_callback_init(G_OBJECT(tree_view), callback, user_data, NULL);
	g_signal_connect(tree_view, "button-press-event",
			 G_CALLBACK(__gtk_tree_view_on_button_pressed), popup_callback);
	g_signal_connect(tree_view, "popup-menu", G_CALLBACK(__gtk_widget_on_popup_menu), popup_callback);
}

struct tooltip_data {
	GebrGuiGtkTreeViewTooltipCallback callback;
	gpointer user_data;
};

static void tooltip_weak_ref(struct tooltip_data *tooltip_data, GtkTreeView * tree_view)
{
	g_free(tooltip_data);
}

static gboolean
on_tooltip_query(GtkTreeView * tree_view, gint x, gint y, gboolean keyboard_mode,
		 GtkTooltip * tooltip, struct tooltip_data *tooltip_data)
{
	GtkTreePath *path;
	GtkTreeViewColumn *column;
	gchar *path_string;
	GtkTreeIter iter;

	gtk_tree_view_convert_widget_to_bin_window_coords(tree_view, x, y, &x, &y);
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
gebr_gui_gtk_tree_view_set_tooltip_callback(GtkTreeView * tree_view, GebrGuiGtkTreeViewTooltipCallback callback,
					    gpointer user_data)
{
	gulong signal_id;
	struct tooltip_data *tooltip_data;

	if ((signal_id = g_signal_handler_find(tree_view, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, on_tooltip_query, NULL))) {
		g_object_set(G_OBJECT(tree_view), "has-tooltip", FALSE, NULL);
		g_signal_handler_disconnect(tree_view, signal_id);
	}
	if (callback == NULL)
		return;

	tooltip_data = g_malloc(sizeof(struct tooltip_data));
	*tooltip_data = (struct tooltip_data) {
	.callback = callback,.user_data = user_data,};

	g_object_set(G_OBJECT(tree_view), "has-tooltip", TRUE, NULL);
	g_signal_connect(tree_view, "query-tooltip", G_CALLBACK(on_tooltip_query), tooltip_data);
	g_object_weak_ref(G_OBJECT(tree_view), (GWeakNotify) tooltip_weak_ref, tooltip_data);
}

gboolean
gtk_tree_view_reorder_callback(GtkTreeView * tree_view, GtkTreeIter * iter, GtkTreeIter * position,
			       GtkTreeViewDropPosition drop_position, struct reorderable_data *data)
{
	GtkTreeModel *tree_model;
	GebrGeoXmlSequence *sequence;
	GebrGeoXmlSequence *position_sequence;

	tree_model = gtk_tree_view_get_model(tree_view);
	gtk_tree_model_get(tree_model, iter, data->gebr_geoxml_sequence_pointer_column, &sequence, -1);
	gtk_tree_model_get(tree_model, position, data->gebr_geoxml_sequence_pointer_column, &position_sequence, -1);

	if (drop_position == GTK_TREE_VIEW_DROP_AFTER) {
		gebr_geoxml_sequence_move_after(sequence, position_sequence);
		gtk_list_store_move_after(GTK_LIST_STORE(tree_model), iter, position);

		gebr_geoxml_sequence_next(&position_sequence);
	} else {
		gebr_geoxml_sequence_move_before(sequence, position_sequence);
		gtk_list_store_move_before(GTK_LIST_STORE(tree_model), iter, position);
	}

	if (data->callback != NULL)
		data->callback(tree_model, sequence, position_sequence, data->user_data);

	return TRUE;
}

void
gebr_gui_gtk_tree_view_set_gebr_geoxml_sequence_moveable(GtkTreeView * tree_view,
							 gint gebr_geoxml_sequence_pointer_column,
							 GebrGuiGtkTreeViewMoveSequenceCallback callback,
							 gpointer user_data)
{
	struct reorderable_data *data;

	data = g_malloc(sizeof(struct reorderable_data));
	*data = (struct reorderable_data) {
	.gebr_geoxml_sequence_pointer_column = gebr_geoxml_sequence_pointer_column,.callback =
		    callback,.user_data = user_data,};

	gebr_gui_gtk_tree_view_set_reorder_callback(tree_view,
						    (GebrGuiGtkTreeViewReorderCallback) gtk_tree_view_reorder_callback,
						    NULL, data);

	g_object_weak_ref(G_OBJECT(tree_view),
			  (GWeakNotify) gebr_gui_gtk_tree_view_set_gebr_geoxml_sequence_moveable_weak_ref, data);
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
gebr_gui_gtk_tree_view_set_reorder_callback(GtkTreeView * tree_view, GebrGuiGtkTreeViewReorderCallback callback,
					    GebrGuiGtkTreeViewReorderCallback can_callback, gpointer user_data)
{
	const static GtkTargetEntry target_entries[] = {
		{"reorder", GTK_TARGET_SAME_WIDGET, 1}
	};
	struct reorder_data *data;

	if (tree_view == NULL || callback == NULL)
		return;

	data = g_malloc(sizeof(struct reorder_data));
	*data = (struct reorder_data) {
	.callback = callback,.can_callback = can_callback,.user_data = user_data,};

	gtk_tree_view_enable_model_drag_source(tree_view, GDK_MODIFIER_MASK, target_entries, 1, GDK_ACTION_MOVE);
	gtk_tree_view_enable_model_drag_dest(tree_view, target_entries, 1, GDK_ACTION_MOVE);

	g_signal_connect(tree_view, "drag-begin", G_CALLBACK(on_gtk_tree_view_drag_begin), data);
	g_signal_connect(tree_view, "drag-drop", G_CALLBACK(on_gtk_tree_view_drag_drop), data);
	g_signal_connect(tree_view, "drag-motion", G_CALLBACK(on_gtk_tree_view_drag_motion), data);

	g_object_weak_ref(G_OBJECT(tree_view), (GWeakNotify) gtk_tree_view_reorder_weak_ref, data);
}

gboolean gebr_gui_message_dialog(GtkMessageType type, GtkButtonsType buttons,
				 const gchar * title, const gchar * message, ...)
{
	va_list argp;
	gboolean ret;

	va_start(argp, message);
	ret = gebr_gui_message_dialog_vararg(type, buttons, title, message, argp);
	va_end(argp);

	return ret;
}

gboolean gebr_gui_confirm_action_dialog(const gchar * title, const gchar * message, ...)
{
	va_list argp;
	gboolean ret;

	va_start(argp, message);
	ret = gebr_gui_message_dialog_vararg(GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, title, message, argp);
	va_end(argp);

	return ret;
}

void gebr_gui_gtk_action_group_set_accel_group(GtkActionGroup * action_group, GtkAccelGroup * accel_group)
{
	GList *i, *list;

	list = gtk_action_group_list_actions(action_group);
	for (i = g_list_first(list); i != NULL; i = g_list_next(i)) {
		gtk_action_set_accel_group((GtkAction *) i->data, accel_group);
		gtk_action_connect_accelerator((GtkAction *) i->data);
	}

	g_list_free(list);
}

void gebr_gui_gtk_widget_set_tooltip(GtkWidget * widget, const gchar * tip)
{
	g_object_set(G_OBJECT(widget), "tooltip-text", tip, NULL);
}

GtkWidget *gebr_gui_gtk_container_add_depth_hbox(GtkWidget * container)
{
	GtkWidget *depth_hbox;
	GtkWidget *depth_widget;

	depth_hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(container), depth_hbox);
	gtk_widget_show(depth_hbox);

	depth_widget = gtk_label_new(""); //FIXME use gtk drawing area?
	gtk_box_pack_start(GTK_BOX(depth_hbox), depth_widget, FALSE, TRUE, 0);
	gtk_widget_set_size_request(depth_widget, 25, -1);
	gtk_widget_show(depth_widget);

	return depth_hbox;
}

void gebr_gui_gtk_expander_hacked_visible(GtkWidget * expander, GtkWidget * label_widget)
{
	g_signal_handlers_unblock_matched(G_OBJECT(label_widget),
					  G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
					  G_CALLBACK(gebr_gui_gtk_expander_hacked_idle), NULL);
}

gboolean gebr_gui_gtk_expander_hacked_idle(GtkWidget * label_widget, GdkEventExpose * event, GtkWidget * expander)
{
	g_signal_handlers_block_matched(G_OBJECT(label_widget),
					G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
					G_CALLBACK(gebr_gui_gtk_expander_hacked_idle), NULL);
	g_object_ref(G_OBJECT(label_widget));
	gtk_expander_set_label_widget(GTK_EXPANDER(expander), NULL);
	gtk_expander_set_label_widget(GTK_EXPANDER(expander), label_widget);
	g_object_unref(G_OBJECT(label_widget));

	return TRUE;
}
