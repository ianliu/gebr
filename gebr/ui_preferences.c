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
#include <locale.h>

#include "ui_preferences.h"
#include "gebr.h"

enum {
	MAESTRO_DEFAULT_ADDR,
	MAESTRO__DEFAULT_DESCRIPTION,
	MAESTRO_DEFAULT_N_COLUMN
};

enum {
	CANCEL_PAGE,
	PREFERENCES_PAGE,
	MAESTRO_INFO_PAGE,
	MAESTRO_PAGE,
	SERVERS_INFO_PAGE,
	SERVERS_PAGE,
};

typedef enum {
	WIZARD_STATUS_COMPLETE,
	WIZARD_STATUS_WITHOUT_MAESTRO,
	WIZARD_STATUS_WITHOUT_DAEMON
} WizardStatus;

#define DEFAULT_TEXT _("Type server hostname or address, and click Add")

/*
 * Prototypes
 */

static void set_status_for_maestro(GebrMaestroController *self,
                                   GebrMaestroServer     *maestro,
                                   struct ui_preferences *up,
                                   GebrCommServerState state);

static WizardStatus get_wizard_status();

static void on_preferences_destroy(GtkWidget *window,
                                   struct ui_preferences *up);

static void on_assistant_destroy(GtkWidget *window,
                                 struct ui_preferences *up);


static void
save_preferences_configuration(struct ui_preferences *up)
{
	gchar *tmp;

	tmp = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(up->usermenus));

	g_string_assign(gebr.config.username, gtk_entry_get_text(GTK_ENTRY(up->username)));
	g_string_assign(gebr.config.email, gtk_entry_get_text(GTK_ENTRY(up->email)));

	if (g_strcmp0(gebr.config.usermenus->str, tmp) != 0){
		g_string_assign(gebr.config.usermenus, tmp);
		gebr_config_apply();
	}

	gebr_config_save(FALSE);

	g_free(tmp);
}

static void
on_response_ok(GtkButton *button,
	       struct ui_preferences *up)
{
	save_preferences_configuration(up);
	gtk_widget_destroy(up->dialog);
}

static void
on_assistant_cancel(GtkAssistant *assistant,
		    struct ui_preferences *up)
{
	gint curr_page = gtk_assistant_get_current_page(assistant);
	up->prev_page = curr_page;

	if (up->first_run) {
		up->cancel_assistant = TRUE;
		gtk_assistant_set_current_page(GTK_ASSISTANT(assistant), CANCEL_PAGE);
	} else {
		up->cancel_assistant = FALSE;
		gtk_widget_destroy(GTK_WIDGET(assistant));
	}
}

static void
on_assistant_close(GtkAssistant *assistant,
                   struct ui_preferences *up)
{
	gint page = gtk_assistant_get_current_page(GTK_ASSISTANT(assistant));
	if (page == CANCEL_PAGE) {
		on_assistant_destroy(GTK_WIDGET(assistant), up);
		gebr_quit(FALSE);
	}
	else if (page == SERVERS_PAGE) {
		gtk_widget_destroy(up->dialog);
	}
}

static void
on_assistant_back_button(GtkButton *button,
                         struct ui_preferences *up)
{
	gtk_assistant_set_current_page(GTK_ASSISTANT(up->dialog), up->prev_page);
}

static void
on_assistant_apply(GtkAssistant *assistant,
		   struct ui_preferences *up)
{
	g_signal_handlers_disconnect_by_func(up->back_button, on_assistant_back_button, up);
	gtk_assistant_remove_action_widget(assistant, up->back_button);
	save_preferences_configuration(up);
}

static void
on_maestro_state_changed(GebrMaestroController *self,
                         GebrMaestroServer     *maestro,
                         struct ui_preferences *up)
{
	GebrCommServerState state = gebr_maestro_server_get_state(maestro);

	if (state != SERVER_STATE_LOGGED && state != SERVER_STATE_DISCONNECTED)
		return;
	set_status_for_maestro(self, maestro, up, state);
}

