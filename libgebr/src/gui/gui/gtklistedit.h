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

#ifndef __LIBGEBR_GUI_GTK_LIST_EDIT_H
#define __LIBGEBR_GUI_GTK_LIST_EDIT_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

GType
gtk_list_edit_get_type(void);

#define GTK_TYPE_LIST_EDIT		(gtk_list_edit_get_type())
#define GTK_LIST_EDIT(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_LIST_EDIT, GtkListEdit))
#define GTK_LIST_EDIT_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_LIST_EDIT, GtkListEditClass))
#define GTK_IS_LIST_EDIT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_LIST_EDIT))
#define GTK_IS_LIST_EDIT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_LIST_EDIT))
#define GTK_LIST_EDIT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_LIST_EDIT, GtkListEditClass))

typedef struct _GtkListEdit		GtkListEdit;
typedef struct _GtkListEditClass	GtkListEditClass;

struct _GtkListEdit {
	GtkHBox		parent;

	GtkWidget *	add_button;
	GtkWidget *	remove_button;
	GtkWidget *	move_up_button;
	GtkWidget *	move_down_button;

	GtkListStore *	list_store;
	GtkWidget *	tree_view;
};
struct _GtkListEditClass {
	GtkHBoxClass	parent;

	/* virtual */
	/* the first store name must be title of the item */
	void		(*create_store)(GtkListEdit * self);
	void		(*add)(GtkListEdit * self, GtkTreeIter * iter);
	void		(*remove)(GtkListEdit * self, GtkTreeIter * iter);
	void		(*move_up)(GtkListEdit * self, GtkTreeIter * iter);
	void		(*move_down)(GtkListEdit * self, GtkTreeIter * iter);
};

GtkWidget *
gtk_list_edit_new();

GtkListStore *
gtk_list_edit_get_store(GtkListEdit * list_edit);

G_END_DECLS

#endif //__LIBGEBR_GUI_GTK_LIST_EDIT_H
