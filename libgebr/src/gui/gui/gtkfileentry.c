/*   libgebr - GÍBR Library
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gtkfileentry.h"
#include "support.h"

/*
 * Prototypes
 */

static void
__gtk_file_entry_entry_changed(GtkEntry * entry, GtkFileEntry * file_entry);

static void
__gtk_file_entry_browse_button_clicked(GtkButton * button, GtkFileEntry * file_entry);

/*
 * gobject stuff
 */

enum {
	PATH_CHANGED = 0,
	LAST_SIGNAL
};
static guint object_signals[LAST_SIGNAL];

static void
gtk_file_entry_class_init(GtkFileEntryClass * class)
{
	/* signals */
	object_signals[PATH_CHANGED] = g_signal_new("path-changed",
		GTK_TYPE_FILE_ENTRY,
		(GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
		G_STRUCT_OFFSET(GtkFileEntryClass, path_changed),
		NULL, NULL, /* acumulators */
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
}

static void
gtk_file_entry_init(GtkFileEntry * file_entry)
{
	GtkWidget *	entry;
	GtkWidget *	browse_button;

	/* entry */
	entry = gtk_entry_new();
	file_entry->entry = entry;
	gtk_widget_show(entry);
	gtk_box_pack_start(GTK_BOX(file_entry), entry, TRUE, TRUE, 0);
	g_signal_connect(GTK_ENTRY(entry), "changed",
		G_CALLBACK(__gtk_file_entry_entry_changed), file_entry);

	/* browse button */
	browse_button = gtk_button_new_from_stock(GTK_STOCK_OPEN);
	gtk_widget_show(browse_button);
	gtk_box_pack_start(GTK_BOX(file_entry), browse_button, FALSE, TRUE, 0);
	g_signal_connect(GTK_OBJECT(browse_button), "clicked",
		G_CALLBACK(__gtk_file_entry_browse_button_clicked), file_entry);

	file_entry->choose_directory = FALSE;
}

G_DEFINE_TYPE(GtkFileEntry, gtk_file_entry, GTK_TYPE_HBOX);

/*
 * Internal functions
 */

static void
__gtk_file_entry_entry_changed(GtkEntry * entry, GtkFileEntry * file_entry)
{
	g_signal_emit(file_entry, object_signals[PATH_CHANGED], 0);
}

static void
__gtk_file_entry_browse_button_clicked(GtkButton * button, GtkFileEntry * file_entry)
{
	GtkWidget *	chooser_dialog;

	chooser_dialog = gtk_file_chooser_dialog_new(
		file_entry->choose_directory == FALSE ? _("Choose file") : _("Choose directory"),
		NULL,
		file_entry->choose_directory == FALSE ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL);

	switch (gtk_dialog_run(GTK_DIALOG(chooser_dialog))) {
	case GTK_RESPONSE_OK:
		gtk_entry_set_text(GTK_ENTRY(file_entry->entry), gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser_dialog)));
		g_signal_emit(file_entry, object_signals[PATH_CHANGED], 0);
		break;
	default:
		break;
	}

	gtk_widget_destroy(GTK_WIDGET(chooser_dialog));
}

/*
 * Library functions
 */

GtkWidget *
gtk_file_entry_new()
{
	return g_object_new(GTK_TYPE_FILE_ENTRY, NULL);
}

void
gtk_file_entry_set_choose_directory(GtkFileEntry * file_entry, gboolean choose_directory)
{
	file_entry->choose_directory = choose_directory;
}

gboolean
gtk_file_entry_get_choose_directory(GtkFileEntry * file_entry)
{
	return file_entry->choose_directory;
}

void
gtk_file_entry_set_path(GtkFileEntry * file_entry, const gchar * path)
{
	gtk_entry_set_text(GTK_ENTRY(file_entry->entry), path);
}

gchar *
gtk_file_entry_get_path(GtkFileEntry * file_entry)
{
	return (gchar*)gtk_entry_get_text(GTK_ENTRY(file_entry->entry));
}
