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

#ifndef __GEBR_GUI_FILE_ENTRY_H
#define __GEBR_GUI_FILE_ENTRY_H

#include <gtk/gtk.h>
#include <libgebr/geoxml/geoxml.h>
#include <libgebr/gebr-maestro-info.h>

G_BEGIN_DECLS

GType gebr_gui_file_entry_get_type(void);

#define GEBR_GUI_TYPE_FILE_ENTRY		(gebr_gui_file_entry_get_type())
#define GEBR_GUI_FILE_ENTRY(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_GUI_TYPE_FILE_ENTRY, GebrGuiFileEntry))
#define GEBR_GUI_FILE_ENTRY_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_GUI_TYPE_FILE_ENTRY, GebrGuiFileEntryClass))
#define GEBR_GUI_IS_FILE_ENTRY(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_GUI_TYPE_FILE_ENTRY))
#define GEBR_GUI_IS_FILE_ENTRY_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_GUI_TYPE_FILE_ENTRY))
#define GEBR_GUI_FILE_ENTRY_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_GUI_TYPE_FILE_ENTRY, GebrGuiFileEntryClass))

typedef struct _GebrGuiFileEntry GebrGuiFileEntry;
typedef struct _GebrGuiFileEntryClass GebrGuiFileEntryClass;

typedef void (*GebrGuiFileEntryCustomize) (GtkFileChooser *, gpointer);

struct _GebrGuiFileEntry {
	GtkHBox parent;

	GdkWindow *event_window;
	GtkWidget *entry;

	gboolean choose_directory;
	gboolean do_overwrite_confirmation;

	GebrGuiFileEntryCustomize customize_function;
	gpointer customize_user_data;

	GebrGeoXmlLine *line;
	gchar *prefix;
	gboolean need_gvfs;
};

struct _GebrGuiFileEntryClass {
	GtkHBoxClass parent;

	/* signals */
	void (*path_changed) (GebrGuiFileEntry * self);
};

GtkWidget *gebr_gui_file_entry_new(GebrGuiFileEntryCustomize customize_function, gpointer user_data);

void gebr_gui_file_entry_set_choose_directory(GebrGuiFileEntry * file_entry, gboolean choose_directory);

gboolean gebr_gui_file_entry_get_choose_directory(GebrGuiFileEntry * file_entry);

void


gebr_gui_file_entry_set_do_overwrite_confirmation(GebrGuiFileEntry * file_entry,
						      gboolean do_overwrite_confirmation);

gboolean gebr_gui_file_entry_get_do_overwrite_confirmation(GebrGuiFileEntry * file_entry);

void gebr_gui_file_entry_set_path(GebrGuiFileEntry * file_entry, const gchar * path);

const gchar *gebr_gui_file_entry_get_path(GebrGuiFileEntry * file_entry);

/**
 * gebr_gui_file_entry_set_activates_default:
 * @self:
 * @setting:
 *
 * Sets this entry to activate the default response for the toplevel window containing @self based on the value of
 * @setting.
 */
void gebr_gui_file_entry_set_activates_default (GebrGuiFileEntry * self, gboolean setting);

void gebr_gui_file_entry_set_warning(GebrGuiFileEntry * self, const gchar * tooltip);

void gebr_gui_file_entry_unset_warning(GebrGuiFileEntry * self, const gchar * tooltip);

void gebr_gui_file_entry_set_paths_from_line(GebrGuiFileEntry *self,
					     const gchar *prefix,
					     GebrGeoXmlLine *line);

void gebr_gui_file_entry_set_need_gvfs(GebrGuiFileEntry *self,
				       gboolean need_gvfs);

G_END_DECLS

#endif /* __GEBR_GUI_FILE_ENTRY_H */
