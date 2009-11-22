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

#ifndef __GEBR_GUI_GTK_FILE_ENTRY_H
#define __GEBR_GUI_GTK_FILE_ENTRY_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

GType
gebr_gui_gtk_file_entry_get_type(void);

#define GEBR_GUI_GTK_TYPE_FILE_ENTRY		(gebr_gui_gtk_file_entry_get_type())
#define GEBR_GUI_GTK_FILE_ENTRY(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_GUI_GTK_TYPE_FILE_ENTRY, GebrGuiGtkFileEntry))
#define GEBR_GUI_GTK_FILE_ENTRY_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_GUI_GTK_TYPE_FILE_ENTRY, GebrGuiGtkFileEntryClass))
#define GEBR_GUI_GTK_IS_FILE_ENTRY(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_GUI_GTK_TYPE_FILE_ENTRY))
#define GEBR_GUI_GTK_IS_FILE_ENTRY_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_GUI_GTK_TYPE_FILE_ENTRY))
#define GEBR_GUI_GTK_FILE_ENTRY_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_GUI_GTK_TYPE_FILE_ENTRY, GebrGuiGtkFileEntryClass))

typedef struct _GebrGuiGtkFileEntry		GebrGuiGtkFileEntry;
typedef struct _GebrGuiGtkFileEntryClass	GebrGuiGtkFileEntryClass;

typedef void (*GebrGuiGtkFileEntryCustomize)(GtkFileChooser *, gpointer);

struct _GebrGuiGtkFileEntry {
	GtkHBox			parent;

	GdkWindow *		event_window;
	GtkWidget *		entry;

	gboolean		choose_directory;
	gboolean		do_overwrite_confirmation;

	GebrGuiGtkFileEntryCustomize	customize_function;
	gpointer		customize_user_data;
};
struct _GebrGuiGtkFileEntryClass {
	GtkHBoxClass		parent;

	/* signals */
	void			(*path_changed)(GebrGuiGtkFileEntry * self);
};

GtkWidget *
gebr_gui_gtk_file_entry_new(GebrGuiGtkFileEntryCustomize customize_function, gpointer user_data);

void
gebr_gui_gtk_file_entry_set_choose_directory(GebrGuiGtkFileEntry * file_entry, gboolean choose_directory);

gboolean
gebr_gui_gtk_file_entry_get_choose_directory(GebrGuiGtkFileEntry * file_entry);

void
gebr_gui_gtk_file_entry_set_do_overwrite_confirmation(GebrGuiGtkFileEntry * file_entry, gboolean do_overwrite_confirmation);

gboolean
gebr_gui_gtk_file_entry_get_do_overwrite_confirmation(GebrGuiGtkFileEntry * file_entry);

void
gebr_gui_gtk_file_entry_set_path(GebrGuiGtkFileEntry * file_entry, const gchar * path);

const gchar *
gebr_gui_gtk_file_entry_get_path(GebrGuiGtkFileEntry * file_entry);

G_END_DECLS

#endif //__GEBR_GUI_GTK_FILE_ENTRY_H
