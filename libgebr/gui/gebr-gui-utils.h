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

#ifndef __GEBR_GUI_UTILS_H
#define __GEBR_GUI_UTILS_H

#include <string.h>
#include <gtk/gtk.h>
#include <geoxml.h>

G_BEGIN_DECLS

/**
 * Frees \p data before \p object is freed.
 */
#define gebr_gui_g_object_set_free_parent(object, data) \
	g_object_weak_ref(G_OBJECT(object), (GWeakNotify)g_free, (gpointer)(data))

/**
 * \deprecated
 *
 * Makes the \p dialog response when pressing Return key.
 * This function should be avoided because the Return key behavior is done with the default widget of the dialog.
 *
 * \see gtk_dialog_set_default_response
 */
void gebr_gui_gtk_dialog_set_response_on_widget_return(GtkDialog * dialog, gint response, GtkWidget * widget);

/**
 * Checks if row pointed by \p iter can move upwards.
 */
gboolean gebr_gui_gtk_list_store_can_move_up(GtkListStore * store, GtkTreeIter * iter);

/**
 * Checks if row pointed by \p iter can move downwards.
 */
gboolean gebr_gui_gtk_list_store_can_move_down(GtkListStore * store, GtkTreeIter * iter);

/**
 * Moves the row pointed by \p iter upwards.
 * \return Wheter the row was moved or not.
 */
gboolean gebr_gui_gtk_list_store_move_up(GtkListStore * store, GtkTreeIter * iter);

/**
 * Moves the row pointed by \p iter downwards.
 * \return Wheter the row was moved or not.
 */
gboolean gebr_gui_gtk_list_store_move_down(GtkListStore * store, GtkTreeIter * iter);

/**
 * Returns the index of the row pointed by \p iter in \p list_store.
 */
gulong gebr_gui_gtk_list_store_get_iter_index(GtkListStore * list_store, GtkTreeIter * iter);

/**
 * Checks if row pointed by \p iter can move upwards.
 */
gboolean gebr_gui_gtk_tree_store_can_move_up(GtkTreeStore * store, GtkTreeIter * iter);

/**
 * Checks if row pointed by \p iter can move downwards.
 */
gboolean gebr_gui_gtk_tree_store_can_move_down(GtkTreeStore * store, GtkTreeIter * iter);

/**
 * Moves the row pointed by \p iter upwards.
 * \return Wheter the row was moved or not.
 */
gboolean gebr_gui_gtk_tree_store_move_up(GtkTreeStore * store, GtkTreeIter * iter);

/**
 * Moves the row pointed by \p iter downwards.
 * \return Wheter the row was moved or not.
 */
gboolean gebr_gui_gtk_tree_store_move_down(GtkTreeStore * store, GtkTreeIter * iter);

/**
 * Reparent row pointed by \p iter as a child of \p parent.
 *
 * This operation is done by copying all values of row \p iter into a new row which is a child of \p parent.
 * The \p iter value is updated to point to the new row and the last row is removed.
 *
 * \return In case \p iter is already a child of \p parent, this function returns FALSE. Otherwise returns TRUE.
 */
gboolean gebr_gui_gtk_tree_store_reparent(GtkTreeStore * store, GtkTreeIter * iter, GtkTreeIter * parent);

/**
 * Inter-level and sort resistant version gtk_tree_store_move_before
 */
gboolean gebr_gui_gtk_tree_store_move_before(GtkTreeStore * store, GtkTreeIter * iter, GtkTreeIter * before);

/**
 * Inter-level and sort resistant version gtk_tree_store_move_after
 */
gboolean gebr_gui_gtk_tree_store_move_after(GtkTreeStore * store, GtkTreeIter * iter, GtkTreeIter * after);

/**
 * Compares two iterators for equality.
 */
gboolean gebr_gui_gtk_tree_model_iter_equal_to(GtkTreeModel * model, GtkTreeIter * iter1, GtkTreeIter * iter2);

