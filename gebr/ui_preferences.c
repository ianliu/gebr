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

enum {
	MAESTRO_DEFAULT_ADDR,
	MAESTRO__DEFAULT_DESCRIPTION,
	MAESTRO_DEFAULT_N_COLUMN
};

/*
 * Prototypes
 */
/**
 * \internal
 * Disable HTML editor entry on radio false state
 */
//static void on_custom_radio_toggled(GtkToggleButton *togglebutton, GtkWidget *htmleditor_entry)
//{
//	gtk_widget_set_sensitive(htmleditor_entry, gtk_toggle_button_get_active(togglebutton));
//}

static void
on_response_ok(GtkButton *button,
               struct ui_preferences *up)
{
	gchar *tmp;

	tmp = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(up->usermenus));

	g_string_assign(gebr.config.username, gtk_entry_get_text(GTK_ENTRY(up->username)));
	g_string_assign(gebr.config.email, gtk_entry_get_text(GTK_ENTRY(up->email)));
//	g_string_assign(gebr.config.editor, gtk_entry_get_text(GTK_ENTRY(up->editor)));

//	gebr.config.native_editor = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(up->user_radio_button));

//	gebr.config.log_load = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(up->log_load));

	if (g_strcmp0(gebr.config.usermenus->str, tmp) != 0){
		g_string_assign(gebr.config.usermenus, tmp);
		gebr_config_apply();
	}

	/* Config maestro */
//	gchar *addr;
//	GtkTreeIter iter;
//	GtkTreeModel *maestro_model = gtk_combo_box_get_model(up->maestro_combo);
//
//	gtk_combo_box_get_active_iter(up->maestro_combo, &iter);
//	gtk_tree_model_get(maestro_model, &iter, MAESTRO_DEFAULT_ADDR, &addr, -1);
//
//	gebr_maestro_controller_connect(gebr.maestro_controller, addr);
//
//	g_free(addr);

	gebr_config_save(TRUE);

	g_free(tmp);

	gtk_widget_destroy(up->dialog);
}

static void
on_assistant_cancel(GtkWidget *widget)
{
	gtk_widget_destroy(widget);
	gebr_quit(FALSE);
}

static void
on_assistant_close(GtkAssistant *assistant,
                   struct ui_preferences *up)
{
	gint page = gtk_assistant_get_current_page(assistant) + 1;

	if (page == 8)
		on_response_ok(NULL, up);
}

static void
on_assistant_apply(GtkAssistant *assistant,
                   struct ui_preferences *up)
{
	gint page = gtk_assistant_get_current_page(assistant) + 1;

	if (page == 7) {
		GtkWidget *page_review = GTK_WIDGET(gtk_builder_get_object(up->builder, "review"));
		gtk_assistant_set_page_complete(assistant, page_review, TRUE);
		gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page_review, GTK_ASSISTANT_PAGE_SUMMARY);
	}
}

static void
on_maestro_state_changed(GebrMaestroController *self,
                         GebrMaestroServer     *maestro,
                         struct ui_preferences *up)
{
	GebrCommServerState state = gebr_maestro_server_get_state(maestro);

	if (state != SERVER_STATE_LOGGED && state != SERVER_STATE_DISCONNECTED)
		return;

	GtkWidget *main_status = GTK_WIDGET(gtk_builder_get_object(up->builder, "main_status"));
	GObject *status_progress = gtk_builder_get_object(up->builder, "status_progress");
	GObject *status_container = gtk_builder_get_object(up->builder, "status_container");
	GObject *status_img = gtk_builder_get_object(up->builder, "status_img");
	GObject *status_label = gtk_builder_get_object(up->builder, "status_label");
	GObject *status_title = gtk_builder_get_object(up->builder, "status_title");

	gtk_widget_hide(GTK_WIDGET(status_progress));
	gtk_widget_show_all(GTK_WIDGET(status_container));

	gchar *summary_txt;

	const gchar *address = gebr_maestro_server_get_address(maestro);

