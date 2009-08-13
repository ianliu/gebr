/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see
 *   <http://www.gnu.org/licenses/>.
 */

#ifndef __LIBGUI_GTK_DIRECTORY_CHOOSER_H
#define __LIBGUI_GTK_DIRECTORY_CHOOSER_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

GType gtk_directory_chooser_get_type (void) G_GNUC_CONST;

#define GTK_TYPE_DIRECTORY_CHOOSER		(gtk_directory_chooser_get_type())
#define GTK_DIRECTORY_CHOOSER(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_DIRECTORY_CHOOSER, GtkDirectoryChooser))
#define GTK_DIRECTORY_CHOOSER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_DIRECTORY_CHOOSER, GtkDirectoryChooserClass))
#define GTK_IS_DIRECTORY_CHOOSER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_DIRECTORY_CHOOSER))
#define GTK_IS_DIRECTORY_CHOOSER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_DIRECTORY_CHOOSER))
#define GTK_DIRECTORY_CHOOSER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_DIRECTORY_CHOOSER, GtkDirectoryChooserClass))

typedef struct _GtkDirectoryChooser		GtkDirectoryChooser;
typedef struct _GtkDirectoryChooserClass	GtkDirectoryChooserClass;

struct _GtkDirectoryChooser {
	GtkVBox			parent;
	GtkWidget *		sequence_edit;
};

struct _GtkDirectoryChooserClass {
	GtkVBoxClass		parent_class;
};

GtkWidget *
gtk_directory_chooser_new ();

gchar **
gtk_directory_chooser_get_paths (GtkDirectoryChooser * widget);

void
gtk_directory_chooser_set_paths (GtkDirectoryChooser * widget, gchar ** paths);

G_END_DECLS

#endif //__UI_PROJECT_LINE_H
