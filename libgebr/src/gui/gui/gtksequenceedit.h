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

#ifndef __LIBGEBR_GUI_GTK_SEQUENCE_EDIT_H
#define __LIBGEBR_GUI_GTK_SEQUENCE_EDIT_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

GType
gtk_sequence_edit_get_type(void);

#define GTK_TYPE_SEQUENCE_EDIT			(gtk_sequence_edit_get_type())
#define GTK_SEQUENCE_EDIT(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_SEQUENCE_EDIT, GtkSequenceEdit))
#define GTK_SEQUENCE_EDIT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_SEQUENCE_EDIT, GtkSequenceEditClass))
#define GTK_IS_SEQUENCE_EDIT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_SEQUENCE_EDIT))
#define GTK_IS_SEQUENCE_EDIT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_SEQUENCE_EDIT))
#define GTK_SEQUENCE_EDIT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_SEQUENCE_EDIT, GtkSequenceEditClass))

typedef struct _GtkSequenceEdit		GtkSequenceEdit;
typedef struct _GtkSequenceEditClass	GtkSequenceEditClass;

struct _GtkSequenceEdit {
	GtkVBox		parent;

	GtkWidget *	widget;
	GtkWidget *	widget_hbox;

	GtkListStore *	list_store;
	GtkWidget *	tree_view;

	gboolean	minimum_one;
};
struct _GtkSequenceEditClass {
	GtkVBoxClass	parent;

	/* signals */
	void		(*add_request)(GtkSequenceEdit * self);
	void		(*changed)(GtkSequenceEdit * self);
	/* virtual */
	void		(*add)(GtkSequenceEdit * self);
	void		(*remove)(GtkSequenceEdit * self, GtkTreeIter * iter);
	void		(*move)(GtkSequenceEdit * self, GtkTreeIter * iter, GtkTreeIter * position, GtkTreeViewDropPosition drop_position);
	void		(*move_top)(GtkSequenceEdit * self, GtkTreeIter * iter);
	void		(*move_bottom)(GtkSequenceEdit * self, GtkTreeIter * iter);
	void		(*rename)(GtkSequenceEdit * self, GtkTreeIter * iter, const gchar * new_text);
	GtkWidget *	(*create_tree_view)(GtkSequenceEdit * self);
};

GtkWidget *
gtk_sequence_edit_new(GtkWidget * widget);

GtkWidget *
gtk_sequence_edit_new_from_store(GtkWidget * widget, GtkListStore * list_store);

GtkTreeIter
gtk_sequence_edit_add(GtkSequenceEdit * sequence_edit, const gchar * text, gboolean show_empty_value_text);

void
gtk_sequence_edit_clear(GtkSequenceEdit * sequence_edit);

G_END_DECLS

#endif //__LIBGEBR_GUI_GTK_SEQUENCE_EDIT_H