static void
set_status_for_maestro(GebrMaestroController *self,
                       GebrMaestroServer     *maestro,
                       struct ui_preferences *up,
                       GebrCommServerState state)
{
	GtkWidget *main_maestro = GTK_WIDGET(gtk_builder_get_object(up->builder, "maestro_chooser"));
	GtkWidget *connections_info = GTK_WIDGET(gtk_builder_get_object(up->builder, "main_connection"));
	GtkWidget *main_status = GTK_WIDGET(gtk_builder_get_object(up->builder, "main_status"));
	GtkWidget *error_box = GTK_WIDGET(gtk_builder_get_object(up->builder, "error_box"));
	GObject *status_img = gtk_builder_get_object(up->builder, "status_img");
	GObject *status_label = gtk_builder_get_object(up->builder, "status_label");
	GObject *error_label = gtk_builder_get_object(up->builder, "error_label");
	GObject *status_title = gtk_builder_get_object(up->builder, "status_title");

	gchar *summary_txt;

	const gchar *address = gebr_maestro_server_get_address(maestro);

	if (state == SERVER_STATE_LOGGED) {
		gtk_image_set_from_stock(GTK_IMAGE(status_img), GTK_STOCK_OK, GTK_ICON_SIZE_DIALOG);
		gtk_label_set_text(GTK_LABEL(status_label), _("Success!"));
		gtk_assistant_set_page_type(GTK_ASSISTANT(up->dialog),
		                            main_maestro, GTK_ASSISTANT_PAGE_INTRO);
		gtk_assistant_set_page_title(GTK_ASSISTANT(up->dialog),
		                             main_maestro, _("Done"));
		gtk_assistant_set_page_complete(GTK_ASSISTANT(up->dialog), main_maestro, TRUE);

		summary_txt = g_markup_printf_escaped(_("<span size='large'>Successfully connected to Maestro <b>%s</b>!</span>"),
		                                      address);

		gtk_label_set_markup(GTK_LABEL(status_title), summary_txt);
		g_free(summary_txt);
	}
	else {
		const gchar *type, *msg;
		gebr_maestro_server_get_error(maestro, &type, &msg);

		if (!g_strcmp0(type, "error:none")) {
			gtk_image_set_from_stock(GTK_IMAGE(status_img), GTK_STOCK_DISCONNECT, GTK_ICON_SIZE_DIALOG);

			gtk_label_set_markup(GTK_LABEL(status_label), "Connecting...");

			gtk_assistant_set_page_complete(GTK_ASSISTANT(up->dialog), main_maestro, FALSE);

			summary_txt = g_markup_printf_escaped(_("<span size='large'>Connecting to Maestro <b>%s</b>!</span>"),
			                                      address);

			gtk_label_set_markup(GTK_LABEL(status_title), summary_txt);
			g_free(summary_txt);
		} else {
			gtk_widget_hide(main_status);
			gtk_widget_show(connections_info);
			gtk_widget_show(error_box);

			gchar *txt = g_markup_printf_escaped(_("Could not connect to Maestro <b>%s</b>!\n"
							       "The connection reported the following error:\n\n<i>%s</i>"), address, msg);
			gtk_label_set_markup(GTK_LABEL(error_label), txt);
			g_free(txt);

			gtk_assistant_set_page_type(GTK_ASSISTANT(up->dialog),
			                            main_maestro, GTK_ASSISTANT_PAGE_CONTENT);
			gtk_assistant_set_page_title(GTK_ASSISTANT(up->dialog),
			                             main_maestro, _("Warning!"));
			gtk_assistant_set_page_complete(GTK_ASSISTANT(up->dialog), main_maestro, FALSE);
		}
	}
}

static void
on_daemons_changed(GebrMaestroServer *maestro,
                   struct ui_preferences *up)
{
	static gboolean has_servers = FALSE;

