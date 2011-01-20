/*   GeBR - An environment for seismic processing.
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

/**
 * \file ui_preferences.c Preferences dialog and config related stuff. Assembly preferences dialog and changes
 * configuration file according to user's change.
 *
 * \ingroup gebr
 */

#include <stdlib.h>
#include <string.h>

#include <glib/gi18n.h>
#include <libgebr/gui/gebr-gui-utils.h>

#include "ui_preferences.h"
#include "gebr.h"

/*
 * Prototypes
 */

static void preferences_actions(GtkDialog * dialog, gint arg1, struct ui_preferences *ui_preferences);

/**
 * \internal
 * Disable HTML editor entry on radio false state
 */
static void on_custom_radio_toggled(GtkToggleButton *togglebutton, GtkWidget *htmleditor_entry)
{
	gtk_widget_set_sensitive(htmleditor_entry, gtk_toggle_button_get_active(togglebutton));
}

/**
 * Assembly preference window.
 *
 * \return The structure containing relevant data. It will be automatically freed when the dialog closes.
 */
struct ui_preferences *preferences_setup_ui(gboolean first_run)
{
	struct ui_preferences *ui_preferences;

	GtkWidget *table;
	guint row;
	GtkWidget *label;
	GtkWidget *eventbox;
	GtkWidget *fake_radio_button;
	GtkWidget *list_widget_hbox;

	ui_preferences = g_new(struct ui_preferences, 1);
	ui_preferences->first_run = first_run;
	ui_preferences->dialog = gtk_dialog_new_with_buttons(_("Preferences"),
							     GTK_WINDOW(gebr.window),
							     (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
							     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
							     GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(ui_preferences->dialog), GTK_RESPONSE_OK);
	gtk_window_set_resizable(GTK_WINDOW(ui_preferences->dialog), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(ui_preferences->dialog), 5);

	/* Take the apropriate action when a button is pressed */
	g_signal_connect(ui_preferences->dialog, "response", G_CALLBACK(preferences_actions), ui_preferences);

	row = 0;
	table = gtk_table_new(6, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(ui_preferences->dialog)->vbox), table, TRUE, TRUE, 0);

