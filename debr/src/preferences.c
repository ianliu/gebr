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

#include "preferences.h"
#include "debr.h"
#include "menu.h"

void preferences_dialog_setup_ui(void)
{
	GtkWidget *window;
	GtkWidget *table;
	GtkWidget *name_entry;
	GtkWidget *email_entry;
	GtkWidget *htmleditor_entry;
	GtkWidget *label;
	GtkWidget *fake_radio_button;
	GtkWidget *builtin_editor;
	GtkWidget *custom_editor;
	GtkWidget *list_widget_hbox;

	window = gtk_dialog_new_with_buttons(_("Preferences"),
					     GTK_WINDOW(debr.window),
					     GTK_DIALOG_MODAL,
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(window), 5);
	gtk_dialog_set_default_response(GTK_DIALOG(window), GTK_RESPONSE_OK);

	table = gtk_table_new(6, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), table, TRUE, TRUE, 0);

	/* Name */
	label = gtk_label_new(_("Name"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	name_entry = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(name_entry), TRUE);
	g_object_set(name_entry, "tooltip-text", _("You should know your name"), NULL);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), name_entry, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
	gtk_entry_set_text(GTK_ENTRY(name_entry), debr.config.name->str);

	/* Email */
	label = gtk_label_new(_("Email"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	email_entry = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(email_entry), TRUE);
	g_object_set(email_entry, "tooltip-text", _("Your email address"), NULL);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), email_entry, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	gtk_entry_set_text(GTK_ENTRY(email_entry), debr.config.email->str);

	/* Editor */
	label = gtk_label_new(_("HTML editor"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5, GTK_FILL, GTK_FILL, 3, 3);

	list_widget_hbox = gtk_hbox_new(FALSE, 0);
	gtk_table_attach(GTK_TABLE(table), list_widget_hbox, 1, 2, 4, 5,
			 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			 (GtkAttachOptions) (0), 0, 0);

	fake_radio_button = gtk_radio_button_new(NULL);
	builtin_editor = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(fake_radio_button), _("Built-in"));
	gtk_box_pack_start(GTK_BOX(list_widget_hbox), builtin_editor, FALSE, FALSE, 2);
	custom_editor = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(fake_radio_button), _("Custom"));
	gtk_box_pack_start(GTK_BOX(list_widget_hbox), custom_editor, FALSE, FALSE, 2);

	htmleditor_entry = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(htmleditor_entry), TRUE);
	g_object_set(htmleditor_entry, "tooltip-text", _("An HTML capable editor to edit helps and reports"), NULL);
	gtk_box_pack_start(GTK_BOX(list_widget_hbox), htmleditor_entry, FALSE, FALSE, 0);
	gtk_entry_set_text(GTK_ENTRY(htmleditor_entry), debr.config.htmleditor->str);

	if (debr.config.native_editor)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(builtin_editor), TRUE);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(custom_editor), TRUE);

	gtk_widget_show_all(window);

	if (gtk_dialog_run(GTK_DIALOG(window)) == GTK_RESPONSE_OK) {
		g_string_assign(debr.config.name, gtk_entry_get_text(GTK_ENTRY(name_entry)));
		g_string_assign(debr.config.email, gtk_entry_get_text(GTK_ENTRY(email_entry)));
		g_string_assign(debr.config.htmleditor, gtk_entry_get_text(GTK_ENTRY(htmleditor_entry)));
		debr.config.native_editor = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(builtin_editor));
		debr_config_save();
	}

	gtk_widget_destroy(window);
}
