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

#ifndef __LIBGEBR_GUI_UTILS_H
#define __LIBGEBR_GUI_UTILS_H

#include <string.h>

#include <gtk/gtk.h>

#include <geoxml.h>

gboolean
libgebr_gui_gtk_list_store_can_move_up(GtkListStore * store, GtkTreeIter * iter);
gboolean
libgebr_gui_gtk_list_store_can_move_down(GtkListStore * store, GtkTreeIter * iter);
gboolean
libgebr_gui_gtk_list_store_move_up(GtkListStore * store, GtkTreeIter * iter);
gboolean
libgebr_gui_gtk_list_store_move_down(GtkListStore * store, GtkTreeIter * iter);
gulong
libgebr_gui_gtk_list_store_get_iter_index(GtkListStore * list_store, GtkTreeIter * iter);

gboolean
libgebr_gui_gtk_tree_store_can_move_up(GtkTreeStore * store, GtkTreeIter * iter);
gboolean
libgebr_gui_gtk_tree_store_can_move_down(GtkTreeStore * store, GtkTreeIter * iter);
gboolean
libgebr_gui_gtk_tree_store_move_up(GtkTreeStore * store, GtkTreeIter * iter);
gboolean
libgebr_gui_gtk_tree_store_move_down(GtkTreeStore * store, GtkTreeIter * iter);
gboolean
libgebr_gui_gtk_tree_store_reparent(GtkTreeStore * store, GtkTreeIter * iter, GtkTreeIter * parent);

#define libgebr_gui_gtk_tree_model_iter_is_valid(iter) \
	((gboolean)(iter)->stamp)
#define libgebr_gui_gtk_tree_model_iter_equal_to(iter1, iter2) \
	((iter1 == NULL || !(iter1)->stamp) && (iter2 == NULL || !(iter2)->stamp) \
		? TRUE : (iter1 == NULL || !(iter1)->stamp) || (iter2 == NULL || !(iter2)->stamp) \
			? FALSE : (gboolean)((iter1)->user_data == (iter2)->user_data))
void
libgebr_gui_gtk_tree_model_iter_copy_values(GtkTreeModel * model, GtkTreeIter * iter, GtkTreeIter * source);
gboolean
libgebr_gui_gtk_tree_model_path_to_iter(GtkTreeModel * model, GtkTreePath * tree_path, GtkTreeIter * iter);

void
libgebr_gui_gtk_tree_view_scroll_to_iter_cell(GtkTreeView * tree_view, GtkTreeIter * iter);
GList *
libgebr_gui_gtk_tree_view_get_selected_iters(GtkTreeView * tree_view);
void
libgebr_gui_gtk_tree_view_turn_to_single_selection(GtkTreeView * tree_view);
gboolean
libgebr_gtk_tree_view_get_selected(GtkTreeView * tree_view, GtkTreeIter * iter);
void
libgebr_gui_gtk_tree_view_expand(GtkTreeView * view, GtkTreeIter * iter, gboolean open_all);
#define libgebr_gtk_tree_view_foreach_selected(iter, tree_view) \
	GList * __list = libgebr_gui_gtk_tree_view_get_selected_iters(GTK_TREE_VIEW(tree_view)); \
	GList * __i = g_list_first(__list); \
	if (__i != NULL || (g_list_free(__list), 0)) \
		for (*iter = *(GtkTreeIter*)__i->data; \
		(__i != NULL && (*iter = *(GtkTreeIter*)__i->data, 1)) || \
			(g_list_foreach(__list, (GFunc)gtk_tree_iter_free, NULL), g_list_free(__list), 0); \
		__i = g_list_next(__i))

#define libgebr_gui_gtk_tree_model_foreach(iter, tree_model) \
	libgebr_gui_gtk_tree_model_foreach_hyg(iter, tree_model, __nohyg)
/* iterates over all toplevel iters in a treemodel
 * made with iter removal protection
 */
