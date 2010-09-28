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

#ifndef __GEBR_GUI_GTK_DIRECTORY_CHOOSER_H
#define __GEBR_GUI_GTK_DIRECTORY_CHOOSER_H

/**
 * @file gebr-gui-directory-chooser.h Widget to choose and list directories
 * @ingroup libgebr-gui
 */

#include <gtk/gtk.h>

G_BEGIN_DECLS

GType gebr_gui_gtk_directory_chooser_get_type(void) G_GNUC_CONST;

#define GEBR_GUI_GTK_TYPE_DIRECTORY_CHOOSER		(gebr_gui_gtk_directory_chooser_get_type())
#define GEBR_GUI_GTK_DIRECTORY_CHOOSER(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_GUI_GTK_TYPE_DIRECTORY_CHOOSER, GebrGuiGtkDirectoryChooser))
#define GEBR_GUI_GTK_DIRECTORY_CHOOSER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_GUI_GTK_TYPE_DIRECTORY_CHOOSER, GebrGuiGtkDirectoryChooserClass))
#define GEBR_GUI_GTK_IS_DIRECTORY_CHOOSER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_GUI_GTK_TYPE_DIRECTORY_CHOOSER))
#define GEBR_GUI_GTK_IS_DIRECTORY_CHOOSER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_GUI_GTK_TYPE_DIRECTORY_CHOOSER))
#define GEBR_GUI_GTK_DIRECTORY_CHOOSER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_GUI_GTK_TYPE_DIRECTORY_CHOOSER, GebrGuiGtkDirectoryChooserClass))

typedef struct _GebrGuiGtkDirectoryChooser GebrGuiGtkDirectoryChooser;
typedef struct _GebrGuiGtkDirectoryChooserClass GebrGuiGtkDirectoryChooserClass;

struct _GebrGuiGtkDirectoryChooser {
	GtkVBox parent;
	GtkWidget *sequence_edit;
};

struct _GebrGuiGtkDirectoryChooserClass {
	GtkVBoxClass parent_class;
};

/**
 * Creates a new #GtkDirectoryChooser widget.
 */
GtkWidget *gebr_gui_gtk_directory_chooser_new();

/**
 * Returns a NULL-terminated array of strings, containing the directories of this widget.
 * Memory is allocated for this operation, free with \ref g_strfreev.
 */
gchar **gebr_gui_gtk_directory_chooser_get_paths(GebrGuiGtkDirectoryChooser * widget);

/**
 * Sets the folders for this widget.
 * \param paths A NULL-terminates array of strings, containing the paths that will be added to the widget.
 */
void gebr_gui_gtk_directory_chooser_set_paths(GebrGuiGtkDirectoryChooser * widget, gchar ** paths);

G_END_DECLS
#endif				//__UI_PROJECT_LINE_H
