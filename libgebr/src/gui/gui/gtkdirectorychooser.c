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

#include "../../intl.h"

#include "gtkdirectorychooser.h"
#include "utils.h"

#include "gtkfileentry.h"
#include "gtksequenceedit.h"

#include <glib.h>
#include <glib/gprintf.h>

G_DEFINE_TYPE(GtkDirectoryChooser, gtk_directory_chooser, GTK_TYPE_VBOX);

/*
 * Prototypes
 */

static void
__gtk_sequence_edit_add_request (GtkSequenceEdit *sequence_edit,
		GtkDirectoryChooser *dir_chooser);

/*
 * gobject
 */

static void
gtk_directory_chooser_class_init(GtkDirectoryChooserClass * class)
{
}

static void
gtk_directory_chooser_init(GtkDirectoryChooser * directory_chooser)
{
	GtkWidget *		file_entry;
	GtkWidget *		sequence_edit;

	file_entry = gtk_file_entry_new (NULL);
	gtk_file_entry_set_choose_directory(GTK_FILE_ENTRY(file_entry), TRUE);
	sequence_edit = gtk_sequence_edit_new (file_entry);

	directory_chooser->sequence_edit = sequence_edit;

	g_signal_connect(sequence_edit, "add-request",
			G_CALLBACK(__gtk_sequence_edit_add_request), directory_chooser);

	gtk_box_pack_start(GTK_BOX(directory_chooser), sequence_edit, TRUE, TRUE, 0);
	gtk_widget_show_all(sequence_edit);
}


/*
 * Public methods
 */

GtkWidget *
gtk_directory_chooser_new()
{
	return g_object_new(GTK_TYPE_DIRECTORY_CHOOSER, NULL);
}

/**
 * gtk_directory_chooser_get_paths:
 *
 * Returns a NULL-terminated array of C strings corresponding
 * to the values contained in this widget. If there is no path
 * for this widget, returns NULL.
 */
gchar **
gtk_directory_chooser_get_paths(GtkDirectoryChooser * widget)
{
	GtkTreeModel *		store;
	GtkTreeIter		iter;
	gchar **		paths;
	gint			length;
	gboolean		valid;

	g_object_get(widget->sequence_edit, "list-store", &store, NULL);

	length = gtk_tree_model_iter_n_children (store, NULL);
	paths = (gchar **) g_malloc (sizeof(gchar*) * (length + 1));

	gint i = 0;
	valid = gtk_tree_model_get_iter_first (store, &iter);
	while (valid) {
		gchar * path;
		gtk_tree_model_get (store, &iter, 0, &path, -1);
		paths[i++] = path;
		valid = gtk_tree_model_iter_next (store, &iter);
	}
	paths[length] = NULL;
	return paths;
}

void
gtk_directory_chooser_set_paths (GtkDirectoryChooser * widget, gchar ** paths)
{
	gint i;

	if (!paths)
		return;

	i = 0;
	while (paths[i] != NULL) {
		gtk_sequence_edit_add(GTK_SEQUENCE_EDIT(widget->sequence_edit),
				paths[i], FALSE);
		i++;
	}
}

/*
 * Internal functions
 */

static void
__gtk_sequence_edit_add_request (GtkSequenceEdit *sequence_edit,
		GtkDirectoryChooser *dir_chooser)
{
	GtkWidget *		file_entry;
	gchar *			path;

	g_object_get(sequence_edit, "value-widget", &file_entry, NULL);
	path = gtk_file_entry_get_path(GTK_FILE_ENTRY(file_entry));
	gtk_sequence_edit_add(sequence_edit, path, TRUE);
}