#define libgebr_gui_gtk_tree_model_foreach_hyg(iter, tree_model, hygid) \
	gboolean valid##hygid; \
	GtkTreeIter iter##hygid; \
	for (valid##hygid = gtk_tree_model_get_iter_first(tree_model, &iter), iter##hygid = iter; \
	valid##hygid == TRUE && ((valid##hygid = gtk_tree_model_iter_next(tree_model, &iter##hygid)), 1); \
	iter = iter##hygid)

typedef GtkMenu * (*GtkPopupCallback)(GtkWidget *, gpointer);
gboolean
libgebr_gui_gtk_widget_set_popup_callback(GtkWidget * widget, GtkPopupCallback callback, gpointer user_data);
void
libgebr_gui_gtk_tree_view_set_popup_callback(GtkTreeView * tree_view, GtkPopupCallback callback, gpointer user_data);

/**
 * Used when the selected iter is about to be removed
 * so the next or previous (if it is the last) is selected
 * If nothing is selected or the selection mode is multiple,
 * then the first iter (if exists) is selected. So this function
 * can be used just to select the first iter.
 */
void
libgebr_gui_gtk_tree_view_select_sibling(GtkTreeView * tree_view);

#if GTK_CHECK_VERSION(2,12,0)
typedef gboolean (*LibGeBRGUIGtkTreeViewTooltipCallback)(GtkTreeView * tree_view, GtkTooltip * tooltip,
	GtkTreeIter * iter, GtkTreeViewColumn * column, gpointer user_data);
void
libgebr_gui_gtk_tree_view_set_tooltip_callback(GtkTreeView * tree_view, LibGeBRGUIGtkTreeViewTooltipCallback callback, gpointer user_data);
#endif

typedef void (*LibGeBRGUIGtkTreeViewMoveSequenceCallback)(GtkTreeModel * tree_model, GeoXmlSequence * sequence,
	GeoXmlSequence * before, gpointer user_data);
void
libgebr_gui_gtk_tree_view_set_geoxml_sequence_moveable(GtkTreeView * tree_view, gint geoxml_sequence_pointer_column,
	LibGeBRGUIGtkTreeViewMoveSequenceCallback callback, gpointer user_data);

/**
 * Callback for \ref libgebr_gui_gtk_tree_view_set_reorder_callback
 */
typedef gboolean (*LibGeBRGUIGtkTreeViewReorderCallback)(GtkTreeView * tree_view, GtkTreeIter * iter,
	GtkTreeIter * position, GtkTreeViewDropPosition drop_position, gpointer user_data);
/**
 * Make \p tree_view reorderable.
 * \p reorder_callback is responsible for reorderation. Its return value
 * is ignored.
 * \p may_reorder_callback is called at each cursor movement. If it returns TRUE, then the
 * drop is allowed; otherwise is denied.
 * If \p may_reorder_callback is NULL every movement will be accepted.
 */
void
libgebr_gui_gtk_tree_view_set_reorder_callback(GtkTreeView * tree_view, LibGeBRGUIGtkTreeViewReorderCallback reorder_callback,
	LibGeBRGUIGtkTreeViewReorderCallback may_reorder_callback, gpointer user_data);

gboolean
libgebr_gui_message_dialog(GtkMessageType type, GtkButtonsType buttons,
	const gchar * title, const gchar * message, ...);
gboolean
libgebr_gui_confirm_action_dialog(const gchar * title, const gchar * message, ...);

void
libgebr_gui_gtk_action_group_set_accel_group(GtkActionGroup * action_group, GtkAccelGroup * accel_group);

void
set_tooltip(GtkWidget * widget, const gchar * tip);

GtkWidget *
libgebr_gui_gtk_container_add_depth_hbox(GtkWidget * container);

#define libgebr_gui_gtk_expander_hacked_define(expander, label_widget) \
	g_signal_connect_after(label_widget, "expose-event", \
		(GCallback)libgebr_gui_gtk_expander_hacked_idle, \
		expander); \
	g_signal_connect(expander, "unmap", \
		(GCallback)libgebr_gui_gtk_expander_hacked_visible, \
		label_widget)
void
libgebr_gui_gtk_expander_hacked_visible(GtkWidget * parent_expander, GtkWidget * hbox);
gboolean
libgebr_gui_gtk_expander_hacked_idle(GtkWidget * hbox, GdkEventExpose *event, GtkWidget * expander);

#if !GTK_CHECK_VERSION(2,16,0)
/* Symbols name compatibility with GTK 2.16 */
#define GtkEntry	SexyIconEntry
#define gtk_entry_new	sexy_icon_entry_new
#define gtk_entry_set_icon_from_stock(entry, icon_pos, stock_id) \
	sexy_icon_entry_set_icon(SEXY_ICON_ENTRY(entry), icon_pos, \
	gtk_image_new_from_stock(stock_id, GTK_ICON_SIZE_MENU))
#endif // !GTK_CHECK_VERSION(2,16,0)

#endif //__LIBGEBR_GUI_UTILS_H
