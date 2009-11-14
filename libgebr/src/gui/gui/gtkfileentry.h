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

#ifndef __GUI_GTK_FILE_ENTRY_H
#define __GUI_GTK_FILE_ENTRY_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

GType
gtk_file_entry_get_type(void);

#define GTK_TYPE_FILE_ENTRY		(gtk_file_entry_get_type())
#define GTK_FILE_ENTRY(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_FILE_ENTRY, GtkFileEntry))
#define GTK_FILE_ENTRY_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_FILE_ENTRY, GtkFileEntryClass))
#define GTK_IS_FILE_ENTRY(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_FILE_ENTRY))
#define GTK_IS_FILE_ENTRY_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_FILE_ENTRY))
#define GTK_FILE_ENTRY_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_FILE_ENTRY, GtkFileEntryClass))

typedef struct _GtkFileEntry		GtkFileEntry;
typedef struct _GtkFileEntryClass	GtkFileEntryClass;

typedef void (*GtkFileEntryCustomize)(GtkFileChooser *, gpointer);

struct _GtkFileEntry {
	GtkHBox			parent;

	GdkWindow *		event_window;
	GtkWidget *		entry;

	gboolean		choose_directory;
	gboolean		do_overwrite_confirmation;

	GtkFileEntryCustomize	customize_function;
	gpointer		customize_user_data;
};
struct _GtkFileEntryClass {
	GtkHBoxClass		parent;

	/* signals */
	void			(*path_changed)(GtkFileEntry * self);
};

GtkWidget *
gtk_file_entry_new(GtkFileEntryCustomize customize_function, gpointer user_data);

void
gtk_file_entry_set_choose_directory(GtkFileEntry * file_entry, gboolean choose_directory);

gboolean
gtk_file_entry_get_choose_directory(GtkFileEntry * file_entry);

void
gtk_file_entry_set_do_overwrite_confirmation(GtkFileEntry * file_entry, gboolean do_overwrite_confirmation);

gboolean
gtk_file_entry_get_do_overwrite_confirmation(GtkFileEntry * file_entry);

void
gtk_file_entry_set_path(GtkFileEntry * file_entry, const gchar * path);

gchar *
gtk_file_entry_get_path(GtkFileEntry * file_entry);

G_END_DECLS

#endif //__GUI_GTK_FILE_ENTRY_H
