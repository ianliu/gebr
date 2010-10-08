/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
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

#include "../../intl.h"

#include "gebr-gui-file-entry.h"
#include "gebr-gui-utils.h"

/*
 * Prototypes
 */

static void __gebr_gui_gtk_file_entry_entry_changed(GtkEntry * entry, GebrGuiGtkFileEntry * file_entry);

static void
__gebr_gui_gtk_file_entry_browse_button_clicked(GtkButton * button, GtkEntryIconPosition icon_pos, GdkEvent * event,
						GebrGuiGtkFileEntry * file_entry);

static gboolean on_mnemonic_activate(GebrGuiGtkFileEntry * file_entry);

/*
 * gobject stuff
 */

enum {
	CUSTOMIZE_FUNCTION = 1,
	LAST_PROPERTY
};

enum {
	PATH_CHANGED = 0,
	LAST_SIGNAL
};
static guint object_signals[LAST_SIGNAL];

static void
gebr_gui_gtk_file_entry_set_property(GebrGuiGtkFileEntry * file_entry, guint property_id, const GValue * value,
				     GParamSpec * param_spec)
{
	switch (property_id) {
	case CUSTOMIZE_FUNCTION:
		file_entry->customize_function = g_value_get_pointer(value);
		break;
	default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(file_entry, property_id, param_spec);
		break;
	}
}

static void
gebr_gui_gtk_file_entry_get_property(GebrGuiGtkFileEntry * file_entry, guint property_id, GValue * value,
				     GParamSpec * param_spec)
{
	switch (property_id) {
	case CUSTOMIZE_FUNCTION:
		g_value_set_pointer(value, file_entry->customize_function);
		break;
	default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(file_entry, property_id, param_spec);
		break;
	}
}

static void gebr_gui_gtk_file_entry_class_init(GebrGuiGtkFileEntryClass * klass)
{
	GObjectClass *gobject_class;
	GParamSpec *param_spec;

	gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = (typeof(gobject_class->set_property)) gebr_gui_gtk_file_entry_set_property;
	gobject_class->get_property = (typeof(gobject_class->get_property)) gebr_gui_gtk_file_entry_get_property;

	param_spec = g_param_spec_pointer("customize-function",
					  "Customize function", "Customize GtkFileChooser of dialog",
					  (GParamFlags)(G_PARAM_READWRITE));
	g_object_class_install_property(gobject_class, CUSTOMIZE_FUNCTION, param_spec);

	/* signals */
	object_signals[PATH_CHANGED] = g_signal_new("path-changed", GEBR_GUI_GTK_TYPE_FILE_ENTRY, (GSignalFlags) (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION), G_STRUCT_OFFSET(GebrGuiGtkFileEntryClass, path_changed), NULL, NULL,	/* acumulators */
						    g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void gebr_gui_gtk_file_entry_init(GebrGuiGtkFileEntry * file_entry)
{
	GtkWidget *entry;

	/* entry */
	entry = gtk_entry_new();
	file_entry->entry = entry;
	gtk_widget_show(entry);
	gtk_box_pack_start(GTK_BOX(file_entry), entry, TRUE, TRUE, 0);
	g_signal_connect(GTK_ENTRY(entry), "changed", G_CALLBACK(__gebr_gui_gtk_file_entry_entry_changed), file_entry);
	g_signal_connect(entry, "icon-release",
			 G_CALLBACK(__gebr_gui_gtk_file_entry_browse_button_clicked), file_entry);
	g_signal_connect(file_entry, "mnemonic-activate", G_CALLBACK(on_mnemonic_activate), NULL);
	gtk_entry_set_icon_from_stock(GTK_ENTRY(entry), GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_OPEN);

	file_entry->choose_directory = FALSE;
	file_entry->do_overwrite_confirmation = TRUE;
}

G_DEFINE_TYPE(GebrGuiGtkFileEntry, gebr_gui_gtk_file_entry, GTK_TYPE_HBOX);

/*
 * Internal functions
 */

static void __gebr_gui_gtk_file_entry_entry_changed(GtkEntry * entry, GebrGuiGtkFileEntry * file_entry)
{
	g_signal_emit(file_entry, object_signals[PATH_CHANGED], 0);
}

static void
__gebr_gui_gtk_file_entry_browse_button_clicked(GtkButton * button, GtkEntryIconPosition icon_pos, GdkEvent * event,
						GebrGuiGtkFileEntry * file_entry)
{
	GtkWidget *chooser_dialog;

	chooser_dialog = gtk_file_chooser_dialog_new(file_entry->choose_directory == FALSE
						     ? _("Choose file") : _("Choose directory"), NULL,
						     file_entry->choose_directory == FALSE
						     ? GTK_FILE_CHOOSER_ACTION_SAVE :
						     GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_STOCK_CANCEL,
						     GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(chooser_dialog),
						       file_entry->do_overwrite_confirmation);

	/* call customize funtion for user changes */
	if (file_entry->customize_function != NULL)
		file_entry->customize_function(GTK_FILE_CHOOSER(chooser_dialog), file_entry->customize_user_data);
	switch (gtk_dialog_run(GTK_DIALOG(chooser_dialog))) {
	case GTK_RESPONSE_OK:
		gtk_entry_set_text(GTK_ENTRY(file_entry->entry),
				   gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser_dialog)));
		g_signal_emit(file_entry, object_signals[PATH_CHANGED], 0);
		break;
	default:
		break;
	}

	gtk_widget_destroy(GTK_WIDGET(chooser_dialog));
}