	if (gebr_maestro_server_has_servers(maestro, TRUE) && !has_servers) {
		has_servers = TRUE;
		g_signal_handlers_disconnect_by_func(maestro, on_daemons_changed, up);

		GtkWidget *main_servers = GTK_WIDGET(gtk_builder_get_object(up->builder, "main_servers"));
		GtkWidget *servers_label = GTK_WIDGET(gtk_builder_get_object(up->builder, "servers_label"));
		GtkWidget *servers_view = GTK_WIDGET(gtk_builder_get_object(up->builder, "servers_view"));

		gtk_assistant_set_page_complete(GTK_ASSISTANT(up->dialog), main_servers, TRUE);
		gtk_widget_hide(servers_label);
		gtk_widget_show(servers_view);
	}
}

static void
on_add_server_clicked(GtkButton *button,
		      struct ui_preferences *up)
{
	GObject *server_entry = gtk_builder_get_object(up->builder, "server_entry");

	const gchar *addr = gtk_entry_get_text(GTK_ENTRY(server_entry));
	if (!*addr)
		return;

	if (!g_strcmp0(addr, DEFAULT_TEXT))
		return;

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);
	g_signal_connect(maestro, "daemons-changed", G_CALLBACK(on_daemons_changed), up);
	gebr_maestro_controller_server_list_add(gebr.maestro_controller, addr);

	gtk_entry_set_text(GTK_ENTRY(server_entry), DEFAULT_TEXT);
}

static void
on_entry_server_activate(GtkEntry *entry,
                         struct ui_preferences *up)
{
	on_add_server_clicked(NULL, up);
}

static gboolean
on_entry_server_focus_out(GtkWidget *widget,
                         GdkEventFocus *event,
                         struct ui_preferences *up)
{
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(widget));

	if (!g_strcmp0(text, ""))
		gtk_entry_set_text(GTK_ENTRY(widget), DEFAULT_TEXT);

	return TRUE;
}

static gboolean
on_entry_server_focus_in(GtkWidget *widget,
                         GdkEventFocus *event,
                         struct ui_preferences *up)
{
	const gchar *text = gtk_entry_get_text(GTK_ENTRY(widget));

	if (!g_strcmp0(text, DEFAULT_TEXT))
		gtk_entry_set_text(GTK_ENTRY(widget), "");

	return TRUE;
}

static gboolean
server_tooltip_callback(GtkTreeView * tree_view, GtkTooltip * tooltip,
                        GtkTreeIter * iter, GtkTreeViewColumn * column, GebrMaestroController *mc)
{
	if (gtk_tree_view_get_column(tree_view, 0) == column) {
		GebrDaemonServer *daemon;

		GtkTreeModel *model = gebr_maestro_controller_get_servers_model(mc);
		gtk_tree_model_get(model, iter, 0, &daemon, -1);

		if (!daemon)
			return FALSE;

		const gchar *error = gebr_daemon_server_get_error(daemon);

		if (!error || !*error)
			return FALSE;

		gtk_tooltip_set_text(tooltip, error);
		return TRUE;
	}
	return FALSE;
}