	if (state == SERVER_STATE_LOGGED) {
		gtk_image_set_from_stock(GTK_IMAGE(status_img), GTK_STOCK_OK, GTK_ICON_SIZE_DIALOG);
		gtk_label_set_text(GTK_LABEL(status_label), _("Success!"));
		gtk_assistant_set_page_type(GTK_ASSISTANT(up->dialog),
		                            main_status, GTK_ASSISTANT_PAGE_INTRO);
		gtk_assistant_set_page_title(GTK_ASSISTANT(up->dialog),
		                             main_status, _("Done"));
		gtk_assistant_set_page_complete(GTK_ASSISTANT(up->dialog), main_status, TRUE);

		summary_txt = g_markup_printf_escaped(_("<span size='large'>Maestro <b>%s</b> successfully connected!</span>"),
		                                      address);

		gtk_label_set_markup(GTK_LABEL(status_title), summary_txt);
		g_free(summary_txt);
	}
	else {
		const gchar *type, *msg;
		gebr_maestro_server_get_error(maestro, &type, &msg);

		if (!g_strcmp0(type, "error:none")) {
			gtk_image_set_from_stock(GTK_IMAGE(status_img), GTK_STOCK_DISCONNECT, GTK_ICON_SIZE_DIALOG);

			gchar *txt = g_markup_printf_escaped(_("Connecting to <b>%s</b> ..."), address);
			gtk_label_set_markup(GTK_LABEL(status_label), txt);
			g_free(txt);

			gtk_assistant_set_page_complete(GTK_ASSISTANT(up->dialog), main_status, FALSE);
		}
		else {
			gtk_image_set_from_stock(GTK_IMAGE(status_img), GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);

			gchar *txt = g_markup_printf_escaped(_("The connection reported the following error: <i>%s</i>"), msg);
			gtk_label_set_markup(GTK_LABEL(status_label), txt);
			g_free(txt);

			gtk_assistant_set_page_type(GTK_ASSISTANT(up->dialog),
						    main_status, GTK_ASSISTANT_PAGE_CONFIRM);
			gtk_assistant_set_page_title(GTK_ASSISTANT(up->dialog),
						     main_status, _("Warning!"));
			gtk_assistant_set_page_complete(GTK_ASSISTANT(up->dialog), main_status, FALSE);

			summary_txt = g_markup_printf_escaped(_("<span size='large'>Could not connect to Maestro <b>%s</b>!</span>"),
			                                      address);

			gtk_label_set_markup(GTK_LABEL(status_title), summary_txt);
			g_free(summary_txt);
		}
	}
}

static void
on_add_server_clicked(GtkButton *button,
		      struct ui_preferences *up)
{
	GObject *server_entry = gtk_builder_get_object(up->builder, "server_entry");
	GtkWidget *main_servers = GTK_WIDGET(gtk_builder_get_object(up->builder, "main_servers"));

	const gchar *addr = gtk_entry_get_text(GTK_ENTRY(server_entry));
	gebr_maestro_controller_server_list_add(gebr.maestro_controller, addr);

	gtk_entry_set_text(GTK_ENTRY(server_entry), "");


	gtk_assistant_set_page_complete(GTK_ASSISTANT(up->dialog), main_servers, TRUE);
}

static GtkTreeView *
create_view_for_servers(struct ui_preferences *up)
{
	GtkWidget *view = GTK_WIDGET(gtk_builder_get_object(up->builder, "servers_view"));

	if (gtk_tree_view_get_model(GTK_TREE_VIEW(view)))
		return NULL;

	//	gebr_gui_gtk_tree_view_set_tooltip_callback(GTK_TREE_VIEW(view),
	//	                                            (GebrGuiGtkTreeViewTooltipCallback) server_tooltip_callback, self);

	// Server Column
	GtkCellRenderer *renderer ;
	GtkTreeViewColumn *col;
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, _("Address"));
	gtk_tree_view_column_set_min_width(col, 100);

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(col), renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, gebr_maestro_controller_daemon_server_status_func,
	                                        NULL, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(col), renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, gebr_maestro_controller_daemon_server_address_func,
	                                        GINT_TO_POINTER(FALSE), NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	// End of Server Column

	return GTK_TREE_VIEW(view);
}

static void
on_assistant_prepare(GtkAssistant *assistant,
		     GtkWidget *current_page,
		     struct ui_preferences *up)
{
	gint page = gtk_assistant_get_current_page(assistant) + 1;

	gchar *addr, *desc;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gtk_combo_box_get_active_iter(up->maestro_combo, &iter);
	model = gtk_combo_box_get_model(up->maestro_combo);

	gtk_tree_model_get(model, &iter,
	                   MAESTRO_DEFAULT_ADDR, &addr,
	                   MAESTRO__DEFAULT_DESCRIPTION, &desc,
	                   -1);

