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

#ifndef __LIBGEBR_GUI_UTILS_H
#define __LIBGEBR_GUI_UTILS_H

#include <string.h>

#include <gtk/gtk.h>

#include <geoxml.h>

gboolean
gtk_list_store_can_move_up(GtkListStore * store, GtkTreeIter * iter);
gboolean
gtk_list_store_can_move_down(GtkListStore * store, GtkTreeIter * iter);
gboolean
gtk_list_store_move_up(GtkListStore * store, GtkTreeIter * iter);
gboolean
gtk_list_store_move_down(GtkListStore * store, GtkTreeIter * iter);
gulong
gtk_list_store_get_iter_index(GtkListStore * list_store, GtkTreeIter * iter);

gboolean
gtk_tree_store_can_move_up(GtkTreeStore * store, GtkTreeIter * iter);
gboolean
gtk_tree_store_can_move_down(GtkTreeStore * store, GtkTreeIter * iter);
gboolean
gtk_tree_store_move_up(GtkTreeStore * store, GtkTreeIter * iter);
gboolean
gtk_tree_store_move_down(GtkTreeStore * store, GtkTreeIter * iter);

#define gtk_tree_model_iter_is_valid(iter) \
	((gboolean)(iter)->stamp)
#define gtk_tree_model_iter_equal_to(iter1, iter2) \
	((iter1 == NULL || !(iter1)->stamp) && (iter2 == NULL || !(iter2)->stamp) \
		? TRUE : (iter1 == NULL || !(iter1)->stamp) || (iter2 == NULL || !(iter2)->stamp) \
			? FALSE : (gboolean)((iter1)->user_data == (iter2)->user_data))
void
gtk_tree_model_iter_copy_values(GtkTreeModel * model, GtkTreeIter * iter, GtkTreeIter * source);
gboolean
gtk_tree_model_path_to_iter(GtkTreeModel * model, GtkTreePath * tree_path, GtkTreeIter * iter);
GList *
libgebr_gtk_tree_model_path_to_iter_list(GtkTreeModel * model, GList * path_list);

GList *
libgebr_gtk_tree_view_get_selected_iters(GtkTreeView * tree_view);
gboolean
libgebr_gtk_tree_view_get_selected(GtkTreeView * tree_view, GtkTreeIter * iter);
#define libgebr_gtk_tree_view_foreach_selected(iter, tree_view) \
	GList * __list = libgebr_gtk_tree_view_get_selected_iters(GTK_TREE_VIEW(tree_view)); \
	GList * __i = g_list_first(__list); \
	if (__i != NULL || (g_list_free(__list), 0)) \
		for (*iter = *(GtkTreeIter*)__i->data; \
		(__i != NULL && (*iter = *(GtkTreeIter*)__i->data, 1)) || \
			(g_list_foreach(__list, (GFunc)gtk_tree_iter_free, NULL), g_list_free(__list), 0); \
		__i = g_list_next(__i))

typedef GtkMenu * (*GtkPopupCallback)(GtkWidget *, gpointer);
gboolean
gtk_widget_set_popup_callback(GtkWidget * widget, GtkPopupCallback callback, gpointer user_data);
void
gtk_tree_view_set_popup_callback(GtkTreeView * tree_view, GtkPopupCallback callback, gpointer user_data);

/**
 * Used when the selected iter is about to be removed
 * so the next or previous (if it is the last) is selected
 * If nothing is selected or the selection mode is multiple,
 * then the first iter (if exists) is selected. So this function
 * can be used just to select the first iter.
 */
void
gtk_tree_view_select_sibling(GtkTreeView * tree_view);

#if GTK_CHECK_VERSION(2,12,0)
typedef gboolean (*GtkTreeViewTooltipCallback)(GtkTreeView * tree_view, GtkTooltip * tooltip,
	GtkTreeIter * iter, GtkTreeViewColumn * column, gpointer user_data);
void
gtk_tree_view_set_tooltip_callback(GtkTreeView * tree_view, GtkTreeViewTooltipCallback callback, gpointer user_data);
#endif

typedef void (*GtkTreeViewMoveSequenceCallback)(GtkTreeModel * tree_model, GeoXmlSequence * sequence,
	GeoXmlSequence * before, gpointer user_data);
void
gtk_tree_view_set_geoxml_sequence_moveable(GtkTreeView * tree_view, gint geoxml_sequence_pointer_column,
	GtkTreeViewMoveSequenceCallback callback, gpointer user_data);

/**
 * Callback for \ref gtk_tree_view_set_reorder_callback
 */
typedef gboolean (*GtkTreeViewReorderCallback)(GtkTreeView * tree_view, GtkTreeIter * iter,
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
gtk_tree_view_set_reorder_callback(GtkTreeView * tree_view, GtkTreeViewReorderCallback reorder_callback,
	GtkTreeViewReorderCallback may_reorder_callback, gpointer user_data);

gboolean
confirm_action_dialog(const gchar * title, const gchar * message, ...);

void
set_tooltip(GtkWidget * widget, const gchar * tip);

GtkWidget *
gtk_container_add_depth_hbox(GtkWidget * container);

#define gtk_expander_hacked_define(expander, label_widget)			\
	g_signal_connect_after ((gpointer) label_widget, "expose-event",	\
			(GCallback)gtk_expander_hacked_idle,			\
			expander);						\
	g_signal_connect((gpointer) expander, "unmap",				\
			(GCallback)gtk_expander_hacked_visible,			\
			label_widget)

void
gtk_expander_hacked_visible(GtkWidget * parent_expander, GtkWidget * hbox);

gboolean
gtk_expander_hacked_idle(GtkWidget * hbox, GdkEventExpose *event, GtkWidget * expander);

#endif //__LIBGEBR_GUI_UTILS_H