static GtkTreeView *
create_view_for_servers(struct ui_preferences *up)
{
	GtkWidget *view = GTK_WIDGET(gtk_builder_get_object(up->builder, "servers_view"));

	if (gtk_tree_view_get_model(GTK_TREE_VIEW(view)))
		return GTK_TREE_VIEW(view);

	gebr_gui_gtk_tree_view_set_tooltip_callback(GTK_TREE_VIEW(view),
	                                            (GebrGuiGtkTreeViewTooltipCallback) server_tooltip_callback, gebr.maestro_controller);

	// Server Column
	GtkCellRenderer *renderer ;
	GtkTreeViewColumn *col;
	col = gtk_tree_view_column_new();
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
on_maestro_info_button_clicked (GtkButton *button, gpointer pointer)
{
	gchar *loc;
	const gchar *path;

	loc = setlocale(LC_MESSAGES, NULL);
	if (g_str_has_prefix (loc, "pt"))
		path = "file://" GEBR_USERDOC_DIR "/pt_BR/html/index.html#servers_configuration";
	else
		path = "file://" GEBR_USERDOC_DIR "/en/html/index.html#servers_configuration";

		g_debug("On '%s', line '%d', loc:'%s', path:'%s' ", __FILE__, __LINE__, loc, path);
	if (!gtk_show_uri(NULL, path, GDK_CURRENT_TIME, NULL)) {
		gtk_show_uri(NULL, "http://www.gebrproject.com", GDK_CURRENT_TIME, NULL);
		gebr_message (GEBR_LOG_ERROR, TRUE, TRUE,
			      _("Could not load help. "
				"Certify it was installed correctly."));
	}
}

static WizardStatus
get_wizard_status(void)
{
	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);
	if (maestro && gebr_maestro_server_get_state(maestro) == SERVER_STATE_LOGGED) {
		if (gebr_maestro_server_has_servers(maestro, TRUE))
			return WIZARD_STATUS_COMPLETE;
		else
			return WIZARD_STATUS_WITHOUT_DAEMON;
	} else
		return WIZARD_STATUS_WITHOUT_MAESTRO;

}

static void
on_connect_maestro_clicked(GtkButton *button,
                           struct ui_preferences *up)
{

	GtkWidget *main_maestro = GTK_WIDGET(gtk_builder_get_object(up->builder, "maestro_chooser"));
	GtkWidget *connections_info = GTK_WIDGET(gtk_builder_get_object(up->builder, "main_connection"));
	GtkWidget *main_status = GTK_WIDGET(gtk_builder_get_object(up->builder, "main_status"));

	gtk_widget_show(main_status);
	gtk_widget_hide(connections_info);

	gtk_assistant_set_page_type(GTK_ASSISTANT(up->dialog), main_maestro, GTK_ASSISTANT_PAGE_PROGRESS);
	gtk_assistant_set_page_title(GTK_ASSISTANT(up->dialog), main_maestro, _("Connecting on Maestro..."));

	if (up->maestro_addr)
		g_free(up->maestro_addr);
	up->maestro_addr = g_strdup(gtk_entry_get_text(up->maestro_entry));

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_address(gebr.maestro_controller, up->maestro_addr);

	g_signal_connect(gebr.maestro_controller, "maestro-state-changed", G_CALLBACK(on_maestro_state_changed), up);

	if (!maestro || gebr_maestro_server_get_state(maestro) != SERVER_STATE_LOGGED || g_strcmp0(up->maestro_addr, gebr_maestro_server_get_address(maestro)))
		gebr_maestro_controller_connect(gebr.maestro_controller, up->maestro_addr);
	else
		set_status_for_maestro(gebr.maestro_controller, maestro, up, gebr_maestro_server_get_state(maestro));
}

static void
on_connect_maestro_activate(GtkEntry *entry,
                            struct ui_preferences *up)
{
	on_connect_maestro_clicked(NULL, up);
}

static void
on_assistant_prepare(GtkAssistant *assistant,
		     GtkWidget *current_page,
		     struct ui_preferences *up)
{
	gint page = gtk_assistant_get_current_page(assistant);

	GtkTreeIter iter;
	GtkWidget *maestro_info_button = GTK_WIDGET(gtk_builder_get_object(up->builder, "maestro_info_button"));

	gtk_widget_hide(up->back_button);

	if (page == CANCEL_PAGE) {
		if (!up->cancel_assistant) {
			gtk_assistant_set_current_page(assistant, (up->prev_page > PREFERENCES_PAGE) ? --up->prev_page : PREFERENCES_PAGE);
			return;
		}

		up->cancel_assistant = FALSE;
		GtkWidget *page_review = GTK_WIDGET(gtk_builder_get_object(up->builder, "review"));
		GtkLabel *maestro_label = GTK_LABEL(gtk_builder_get_object(up->builder, "review_maestro_label"));
		GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);