/**
 * Find an iterator for a string column
 */
gboolean gebr_gui_gtk_tree_model_find_by_column(GtkTreeModel * model, GtkTreeIter * iter, int column, const gchar *value);

/**
 * \deprecated
 *
 * Checks if \p iter is valid.
 * If you really need this function, check #GtkListStore or #GtkTreeStore classes
 * for their own function.
 *
 * \see gtk_list_store_iter_is_valid gtk_tree_store_iter_is_valid
 */
#define gebr_gui_gtk_tree_model_iter_is_valid(iter) \
	((gboolean)(iter)->stamp)

/**
 * \deprecated
 *
 * Compares two iterators and return whether they are equal or not.
 * You should avoid this function since it might be broken in future releases of Gtk.
 *
 * \see gebr_gui_gtk_tree_model_iter_equal_to
 */
#define gebr_gui_gtk_tree_iter_equal_to(iter1, iter2) \
	((iter1 == NULL || !(iter1)->stamp) && (iter2 == NULL || !(iter2)->stamp) \
		? TRUE : (iter1 == NULL || !(iter1)->stamp) || (iter2 == NULL || !(iter2)->stamp) \
			? FALSE : (gboolean)((iter1)->user_data == (iter2)->user_data))

/**
 * Copies the values of row pointed by \p source into \p iter.
 *
 * \param model The model in which \p source and \p iter are valid.
 * \param iter The row that will hold the copy.
 * \param source The row that will be copied.
 */
void gebr_gui_gtk_tree_model_iter_copy_values(GtkTreeModel * model, GtkTreeIter * iter, GtkTreeIter * source);

/**
 * Scrolls \p tree_view until row pointed by \p iter is visible.
 */
void gebr_gui_gtk_tree_view_scroll_to_iter_cell(GtkTreeView * tree_view, GtkTreeIter * iter);

/**
 * Returns a #GList of #GtkTreeIter that point to selected rows.
 */
GList *gebr_gui_gtk_tree_view_get_selected_iters(GtkTreeView * tree_view);

/**
 * When the \p tree_view has multiple selections, this function restrict its number to a single selection by choosing
 * the topmost one.
 */
void gebr_gui_gtk_tree_view_turn_to_single_selection(GtkTreeView * tree_view);

/**
 * Sets \p iter to point to the first selected row in \p tree_view.
 * \param iter[out] The iterator that will point to the selected row.
 * \return TRUE if there is a selected row, FALSE otherwise. In case FALSE is returned, \p iter is invalid.
 */
gboolean gebr_gui_gtk_tree_view_get_selected(GtkTreeView * tree_view, GtkTreeIter * iter);

/**
 * Sets \p iter to point to the last selected row in \p tree_view.
 * \param iter[out] The iterator that will point to the selected row.
 * \return TRUE if there is a selected row, FALSE otherwise. In case FALSE is returned, \p iter is invalid.
 */
gboolean gebr_gui_gtk_tree_view_get_last_selected(GtkTreeView * tree_view, GtkTreeIter * iter);

/**
 * Clear all selection, expand to \p iter, scroll to it and select it, and focus the first column of it.
 * If \p iter is NULL, selection is cleared.
 */
void gebr_gui_gtk_tree_view_select_iter(GtkTreeView * tree_view, GtkTreeIter * iter);

/**
 * Expands the rows of \p view so that the row pointed by \p iter is visible.
 * Fixes extrange behaviour of gtk_tree_view_expand_to_path that expand the \p iter it self.
 */
void gebr_gui_gtk_tree_view_expand_to_iter(GtkTreeView * view, GtkTreeIter * iter);

/**
 * Gets the #GtkTreeViewColumn that packs \p renderer.
 */
GtkTreeViewColumn *gebr_gui_gtk_tree_view_get_column_from_renderer(GtkTreeView * tree_view, GtkCellRenderer * renderer);

