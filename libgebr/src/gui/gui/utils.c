/*   libgebr - GÍBR Library
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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

	if (!(event->type == GDK_BUTTON_PRESS && event->button == 3))
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

	if (!(event->type == GDK_BUTTON_PRESS && event->button == 3))
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

	gtk_tree_model_get_iter(GTK_TREE_MODEL(store),
				&previous, previous_path);
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
gtk_widget_set_popup_callback(GtkWidget * widget, GtkPopupCallback callback, gpointer user_data)
{
	struct popup_callback *	popup_callback;

// 	if (GTK_WIDGET_FLAGS(widget) & GTK_NO_WINDOW) {
// 		GtkWidget *	event_box;
//
// 		event_box = gtk_event_box_new();
// 		gtk_widget_show(event_box);
// 		gtk_container_add(GTK_CONTAINER(event_box), widget);
// 		gtk_event_box_set_above_child(GTK_EVENT_BOX(event_box), TRUE);
// puts("eve");
// 		popup_callback = __popup_callback_init(G_OBJECT(widget), callback, user_data, event_box);
// 		g_signal_connect(event_box, "button-press-event",
// 			(GCallback)__gtk_widget_on_button_pressed, popup_callback);
// 	} else {
		popup_callback = __popup_callback_init(G_OBJECT(widget), callback, user_data, NULL);
		g_signal_connect(widget, "button-press-event",
			(GCallback)__gtk_widget_on_button_pressed, popup_callback);
// 	}

	g_signal_connect(widget, "popup-menu",
		(GCallback)__gtk_widget_on_popup_menu, popup_callback);
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