static gboolean on_mnemonic_activate(GebrGuiGtkFileEntry * file_entry)
{
	gtk_widget_mnemonic_activate(file_entry->entry, TRUE);
	return TRUE;
}

/*
 * Library functions
 */

GtkWidget *gebr_gui_gtk_file_entry_new(GebrGuiGtkFileEntryCustomize customize_function, gpointer user_data)
{
	GebrGuiGtkFileEntry *file_entry;

	file_entry = g_object_new(GEBR_GUI_GTK_TYPE_FILE_ENTRY, "customize-function", customize_function, NULL);
	file_entry->customize_user_data = user_data;

	return GTK_WIDGET(file_entry);
}

void gebr_gui_gtk_file_entry_set_choose_directory(GebrGuiGtkFileEntry * file_entry, gboolean choose_directory)
{
	file_entry->choose_directory = choose_directory;
}

gboolean gebr_gui_gtk_file_entry_get_choose_directory(GebrGuiGtkFileEntry * file_entry)
{
	return file_entry->choose_directory;
}

void
gebr_gui_gtk_file_entry_set_do_overwrite_confirmation(GebrGuiGtkFileEntry * file_entry,
						      gboolean do_overwrite_confirmation)
{
	file_entry->do_overwrite_confirmation = do_overwrite_confirmation;
}

gboolean gebr_gui_gtk_file_entry_get_do_overwrite_confirmation(GebrGuiGtkFileEntry * file_entry)
{
	return file_entry->do_overwrite_confirmation;
}

void gebr_gui_gtk_file_entry_set_path(GebrGuiGtkFileEntry * file_entry, const gchar * path)
{
	gtk_entry_set_text(GTK_ENTRY(file_entry->entry), path);
}

const gchar *gebr_gui_gtk_file_entry_get_path(GebrGuiGtkFileEntry * file_entry)
{
	return gtk_entry_get_text(GTK_ENTRY(file_entry->entry));
}

void gebr_gui_file_entry_set_activates_default (GebrGuiGtkFileEntry * self, gboolean setting)
{
	g_return_if_fail (GEBR_GUI_GTK_IS_FILE_ENTRY (self));

	gtk_entry_set_activates_default (GTK_ENTRY (self->entry), setting);
}
