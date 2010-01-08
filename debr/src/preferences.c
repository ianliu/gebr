/*   DeBR - GeBR Designer
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

#include <string.h>

#include <libgebr/intl.h>
#include <libgebr/gui/gtkdirectorychooser.h>

#include "preferences.h"
#include "debr.h"
#include "menu.h"

/*
 * File: preferences.c
 * Preferences interface and configuration stuff
 */

const gchar *browser[] = {
	"epiphany",
	"firefox",
	"galeon",
	"konqueror",
	"mozilla"
};

#define NBROWSER 5

/*
 * Function: preferences_dialog_setup_ui
 * Create and show preferences dialog. If ok, save config.
 */
void preferences_dialog_setup_ui(void)
{
	GtkWidget *window;
	GtkWidget *table;
	GtkWidget *name_entry;
	GtkWidget *email_entry;
	GtkWidget *menudir_dirchooser;
	GtkWidget *browser_combo;
	GtkWidget *htmleditor_entry;
	GtkWidget *label;
	GtkTooltips *tips;
	GtkWidget *eventbox;
	int i;
	gboolean newbrowser = TRUE;

	/* dialog */
	window = gtk_dialog_new_with_buttons(_("Preferences"),
					     GTK_WINDOW(debr.window),
					     GTK_DIALOG_MODAL,
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	/* tooltips */
	tips = gtk_tooltips_new();
	gtk_widget_set_size_request(window, 380, 300);
	/* table */
	table = gtk_table_new(6, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), table, TRUE, TRUE, 0);

	/* Name */
	label = gtk_label_new(_("Name"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	name_entry = gtk_entry_new();
	gtk_tooltips_set_tip(tips, name_entry, _("You should know your name"), "");
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), name_entry, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
	gtk_entry_set_text(GTK_ENTRY(name_entry), debr.config.name->str);

	/* Email */
	label = gtk_label_new(_("Email"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	email_entry = gtk_entry_new();
	gtk_tooltips_set_tip(tips, email_entry, _("Your email address"), "");
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), email_entry, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	gtk_entry_set_text(GTK_ENTRY(email_entry), debr.config.email->str);

	/* Menus dir */
	label = gtk_label_new(_("Menus directory"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	/* Browse button for user's menus dir */
	menudir_dirchooser = gebr_gui_gtk_directory_chooser_new();
	// menudir_dirchooser = gtk_file_chooser_button_new("GêBR dir",
	//                                      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
	eventbox = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(eventbox), menudir_dirchooser);
	gtk_tooltips_set_tip(tips, eventbox, _("Path to look for private user's menus"), "");
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), eventbox, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 3, 3);
	/* read config */
	gebr_gui_gtk_directory_chooser_set_paths(GEBR_GUI_GTK_DIRECTORY_CHOOSER(menudir_dirchooser),
						 debr.config.menu_dir);

	/* Browser */
	label = gtk_label_new(_("Browser"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	browser_combo = gtk_combo_box_entry_new_text();
	for (i = 0; i < NBROWSER; i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(browser_combo), browser[i]);
		if (debr.config.browser && newbrowser) {
			if (strcmp(browser[i], debr.config.browser->str) == 0) {
				newbrowser = FALSE;
				gtk_combo_box_set_active(GTK_COMBO_BOX(browser_combo), i);
			}
		}
	}
	if (strlen(debr.config.browser->str) && newbrowser) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(browser_combo), debr.config.browser->str);
		gtk_combo_box_set_active(GTK_COMBO_BOX(browser_combo), NBROWSER);
	}
	eventbox = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(eventbox), browser_combo);
	gtk_tooltips_set_tip(tips, eventbox, _("An HTML browser to display helps and reports"), "");
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 5, 6, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), eventbox, 1, 2, 5, 6, GTK_FILL, GTK_FILL, 3, 3);

	/* Editor */
	label = gtk_label_new(_("HTML editor"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	htmleditor_entry = gtk_entry_new();
	gtk_tooltips_set_tip(tips, htmleditor_entry, _("An HTML capable editor to edit helps and reports"), "");
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), htmleditor_entry, 1, 2, 4, 5, GTK_FILL, GTK_FILL, 3, 3);
	gtk_entry_set_text(GTK_ENTRY(htmleditor_entry), debr.config.htmleditor->str);

	gtk_widget_show_all(window);
	switch (gtk_dialog_run(GTK_DIALOG(window))) {
	case GTK_RESPONSE_OK:{
			g_strfreev(debr.config.menu_dir);

			/* update settings */
			debr.config.menu_dir =
			    gebr_gui_gtk_directory_chooser_get_paths(GEBR_GUI_GTK_DIRECTORY_CHOOSER
								     (menudir_dirchooser));
			g_string_assign(debr.config.name, gtk_entry_get_text(GTK_ENTRY(name_entry)));
			g_string_assign(debr.config.email, gtk_entry_get_text(GTK_ENTRY(email_entry)));
			g_string_assign(debr.config.browser,
					gtk_combo_box_get_active_text(GTK_COMBO_BOX(browser_combo)));
			g_string_assign(debr.config.htmleditor, gtk_entry_get_text(GTK_ENTRY(htmleditor_entry)));

			/* save */
			debr_config_save();

			/* apply settings */
			// menu_load_user_directory();
			menu_reset();

			break;
		}
	case GTK_RESPONSE_CANCEL:
		break;
	}

	gtk_widget_destroy(window);
}