	/*
	 * User name
	 */
	label = gtk_label_new(_("User name"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, row, row + 1, (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 3, 3);
	ui_preferences->username = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(ui_preferences->username), TRUE);
	gebr_gui_gtk_widget_set_tooltip(ui_preferences->username, _("You should know your name"));
	gtk_table_attach(GTK_TABLE(table), ui_preferences->username, 1, 2, row, row + 1, GTK_EXPAND |
			 (GtkAttachOptions)GTK_FILL, (GtkAttachOptions)GTK_FILL, 3, 3), ++row;

	/* read config */
	if (strlen(gebr.config.username->str))
		gtk_entry_set_text(GTK_ENTRY(ui_preferences->username), gebr.config.username->str);
	else
		gtk_entry_set_text(GTK_ENTRY(ui_preferences->username), g_get_real_name());

	/*
	 * User email
	 */
	label = gtk_label_new(_("Email"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, row, row + 1, (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 3, 3);
	ui_preferences->email = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(ui_preferences->email), TRUE);
	gebr_gui_gtk_widget_set_tooltip(ui_preferences->email, _("Your email address"));
	gtk_table_attach(GTK_TABLE(table), ui_preferences->email, 1, 2, row, row + 1, (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 3, 3), ++row;

	/* read config */
	if (strlen(gebr.config.email->str))
		gtk_entry_set_text(GTK_ENTRY(ui_preferences->email), gebr.config.email->str);
	else
		gtk_entry_set_text(GTK_ENTRY(ui_preferences->email), g_get_user_name());

	/*
	 * User's menus directory
	 */
	label = gtk_label_new(_("User's menus directory"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, row, row + 1, (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 3, 3);
	eventbox = gtk_event_box_new();
	ui_preferences->usermenus = gtk_file_chooser_button_new(_("User's menus directory"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
	gtk_container_add(GTK_CONTAINER(eventbox), ui_preferences->usermenus);
	gebr_gui_gtk_widget_set_tooltip(eventbox, _("Path to look for private user's menus"));
	gtk_table_attach(GTK_TABLE(table), eventbox, 1, 2, row, row + 1, (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 3, 3), ++row;

	/* read config */
	if (gebr.config.usermenus->len > 0)
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(ui_preferences->usermenus),
						    gebr.config.usermenus->str);

	/*
	 * Editor
	 */
	label = gtk_label_new(_("HTML editor"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, row, row + 1, (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 3, 3);

	list_widget_hbox = gtk_hbox_new(FALSE, 0);
	gtk_table_attach(GTK_TABLE(table), list_widget_hbox, 1, 2, row, row + 1,
			 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
				 (GtkAttachOptions) (0), 0, 0), ++row;

	fake_radio_button = gtk_radio_button_new(NULL);
	ui_preferences->built_in_radio_button =  gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(fake_radio_button), _("Built-in"));
	ui_preferences->user_radio_button =  gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(fake_radio_button), _("Custom"));
	ui_preferences->editor = gtk_entry_new();
	gtk_widget_set_sensitive(ui_preferences->editor, FALSE);
	gtk_entry_set_activates_default(GTK_ENTRY(ui_preferences->editor), TRUE);
	gebr_gui_gtk_widget_set_tooltip(ui_preferences->editor, _("An HTML capable editor to edit helps and reports"));

	g_signal_connect(ui_preferences->user_radio_button, "toggled", G_CALLBACK(on_custom_radio_toggled), ui_preferences->editor);

	gtk_box_pack_start(GTK_BOX(list_widget_hbox), ui_preferences->built_in_radio_button, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(list_widget_hbox), ui_preferences->user_radio_button, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(list_widget_hbox), ui_preferences->editor, FALSE, FALSE, 0);

	gtk_entry_set_text(GTK_ENTRY(ui_preferences->editor), gebr.config.editor->str);

	/* read config */
	if (!gebr.config.native_editor) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_preferences->user_radio_button), TRUE);
	} else {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_preferences->built_in_radio_button), TRUE);
	}

	/* Load log */
	ui_preferences->log_load = label = gtk_check_button_new_with_label(_("Load past-execution log"));
	gtk_table_attach(GTK_TABLE(table), label, 0, 2, row, row + 1, (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 3, 3), ++row;

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(label), gebr.config.log_load);

	/* finally... */
	gtk_widget_show_all(ui_preferences->dialog);

	return ui_preferences;
}

/**
 * \internal
 * Take the appropriate action when the parameter dialog emmits a response signal.
 */
static void preferences_actions(GtkDialog * dialog, gint arg1, struct ui_preferences *ui_preferences)
{
	switch (arg1) {
	case GTK_RESPONSE_OK:{
		gchar *tmp;

		tmp = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(ui_preferences->usermenus));

		g_string_assign(gebr.config.username, gtk_entry_get_text(GTK_ENTRY(ui_preferences->username)));
		g_string_assign(gebr.config.email, gtk_entry_get_text(GTK_ENTRY(ui_preferences->email)));
		g_string_assign(gebr.config.editor, gtk_entry_get_text(GTK_ENTRY(ui_preferences->editor)));
		gebr.config.native_editor = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_preferences->user_radio_button));

		gebr.config.log_load = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui_preferences->log_load));

		if (g_strcmp0(gebr.config.usermenus->str, tmp) != 0){
			g_string_assign(gebr.config.usermenus, tmp);
			gebr_config_apply();
		}
		gebr_config_save(TRUE);

		g_free(tmp);

		break;
		}
	case GTK_RESPONSE_CANCEL:
	default:
		if (ui_preferences->first_run == TRUE)
			gebr_quit(FALSE);
		break;
	}

	gtk_widget_destroy(ui_preferences->dialog);
	g_free(ui_preferences);
}
