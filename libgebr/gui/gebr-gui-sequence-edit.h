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

#ifndef __GEBR_GUI_SEQUENCE_EDIT_H
#define __GEBR_GUI_SEQUENCE_EDIT_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

GType gebr_gui_sequence_edit_get_type(void);

#define GEBR_GUI_TYPE_SEQUENCE_EDIT		(gebr_gui_sequence_edit_get_type())
#define GEBR_GUI_SEQUENCE_EDIT(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_GUI_TYPE_SEQUENCE_EDIT, GebrGuiSequenceEdit))
#define GEBR_GUI_SEQUENCE_EDIT_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_GUI_TYPE_SEQUENCE_EDIT, GebrGuiSequenceEditClass))
#define GEBR_GUI_IS_SEQUENCE_EDIT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_GUI_TYPE_SEQUENCE_EDIT))
#define GEBR_GUI_IS_SEQUENCE_EDIT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_GUI_TYPE_SEQUENCE_EDIT))
#define GEBR_GUI_SEQUENCE_EDIT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_GUI_TYPE_SEQUENCE_EDIT, GebrGuiSequenceEditClass))

typedef struct _GebrGuiSequenceEdit GebrGuiSequenceEdit;
typedef struct _GebrGuiSequenceEditClass GebrGuiSequenceEditClass;
typedef struct _GebrGuiSequenceEditPriv GebrGuiSequenceEditPriv;

struct _GebrGuiSequenceEdit {
	GtkVBox parent;

	GtkWidget *widget;
	GtkWidget *widget_hbox;

	GtkListStore *list_store;
	GtkWidget *tree_view;
	GtkCellRenderer *renderer;

	gboolean may_rename;
	gboolean has_scroll;

	GebrGuiSequenceEditPriv *priv;
};

/**
 * GebrGuiSequenceEditClass:
 * @add: Called when 
 * @create_tree_view: 
 */
struct _GebrGuiSequenceEditClass {
	/*< private >*/
	GtkVBoxClass parent;

	/* signals */
	void		(*add_request)	(GebrGuiSequenceEdit *self);

	void		(*changed)	(GebrGuiSequenceEdit *self);

	void		(*removed)	(GebrGuiSequenceEdit *self,
					 const gchar *old_text);

	gboolean	(*renamed)	(GebrGuiSequenceEdit *self,
					 const gchar *old_text,
					 const gchar *new_text);

	/* Abstract methods */
	void		(*add)		(GebrGuiSequenceEdit *self);

	void		(*remove)	(GebrGuiSequenceEdit *self,
					 GtkTreeIter *iter);

	void		(*move)		(GebrGuiSequenceEdit *self,
					 GtkTreeIter *iter,
					 GtkTreeIter *position,
					 GtkTreeViewDropPosition drop_position);

	void		(*move_top)	(GebrGuiSequenceEdit *self,
					 GtkTreeIter *iter);

	void		(*move_bottom)	(GebrGuiSequenceEdit *self,
					 GtkTreeIter *iter);

	void		(*rename)	(GebrGuiSequenceEdit *self,
					 GtkTreeIter *iter,
					 const gchar *new_text);

	GtkWidget *	(*create_tree_view) (GebrGuiSequenceEdit *self);
};

/**
 * gebr_gui_sequence_edit_add:
 * @self: a #GebrGuiSequenceEdit widget
 * @text: the c-string to be added into this sequence edit widget
 * @show_empty: if %TRUE and @text is an empty string, insert '&lt;empty value&gt;' instead
 *
 * Appends @text into this sequence edit widget. If @show_empty is %TRUE and @text is empty, the value inserted will be
 * '&lt;empty value&gt;'.
 *
 * Returns: the #GtkTreeIter pointing to the new element added
 */
GtkTreeIter gebr_gui_sequence_edit_add (GebrGuiSequenceEdit *self,
					const gchar *text,
					gboolean show_empty);

/**
 * gebr_gui_sequence_edit_clear:
 * @self: a #GebrGuiSequenceEdit widget
 *
 * Removes all values from this sequence edit. This method only calls gtk_list_store_clear() on @self<!-- -->s
 * #GtkListStore.
 */
void gebr_gui_sequence_edit_clear (GebrGuiSequenceEdit *self);

/**
 * gebr_gui_sequence_edit_remove:
 * @self: a #GebrGuiSequenceEdit widget
 * @iter: the row to be removed
 *
 * Removes @iter from this sequence edit widget.
 */
void gebr_gui_sequence_edit_remove (GebrGuiSequenceEdit *self,
				    GtkTreeIter *iter);
/**
 * gebr_gui_sequence_edit_move:
 * @self: a #GebrGuiSequenceEdit widget
 * @iter: a #GtkTreeIter pointing to the row to be moved
 * @position: position to place @iter
 * @pos: a #GtkTreeViewDropPosition
 *
 * Moves @iter above or below @position, depending on the value of @pos.
 */
void gebr_gui_sequence_edit_move (GebrGuiSequenceEdit *self,
				  GtkTreeIter *iter,
				  GtkTreeIter *position,
				  GtkTreeViewDropPosition pos);

/**
 * gebr_gui_sequence_edit_move_top:
 * @self: a #GebrGuiSequenceEdit widget
 * @iter: the iterator to be moved to the top of the list
 */
void gebr_gui_sequence_edit_move_top (GebrGuiSequenceEdit *self,
				      GtkTreeIter *iter);

/**
 * gebr_gui_sequence_edit_move_bottom:
 * @self: a #GebrGuiSequenceEdit widget
 * @iter: the iterator to be moved to the bottom of the list
 */
void gebr_gui_sequence_edit_move_bottom (GebrGuiSequenceEdit *self,
					 GtkTreeIter *iter);

/**
 * gebr_gui_sequence_edit_rename:
 * @self: a #GebrGuiSequenceEdit widget
 * @iter: the iterator to be renamed
 * @new_text: the new value to be put on previous iterator
 */
void gebr_gui_sequence_edit_rename(GebrGuiSequenceEdit *self,
                                   GtkTreeIter *iter,
                                   const gchar *new_text);

/**
 * gebr_gui_sequence_edit_toggle_keypresses:
 * @self:
 * @setting:
 */
void gebr_gui_sequence_edit_set_keypresses(GebrGuiSequenceEdit *self,
                                              gboolean setting);

G_END_DECLS

#endif				//__GEBR_GUI_SEQUENCE_EDIT_H