	if (page == 5) {
		GObject *status_progress = gtk_builder_get_object(up->builder, "status_progress");
		GObject *status_container = gtk_builder_get_object(up->builder, "status_container");

		gtk_widget_hide(GTK_WIDGET(status_container));
		gtk_widget_show(GTK_WIDGET(status_progress));

		g_signal_connect(gebr.maestro_controller, "maestro-state-changed", G_CALLBACK(on_maestro_state_changed), up);

		gebr_maestro_controller_connect(gebr.maestro_controller, addr);
	}
	else if (page == 7) {
		GtkTreeView *view = create_view_for_servers(up);
		if (view) {
			GObject *server_add = gtk_builder_get_object(up->builder, "server_add");
			GObject *server_entry = gtk_builder_get_object(up->builder, "server_entry");

			GtkWidget *main_servers_label = GTK_WIDGET(gtk_builder_get_object(up->builder, "main_servers_label"));
			GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);

			gchar *main_servers_text = g_markup_printf_escaped("Now you need to add <b>Servers</b> to be handled by your Maestro <b>%s</b>. "
									   "Now you can be asked for the Server password.\n\n"
									   "Put the name (hostname or address) of the Server in the blank space and click on Add.",
									   gebr_maestro_server_get_address(maestro));

			gtk_label_set_markup(GTK_LABEL(main_servers_label), main_servers_text);
			g_free(main_servers_text);

			GtkTreeModel *model = gebr_maestro_controller_get_servers_model(gebr.maestro_controller);
			gtk_tree_view_set_model(view, model);

			gtk_entry_set_text(GTK_ENTRY(server_entry), gebr_maestro_server_get_address(maestro));

			g_signal_connect(GTK_BUTTON(server_add), "clicked", G_CALLBACK(on_add_server_clicked), up);
		}
	}
	else if (page == 8) {
		GtkLabel *maestro_label = GTK_LABEL(gtk_builder_get_object(up->builder, "review_maestro_label"));
		gchar *maestro_text = g_markup_printf_escaped("%s (<i>%s</i>)", addr, desc);
		gtk_label_set_markup(maestro_label, maestro_text);
		g_free(maestro_text);

		GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_address(gebr.maestro_controller, addr);
		GtkTreeModel *model_servers = gebr_maestro_server_get_model(maestro, FALSE, NULL);

		GString *servers = g_string_new("");
		gboolean active;

		active = gtk_tree_model_get_iter_first(model_servers, &iter);
		while (active) {
			GebrDaemonServer *daemon;
			gtk_tree_model_get(model_servers, &iter, 0, &daemon, -1);
			g_string_append(servers, " ,");
			g_string_append(servers, gebr_daemon_server_get_address(daemon));
			active = gtk_tree_model_iter_next(model_servers, &iter);
		}
		if (servers->len)
			g_string_erase(servers, 0, 2);

		GtkLabel *servers_label = GTK_LABEL(gtk_builder_get_object(up->builder, "servers_maestro"));
		gtk_label_set_text(servers_label, servers->str);

		g_string_free(servers, TRUE);

		GtkLabel *pwd_label = GTK_LABEL(gtk_builder_get_object(up->builder, "review_pwd_label"));
		GtkWidget *sshkey_button = GTK_WIDGET(gtk_builder_get_object(up->builder, "sshkey_button"));
		GtkWidget *storepass_button = GTK_WIDGET(gtk_builder_get_object(up->builder, "storepass_button"));
		GtkWidget *none_button = GTK_WIDGET(gtk_builder_get_object(up->builder, "none_button"));

		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(sshkey_button)))
			gtk_label_set_text(pwd_label,gtk_button_get_label(GTK_BUTTON(sshkey_button)));
		else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(storepass_button)))
			gtk_label_set_text(pwd_label,gtk_button_get_label(GTK_BUTTON(storepass_button)));
		else
			gtk_label_set_text(pwd_label,gtk_button_get_label(GTK_BUTTON(none_button)));
	}

	g_free(addr);
	g_free(desc);
}

void
on_preferences_destroy(GtkWindow * window,
                       struct ui_preferences *up)
{
	gtk_widget_destroy(up->dialog);
	g_free(up);
}

static void
set_preferences_page(GtkBuilder *builder,
                     struct ui_preferences *ui_preferences)
{
	/*
	 * User name
	 */
	ui_preferences->username = GTK_WIDGET(gtk_builder_get_object(builder, "entry_name"));
	gtk_entry_set_activates_default(GTK_ENTRY(ui_preferences->username), TRUE);
	gebr_gui_gtk_widget_set_tooltip(ui_preferences->username, _("You should know your name"));

