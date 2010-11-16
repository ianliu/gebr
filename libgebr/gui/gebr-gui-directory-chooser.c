/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or * (at your option) any later version.
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

#include <config.h>
#include <glib/gi18n-lib.h>

#include "gebr-gui-directory-chooser.h"
#include "gebr-gui-utils.h"

#include "gebr-gui-file-entry.h"
#include "gebr-gui-sequence-edit.h"

#include <glib.h>
#include <glib/gprintf.h>

G_DEFINE_TYPE(GebrGuiDirectoryChooser, gebr_gui_directory_chooser, GTK_TYPE_VBOX);

/*
 * Prototypes
 */

static void __gebr_gui_sequence_edit_add_request(GebrGuiSequenceEdit * sequence_edit, GebrGuiDirectoryChooser * dir_chooser);

/*
 * gobject
 */

static void gebr_gui_directory_chooser_class_init(GebrGuiDirectoryChooserClass * class)
{
}

static void gebr_gui_directory_chooser_init(GebrGuiDirectoryChooser * directory_chooser)
{
	GtkWidget *file_entry;
	GtkWidget *sequence_edit;

	file_entry = gebr_gui_gtk_file_entry_new(NULL, NULL);
	gebr_gui_gtk_file_entry_set_choose_directory(GEBR_GUI_GTK_FILE_ENTRY(file_entry), TRUE);
	sequence_edit = gebr_gui_sequence_edit_new(file_entry);

	directory_chooser->sequence_edit = sequence_edit;

	g_signal_connect(sequence_edit, "add-request", G_CALLBACK(__gebr_gui_sequence_edit_add_request), directory_chooser);

	gtk_box_pack_start(GTK_BOX(directory_chooser), sequence_edit, TRUE, TRUE, 0);
	gtk_widget_show_all(sequence_edit);
}

/*
 * Public methods
 */

GtkWidget *gebr_gui_directory_chooser_new()
{
	return g_object_new(GEBR_GUI_GTK_TYPE_DIRECTORY_CHOOSER, NULL);
}

/**
 * gebr_gui_directory_chooser_get_paths:
 *
 * Returns a NULL-terminated array of C strings corresponding
 * to the values contained in this widget. If there is no path
 * for this widget, returns NULL.
 */
gchar **gebr_gui_directory_chooser_get_paths(GebrGuiDirectoryChooser * widget)
{
	GtkTreeModel *store;
	GtkTreeIter iter;
	gchar **paths;
	gint length;
	gboolean valid;

	g_object_get(widget->sequence_edit, "list-store", &store, NULL);

	length = gtk_tree_model_iter_n_children(store, NULL);
	paths = (gchar **) g_malloc(sizeof(gchar *) * (length + 1));

	gint i = 0;
	valid = gtk_tree_model_get_iter_first(store, &iter);
	while (valid) {
		gchar *path;
		gtk_tree_model_get(store, &iter, 0, &path, -1);
		paths[i++] = path;
		valid = gtk_tree_model_iter_next(store, &iter);
	}
	paths[length] = NULL;
	return paths;
}

void gebr_gui_directory_chooser_set_paths(GebrGuiDirectoryChooser * widget, gchar ** paths)
{
	gint i;

	if (!paths)
		return;

	i = 0;
	while (paths[i] != NULL) {
		gebr_gui_sequence_edit_add(GEBR_GUI_SEQUENCE_EDIT(widget->sequence_edit), paths[i], FALSE);
		i++;
	}
}

/*
 * Internal functions
 */

static void __gebr_gui_sequence_edit_add_request(GebrGuiSequenceEdit * sequence_edit, GebrGuiDirectoryChooser * dir_chooser)
{
	GtkWidget *file_entry;
	const gchar *path;

	g_object_get(sequence_edit, "value-widget", &file_entry, NULL);
	path = gebr_gui_gtk_file_entry_get_path(GEBR_GUI_GTK_FILE_ENTRY(file_entry));
	gebr_gui_sequence_edit_add(sequence_edit, path, TRUE);
}

