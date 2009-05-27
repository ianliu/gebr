/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
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

/*
 * File: ui_preferences.c
 * Preferences dialog and config related stuff
 *
 * Assembly preferences dialog and changes configuration file
 * according to user's change
 */

#include <string.h>

#include <libgebrintl.h>
#include <gui/utils.h>

#include "ui_preferences.h"
#include "gebr.h"

/* Pre-defined browser options */
#define NBROWSER 5
static const gchar * browser [] = {
	"epiphany", "firefox", "galeon", "konqueror", "mozilla"
};

/*
 * Prototypes
 */

static void
preferences_actions(GtkDialog * dialog, gint arg1, struct ui_preferences * ui_preferences);

static gboolean
preferences_on_delete_event(GtkDialog * dialog, GdkEventAny * event, struct ui_preferences * ui_preferences);

/*
 * Section: Public
 * Public functions.
 */

/*
 * Function: assembly_preference_win
 * Assembly preference window.
 *
 * Return:
 * The structure containing relevant data. It will be automatically freed when the
 * dialog closes.
 */
struct ui_preferences *
preferences_setup_ui(gboolean first_run)
{
	struct ui_preferences *		ui_preferences;

	GtkWidget *			table;
	GtkWidget *			label;
	GtkWidget *                     eventbox;

	ui_preferences = g_malloc(sizeof(struct ui_preferences));
	ui_preferences->first_run = first_run;
	ui_preferences->dialog = gtk_dialog_new_with_buttons(_("Preferences"),
		GTK_WINDOW(gebr.window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		NULL);
	gtk_widget_set_size_request(ui_preferences->dialog, 400, 280);
	g_signal_connect(ui_preferences->dialog, "delete-event",
		G_CALLBACK(preferences_on_delete_event), ui_preferences);

	/* Take the apropriate action when a button is pressed */
	g_signal_connect(ui_preferences->dialog, "response",
			G_CALLBACK(preferences_actions), ui_preferences);

	table = gtk_table_new(6, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(ui_preferences->dialog)->vbox), table, TRUE, TRUE, 0);

	/*
	 * User name
	 */
	label = gtk_label_new(_("User name"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 3, 3);

	ui_preferences->username = gtk_entry_new();
	set_tooltip(ui_preferences->username, _("You should know your name"));
	gtk_table_attach(GTK_TABLE(table), ui_preferences->username, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);

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
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 3, 3);

	ui_preferences->email = gtk_entry_new();
	set_tooltip(ui_preferences->email, _("Your email address"));
	gtk_table_attach(GTK_TABLE(table), ui_preferences->email, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 3, 3);

	/* read config */
	if (strlen(gebr.config.email->str))
		gtk_entry_set_text(GTK_ENTRY(ui_preferences->email), gebr.config.email->str);
	else
		gtk_entry_set_text(GTK_ENTRY(ui_preferences->email), g_get_user_name());

	/*
	 * User menus dir
	 */
	label = gtk_label_new(_("User's menus directory"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 3, 3);

	/* Browse button for user's menus dir */
	eventbox = gtk_event_box_new();
	ui_preferences->usermenus = gtk_file_chooser_button_new(_("GêBR dir"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
	gtk_container_add(GTK_CONTAINER(eventbox), ui_preferences->usermenus);
	set_tooltip(eventbox, _("Path to look for private user's menus"));
	gtk_table_attach(GTK_TABLE(table), eventbox, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 3, 3);

	/* read config */
	if (gebr.config.usermenus->len > 0)
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(ui_preferences->usermenus),
			gebr.config.usermenus->str);

	/*
	 * Editor
	 */
	label = gtk_label_new(_("HTML editor"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5, GTK_FILL, GTK_FILL, 3, 3);

	ui_preferences->editor = gtk_entry_new();
	set_tooltip(ui_preferences->editor, _("An HTML capable editor to edit helps and reports"));
	gtk_table_attach(GTK_TABLE(table), ui_preferences->editor, 1, 2, 4, 5, GTK_FILL, GTK_FILL, 3, 3);

	/* read config */
	gtk_entry_set_text(GTK_ENTRY(ui_preferences->editor), gebr.config.editor->str);

	/* Browser */
	label = gtk_label_new(_("Browser"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 5, 6, GTK_FILL, GTK_FILL, 3, 3);

	eventbox = gtk_event_box_new();
	ui_preferences->browser = gtk_combo_box_entry_new_text();
	gtk_container_add(GTK_CONTAINER(eventbox), ui_preferences->browser);
	set_tooltip(eventbox, _("An HTML browser to display helps and reports"));
	gtk_table_attach(GTK_TABLE(table), eventbox, 1, 2, 5, 6, GTK_FILL, GTK_FILL, 3, 3);
	/* read config */
	{
		int		i;
		gboolean	new_browser;

		new_browser = TRUE;
		for (i=0; i < NBROWSER; i++) {
			gtk_combo_box_append_text(GTK_COMBO_BOX(ui_preferences->browser), browser[i]);
			if (gebr.config.browser->len > 0 && new_browser) {
				if (strcmp(browser[i], gebr.config.browser->str)==0) {
					new_browser = FALSE;
					gtk_combo_box_set_active(GTK_COMBO_BOX(ui_preferences->browser), i );
				}
			}
		}
		if (gebr.config.browser->len > 0 && new_browser) {
			gtk_combo_box_append_text(GTK_COMBO_BOX(ui_preferences->browser), gebr.config.browser->str);
			gtk_combo_box_set_active(GTK_COMBO_BOX(ui_preferences->browser), NBROWSER );
		}
	}

	/* finally... */
	gtk_widget_show_all(ui_preferences->dialog);

	return ui_preferences;
}

/*
 * Section: Private
 * Private functions.
 */

/*
 * Function: preferences_actions
 * Take the appropriate action when the parameter dialog emmits
 * a response signal.
 */
static void
preferences_actions(GtkDialog * dialog, gint arg1, struct ui_preferences * ui_preferences)
{
	switch (arg1) {
	case GTK_RESPONSE_OK: {
		gchar *	tmp;
		gchar *	tmp2;
		gchar *	tmp3;

		tmp = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(ui_preferences->usermenus));
		tmp2 = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(ui_preferences->data));
		tmp3 = gtk_combo_box_get_active_text(GTK_COMBO_BOX(ui_preferences->browser));
		if (tmp3 == NULL)
			tmp3 = "";

		g_string_assign(gebr.config.username,
				gtk_entry_get_text(GTK_ENTRY(ui_preferences->username)));
		g_string_assign(gebr.config.email,
				gtk_entry_get_text(GTK_ENTRY(ui_preferences->email)));
		g_string_assign(gebr.config.usermenus,
				tmp);
		g_string_assign(gebr.config.data,
				tmp2);
		g_string_assign(gebr.config.editor,
				gtk_entry_get_text(GTK_ENTRY(ui_preferences->editor)));
		g_string_assign(gebr.config.browser,
				tmp3);

		gebr_config_save(TRUE);
		gebr_config_apply();

		g_free(tmp);
		g_free(tmp2);
		g_free(tmp3);
		break;
	} case GTK_RESPONSE_CANCEL: /* does nothing */
		if (ui_preferences->first_run == TRUE)
			gebr_quit();
		break;
	default:
		break;
	}

	gtk_widget_destroy(ui_preferences->dialog);
	g_free(ui_preferences);
}

static gboolean
preferences_on_delete_event(GtkDialog * dialog, GdkEventAny * event, struct ui_preferences * ui_preferences)
{
	preferences_actions(dialog, GTK_RESPONSE_CANCEL, ui_preferences);
	return FALSE;
}