	/* read config */
	if (strlen(gebr.config.username->str))
		gtk_entry_set_text(GTK_ENTRY(ui_preferences->username), gebr.config.username->str);
	else
		gtk_entry_set_text(GTK_ENTRY(ui_preferences->username), g_get_real_name());

	/*
	 * User email
	 */
	ui_preferences->email = GTK_WIDGET(gtk_builder_get_object(builder, "entry_email"));
	gtk_entry_set_activates_default(GTK_ENTRY(ui_preferences->email), TRUE);
	gebr_gui_gtk_widget_set_tooltip(ui_preferences->email, _("Your email address"));

	/* read config */
	if (strlen(gebr.config.email->str))
		gtk_entry_set_text(GTK_ENTRY(ui_preferences->email), gebr.config.email->str);
	else
		gtk_entry_set_text(GTK_ENTRY(ui_preferences->email), g_get_user_name());

	/*
	 * User's menus directory
	 */
	GtkWidget *eventbox = GTK_WIDGET(gtk_builder_get_object(builder, "menus_box"));
	ui_preferences->usermenus = gtk_file_chooser_button_new(_("User's menu directory"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
	gtk_container_add(GTK_CONTAINER(eventbox), ui_preferences->usermenus);
	gebr_gui_gtk_widget_set_tooltip(eventbox, _("Path to look for user's private menus"));

	/* read config */
	if (gebr.config.usermenus->len > 0)
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(ui_preferences->usermenus),
		                                    gebr.config.usermenus->str);

	/*
	 * Editor
	 */
//	GtkWidget *list_widget_hbox = GTK_WIDGET(gtk_builder_get_object(builder, "html_box"));
//
//	/* Radio Buttons of HTML */
//	GtkWidget *fake_radio_button = gtk_radio_button_new(NULL);
//	ui_preferences->built_in_radio_button =  gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(fake_radio_button), _("Built-in"));
//	ui_preferences->user_radio_button =  gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(fake_radio_button), _("Custom"));
//	ui_preferences->editor = gtk_entry_new();
//	gtk_widget_set_sensitive(ui_preferences->editor, FALSE);
//	gtk_entry_set_activates_default(GTK_ENTRY(ui_preferences->editor), TRUE);
//	gebr_gui_gtk_widget_set_tooltip(ui_preferences->editor, _("An HTML editor capable of editing help files and reports"));
//
//	g_signal_connect(ui_preferences->user_radio_button, "toggled", G_CALLBACK(on_custom_radio_toggled), ui_preferences->editor);
//
//	gtk_box_pack_start(GTK_BOX(list_widget_hbox), ui_preferences->built_in_radio_button, FALSE, FALSE, 2);
//	gtk_box_pack_start(GTK_BOX(list_widget_hbox), ui_preferences->user_radio_button, FALSE, FALSE, 2);
//	gtk_box_pack_start(GTK_BOX(list_widget_hbox), ui_preferences->editor, FALSE, FALSE, 0);
//
//	gtk_entry_set_text(GTK_ENTRY(ui_preferences->editor), gebr.config.editor->str);
//
//	/* read config */
//	if (!gebr.config.native_editor) {
//		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_preferences->user_radio_button), TRUE);
//	} else {
//		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_preferences->built_in_radio_button), TRUE);
//	}

	//	/* Load log */
	//	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(label), gebr.config.log_load);

}

static void
on_combo_set_text(GtkCellLayout   *cell_layout,
                  GtkCellRenderer *cell,
                  GtkTreeModel    *tree_model,
                  GtkTreeIter     *iter,
                  gpointer 	   data)
{
	gchar *addr;
	gchar *description;

	gtk_tree_model_get(tree_model, iter,
	                   MAESTRO_DEFAULT_ADDR, &addr,
	                   MAESTRO__DEFAULT_DESCRIPTION, &description,
	                   -1);

	gchar *text = g_markup_printf_escaped("%s (<i>%s</i>)", addr, description);

	g_object_set(cell, "markup", text, NULL);

	g_free(text);
	g_free(addr);
	g_free(description);
}

static void
set_maestro_chooser_page(GtkBuilder *builder,
                         struct ui_preferences *ui_preferences)
{
	const gchar *maestros_default = g_getenv("GEBR_DEFAULT_MAESTRO");
	GtkComboBox *combo = GTK_COMBO_BOX(gtk_builder_get_object(builder, "maestro_combo"));
	ui_preferences->maestro_combo = combo;

	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, TRUE);
	gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(combo), renderer, on_combo_set_text, NULL, NULL);

	GtkTreeIter iter;
	GtkListStore *model = gtk_list_store_new(MAESTRO_DEFAULT_N_COLUMN, G_TYPE_STRING, G_TYPE_STRING);

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);

	if (maestro) {
		gtk_list_store_append(model, &iter);
		gtk_list_store_set(model, &iter,
				   MAESTRO_DEFAULT_ADDR, gebr_maestro_server_get_address(maestro),
				   MAESTRO__DEFAULT_DESCRIPTION, _("Current Maestro"),
				   -1);
	}
	if (!maestros_default) {
		gtk_list_store_append(model, &iter);
		gtk_list_store_set(model, &iter,
		                   MAESTRO_DEFAULT_ADDR, gebr.config.maestro_address->str,
		                   MAESTRO__DEFAULT_DESCRIPTION, _("Default Maestro from File"),
		                   -1);
	}
	else {
		if (!maestro && g_strcmp0(gebr.config.maestro_address->str, "")) {
			gtk_list_store_append(model, &iter);
			gtk_list_store_set(model, &iter,
			                   MAESTRO_DEFAULT_ADDR, gebr.config.maestro_address->str,
			                   MAESTRO__DEFAULT_DESCRIPTION, _("Current maestro"),
			                   -1);
		}

		gchar **options = g_strsplit(maestros_default, ";", -1);
		for (gint i = 0; options[i]; i++) {
			gchar **m = g_strsplit(options[i], ",", -1);

			gtk_list_store_append(model, &iter);
			gtk_list_store_set(model, &iter,
			                   MAESTRO_DEFAULT_ADDR, m[0],
			                   MAESTRO__DEFAULT_DESCRIPTION, m[1],
			                   -1);

			g_strfreev(m);
		}
		g_strfreev(options);
	}
	gtk_combo_box_set_model(combo, GTK_TREE_MODEL(model));
	gtk_combo_box_set_active(combo, 0);
}

