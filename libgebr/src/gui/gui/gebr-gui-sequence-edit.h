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

#ifndef __GEBR_GUI_gebr_gui_sequence_edit_H
#define __GEBR_GUI_gebr_gui_sequence_edit_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

GType gebr_gui_sequence_edit_get_type(void);

#define GEBR_GUI_GTK_TYPE_SEQUENCE_EDIT			(gebr_gui_sequence_edit_get_type())
#define GEBR_GUI_gebr_gui_sequence_edit(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_GUI_GTK_TYPE_SEQUENCE_EDIT, GebrGuiSequenceEdit))
#define GEBR_GUI_gebr_gui_sequence_edit_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_GUI_GTK_TYPE_SEQUENCE_EDIT, GebrGuiSequenceEditClass))
#define GEBR_GUI_GTK_IS_SEQUENCE_EDIT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_GUI_GTK_TYPE_SEQUENCE_EDIT))
#define GEBR_GUI_GTK_IS_SEQUENCE_EDIT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_GUI_GTK_TYPE_SEQUENCE_EDIT))
#define GEBR_GUI_gebr_gui_sequence_edit_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_GUI_GTK_TYPE_SEQUENCE_EDIT, GebrGuiSequenceEditClass))

typedef struct _GebrGuiSequenceEdit GebrGuiSequenceEdit;
typedef struct _GebrGuiSequenceEditClass GebrGuiSequenceEditClass;

struct _GebrGuiSequenceEdit {
	GtkVBox parent;

	GtkWidget *widget;
	GtkWidget *widget_hbox;

	GtkListStore *list_store;
	GtkWidget *tree_view;
	GtkCellRenderer *renderer;

	gboolean may_rename;
};
struct _GebrGuiSequenceEditClass {
	GtkVBoxClass parent;

	/* signals */
	void (*add_request) (GebrGuiSequenceEdit * self);
	void (*changed) (GebrGuiSequenceEdit * self);
	void (*renamed) (GebrGuiSequenceEdit * self, const gchar * old_text, const gchar * new_text);
	void (*removed) (GebrGuiSequenceEdit * self, const gchar * old_text);
	/* virtual */
	void (*add) (GebrGuiSequenceEdit * self);
	void (*remove) (GebrGuiSequenceEdit * self, GtkTreeIter * iter);
	void (*move) (GebrGuiSequenceEdit * self, GtkTreeIter * iter, GtkTreeIter * position,
		      GtkTreeViewDropPosition drop_position);
	void (*move_top) (GebrGuiSequenceEdit * self, GtkTreeIter * iter);
	void (*move_bottom) (GebrGuiSequenceEdit * self, GtkTreeIter * iter);
	void (*rename) (GebrGuiSequenceEdit * self, GtkTreeIter * iter, const gchar * new_text);
	GtkWidget *(*create_tree_view) (GebrGuiSequenceEdit * self);
};

GtkWidget *gebr_gui_sequence_edit_new(GtkWidget * widget);

GtkWidget *gebr_gui_sequence_edit_new_from_store(GtkWidget * widget, GtkListStore * list_store);

GtkTreeIter gebr_gui_sequence_edit_add(GebrGuiSequenceEdit * sequence_edit, const gchar * text, gboolean show_empty_value_text);

void gebr_gui_sequence_edit_clear(GebrGuiSequenceEdit * sequence_edit);

G_END_DECLS
#endif				//__GEBR_GUI_gebr_gui_sequence_edit_H