		if (maestro && gebr_maestro_server_get_state(maestro) == SERVER_STATE_LOGGED
		    && !g_strcmp0(up->maestro_addr, gebr_maestro_server_get_address(maestro))) {
			gchar *maestro_text = g_markup_printf_escaped("%s", up->maestro_addr);
			gtk_label_set_markup(maestro_label, maestro_text);
			g_free(maestro_text);
		} else {
			gtk_label_set_markup(maestro_label, "<i>Not connected</i>");
		}

		gtk_widget_show(up->back_button);

		GtkLabel *review_first_label = GTK_LABEL(gtk_builder_get_object(up->builder, "review_first_label"));
		GtkLabel *review_orientations_label = GTK_LABEL(gtk_builder_get_object(up->builder, "review_orientation"));

		WizardStatus wizard_status = get_wizard_status();
		if (wizard_status == WIZARD_STATUS_COMPLETE) {
			GtkTreeModel *model_servers = gebr_maestro_server_get_model(maestro, FALSE, NULL);
			gboolean active = gtk_tree_model_get_iter_first(model_servers, &iter);
			gtk_label_set_markup(review_first_label, _("<b><span size='large'>Here is the summary of you Connections Settings.</span></b>"));

			GtkWidget *warning_img = GTK_WIDGET(gtk_builder_get_object(up->builder, "review_dialog_img"));
			gtk_widget_hide(warning_img);
			GString *servers = g_string_new("");

			while (active) {
				GebrDaemonServer *daemon;
				gtk_tree_model_get(model_servers, &iter, 0, &daemon, -1);
				g_string_append(servers, ", ");
				g_string_append(servers, gebr_daemon_server_get_address(daemon));
				active = gtk_tree_model_iter_next(model_servers, &iter);
			}
			if (servers->len)
				g_string_erase(servers, 0, 2);

			GtkLabel *servers_label = GTK_LABEL(gtk_builder_get_object(up->builder, "servers_maestro"));
			gtk_label_set_text(servers_label, servers->str);
			gtk_label_set_markup(review_orientations_label, _("Changes can be made by going to Actions->servers."));
			gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page_review, _("Review"));
			g_string_free(servers, TRUE);
		} else {
			gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page_review, _("Warning"));
			gtk_label_set_markup(review_orientations_label, _("Click on <tt>Back</tt> to connect to it or on <tt>Cancel</tt> to close GêBR."));

			if (wizard_status == WIZARD_STATUS_WITHOUT_MAESTRO) {
				gtk_label_set_markup(review_first_label, _("<b><span size='large'>You need at least a connected Maestro!</span></b>"));
			} else {
				gchar *daemon_msg = g_strdup_printf(_("<b><span size='large'>You need at least a server connected to %s!</span></b>"),
				                                    gebr_maestro_server_get_address(maestro));
				gtk_label_set_markup(review_first_label, daemon_msg);
				g_free(daemon_msg);
			}
		}
	}
	else if (page == MAESTRO_INFO_PAGE) {
		g_signal_connect(GTK_BUTTON(maestro_info_button), "clicked", G_CALLBACK(on_maestro_info_button_clicked), NULL);
	}
	else if (page == MAESTRO_PAGE) {
		GtkWidget *main_maestro = GTK_WIDGET(gtk_builder_get_object(up->builder, "maestro_chooser"));
		GtkWidget *connections_info = GTK_WIDGET(gtk_builder_get_object(up->builder, "main_connection"));
		GtkWidget *main_status = GTK_WIDGET(gtk_builder_get_object(up->builder, "main_status"));
		GtkWidget *error_box = GTK_WIDGET(gtk_builder_get_object(up->builder, "error_box"));

		GtkWidget *connect_button = GTK_WIDGET(gtk_builder_get_object(up->builder, "connect_button"));

		g_signal_connect(connect_button, "clicked", G_CALLBACK(on_connect_maestro_clicked), up);
		g_signal_connect(up->maestro_entry, "activate", G_CALLBACK(on_connect_maestro_activate), up);

		gtk_widget_hide(main_status);
		gtk_widget_hide(error_box);
		gtk_widget_show(connections_info);

		GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);
		if (maestro && gebr_maestro_server_get_state(maestro) == SERVER_STATE_LOGGED)
			gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), main_maestro, TRUE);
		else
			gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), main_maestro, FALSE);

		gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), main_maestro, GTK_ASSISTANT_PAGE_CONTENT);
		gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), main_maestro, _("Choose your Maestro"));
	}
	else if (page == SERVERS_PAGE) {
		GtkTreeView *view = create_view_for_servers(up);
		if (view) {
			GtkWidget *main_servers = GTK_WIDGET(gtk_builder_get_object(up->builder, "main_servers"));
			GObject *server_add = gtk_builder_get_object(up->builder, "server_add");
			GObject *server_entry = gtk_builder_get_object(up->builder, "server_entry");

			GtkWidget *servers_label = GTK_WIDGET(gtk_builder_get_object(up->builder, "servers_label"));
			GtkWidget *main_servers_label = GTK_WIDGET(gtk_builder_get_object(up->builder, "main_servers_label"));
			GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);

			gchar *main_servers_text = g_markup_printf_escaped(_("Now you need to add <b>Servers</b> to be handled by your Maestro <b>%s</b>.\n\n"),
									   gebr_maestro_server_get_address(maestro));

			gtk_label_set_markup(GTK_LABEL(main_servers_label), main_servers_text);
			g_free(main_servers_text);

			gebr_maestro_controller_update_daemon_model(maestro, gebr.maestro_controller);
			GtkTreeModel *model = gebr_maestro_controller_get_servers_model(gebr.maestro_controller);
			gtk_tree_view_set_model(view, model);

			GtkTreeIter it;
			GtkTreeModel *store = gebr_maestro_server_get_model(maestro, FALSE, NULL);
			if (up->first_run && !gtk_tree_model_get_iter_first(store, &it)) {
				gtk_widget_hide(GTK_WIDGET(view));
				gtk_widget_show(servers_label);
				gtk_entry_set_text(GTK_ENTRY(server_entry), gebr_maestro_server_get_address(maestro));
			}

			WizardStatus wizard_status = get_wizard_status();
			if (wizard_status == WIZARD_STATUS_COMPLETE) {
				gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), main_servers, TRUE);
				gtk_widget_hide(servers_label);
			}

			g_signal_connect(GTK_BUTTON(server_add), "clicked", G_CALLBACK(on_add_server_clicked), up);
			g_signal_connect(GTK_ENTRY(server_entry), "activate", G_CALLBACK(on_entry_server_activate), up);
			g_signal_connect(server_entry, "focus-in-event", G_CALLBACK(on_entry_server_focus_in), up);
			g_signal_connect(server_entry, "focus-out-event", G_CALLBACK(on_entry_server_focus_out), up);

			gtk_widget_set_tooltip_text(GTK_WIDGET(server_entry), DEFAULT_TEXT);
		}
	}
}