/**
 * Assembly preference window.
 *
 * \return The structure containing relevant data. It will be automatically freed when the dialog closes.
 */
struct ui_preferences *
preferences_setup_ui(gboolean first_run)
{
	struct ui_preferences *ui_preferences;

	ui_preferences = g_new(struct ui_preferences, 1);
	ui_preferences->first_run = first_run;

	/* Load pages from Glade */
	GtkBuilder *builder = gtk_builder_new();
	const gchar *glade_file = GEBR_GLADE_DIR "/gebr-connections-settings.glade";
	gtk_builder_add_from_file(builder, glade_file, NULL);

	ui_preferences->builder = builder;

	GtkWidget *page_preferences = GTK_WIDGET(gtk_builder_get_object(builder, "main_preferences"));
	GtkWidget *page_minfo = GTK_WIDGET(gtk_builder_get_object(builder, "maestro_info"));
	GtkWidget *page_mchooser = GTK_WIDGET(gtk_builder_get_object(builder, "maestro_chooser"));
	GtkWidget *page_pwdinfo = GTK_WIDGET(gtk_builder_get_object(builder, "pwd_info"));
	GtkWidget *page_review = GTK_WIDGET(gtk_builder_get_object(builder, "review"));
	GtkWidget *main_status = GTK_WIDGET(gtk_builder_get_object(builder, "main_status"));
	GtkWidget *servers_info = GTK_WIDGET(gtk_builder_get_object(builder, "servers_info"));
	GtkWidget *main_servers = GTK_WIDGET(gtk_builder_get_object(builder, "main_servers"));

	/* Create Wizard if the first_run of GeBR */
	if (first_run) {
		GtkWidget *assistant = gtk_assistant_new();
		gtk_window_set_transient_for(GTK_WINDOW(assistant), GTK_WINDOW(gebr.window));
		gtk_window_set_position(GTK_WINDOW(assistant), GTK_WIN_POS_CENTER_ON_PARENT);
		gtk_window_set_title(GTK_WINDOW(assistant), _("Configuring GêBR"));

		g_signal_connect(assistant, "cancel", G_CALLBACK(on_assistant_cancel), NULL);
		g_signal_connect(assistant, "close", G_CALLBACK(on_assistant_close), ui_preferences);
		g_signal_connect(assistant, "prepare", G_CALLBACK(on_assistant_prepare), ui_preferences);
		g_signal_connect(assistant, "apply", G_CALLBACK(on_assistant_apply), ui_preferences);

		gtk_assistant_append_page(GTK_ASSISTANT(assistant), page_preferences);
		gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page_preferences, TRUE);
		gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page_preferences, GTK_ASSISTANT_PAGE_INTRO);
		gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page_preferences, _("Preferences"));

		gtk_assistant_append_page(GTK_ASSISTANT(assistant), page_minfo);
		gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page_minfo, TRUE);
		gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page_minfo, GTK_ASSISTANT_PAGE_CONTENT);
		gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page_minfo, _("Maestro Information"));

		gtk_assistant_append_page(GTK_ASSISTANT(assistant), page_mchooser);
		gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page_mchooser, TRUE);
		gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page_mchooser, GTK_ASSISTANT_PAGE_CONTENT);
		gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page_mchooser, _("Choose your Maestro"));

		gtk_assistant_append_page(GTK_ASSISTANT(assistant), page_pwdinfo);
		gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page_pwdinfo, TRUE);
		gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page_pwdinfo, GTK_ASSISTANT_PAGE_CONTENT);
		gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page_pwdinfo, _("Connections informations"));

		gtk_assistant_append_page(GTK_ASSISTANT(assistant), main_status);
		gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), main_status, GTK_ASSISTANT_PAGE_PROGRESS);
		gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), main_status, _("Connecting on Maestro..."));

		gtk_assistant_append_page(GTK_ASSISTANT(assistant), servers_info);
		gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), servers_info, TRUE);
		gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), servers_info, GTK_ASSISTANT_PAGE_INTRO);
		gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), servers_info, _("Servers Information"));

		gtk_assistant_append_page(GTK_ASSISTANT(assistant), main_servers);
		gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), main_servers, FALSE);
		gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), main_servers, GTK_ASSISTANT_PAGE_CONFIRM);
		gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), main_servers, _("Insert Servers on Maestro"));

		gtk_assistant_append_page(GTK_ASSISTANT(assistant), page_review);
		gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page_review, FALSE);
		gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page_review, GTK_ASSISTANT_PAGE_CONFIRM);
		gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page_review, _("Review"));

		ui_preferences->dialog = assistant;
	}

	/* Create dialog with tabs if the other run of GeBR */
	else {
		GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_window_set_modal(GTK_WINDOW(window), TRUE);
		gtk_container_set_border_width(GTK_CONTAINER(window), 5);
		gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(gebr.window));
		gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ON_PARENT);
		gtk_window_set_destroy_with_parent(GTK_WINDOW(window), TRUE);

		gchar *window_title = g_strdup_printf(_("Preferences of GêBR"));
		gtk_window_set_title(GTK_WINDOW(window), window_title);
		g_free(window_title);

		GtkWidget *vbox;
		vbox = gtk_vbox_new(FALSE, 5);
		GtkWidget *notebook = gtk_notebook_new();
		gtk_container_add(GTK_CONTAINER(vbox), notebook);
		gtk_container_add(GTK_CONTAINER(window), vbox);
		gtk_window_set_default_size(GTK_WINDOW(window), 400, -1);
		gtk_widget_show(vbox);
		gtk_widget_show(notebook);

		GtkWidget *button_box;
		GtkWidget *ok_button;
		GtkWidget *cancel_button;
		button_box = gtk_hbutton_box_new();
		ok_button = gtk_button_new_from_stock(GTK_STOCK_OK);
		cancel_button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);

		gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
		gtk_box_pack_start(GTK_BOX(button_box), cancel_button, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(button_box), ok_button, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, TRUE, 0);
		gtk_widget_show(button_box);

		g_signal_connect(window, "destroy", G_CALLBACK(on_preferences_destroy), ui_preferences);
		g_signal_connect(ok_button, "clicked", G_CALLBACK(on_response_ok), ui_preferences);
		g_signal_connect_swapped(cancel_button, "clicked", G_CALLBACK(gtk_widget_destroy), window);

		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_preferences, gtk_label_new(_("Preferences")));
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_mchooser, gtk_label_new(_("Choose your maestro")));

		ui_preferences->dialog = window;
	}

	/* Set Preferences Page */
	set_preferences_page(builder, ui_preferences);

	/* Set Maestro Chooser Page */
	set_maestro_chooser_page(builder, ui_preferences);

	/* finally... */
	gtk_widget_show_all(ui_preferences->dialog);

	return ui_preferences;
}