/**
 * Gets the column to the right of \p column.
 */
GtkTreeViewColumn *gebr_gui_gtk_tree_view_get_next_column(GtkTreeView * tree_view, GtkTreeViewColumn * column);

/**
 * Sets the focus on row pointed by \p iter in \p column.
 * \param start_editing Whether to start editing the first editable #GtkCellRenderer in coordinates given by \p iter
 * and \p column.
 */
void gebr_gui_gtk_tree_view_set_cursor(GtkTreeView * tree_view, GtkTreeIter * iter, GtkTreeViewColumn * column, gboolean start_editing);

/**
 * When collapsing a row in gtk, the selection changes but its signal is not emited; this function fixes this.
 */
void gebr_gui_gtk_tree_view_change_cursor_on_row_collapsed(GtkTreeView * tree_view);

/**
 * Sets the search column for this tree view to \p column and automatically expands all rows before searching.
 */
void gebr_gui_gtk_tree_view_fancy_search(GtkTreeView * tree_view, gint column);

/**
 * Iterates over all selected rows in a tree view, postfixing iteration variables with \p hyg.
 * \param iter A #GtkTreeIter that will iterate over selected rows.
 * \param tree_view The #GtkTreeView to work with.
 * \param hyg Postfix for iteration variables. Use this if you need to iterate two or more times in the same scope.
 * \see gebr_gui_gtk_tree_view_foreach_selected
 */