static void
on_preferences_destroy(GtkWidget *window,
                       struct ui_preferences *up)
{
	g_free(up);
}

static void
on_assistant_destroy(GtkWidget *window,
                     struct ui_preferences *up)
{
	g_signal_handlers_disconnect_by_func(gebr_maestro_controller_get_maestro(gebr.maestro_controller), on_daemons_changed, up);
	g_signal_handlers_disconnect_by_func(gebr.maestro_controller, on_maestro_state_changed, up);
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
}

static void
on_combo_set_text(GtkCellLayout   *cell_layout,
                  GtkCellRenderer *cell,
                  GtkTreeModel    *tree_model,
                  GtkTreeIter     *iter,
                  gpointer 	   data)
{
	gchar *description;

	gtk_tree_model_get(tree_model, iter,
	                   MAESTRO__DEFAULT_DESCRIPTION, &description,
	                   -1);

	gchar *text = g_markup_printf_escaped("<i>%s</i>", description);

	g_object_set(cell, "markup", text, NULL);

	g_free(text);
	g_free(description);
}

static void
set_maestro_chooser_page(GtkBuilder *builder,
                         struct ui_preferences *up)
{
	const gchar *maestros_default = g_getenv("GEBR_DEFAULT_MAESTRO");
	GtkComboBox *combo = GTK_COMBO_BOX(gtk_builder_get_object(builder, "maestro_combo"));
	GtkEntry *entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo)));
	up->maestro_entry = entry;

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
				   MAESTRO__DEFAULT_DESCRIPTION, _("Current maestro"),
				   -1);
	}
	if (maestros_default) {
		gchar **options = g_strsplit(maestros_default, ";", -1);
		for (gint i = 0; options[i] && *options[i]; i++) {
			gchar **m = g_strsplit(options[i], ",", -1);

			gtk_list_store_append(model, &iter);
			gtk_list_store_set(model, &iter,
			                   MAESTRO_DEFAULT_ADDR, m[0],
			                   MAESTRO__DEFAULT_DESCRIPTION, m[1],
			                   -1);

			g_strfreev(m);
		}
		g_strfreev(options);

		if (!maestro && g_strcmp0(gebr.config.maestro_address->str, "")) {
			gtk_list_store_append(model, &iter);
			gtk_list_store_set(model, &iter,
			                   MAESTRO_DEFAULT_ADDR, gebr.config.maestro_address->str,
			                   MAESTRO__DEFAULT_DESCRIPTION, _("Suggested maestro"),
			                   -1);
		}
	} else {
		gtk_list_store_append(model, &iter);
		gtk_list_store_set(model, &iter,
		                   MAESTRO_DEFAULT_ADDR, gebr.config.maestro_address->str,
		                   MAESTRO__DEFAULT_DESCRIPTION, _("Default Maestro from File"),
		                   -1);
	}

	gtk_combo_box_set_model(combo, GTK_TREE_MODEL(model));
	gtk_combo_box_entry_set_text_column(GTK_COMBO_BOX_ENTRY(combo), MAESTRO_DEFAULT_ADDR);
	gtk_combo_box_set_active(combo, 0);
}

