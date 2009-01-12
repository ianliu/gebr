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

#define gtk_tree_model_iter_equal_to(iter1, iter2) \
	((gboolean)((iter1)->user_data == (iter2)->user_data))

typedef GtkMenu * (*GtkPopupCallback)(GtkWidget *, gpointer);

gboolean
gtk_widget_set_popup_callback(GtkWidget * widget, GtkPopupCallback callback, gpointer user_data);

void
gtk_tree_view_set_popup_callback(GtkTreeView * tree_view, GtkPopupCallback callback, gpointer user_data);

gboolean
gtk_tree_view_get_iter_from_coords(GtkTreeView * tree_view, GtkTreeIter * iter, gint x, gint y);

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

typedef gboolean (*GtkTreeViewReorderCallback)(GtkTreeView * tree_view, GtkTreeIter * iter,
	GtkTreeIter * before, gpointer user_data);

void
gtk_tree_view_set_reorder_callback(GtkTreeView * tree_view, GtkTreeViewReorderCallback callback,
	GtkTreeViewReorderCallback can_callback, gpointer user_data);

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