#define gebr_gui_gtk_tree_view_foreach_selected_hyg(iter, tree_view, hyg) \
	GList * __list##hyg = gebr_gui_gtk_tree_view_get_selected_iters(GTK_TREE_VIEW(tree_view)); \
	GList * __i##hyg = g_list_first(__list##hyg); \
	if (__i##hyg != NULL || (g_list_free(__list##hyg), 0)) \
		for (*iter = *(GtkTreeIter*)__i##hyg->data; \
		(__i##hyg != NULL && (*iter = *(GtkTreeIter*)__i##hyg->data, 1)) || \
			(g_list_foreach(__list##hyg, (GFunc)gtk_tree_iter_free, NULL), g_list_free(__list##hyg), 0); \
		__i##hyg = g_list_next(__i##hyg))

/**
 * Iterates over all selected rows in a tree view.
 * \param iter A #GtkTreeIter that will iterate over selected rows.
 * \param tree_view The #GtkTreeView to work with.
 * \see gebr_gui_gtk_tree_view_foreach_selected_hyg
 */
#define gebr_gui_gtk_tree_view_foreach_selected(iter, tree_view) gebr_gui_gtk_tree_view_foreach_selected_hyg(iter, tree_view, nohyg)

/**
 * Fetches the first #GtkCellRenderer of \p column.
 * \see gebr_gui_gtk_tree_view_column_get_first_renderer_with_mode
 */
GtkCellRenderer *gebr_gui_gtk_tree_view_column_get_first_renderer(GtkTreeViewColumn * column);

/**
 * Fetches the first #GtkCellRenderer from \p column with "mode" property equal to \p mode.
 * \see gebr_gui_gtk_tree_view_column_get_first_renderer
 */
GtkCellRenderer *gebr_gui_gtk_tree_view_column_get_first_renderer_with_mode(GtkTreeViewColumn * column,
									    GtkCellRendererMode mode);

/**
 * Iterates over all toplevel iters in a treemodel.
 *
 * This macro is similar to #gebr_gui_gtk_tree_model_foreach_hyg, postfixing variables with '__nohyg'.
 *
 * \see gebr_gui_gtk_tree_model_foreach_hyg
 */
#define gebr_gui_gtk_tree_model_foreach(iter, tree_model) \
	gebr_gui_gtk_tree_model_foreach_hyg(iter, tree_model, __nohyg)

/**
 * Iterates over all toplevel iters in a treemodel made with name clashing protection.
 *
 * This macro is meant to be used as a for loop, which iterates over all rows in a #GtkTreeModel.
 * An example is shown below:
 * \code
 * GtkTreeIter iter;
 * gebr_gui_gtk_tree_model_foreach_hyg(iter, model, loop1) { // postfix 'loop1' to avoid naming clashes
 * 	...
 * }
 * gebr_gui_gtk_tree_model_foreach_hyg(iter, model, loop2) {
 * 	...
 * }
 * \endcode
 *
 * \param iter The needle that will iterate over all rows of \p tree_model.
 * \param hygid A postfix name for naming clashes protection.
 *
 * \see gebr_gui_gtk_tree_model_foreach
 */
#define gebr_gui_gtk_tree_model_foreach_hyg(iter, tree_model, hygid) \
	gboolean valid##hygid; \
	GtkTreeIter iter##hygid; \
	for (valid##hygid = gtk_tree_model_get_iter_first(tree_model, &iter), iter##hygid = iter; \
	valid##hygid == TRUE && ((valid##hygid = gtk_tree_model_iter_next(tree_model, &iter##hygid)), 1); \
	iter = iter##hygid)

/**
 * Similar to #gebr_gui_gtk_tree_model_foreach_hyg, but iterates on each child of a given \p parent.
 * If \p parent is NULL then it works the same.
 */
#define gebr_gui_gtk_tree_model_foreach_child_hyg(iter, parent, tree_model, hygid) \
	gboolean valid##hygid; \
	GtkTreeIter iter##hygid; \
	for (valid##hygid = gtk_tree_model_iter_nth_child(tree_model, &iter, parent, 0), iter##hygid = iter; \
	valid##hygid == TRUE && ((valid##hygid = gtk_tree_model_iter_next(tree_model, &iter##hygid)), 1); \
	iter = iter##hygid)
#define gebr_gui_gtk_tree_model_foreach_child(iter, parent, tree_model) \
	gebr_gui_gtk_tree_model_foreach_child_hyg(iter, parent, tree_model, __nohyg)

/**
 * Alternative to gtk_tree_model_foreach that works with removal of iter
 */
void gebr_gui_gtk_tree_model_foreach_recursive(GtkTreeModel *tree_model, GtkTreeModelForeachFunc func,
					       gpointer user_data);

/**
 */
typedef void (*GebrGuiGtkTextViewLinkClickCallback)(GtkTextView * text_view, GtkTextTag * tag, const gchar * url, gpointer user_data);

/**
 * Create a tag with reference to \p url.
 * When it is clicked \p callback is called
 * You should take care to free \p url.
 * Returns the tag used.
 *
 * \see GebrGuiGtkTextViewLinkClickCallback
 */
GtkTextTag *gebr_gui_gtk_text_view_create_link_tag(GtkTextView * text_view, const gchar * url,
						   GebrGuiGtkTextViewLinkClickCallback callback, gpointer user_data);


/**
 * Create a mark before the after the last caracter
 */
GtkTextMark *gebr_gui_gtk_text_buffer_create_mark_before_last_char(GtkTextBuffer * text_buffer);

/**
 * Sets a \p tooltip markup for \p tag.
 * This functions set a tooltip key with #g_object_set_data
 * You should take care to free \p toolip.
 */
void gebr_gui_gtk_text_view_set_tooltip_on_tag(GtkTextView * text_view, GtkTextTag * tag, const gchar* tooltip);

/**
 * Callback called when a popup is requested.
 * \see gebr_gui_gtk_widget_set_popup_callback
 */
typedef GtkMenu *(*GebrGuiGtkPopupCallback) (GtkWidget *, gpointer);

/**
 * Sets \p callback to fire when user right click or request for a context menu on \p widget.
 * \see gebr_gui_gtk_tree_view_set_popup_callback
 */
gboolean gebr_gui_gtk_widget_set_popup_callback(GtkWidget * widget, GebrGuiGtkPopupCallback callback,
						gpointer user_data);

/**
 * Shows \p menu below \p widget.
 * \see gebr_gui_gtk_widget_set_drop_down_menu_on_click
 */
void gebr_gui_gtk_widget_drop_down_menu(GtkWidget * widget, GtkMenu * menu);

/**
 * The purpose of this callback is to create a menu for displaying below the widget given in the \ref
 * gebr_gui_gtk_widget_set_drop_down_menu_on_click.
 */
typedef GtkMenu * (*GebrGuiDropDownFunc)(GtkWidget * widget, gpointer user_data);

/**
 * Sets a drop-down menu to open below \p widget on click event.
 * \param callback Called when the drop down is requested. It must return a \ref GtkMenu.
 * \see GebrGuiDropDown
 */
void gebr_gui_gtk_widget_set_drop_down_menu_on_click(GtkWidget * widget, GebrGuiDropDownFunc dropdown,
						     gpointer user_data);

/**
 * Sets \p callback to fire when user right click or request for a context menu on \p tree_view.
 * \see gebr_gui_gtk_widget_set_popup_callback
 */
void
gebr_gui_gtk_tree_view_set_popup_callback(GtkTreeView * tree_view, GebrGuiGtkPopupCallback callback,
					  gpointer user_data);

/**
 * Callback for \ref gebr_gui_gtk_tree_view_set_tooltip_callback.
 * \see gebr_gui_gtk_tree_view_set_tooltip_callback
 */
typedef gboolean(*GebrGuiGtkTreeViewTooltipCallback) (GtkTreeView * tree_view, GtkTooltip * tooltip,
						      GtkTreeIter * iter, GtkTreeViewColumn * column,
						      gpointer user_data);
/**
 * Sets a callback for when a tooltip is requested.
 * \see GebrGuiGtkTreeViewTooltipCallback
 */
void gebr_gui_gtk_tree_view_set_tooltip_callback(GtkTreeView * tree_view,
						 GebrGuiGtkTreeViewTooltipCallback callback, gpointer user_data);

/**
 * Callback for when \p sequence have been moved before \p item node.
 * \see gebr_gui_gtk_tree_view_set_gebr_geoxml_sequence_moveable
 */
typedef void (*GebrGuiGtkTreeViewMoveSequenceCallback) (GtkTreeModel * tree_model, GebrGeoXmlSequence * sequence,
							GebrGeoXmlSequence * item, gpointer user_data);

/**
 * Make the rows of \p tree_view drag able and reorder the \ref GebrGeoXmlSequence
 * of the model in column \p sequence_pointer_column to match the \p tree_view.
 *
 * #GebrGeoXmlSequence is used very often in GeBR, and it is mapped into a TreeView.
 * This function ease the task of mantaining the order of the sequence equivalent to
 * the order of the TreeView. The \p callback is called whenever a reordering happens.
 * \see GebrGuiGtkTreeViewMoveSequenceCallback
 */
void
gebr_gui_gtk_tree_view_set_gebr_geoxml_sequence_moveable(GtkTreeView * tree_view,
							 gint sequence_pointer_column,
							 GebrGuiGtkTreeViewMoveSequenceCallback callback,
							 gpointer user_data);

/**
 * Callback for \ref gebr_gui_gtk_tree_view_set_reorder_callback.
 */
typedef gboolean(*GebrGuiGtkTreeViewReorderCallback) (GtkTreeView * tree_view, GtkTreeIter * iter, GtkTreeIter * position,
						      GtkTreeViewDropPosition drop_position, gpointer user_data);
/**
 * Make \p tree_view reorderable.
 * \param reorder_callback Responsible for reordering. Its return value is ignored.
 * \param may_reorder_callback Called at each cursor movement. If it returns TRUE, drop is allowed; otherwise is denied.
 * If NULL, every movement will be accepted.
 */
void
gebr_gui_gtk_tree_view_set_reorder_callback(GtkTreeView * tree_view, GebrGuiGtkTreeViewReorderCallback reorder_callback,
					    GebrGuiGtkTreeViewReorderCallback may_reorder_callback, gpointer user_data);

/**
 * Creates a modal message dialog of \p type; the message is a printf-like format.
 * \param type A #GtkMessageType that will specify the icon of the dialog.
 * \param buttons A #GtkButtonsType to place buttons in the dialog.
 * \param title The title of the dialog.
 * \param message A printf-like message format.
 * \return TRUE if response was GTK_RESPONSE_YES or GTK_RESPONSE_OK.
 */
gboolean gebr_gui_message_dialog(GtkMessageType type, GtkButtonsType buttons,
				 const gchar * title, const gchar * message, ...);

/**
 * Show an action confirmation dialog with printf-like formated \p message.
 * \return TRUE if user pressed YES; FALSE otherwise.
 */
gboolean gebr_gui_confirm_action_dialog(const gchar * title, const gchar * message, ...);

/**
 * Sets \p accel_group to all actions in \p action_group.
 */
void gebr_gui_gtk_action_group_set_accel_group(GtkActionGroup * action_group, GtkAccelGroup * accel_group);

/**
 * Sets \p tip as a tooltip for \p widget.
 */
void gebr_gui_gtk_widget_set_tooltip(GtkWidget * widget, const gchar * tip);

/**
 * Adds depth effect for expanders.
 */
GtkWidget *gebr_gui_gtk_container_add_depth_hbox(GtkWidget * container);

/**
 * Hacking the #GtkExpander to allow widgets other than #GtkLabel.
 */
#define gebr_gui_gtk_expander_hacked_define(expander, label_widget) \
	g_signal_connect_after(label_widget, "expose-event", \
		G_CALLBACK(gebr_gui_gtk_expander_hacked_idle), \
		expander); \
	g_signal_connect(expander, "unmap", \
		G_CALLBACK(gebr_gui_gtk_expander_hacked_visible), \
		label_widget)

void gebr_gui_gtk_expander_hacked_visible(GtkWidget * parent_expander, GtkWidget * hbox);

gboolean gebr_gui_gtk_expander_hacked_idle(GtkWidget * hbox, GdkEventExpose * event, GtkWidget * expander);

#if !GTK_CHECK_VERSION(2,16,0)
#include "sexy-icon-entry.h"
/* Symbols name compatibility with GTK 2.16 using SexyIconEntry */
#define gtk_entry_new sexy_icon_entry_new
#define gtk_entry_get_icon_from_stock(entry, icon_pos, stock_id) \
	sexy_icon_entry_get_icon(SEXY_ICON_ENTRY(entry), icon_pos, \
	stock_id != NULL ? GTK_IMAGE(gtk_image_new_from_stock(stock_id, GTK_ICON_SIZE_MENU)) : NULL)
#define gtk_entry_set_icon_from_stock(entry, icon_pos, stock_id) \
	sexy_icon_entry_set_icon(SEXY_ICON_ENTRY(entry), icon_pos, \
	stock_id != NULL ? GTK_IMAGE(gtk_image_new_from_stock(stock_id, GTK_ICON_SIZE_MENU)) : NULL)
#define gtk_entry_set_icon_tooltip_text(entry, icon_pos, tooltip) \
	gtk_widget_set_tooltip_text(GTK_WIDGET(entry), tooltip)
#define gtk_entry_set_icon_tooltip_markup(entry, icon_pos, tooltip) \
	gtk_widget_set_tooltip_markup(GTK_WIDGET(entry), tooltip)
#endif

#if !GTK_CHECK_VERSION(2,14,0)
gboolean gtk_show_uri(GdkScreen *screen, const gchar *uri, guint32 timestamp, GError **error);
#endif

gboolean gebr_gui_show_uri(const gchar * uri);

G_END_DECLS
#endif				//__GEBR_GUI_UTILS_H