/**
 * Assembly preference window.
 *
 * \return The structure containing relevant data. It will be automatically freed when the dialog closes.
 */
struct ui_preferences *
preferences_setup_ui(gboolean first_run,
                     gboolean wizard_run)
{
	struct ui_preferences *ui_preferences;

	ui_preferences = g_new(struct ui_preferences, 1);
	ui_preferences->first_run = first_run;
	ui_preferences->cancel_assistant = FALSE;
	ui_preferences->maestro_addr = NULL;

	/* Load pages from Glade */
	GtkBuilder *builder = gtk_builder_new();
	const gchar *glade_file = GEBR_GLADE_DIR "/gebr-connections-settings.glade";
	gtk_builder_add_from_file(builder, glade_file, NULL);

	ui_preferences->builder = builder;

	GtkWidget *page_preferences = GTK_WIDGET(gtk_builder_get_object(builder, "main_preferences"));
	GtkWidget *page_minfo = GTK_WIDGET(gtk_builder_get_object(builder, "maestro_info"));
	GtkWidget *main_maestro = GTK_WIDGET(gtk_builder_get_object(builder, "maestro_chooser"));
	GtkWidget *page_review = GTK_WIDGET(gtk_builder_get_object(builder, "review"));
	GtkWidget *servers_info = GTK_WIDGET(gtk_builder_get_object(builder, "servers_info"));
	GtkWidget *main_servers = GTK_WIDGET(gtk_builder_get_object(builder, "main_servers"));

	/* Create Wizard if the first_run of GeBR */
	if (first_run || wizard_run) {
		GtkWidget *assistant = gtk_assistant_new();
		gtk_window_set_position(GTK_WINDOW(assistant), GTK_WIN_POS_CENTER_ON_PARENT);
		gtk_window_set_title(GTK_WINDOW(assistant), _("Configuring GêBR"));
		gtk_window_set_transient_for(GTK_WINDOW(assistant), gebr_maestro_controller_get_window(gebr.maestro_controller));

		g_signal_connect(assistant, "destroy", G_CALLBACK(on_assistant_destroy), ui_preferences);
		g_signal_connect(assistant, "cancel", G_CALLBACK(on_assistant_cancel), ui_preferences);
		g_signal_connect(assistant, "close", G_CALLBACK(on_assistant_close), ui_preferences);
		g_signal_connect(assistant, "prepare", G_CALLBACK(on_assistant_prepare), ui_preferences);
		g_signal_connect(assistant, "apply", G_CALLBACK(on_assistant_apply), ui_preferences);

		// CANCEL_PAGE
		gtk_assistant_append_page(GTK_ASSISTANT(assistant), page_review);
		gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page_review, TRUE);
		gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page_review, GTK_ASSISTANT_PAGE_SUMMARY);
		gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page_review, _("Warning"));

		// PREFERENCES_PAGE
		gtk_assistant_append_page(GTK_ASSISTANT(assistant), page_preferences);
		gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page_preferences, TRUE);
		gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page_preferences, GTK_ASSISTANT_PAGE_INTRO);
		gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page_preferences, _("Preferences"));

		// MAESTRO_INFO_PAGE
		gtk_assistant_append_page(GTK_ASSISTANT(assistant), page_minfo);
		gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page_minfo, TRUE);
		gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page_minfo, GTK_ASSISTANT_PAGE_CONTENT);
		gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page_minfo, _("Maestro"));

		// MAESTRO_PAGE
		gtk_assistant_append_page(GTK_ASSISTANT(assistant), main_maestro);
		gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), main_maestro, FALSE);
		gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), main_maestro, GTK_ASSISTANT_PAGE_CONTENT);
		gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), main_maestro, _("Choose your Maestro"));

		// SERVERS_INFO_PAGE
		gtk_assistant_append_page(GTK_ASSISTANT(assistant), servers_info);
		gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), servers_info, TRUE);
		gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), servers_info, GTK_ASSISTANT_PAGE_CONTENT);
		gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), servers_info, _("Servers Information"));

		// SERVERS_PAGE
		gtk_assistant_append_page(GTK_ASSISTANT(assistant), main_servers);
		gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), main_servers, FALSE);
		gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), main_servers, GTK_ASSISTANT_PAGE_CONFIRM);
		gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), main_servers, _("Servers"));

		ui_preferences->prev_page = 1;
		ui_preferences->back_button = gtk_button_new_with_mnemonic("_Back");
		gtk_assistant_add_action_widget(GTK_ASSISTANT(assistant), ui_preferences->back_button);
		g_signal_connect(ui_preferences->back_button, "clicked", G_CALLBACK(on_assistant_back_button), ui_preferences);

		ui_preferences->dialog = assistant;

		/* Set Preferences Page */
		set_preferences_page(builder, ui_preferences);

		/* Set Maestro Chooser Page */
		set_maestro_chooser_page(builder, ui_preferences);

		/* finally... */
		gtk_widget_show_all(ui_preferences->dialog);

		/* Goto second page, because first used for error */
		gtk_assistant_set_current_page(GTK_ASSISTANT(assistant), PREFERENCES_PAGE);
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

		ui_preferences->dialog = window;

		/* Set Preferences Page */
		set_preferences_page(builder, ui_preferences);

		/* finally... */
		gtk_widget_show_all(ui_preferences->dialog);
	}

	return ui_preferences;
}
