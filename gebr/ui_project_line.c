/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>

#include <glib/gstdio.h>

#include <glib/gi18n.h>
#include <libgebr/date.h>
#include <libgebr/utils.h>
#include <libgebr/gebr-tar.h>
#include <libgebr/geoxml/document.h>
#include <libgebr/gui/gebr-gui-utils.h>
#include <libgebr/gui/gebr-gui-save-dialog.h>
#include <libgebr/comm/gebr-comm-protocol.h>

#include "ui_project_line.h"
#include "ui_document.h"
#include "gebr.h"
#include "line.h"
#include "document.h"
#include "project.h"
#include "line.h"
#include "flow.h"
#include "ui_help.h"
#include "callbacks.h"

typedef struct {
	const gchar *path;
	GtkDialog *dialog;
	GtkLabel *label;
	GtkProgressBar *progress;
	gint var;
	GList *line_paths_creation_sugest;
} TimeoutData;

/*
 * Prototypes
 */

static void line_info_update(void);

static void project_line_load(void);

static void pl_change_selection_update_validator(GtkTreeSelection *selection);

static void project_line_on_row_activated (GtkTreeView * tree_view,
					   GtkTreePath * path,
					   GtkTreeViewColumn * column,
					   struct ui_project_line *ui_project_line);

static GtkMenu *project_line_popup_menu (GtkWidget * widget,
					 struct ui_project_line *ui_project_line);

static gboolean line_reorder (GtkTreeView *tree_view,
			      GtkTreeIter *source_iter,
			      GtkTreeIter *target_iter,
			      GtkTreeViewDropPosition drop_position);

static gboolean line_can_reorder (GtkTreeView *tree_view,
				  GtkTreeIter *source_iter,
				  GtkTreeIter *target_iter,
				  GtkTreeViewDropPosition drop_position);

static void on_maestro_state_change(GebrMaestroController *mc,
                                    GebrMaestroServer *maestro,
                                    GebrUiProjectLine *upl);

static void update_control_sensitive(GebrUiProjectLine *upl);

static void on_maestro_button_clicked(GtkButton *button,
                                      GebrUiProjectLine *upl);

void save_maestro_changed(GebrUiProjectLine *upl, const gchar *change_nfsid);

void
gebr_project_line_hide(GebrUiProjectLine *self)
{
	return;
}

void
gebr_project_line_show(GebrUiProjectLine *self)
{
	project_line_load();
	return;
}

struct ui_project_line *project_line_setup_ui(void)
{
	struct ui_project_line *ui_project_line;

	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
	GtkWidget *scrolled_window;
	GtkWidget *hpanel;

	/* alloc */
	ui_project_line = g_new(struct ui_project_line, 1);

	ui_project_line->servers_filter = NULL;
	ui_project_line->servers_sort = NULL;

	/* Create projects/lines ui_project_line->widget */
	ui_project_line->widget = gtk_vbox_new(FALSE, 0);
	hpanel = gtk_hpaned_new();
	gtk_container_add(GTK_CONTAINER(ui_project_line->widget), hpanel);

	/* Left side */
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_paned_pack1(GTK_PANED(hpanel), scrolled_window, FALSE, FALSE);
	gtk_widget_set_size_request(scrolled_window, 300, -1);

	ui_project_line->store = gtk_tree_store_new(PL_N_COLUMN,
						    G_TYPE_STRING,
						    G_TYPE_STRING,
						    G_TYPE_POINTER,
						    G_TYPE_BOOLEAN);

	ui_project_line->view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ui_project_line->store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ui_project_line->view), FALSE);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (ui_project_line->view));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(ui_project_line->view),
						  (GebrGuiGtkPopupCallback) project_line_popup_menu, ui_project_line);
	gebr_gui_gtk_tree_view_set_reorder_callback(GTK_TREE_VIEW(ui_project_line->view),
						    (GebrGuiGtkTreeViewReorderCallback) line_reorder,
						    (GebrGuiGtkTreeViewReorderCallback) line_can_reorder, NULL);

	g_signal_connect(ui_project_line->view, "row-activated",
			 G_CALLBACK(project_line_on_row_activated), ui_project_line);
	gtk_container_add(GTK_CONTAINER(scrolled_window), ui_project_line->view);

	/* Projects/lines column */
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Index"), renderer, NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_project_line->view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", PL_TITLE);
	gtk_tree_view_column_add_attribute(col, renderer, "sensitive", PL_SENSITIVE);
	gebr_gui_gtk_tree_view_fancy_search(GTK_TREE_VIEW(ui_project_line->view), PL_TITLE);
	g_signal_connect(selection, "changed", G_CALLBACK(project_line_load), NULL);
	g_signal_connect(selection, "changed", G_CALLBACK(pl_change_selection_update_validator), NULL);
	g_signal_connect_swapped(selection, "changed", G_CALLBACK(update_control_sensitive), ui_project_line);
	g_signal_connect_swapped(gebr.maestro_controller, "maestro-state-changed", G_CALLBACK(update_control_sensitive), ui_project_line);
	g_signal_connect(gebr.maestro_controller, "maestro-state-changed", G_CALLBACK(on_maestro_state_change), ui_project_line);

	/* Right side */
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_paned_pack2(GTK_PANED(hpanel), scrolled_window, TRUE, FALSE);

	/* Get glade files */
	ui_project_line->info.builder_proj = gtk_builder_new();
	ui_project_line->info.builder_line = gtk_builder_new();
	gtk_builder_add_from_file(ui_project_line->info.builder_proj, GEBR_GLADE_DIR "/project-properties.glade", NULL);
	gtk_builder_add_from_file(ui_project_line->info.builder_line, GEBR_GLADE_DIR "/line-properties.glade", NULL);

	GtkWidget *infopage = gtk_vbox_new(FALSE, 0);
	GObject *infopage_proj = gtk_builder_get_object(ui_project_line->info.builder_proj, "main");
	GObject *infopage_line = gtk_builder_get_object(ui_project_line->info.builder_line, "main");

	GtkWidget *warn_label = gtk_label_new(NULL);
	gtk_widget_set_sensitive(warn_label, FALSE);
	ui_project_line->info.warn_label = warn_label;

	GtkButton *maestro_button = GTK_BUTTON(gtk_builder_get_object(ui_project_line->info.builder_line, "maestro_button"));
	g_signal_connect(maestro_button, "clicked", G_CALLBACK(on_maestro_button_clicked), ui_project_line);

	gtk_box_pack_start(GTK_BOX(infopage), GTK_WIDGET(infopage_proj), FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(infopage), GTK_WIDGET(infopage_line), FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(infopage), warn_label, TRUE, TRUE, 0);

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), infopage);
	return ui_project_line;
}

static void
on_connect_line_maestro(GtkWidget *widget,
			GtkWidget *dialog)
{
	gchar *connect_nfsid = gebr_geoxml_line_get_maestro(gebr.line);
	const gchar *addr = gebr_maestro_settings_get_addr_for_domain(gebr.config.maestro_set, connect_nfsid, 0);

	gebr_maestro_controller_connect(gebr.maestro_controller, addr);

	gtk_dialog_response(GTK_DIALOG(dialog), 0);
}

static void
on_change_line_maestro_disconnected(GtkWidget *widget,
                                    GtkWidget *dialog)
{
	on_configure_servers_activate();
	gtk_widget_destroy(dialog);
	on_maestro_button_clicked(NULL, NULL);
}

static void
on_change_line_maestro(GtkWidget *widget,
		       GtkWidget *dialog)
{
	const gchar *change_nfsid;
	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);
	change_nfsid = gebr_maestro_server_get_nfsid(maestro);
	save_maestro_changed(gebr.ui_project_line, change_nfsid);
	gtk_dialog_response(GTK_DIALOG(dialog), 0);
}

static void
on_maestro_button_clicked(GtkButton *button,
                          GebrUiProjectLine *upl)
{
	if (!gebr.line)
		return;

	GebrMaestroServer *maestro_line = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line);
	if (maestro_line && gebr_maestro_server_get_state(maestro_line) == SERVER_STATE_LOGGED)
		return;

	GtkBuilder *builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, GEBR_GLADE_DIR "/connect-maestro-dialog.glade", NULL);
	GObject *dialog = gtk_builder_get_object(builder, "main");
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gebr.window));
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);

	GObject *connect_button = gtk_builder_get_object(builder, "connect_maestro");
	GObject *change_button = gtk_builder_get_object(builder, "change_maestro");
	GObject *connect_label = gtk_builder_get_object(builder, "label_connect_maestro");
	GObject *change_label = gtk_builder_get_object(builder, "label_change_maestro");
	GObject *head_label = gtk_builder_get_object(builder, "label_header");
	GObject *image_change_button = gtk_builder_get_object(builder, "image_change_maestro");

	const gchar *connect_nfsid = gebr_geoxml_line_get_maestro(gebr.line);
	const gchar *nfslabel = gebr_maestro_settings_get_label_for_domain(gebr.config.maestro_set, connect_nfsid, TRUE);

	g_signal_connect(connect_button, "clicked", G_CALLBACK(on_connect_line_maestro), dialog);

	/* Head Label */
	gchar *new_head;
	new_head = g_markup_printf_escaped(_("<b><span size=\"large\">This line does not belong to any domain</span></b>"));

	if (g_strcmp0(connect_nfsid, "") != 0)
		new_head = g_markup_printf_escaped(_("<b><span size=\"large\">This line belongs to domain %s</span></b>"), nfslabel);

	gtk_label_set_markup(GTK_LABEL(head_label), new_head);

	/* Change maestro */
	const gchar *change_nfslabel;
	gchar *change_text;
	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);
	if (maestro && gebr_maestro_server_get_state(maestro) == SERVER_STATE_LOGGED) {
		change_nfslabel = gebr_maestro_server_get_nfs_label(maestro);
		change_text = g_markup_printf_escaped(_("Bring this line to <b>%s</b>"), change_nfslabel);
		gtk_label_set_markup(GTK_LABEL(change_label), change_text);

		g_signal_connect(change_button, "clicked", G_CALLBACK(on_change_line_maestro), dialog);
	} else {
		gtk_image_set_from_stock(GTK_IMAGE(image_change_button), "gtk-disconnect", GTK_ICON_SIZE_DND);
		change_text = g_markup_printf_escaped(_("Connect to another domain"));
		gtk_label_set_markup(GTK_LABEL(change_label), change_text);
		g_signal_connect(change_button, "clicked", G_CALLBACK(on_change_line_maestro_disconnected), dialog);
	}

	/* Connect maestro */
	gchar *connect_text;
	if (g_strcmp0(connect_nfsid, "") != 0) {
		connect_text = g_markup_printf_escaped(_("Connect to <b>%s</b>"), nfslabel);
	} else {
		connect_text = g_strdup(_("This Line does not belong to any domain"));
		gtk_widget_set_sensitive(GTK_WIDGET(connect_button), FALSE);
	}

	gtk_label_set_markup(GTK_LABEL(connect_label), connect_text);
	gtk_widget_show(GTK_WIDGET(dialog));
	gtk_dialog_run(GTK_DIALOG(dialog));

	gtk_widget_destroy(GTK_WIDGET(dialog));
	g_object_unref(builder);
	g_free(new_head);
	g_free(connect_text);
	g_free(change_text);
}

static void
gebr_document_send_path_message(GebrGeoXmlLine *line,
                                gint option,
                                const gchar *old_base)
{
	gchar ***paths = gebr_geoxml_line_get_paths(line);
	GString *buffer = g_string_new(NULL);

	for (gint i = 0; paths[i]; i++) {
		if (g_strcmp0(paths[i][1], "IMPORT") == 0)
			continue;
		gchar *resolved = gebr_resolve_relative_path(paths[i][0], paths);
		gchar *escaped = gebr_geoxml_escape_path(resolved);
		g_string_append_c(buffer, ',');
		g_string_append(buffer, escaped);
		g_free(escaped);
		g_free(resolved);
	}
	if (buffer->len)
		g_string_erase(buffer, 0, 1);

	gebr_pairstrfreev(paths);

	GebrMaestroServer *maestro_server = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, line);
	if (!maestro_server)
		return;
	GebrCommServer *comm_server = gebr_maestro_server_get_server(maestro_server);

	gebr_comm_protocol_socket_oldmsg_send(comm_server->socket, FALSE,
	                                      gebr_comm_protocol_defs.path_def, 3,
	                                      buffer->str,
	                                      old_base,
	                                      gebr_comm_protocol_path_enum_to_str (option));

	g_string_free(buffer, TRUE);
}

void
save_maestro_changed(GebrUiProjectLine *upl, const gchar *change_nfsid)
{
	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);
	const gchar *label = gebr_maestro_server_get_nfs_label(maestro);
	const gchar *header = g_markup_printf_escaped(_("Are you sure you want to bring this line to %s?"), label);

	gboolean confirm = gebr_gui_message_dialog(GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
						   NULL,
						   _("Bring this line to current domain?"), header,
						   _("If you choose to bring this line to current domain,"
						     " be sure to correct the paths of this line "
						     "and its respective flows that can be broken."));
	if (confirm) {
		gebr_geoxml_line_set_maestro(gebr.line, change_nfsid);
		GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_nfsid(gebr.maestro_controller, change_nfsid);
		const gchar *home = gebr_maestro_server_get_home_dir(maestro);
		gebr_geoxml_line_set_path_by_name(gebr.line, "HOME", home);
		gebr_document_send_path_message(gebr.line, GEBR_COMM_PROTOCOL_PATH_CREATE, NULL);

		document_save(GEBR_GEOXML_DOCUMENT(gebr.line), TRUE, FALSE);

		GtkTreeIter iter;
		GtkTreeModel *model;
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(upl->view));
		GList *paths = gtk_tree_selection_get_selected_rows(selection, &model);
		gtk_tree_model_get_iter(model, &iter, paths->data);
		gtk_tree_store_set(upl->store, &iter, PL_SENSITIVE, TRUE, -1);

		project_line_load();
		update_control_sensitive(upl);
	}
	line_info_update();
}




static void
line_info_update(void)
{
	if (!gebr.ui_project_line->info.builder_line)
		return;
	if (!gebr.line)
		return;

	GObject *label_title = gtk_builder_get_object(gebr.ui_project_line->info.builder_line, "label_title");
	GObject *linkbutton_email = gtk_builder_get_object(gebr.ui_project_line->info.builder_line, "linkbutton_email");
	GObject *label_description = gtk_builder_get_object(gebr.ui_project_line->info.builder_line, "label_description");
	GObject *label_changed = gtk_builder_get_object(gebr.ui_project_line->info.builder_line, "label_changed");
	GObject *label_created = gtk_builder_get_object(gebr.ui_project_line->info.builder_line, "label_created");
	GObject *label_nflows = gtk_builder_get_object(gebr.ui_project_line->info.builder_line, "label_nflows");
	gchar *tmp;

	GObject *image_maestro_connect = gtk_builder_get_object(gebr.ui_project_line->info.builder_line, "maestro_connect_img");
	GObject *maestro_button = gtk_builder_get_object(gebr.ui_project_line->info.builder_line, "maestro_button");
	GObject *image_maestro = gtk_builder_get_object(gebr.ui_project_line->info.builder_line, "image_maestro");
	GObject *maestro_label = gtk_builder_get_object(gebr.ui_project_line->info.builder_line, "label_maestro");

	GObject *label_home = gtk_builder_get_object(gebr.ui_project_line->info.builder_line, "label_home");
	GObject *label_base = gtk_builder_get_object(gebr.ui_project_line->info.builder_line, "label_base");
	GObject *label_import = gtk_builder_get_object(gebr.ui_project_line->info.builder_line, "label_import");

	/* Set title */
	tmp = g_markup_printf_escaped("<span size='xx-large'>%s</span>", gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(gebr.line)));
	gtk_label_set_markup(GTK_LABEL(label_title), tmp);
	g_free(tmp);

	/* Set email/Author */
	gchar *email_text = gebr_geoxml_document_get_email(GEBR_GEOXML_DOCUMENT(gebr.line));
	if (!*email_text){
		gtk_link_button_set_uri(GTK_LINK_BUTTON(linkbutton_email), "");
		gtk_widget_set_tooltip_text(GTK_WIDGET(linkbutton_email),NULL);
		tmp = g_strconcat(gebr_geoxml_document_get_author(GEBR_GEOXML_DOCUMENT(gebr.line)), NULL);
		gtk_widget_set_sensitive (GTK_WIDGET(linkbutton_email), FALSE);
		gtk_button_set_label(GTK_BUTTON(linkbutton_email), tmp);
		g_free(tmp);
	}
	else {
		gtk_widget_set_tooltip_text(GTK_WIDGET(linkbutton_email), email_text);
		tmp = g_strconcat("mailto:", email_text, NULL);
		gtk_link_button_set_uri(GTK_LINK_BUTTON(linkbutton_email), tmp);
		g_free(tmp);

		tmp = g_strconcat(gebr_geoxml_document_get_author(GEBR_GEOXML_DOCUMENT(gebr.line)), " <", gebr_geoxml_document_get_email(GEBR_GEOXML_DOCUMENT(gebr.line)), ">", NULL);
		gtk_widget_set_sensitive (GTK_WIDGET(linkbutton_email), TRUE);
		gtk_button_set_label(GTK_BUTTON(linkbutton_email), tmp);
		g_free(tmp);
	}
	g_free(email_text);
	/* Set description */
	gchar *description = gebr_geoxml_document_get_description(GEBR_GEOXML_DOC(gebr.line));
	if (!description || !*description){
		tmp = g_markup_printf_escaped(_("<span size='x-large'>No description available</span>"));
		gtk_widget_set_sensitive(GTK_WIDGET(label_description), FALSE);
	}
	else {
		tmp = g_markup_printf_escaped("<span size='large'><i>%s</i></span>",
		                                 gebr_geoxml_document_get_description(GEBR_GEOXML_DOCUMENT(gebr.line)));
		gtk_widget_set_sensitive(GTK_WIDGET(label_description), TRUE);
	}
	gtk_label_set_markup(GTK_LABEL(label_description), tmp);
	g_free(tmp);
	g_free(description);
	/* Set dates */
	gtk_label_set_text(GTK_LABEL(label_changed),
			   gebr_localized_date(gebr_geoxml_document_get_date_modified(GEBR_GEOXML_DOCUMENT(gebr.line))));

	gtk_label_set_text(GTK_LABEL(label_created),
			   gebr_localized_date(gebr_geoxml_document_get_date_created(GEBR_GEOXML_DOCUMENT(gebr.line))));

	/* Set number of flows */
	gint nflows = gebr_geoxml_line_get_flows_number(gebr.line);
	if (nflows == 0)
		tmp = g_markup_escape_text(_("This line has no flows."), -1);
	else if (nflows == 1)
		tmp = g_markup_escape_text(_("This line has one flow."), -1);
	else
		tmp = g_markup_printf_escaped(_("This line has %d flows."), nflows);
	gtk_label_set_markup(GTK_LABEL(label_nflows), tmp);
	g_free(tmp);


	/* Line's Maestro information */

	const gchar *nfs_label;
	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, GEBR_GEOXML_LINE(gebr.line));
	if (maestro) {
		nfs_label = gebr_maestro_server_get_nfs_label(maestro);
	} else {
		const gchar *nfsline = gebr_geoxml_line_get_maestro(gebr.line);
		nfs_label = gebr_maestro_settings_get_label_for_domain(gebr.config.maestro_set, nfsline, TRUE);
	}

	gchar *text;
	if (nfs_label && *nfs_label)
		text = g_markup_printf_escaped(_("On <b>%s</b>"), nfs_label);
	else
		text = g_strdup("");

	gtk_label_set_markup(GTK_LABEL(maestro_label), text);
	g_free(text);

	const gchar *stockid;
	gchar *home_path;
	gchar *tooltip;

	if (maestro) {
		if (gebr_maestro_server_get_state(maestro) == SERVER_STATE_LOGGED) {
			gtk_widget_show(GTK_WIDGET(image_maestro_connect));
			gtk_widget_hide(GTK_WIDGET(maestro_button));
			stockid = GTK_STOCK_CONNECT;
		} else {
			const gchar *type;
			const gchar *msg;
			gebr_maestro_server_get_error(maestro, &type, &msg);

			if (g_strcmp0(type, "error:none") == 0) {
				stockid = GTK_STOCK_DISCONNECT;
				tooltip = g_strdup(_("Click here to connect or change the domain for this line"));
			} else {
				stockid = GTK_STOCK_DIALOG_WARNING;
				tooltip = gebr_maestro_server_translate_error(type, msg);
			}
			gtk_widget_show(GTK_WIDGET(maestro_button));
			gtk_widget_hide(GTK_WIDGET(image_maestro_connect));
			gtk_image_set_from_stock(GTK_IMAGE(image_maestro), stockid, GTK_ICON_SIZE_DIALOG);
			gtk_widget_set_tooltip_text(GTK_WIDGET(maestro_button), tooltip);
			g_free(tooltip);
		}
		home_path = g_strdup(gebr_maestro_server_get_home_dir(maestro));
	} else {
		stockid = GTK_STOCK_DISCONNECT;
		tooltip = g_strdup(_("Click here to connect or change the domain for this line"));
		home_path = g_strdup("");
		gtk_widget_show(GTK_WIDGET(maestro_button));
		gtk_widget_hide(GTK_WIDGET(image_maestro_connect));
		gtk_image_set_from_stock(GTK_IMAGE(image_maestro), stockid, GTK_ICON_SIZE_DIALOG);
		gtk_widget_set_tooltip_text(GTK_WIDGET(maestro_button), tooltip);
		g_free(tooltip);
	}

	/* Line's paths information */
	gchar ***paths = gebr_geoxml_line_get_paths(gebr.line);
	gchar *base_path = NULL;
	gchar *import_path = NULL;

	for (gint i = 0; paths[i]; i++) {
		if (base_path && import_path)
			break;
		else if (g_strcmp0(paths[i][1], "BASE") == 0)
			base_path = g_strdup(paths[i][0]);
		else if (g_strcmp0(paths[i][1], "IMPORT") == 0)
			import_path = g_strdup(paths[i][0]);
	}
	if (!base_path || !*base_path)
		base_path = g_strdup(_("None"));
	if (!import_path || !*import_path)
		import_path = g_strdup(_("None"));
	if (!home_path || !*home_path)
		home_path = g_strdup(_("None"));

	gtk_label_set_text(GTK_LABEL(label_home), home_path);
	gtk_label_set_text(GTK_LABEL(label_base), base_path);
	gtk_label_set_text(GTK_LABEL(label_import), import_path);

	gebr_pairstrfreev(paths);
	g_free(base_path);
	g_free(import_path);
	g_free(home_path);
}

static void
project_info_update(void)
{
	if (!gebr.ui_project_line->info.builder_proj)
			return;
	if (!gebr.project)
		return;

	GObject *label_title = gtk_builder_get_object(gebr.ui_project_line->info.builder_proj, "label_title");
	GObject *linkbutton_email = gtk_builder_get_object(gebr.ui_project_line->info.builder_proj, "linkbutton_email");
	GObject *label_description = gtk_builder_get_object(gebr.ui_project_line->info.builder_proj, "label_description");
	GObject *label_changed = gtk_builder_get_object(gebr.ui_project_line->info.builder_proj, "label_changed");
	GObject *label_created = gtk_builder_get_object(gebr.ui_project_line->info.builder_proj, "label_created");
	GObject *label_nlines = gtk_builder_get_object(gebr.ui_project_line->info.builder_proj, "label_nlines");
	gchar *tmp;

	/* Set title */
	tmp = g_markup_printf_escaped("<span size='xx-large'>%s</span>", gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(gebr.project)));
	gtk_label_set_markup(GTK_LABEL(label_title), tmp);
	g_free(tmp);

	/* Set email/author */
	gchar *email_text = gebr_geoxml_document_get_email(GEBR_GEOXML_DOCUMENT(gebr.project));
	if(!*email_text){
		gtk_link_button_set_uri(GTK_LINK_BUTTON(linkbutton_email), "");
		gtk_widget_set_tooltip_text(GTK_WIDGET(linkbutton_email),NULL);
		tmp = g_strconcat(gebr_geoxml_document_get_author(GEBR_GEOXML_DOCUMENT(gebr.project)), NULL);
		gtk_widget_set_sensitive (GTK_WIDGET(linkbutton_email), FALSE);
		gtk_button_set_label(GTK_BUTTON(linkbutton_email), tmp);
		g_free(tmp);
	}
	else{
		tmp = g_strconcat("mailto:", email_text, NULL);
		gtk_widget_set_tooltip_text(GTK_WIDGET(linkbutton_email), email_text);
		gtk_link_button_set_uri(GTK_LINK_BUTTON(linkbutton_email), tmp);
		g_free(tmp);

		tmp = g_strconcat(gebr_geoxml_document_get_author(GEBR_GEOXML_DOCUMENT(gebr.project)), " <", gebr_geoxml_document_get_email(GEBR_GEOXML_DOCUMENT(gebr.project)), ">", NULL);
		gtk_widget_set_sensitive (GTK_WIDGET(linkbutton_email), TRUE);
		gtk_button_set_label(GTK_BUTTON(linkbutton_email), tmp);
		g_free(tmp);
	}
	g_free(email_text);

	/* Set description */
	tmp = g_markup_printf_escaped("<span size='large'><i>%s</i></span>",
				      gebr_geoxml_document_get_description(GEBR_GEOXML_DOCUMENT(gebr.project)));
	gtk_label_set_markup(GTK_LABEL(label_description), tmp);
	g_free(tmp);

	/* Set dates */
	gtk_label_set_text(GTK_LABEL(label_changed),
			   gebr_localized_date(gebr_geoxml_document_get_date_modified(GEBR_GEOXML_DOCUMENT(gebr.project))));

	gtk_label_set_text(GTK_LABEL(label_created),
			   gebr_localized_date(gebr_geoxml_document_get_date_created(GEBR_GEOXML_DOCUMENT(gebr.project))));

	/* Set number of lines */
	gint nlines = gebr_geoxml_project_get_lines_number(gebr.project);
	if (nlines == 0)
		tmp = g_markup_escape_text(_("This project has no line."), -1);
	else if (nlines == 1)
		tmp = g_markup_escape_text(_("This project has one line."), -1);
	else
		tmp = g_markup_printf_escaped(_("This project has %d Lines."), nlines);
	gtk_label_set_markup(GTK_LABEL(label_nlines), tmp);
	g_free(tmp);
}

static void
project_line_update_warn_message(struct ui_project_line *upl)
{
	GtkTreeModel *model;

	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(upl->view));
	GList *rows = gtk_tree_selection_get_selected_rows(selection, &model);

	gint n_lines = 0;
	gint n_projs = 0;
	for(GList *i = rows; i; i = i->next) {
		if (gtk_tree_path_get_depth(i->data) == 2)
			n_lines++;
		else
			n_projs++;
	}

	gchar *message;
	if (!n_lines && !n_projs)
		message = g_strdup_printf(_("No project or line selected."));
	else if (!n_lines)
		message = g_strdup_printf(_("%d projects selected."), n_projs);
	else if (!n_projs)
		message = g_strdup_printf(_("%d lines selected."), n_lines);
	else
		message = g_strdup_printf(_("%d %s and %d %s selected."),
		                          n_projs, n_projs > 1? "projects" : "project",
		                          n_lines, n_lines > 1? "lines" : "line");

	gtk_label_set_text(GTK_LABEL(upl->info.warn_label), message);
	g_free(message);

	g_list_foreach(rows, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(rows);
}

void project_line_info_update(void)
{
	GObject *infopage_proj = gtk_builder_get_object(gebr.ui_project_line->info.builder_proj, "main");
	GObject *infopage_line = gtk_builder_get_object(gebr.ui_project_line->info.builder_line, "main");

	if (!gebr.project_line) {
		project_line_update_warn_message(gebr.ui_project_line);

		gtk_widget_hide(GTK_WIDGET(infopage_proj));
		gtk_widget_hide(GTK_WIDGET(infopage_line));
		gtk_widget_show(GTK_WIDGET(gebr.ui_project_line->info.warn_label));

		return;
	}

	gtk_widget_hide(GTK_WIDGET(gebr.ui_project_line->info.warn_label));

	if (gebr_geoxml_document_get_type(gebr.project_line) == GEBR_GEOXML_DOCUMENT_TYPE_PROJECT) {
		project_info_update();
		gtk_widget_show(GTK_WIDGET(infopage_proj));
		gtk_widget_hide(GTK_WIDGET(infopage_line));
	} else {
		line_info_update();
		gtk_widget_hide(GTK_WIDGET(infopage_proj));
		gtk_widget_show(GTK_WIDGET(infopage_line));
	}
}

gboolean project_line_get_selected(GtkTreeIter * _iter, enum ProjectLineSelectionType check_type)
{
	GList *rows;
	gboolean is_line;
	GtkTreeIter iter;
	GtkTreePath *path;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	static const gchar *no_line_selected = N_("Please select a line.");
	static const gchar *no_project_selected = N_("Please select a project.");

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gebr.ui_project_line->view));
	rows = gtk_tree_selection_get_selected_rows (selection, &model);

	if (!rows) {
		switch (check_type) {
		case DontWarnUnselection:
			break;
		case ProjectSelection:
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _(no_project_selected));
			break;
		case LineSelection:
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _(no_line_selected));
			break;
		case ProjectLineSelection:
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Please select a project or a line."));
			break;
		}
		return FALSE;
	}

	path = rows->data;
	is_line = gtk_tree_path_get_depth (path) == 2 ? TRUE : FALSE;
	gtk_tree_model_get_iter (model, &iter, path);
	g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (rows);

	if (_iter != NULL)
		*_iter = iter;

	if (check_type == LineSelection && !is_line) {
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _(no_line_selected));
		return FALSE;
	}

	if (check_type == ProjectSelection && is_line) {
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _(no_project_selected));
		return FALSE;
	}

	return TRUE;
}

/**
 * Import line with basename \p line_filename inside \p at_dir.
 * Also import its flows.
 * Return error from #document_load_path.
 */
static int
line_import(GtkTreeIter *project_iter, GebrGeoXmlLine ** line, const gchar * line_filename, const gchar * at_dir, GList **line_paths_creation_sugest)
{
	GebrGeoXmlSequence *i;
	int ret;

	gdk_threads_enter();
	if ((ret = document_load_at((GebrGeoXmlDocument**)line, line_filename, at_dir))) {
		gdk_threads_leave();
		return ret;
	}
	gdk_threads_leave();

	gebr_validator_push_document(gebr.validator, (GebrGeoXmlDocument**) line, GEBR_GEOXML_DOCUMENT_TYPE_LINE);

	gdk_threads_enter();
	document_import(GEBR_GEOXML_DOCUMENT(*line), TRUE);
	gdk_threads_leave();

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);

	if (maestro) {
		const gchar *nfsid = gebr_maestro_server_get_nfsid(maestro);
		gebr_geoxml_line_set_maestro(*line, nfsid);

		const gchar *home = gebr_maestro_server_get_home_dir(maestro);
		gchar *mount_point = gebr_maestro_server_get_sftp_root(maestro);

		gebr_geoxml_line_set_path_by_name(*line, "HOME", home);

		gchar *base = gebr_geoxml_line_get_path_by_name(*line, "BASE");
		if (!base || !*base) {
			gchar *title = gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(*line));
			gchar *line_key = gebr_geoxml_line_create_key(title);

			if (base)
				g_free(base);

			base = g_build_filename(home, "GeBR", line_key, NULL);

			g_free(title);
			g_free(line_key);

			gebr_geoxml_line_set_base_path(*line, base);
		}

		/* check for paths that could be created; */
		gchar ***pvector = gebr_geoxml_line_get_paths(*line);
		GebrGeoXmlSequence *line_path;
		gebr_geoxml_line_get_path(*line, &line_path, 0);
		for (; line_path != NULL; gebr_geoxml_sequence_next(&line_path)) {
			gchar *path = gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(line_path));
			if (!g_strcmp0(path, home))
				continue;
			gchar *rel_path = gebr_resolve_relative_path(path, pvector);
			if (!g_list_find_custom(*line_paths_creation_sugest, rel_path, (GCompareFunc) g_strcmp0))
				*line_paths_creation_sugest = g_list_append(*line_paths_creation_sugest, g_strdup(rel_path));

			g_free(path);
			g_free(rel_path);
		}
		gebr_pairstrfreev(pvector);

		g_free(mount_point);
		g_free(base);
	}

	gebr_geoxml_line_get_flow(*line, &i, 0);

	/* To import a flow, you need his parent line access
	 * to relativise their paths.
	 */
	GebrGeoXmlLine *backup_line = gebr.line;
	gebr.line = *line;

	for (; i; gebr_geoxml_sequence_next(&i)) {
		GebrGeoXmlFlow *flow;

		gdk_threads_enter();
		int ret = document_load_at_with_parent((GebrGeoXmlDocument**)(&flow),
						       gebr_geoxml_line_get_flow_source(GEBR_GEOXML_LINE_FLOW(i)),
						       at_dir, project_iter);
		gdk_threads_leave();

		if (ret)
			continue;

		gdk_threads_enter();
		document_import(GEBR_GEOXML_DOCUMENT(flow), FALSE);
		gdk_threads_leave();
		gebr_geoxml_line_set_flow_source(GEBR_GEOXML_LINE_FLOW(i),
						 gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(flow)));
		gdk_threads_enter();
		gebr_validator_push_document(gebr.validator, (GebrGeoXmlDocument**) &flow, GEBR_GEOXML_DOCUMENT_TYPE_FLOW);
		gebr_geoxml_flow_revalidate(flow, gebr.validator);

		/* Reset last date run */
		gebr_geoxml_flow_set_date_last_run(flow, "");

		document_save(GEBR_GEOXML_DOCUMENT(flow), FALSE, FALSE);
		gebr_validator_pop_document(gebr.validator, GEBR_GEOXML_DOCUMENT_TYPE_FLOW);
		gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
		gdk_threads_leave();
	}
	gdk_threads_enter();

	gebr.line = backup_line;

	document_save(GEBR_GEOXML_DOCUMENT(*line), FALSE, FALSE);
	gdk_threads_leave();

	gebr_validator_pop_document(gebr.validator, GEBR_GEOXML_DOCUMENT_TYPE_LINE);

	return ret;
}

struct ProjectLineImportPathData {
	gboolean is_project;
	GtkTreeIter iter;
	GebrTar *tar;
	GList **line_paths_creation_sugest;
};

static void
document_import_single (const gchar *path,
			struct ProjectLineImportPathData *data)
{
	GebrGeoXmlDocument *document;

	if (data->is_project && g_str_has_suffix(path, ".prj")) {
		GebrGeoXmlProject *project;
		GebrGeoXmlSequence *project_line;

		gdk_threads_enter();
		if (document_load_path((GebrGeoXmlDocument**)(&project), path)) {
			gdk_threads_leave();
			return;
		}
		gdk_threads_leave();

		gdk_threads_enter();
		document_import(GEBR_GEOXML_DOCUMENT(project), TRUE);
		data->iter = project_append_iter(project);
		gdk_threads_leave();

		gebr_validator_push_document(gebr.validator, (GebrGeoXmlDocument**) &project, GEBR_GEOXML_DOCUMENT_TYPE_PROJECT);

		gebr_geoxml_project_get_line(project, &project_line, 0);
		for (; project_line; gebr_geoxml_sequence_next(&project_line)) {
			GebrGeoXmlLine *line;

			int ret = line_import(&data->iter, &line, gebr_geoxml_project_get_line_source
					      (GEBR_GEOXML_PROJECT_LINE(project_line)), gebr_tar_get_dir(data->tar),
					      data->line_paths_creation_sugest);
			if (ret)
				continue;

			gebr_geoxml_project_set_line_source(GEBR_GEOXML_PROJECT_LINE(project_line),
							    gebr_geoxml_document_get_filename
							    (GEBR_GEOXML_DOCUMENT(line)));

			gdk_threads_enter();
			project_append_line_iter(&data->iter, line);
			document_save(GEBR_GEOXML_DOCUMENT(line), FALSE, FALSE);
			gdk_threads_leave();
		}
		gebr_validator_pop_document(gebr.validator, GEBR_GEOXML_DOCUMENT_TYPE_PROJECT);
		document = GEBR_GEOXML_DOCUMENT(project);
	} else if (!data->is_project && g_str_has_suffix(path, ".lne")) {
		GebrGeoXmlLine *line;
		GtkTreeIter parent;

		gchar *filename = g_path_get_basename(path);
		line_import(&parent, &line, filename, gebr_tar_get_dir(data->tar),
			    data->line_paths_creation_sugest);
		g_free(filename);
		if (line == NULL)
			return;
		gebr_geoxml_project_append_line(gebr.project,
						gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(line)));
		gdk_threads_enter();
		document_save(GEBR_GEOXML_DOCUMENT(gebr.project), TRUE, FALSE);

		project_line_get_selected(&data->iter, DontWarnUnselection);
		parent = data->iter;
		if (!gtk_tree_model_iter_parent(GTK_TREE_MODEL(gebr.ui_project_line->store), &parent, &data->iter))
			parent = data->iter;
		data->iter = project_append_line_iter(&parent, line);
		gdk_threads_leave();

		document = GEBR_GEOXML_DOCUMENT(line);
	} else
		document = NULL;

	if (document != NULL) {
		gdk_threads_enter();
		project_line_select_iter(&data->iter);

		GString *new_title = g_string_new(NULL);

		g_string_printf(new_title, _("%s (Imported)"), gebr_geoxml_document_get_title(document));
		gtk_tree_store_set(gebr.ui_project_line->store, &data->iter, PL_TITLE, new_title->str, -1);
		gebr_geoxml_document_set_title(document, new_title->str);
		document_save(document, FALSE, FALSE);
		gdk_threads_leave();
		g_string_free(new_title, TRUE);
	}
}

static gboolean _project_line_import_path(const gchar *filename, GList **line_paths_creation_sugest)
{
	struct ProjectLineImportPathData data;

	data.line_paths_creation_sugest = line_paths_creation_sugest;
	if (g_str_has_suffix(filename, ".prjz") || g_str_has_suffix(filename, ".prjx"))
		data.is_project = TRUE;
	else if (g_str_has_suffix(filename, ".lnez") || g_str_has_suffix(filename, ".lnex")) {
		data.is_project = FALSE;
		gdk_threads_enter();
		if (!project_line_get_selected(NULL, ProjectLineSelection)) {
			gdk_threads_leave();
			goto err;
		}
		gdk_threads_leave();
	} else {
		gdk_threads_enter();
		gebr_message(GEBR_LOG_ERROR, FALSE, TRUE, _("Unrecognized file type."));
		gdk_threads_leave();
		return FALSE;
	}

	gdk_threads_enter();
	gebr.last_notebook = gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook));
	gtk_notebook_set_current_page(GTK_NOTEBOOK(gebr.notebook), NOTEBOOK_PAGE_PROJECT_LINE);
	gdk_threads_leave();

	GebrTar *tar;
	tar = gebr_tar_new_from_file (filename);
	data.tar = tar;
	if (!gebr_tar_extract (tar)) {
		gdk_threads_enter();
		gebr_message (GEBR_LOG_ERROR, TRUE, TRUE,
			      _("Could not import Flow from the file %s"), filename);
		gdk_threads_leave();
		gebr_tar_free (tar);
		goto err;
	} else
		gebr_tar_foreach (tar, (GebrTarFunc) document_import_single, &data);

	gebr_tar_free (tar);

	gdk_threads_enter();
	gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Import successful."));
	gdk_threads_leave();
	return TRUE;

err:
	gdk_threads_enter();
	gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Failed to import."));
	gdk_threads_leave();
	return FALSE;
}

void project_line_select_iter(GtkTreeIter * iter)
{
	gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(gebr.ui_project_line->view), iter);
}

static void *import_demo_thread(gpointer user_data)
{
	TimeoutData *data = user_data;

	_project_line_import_path(data->path, &(data->line_paths_creation_sugest));
	data->var = 1;
	g_thread_exit(NULL);
	return NULL;
}

static gboolean update_progress(gpointer user_data)
{
	TimeoutData *data = user_data;
	gtk_progress_bar_pulse(data->progress);

	GtkTextBuffer *text_buffer;
	GtkWidget *text_view;
	GtkWidget *scrolled_window;

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request(scrolled_window, 400, 200);
	text_buffer = gtk_text_buffer_new(NULL);
	text_view = gtk_text_view_new_with_buffer(text_buffer);
	gtk_text_view_set_editable(GTK_TEXT_VIEW (text_view), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW (text_view), FALSE);
	gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);

	if (data->var == 1) {
		//gtk_progress_bar_set_fraction(data->progress, 1);
		gtk_widget_destroy(GTK_WIDGET(data->progress));

		if (data->line_paths_creation_sugest) {

			GString *paths = g_string_new("");
			for (GList *i = data->line_paths_creation_sugest; i != NULL; i = g_list_next(i))
				g_string_append_printf(paths, "%s\n", (gchar*)i->data);

			gtk_text_buffer_insert_at_cursor (text_buffer, paths->str, paths->len);

			gchar *text = g_strdup_printf("%s\n\n%s",
			                              _("<span size='large' weight='bold'>Create directories?</span>"),
			                              _("There are some line's paths located on your home directory that\n"
			                        	" do not exist. Do you want to create the following folders?"));


			gtk_label_set_markup(data->label, text);
			g_free(text);

			gtk_dialog_add_buttons(data->dialog,
			                       GTK_STOCK_NO, GTK_RESPONSE_NO,
			                       GTK_STOCK_YES, GTK_RESPONSE_YES,
			                       NULL);
			gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(data->dialog)), scrolled_window, TRUE, TRUE, 0);

			gtk_widget_show_all(gtk_dialog_get_content_area(data->dialog));

			g_string_free(paths, TRUE);

		} else
			gtk_widget_destroy(GTK_WIDGET(data->dialog));
	}
	return data->var != 1;
}

static void
on_path_error(GebrMaestroServer *maestro,
	      GebrCommProtocolStatusPath error_id)
{
	g_signal_handlers_disconnect_by_func(maestro, on_path_error, NULL);

	if (error_id == GEBR_COMM_PROTOCOL_STATUS_PATH_OK)
		return;

	GtkWidget * warning;
	warning = gtk_message_dialog_new_with_markup (GTK_WINDOW (gebr.window),
	                                              GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	                                              GTK_MESSAGE_WARNING,
	                                              GTK_BUTTONS_OK,
	                                              "<span size='larger' weight='bold'>%s</span>",
	                                              _("Could not create the directories"));

	gchar *escaped;
	if (error_id == GEBR_COMM_PROTOCOL_STATUS_PATH_ERROR) {
		escaped = g_markup_printf_escaped(_("The directories could not be created. "
				"You do not have the permissions necessary to create the directories."));
	} else if (error_id == GEBR_COMM_PROTOCOL_STATUS_PATH_EXISTS) {
		escaped = g_markup_printf_escaped(_("The directories already exists."));
	} else {
		g_warn_if_reached();
		escaped = g_strdup("");
	}

	gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (warning), "%s", escaped);
	g_free(escaped);

	gdk_threads_enter();
	gtk_dialog_run (GTK_DIALOG (warning));
	gtk_widget_destroy (warning);
	gdk_threads_leave();
}

static void on_dialog_response(GtkWidget *dialog, gint response_id, gpointer user_data)
{
	TimeoutData *data = user_data;
	GString *buffer = g_string_new(NULL);
	if (response_id == GTK_RESPONSE_YES) {
		GString *cmd_line = g_string_new(NULL);
		for (GList *i = data->line_paths_creation_sugest; i != NULL; i = g_list_next(i)) {
			gchar *escaped = gebr_geoxml_escape_path(i->data);
			g_string_append_c(buffer, ',');
			buffer = g_string_append(buffer, escaped);
			g_free(escaped);
		}
		if (buffer->len)
			g_string_erase(buffer, 0, 1);

		GebrMaestroServer *maestro_server = gebr_maestro_controller_get_maestro(gebr.maestro_controller);
		if (maestro_server) {
			GebrCommServer *comm_server = gebr_maestro_server_get_server(maestro_server);

			gint option = GEBR_COMM_PROTOCOL_PATH_CREATE;
			gebr_comm_protocol_socket_oldmsg_send(comm_server->socket, FALSE,
							      gebr_comm_protocol_defs.path_def, 3,
							      buffer->str,
							      "",
							      gebr_comm_protocol_path_enum_to_str (option));

			g_signal_connect(maestro_server, "path-error", G_CALLBACK(on_path_error), NULL);
		}
		g_string_free(cmd_line, TRUE);
	}

	g_list_foreach(data->line_paths_creation_sugest, (GFunc)g_free, NULL);
	g_list_free(data->line_paths_creation_sugest);
	g_free(data);
	gtk_widget_destroy(dialog);
}

static gboolean on_delete_event(GtkWidget *dialog)
{
	return TRUE;
}

void project_line_import_path(const gchar *path)
{
	GtkWidget *dialog;
	GtkWidget *progress;
	GtkWidget *label;
	TimeoutData *data;
	GtkBox *box;

	data = g_new(TimeoutData, 1);
	dialog = gtk_dialog_new_with_buttons("",
	                                     GTK_WINDOW(gebr.window),
	                                     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	                                     NULL);

	g_signal_connect(dialog, "response", G_CALLBACK(on_dialog_response), data);
	g_signal_connect(dialog, "delete-event", G_CALLBACK(on_delete_event), NULL);
	gtk_window_set_deletable(GTK_WINDOW(dialog), FALSE);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 10);
	gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);

	label = gtk_label_new(_("<span size='large' weight='bold'>Importing documents, please wait...</span>"));
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	progress = gtk_progress_bar_new();
	box = GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog)));
	gtk_progress_bar_set_pulse_step(GTK_PROGRESS_BAR(progress), 0.1);
	gtk_box_set_spacing(box, 10);
	gtk_box_pack_start(box, label, TRUE, TRUE, 0);
	gtk_box_pack_start(box, progress, FALSE, TRUE, 0);

	data->dialog = GTK_DIALOG(dialog);
	data->progress = GTK_PROGRESS_BAR(progress);
	data->label = GTK_LABEL(label);
	data->var = 0;
	data->path = path;
	data->line_paths_creation_sugest = NULL;
	g_timeout_add(100, update_progress, data);
	gtk_widget_show(label);
	gtk_widget_show(progress);
	g_thread_create(import_demo_thread, data, FALSE, NULL);
	gtk_dialog_run(GTK_DIALOG(dialog));
}

void project_line_import(void)
{
	GtkWidget *chooser_dialog;
	GtkFileFilter *file_filter;
	gchar *filename;

	chooser_dialog = gtk_file_chooser_dialog_new(_("Choose a project or line to open"),
						     GTK_WINDOW(gebr.window),
						     GTK_FILE_CHOOSER_ACTION_OPEN,
						     GTK_STOCK_OPEN, GTK_RESPONSE_YES,
						     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(chooser_dialog), FALSE);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(chooser_dialog), TRUE);

	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser_dialog), g_get_home_dir());
	// FIXME: Use the global variable for adding shortcuts
	gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(chooser_dialog), "/usr/share/gebr/demos/", NULL);
	file_filter = gtk_file_filter_new();
	gtk_file_filter_set_name(file_filter, _("Project (prjz, prjx) or line (lnez, lnex)"));
	gtk_file_filter_add_pattern(file_filter, "*.prjz");
	gtk_file_filter_add_pattern(file_filter, "*.lnez");
	gtk_file_filter_add_pattern(file_filter, "*.prjx");
	gtk_file_filter_add_pattern(file_filter, "*.lnex");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_dialog), file_filter);

	/* show file chooser */
	gtk_widget_show(chooser_dialog);
	if (gtk_dialog_run(GTK_DIALOG(chooser_dialog)) == GTK_RESPONSE_YES) {
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser_dialog));
		gtk_widget_destroy(chooser_dialog);
		project_line_import_path(filename);
		g_free(filename);
	} else
		gtk_widget_destroy(chooser_dialog);
}

static void
parse_line(GebrGeoXmlDocument * _line, GebrGeoXmlDocument *proj, GString *tmpdir)
{
	gchar *filename;
	GebrGeoXmlSequence *j;
	GebrGeoXmlLine *line;

	line = GEBR_GEOXML_LINE (gebr_geoxml_document_clone (_line));

	line_set_paths_to_relative(line, TRUE);
	filename = g_build_path ("/", tmpdir->str,
				 gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(line)),
				 NULL);

	document_save_at(GEBR_GEOXML_DOCUMENT(line), filename, FALSE, FALSE, FALSE);
	g_free (filename);

	gebr_geoxml_line_get_flow(line, &j, 0);
	for (; j != NULL; gebr_geoxml_sequence_next(&j)) {
		const gchar *flow_filename;
		GebrGeoXmlFlow *flow;

		flow_filename = gebr_geoxml_line_get_flow_source(GEBR_GEOXML_LINE_FLOW(j));
		if (document_load((GebrGeoXmlDocument**)(&flow), flow_filename, FALSE))
			continue;

		gchar ***paths = gebr_geoxml_line_get_paths(GEBR_GEOXML_LINE(_line));
		flow_set_paths_to_relative(flow, line, paths, TRUE);
		gebr_pairstrfreev(paths);
		filename = g_build_path ("/", tmpdir->str, flow_filename, NULL);
		document_save_at(GEBR_GEOXML_DOCUMENT(flow), filename, FALSE, FALSE, FALSE);
		g_free (filename);

		gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
	}
	document_free(GEBR_GEOXML_DOCUMENT(line));
}

void project_line_export(void)
{
	GebrTar *tar;
	GString *tmpdir;
	const gchar *extension;
	gchar *tmp;

	GtkWidget *chooser_dialog;
	GtkFileFilter *file_filter;

	GList *rows;
	GList *lines;
	GList *projects;
	GtkTreeModel *model;
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gebr.ui_project_line->view));
	rows = gtk_tree_selection_get_selected_rows (selection, &model);

	if (!rows) {
		gebr_message (GEBR_LOG_ERROR, TRUE, FALSE,
			      _("Please select a project or a line."));
		return;
	}

	lines = NULL;
	projects = NULL;
	for (GList *i = rows; i; i = i->next) {
		GtkTreePath *path;
		path = i->data;
		if (gtk_tree_path_get_depth (path) == 1)
			projects = g_list_prepend (projects, path);
		else {
			GtkTreePath **data = g_new(GtkTreePath*, 2);
			data[0] = path;
			data[1] = gtk_tree_path_copy(path);
			gtk_tree_path_up(data[1]);
			lines = g_list_prepend (lines, data);
		}
	}
	g_list_free (rows);

	if (projects != NULL) {
		for (GList *i = lines; i; i = i->next) {
			GtkTreePath **data;
			data = i->data;
			if (!gtk_tree_selection_path_is_selected (selection, data[1]))
				gtk_tree_selection_unselect_path (selection, data[0]);
		}

		for (GList *i = projects; i; i = i->next) {
			gboolean valid;
			GtkTreePath *path;
			GtkTreeIter iter;
			GtkTreeIter child;

			path = i->data;
			gtk_tree_model_get_iter (model, &iter, path);
			valid = gtk_tree_model_iter_children (model, &child, &iter);
			while (valid) {
				gtk_tree_selection_select_iter (selection, &child);
				valid = gtk_tree_model_iter_next (model, &child);
			}
		}
	}
	
	file_filter = gtk_file_filter_new();
	if (projects) {
		gtk_file_filter_set_name(file_filter, _("Project (*.prjx)"));
		gtk_file_filter_add_pattern(file_filter, "*.prjx");
		extension = ".prjx";
	} else {
		gtk_file_filter_set_name(file_filter, _("Line (*.lnex)"));
		gtk_file_filter_add_pattern(file_filter, "*.lnex");
		extension = ".lnex";
	}

	GtkWidget *box;
	box = gtk_vbox_new(FALSE, 5);
	/* run file chooser */

	chooser_dialog = gebr_gui_save_dialog_new(_("Choose filename to save"), GTK_WINDOW(gebr.window));
	gebr_gui_save_dialog_set_default_extension(GEBR_GUI_SAVE_DIALOG(chooser_dialog), extension);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_dialog), file_filter);
	gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(chooser_dialog), box);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser_dialog), g_get_home_dir());
	gtk_widget_show_all(box);

	/* show file chooser */
	tmp = gebr_gui_save_dialog_run(GEBR_GUI_SAVE_DIALOG(chooser_dialog));
	if (!tmp)
		return;
	
	tmpdir = gebr_temp_directory_create();

	if (!projects)
		for (GList *i = lines; i; i = i->next) {
			GtkTreePath **data;
			GtkTreeIter iter;
			GebrGeoXmlDocument *line;
			GebrGeoXmlDocument *proj;

			data = i->data;
			gtk_tree_model_get_iter (model, &iter, data[0]);
			gtk_tree_model_get (model, &iter, PL_XMLPOINTER, &line, -1);
			gtk_tree_model_get_iter (model, &iter, data[1]);
			gtk_tree_model_get (model, &iter, PL_XMLPOINTER, &proj, -1);
			parse_line (line, proj, tmpdir);
			gtk_tree_path_free(data[0]);
			gtk_tree_path_free(data[1]);
			g_free(data);
		}
	else for (GList *i = projects; i; i = i->next) {
		gchar *filename;
		GtkTreeIter iter;
		GtkTreePath *path;
		GebrGeoXmlDocument *prj;
		GebrGeoXmlSequence *seq;

		path = i->data;
		gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_path_free(path);
		gtk_tree_model_get (model, &iter, PL_XMLPOINTER, &prj, -1);
		filename = g_build_path ("/",
					 tmpdir->str,
					 gebr_geoxml_document_get_filename (prj),
					 NULL);
		document_save_at (prj, filename, FALSE, FALSE, FALSE);
		g_free (filename);

		gebr_geoxml_project_get_line (GEBR_GEOXML_PROJECT (prj), &seq, 0);
		for (; seq != NULL; gebr_geoxml_sequence_next (&seq)) {
			GebrGeoXmlDocument *line;
			GebrGeoXmlProjectLine *pl;

			pl = GEBR_GEOXML_PROJECT_LINE (seq);

			if (document_load (&line, gebr_geoxml_project_get_line_source (pl), FALSE))
				continue;

			parse_line (line, NULL, tmpdir);
			document_free (line);
		}
	}

	tar = gebr_tar_create (tmp);

	if (gebr_tar_compact (tar, tmpdir->str))
		gebr_message(GEBR_LOG_INFO, TRUE, TRUE, _("Export succesful."));
	else
		gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Could not export."));
	g_list_free(lines);
	g_list_free(projects);

	g_free(tmp);
	gebr_tar_free (tar);
	gebr_temp_directory_destroy(tmpdir);
}

static gint
project_line_delete_compare_func(GtkTreeIter * iter1, GtkTreeIter * iter2)
{
	return gebr_gui_gtk_tree_model_iter_equal_to(GTK_TREE_MODEL(gebr.ui_project_line->store), iter1, iter2) == TRUE
		? 0 : 1;
}

void project_line_delete(void)
{
	GList *selected = gebr_gui_gtk_tree_view_get_selected_iters(GTK_TREE_VIEW(gebr.ui_project_line->view));
	if (selected == NULL) {
		project_line_get_selected(NULL, ProjectLineSelection); //show a message to the user
		return;
	}

	GString *delete_list = g_string_new("");
	GtkTextBuffer *text_buffer;
	gboolean can_delete = TRUE;
	gint quantity_selected = 0;
	GtkWidget *text_view;
	GtkWidget *dialog;
	gint ret;

	text_buffer = gtk_text_buffer_new(NULL);
	text_view = gtk_text_view_new_with_buffer(text_buffer);
	gtk_text_view_set_editable(GTK_TEXT_VIEW (text_view), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW (text_view), FALSE);

	for (GList *i = selected; i != NULL; i = g_list_next(i)) {
		GtkTreeIter * iter = (GtkTreeIter*)i->data;
		GtkTreeIter parent;
		gboolean is_line = gtk_tree_model_iter_parent(GTK_TREE_MODEL(gebr.ui_project_line->store), &parent, iter);
		GebrGeoXmlDocument *document;
		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->store), iter,
				   PL_XMLPOINTER, &document, -1);
		
		if (!is_line) {
			gboolean can_delete_project;
			quantity_selected++;

			if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(gebr.ui_project_line->store), iter)) {
				gboolean all_line_selected = TRUE;
				GtkTreeIter i_iter;
				gebr_gui_gtk_tree_model_foreach_child(i_iter, iter, GTK_TREE_MODEL(gebr.ui_project_line->store)) {
					gboolean found = g_list_find_custom(selected, &i_iter, (GCompareFunc)project_line_delete_compare_func) != NULL ? TRUE : FALSE;
					if (!found) {
						all_line_selected = FALSE;
						break;
					}
				}
				can_delete_project = all_line_selected;
			} else {
				can_delete_project = TRUE;
			}

			if (!can_delete_project) {
				can_delete = FALSE;
				project_delete(iter, TRUE); //will fail and show a status message to the user
				break;
			}

			g_string_append_printf(delete_list, _("Project '%s'.\n"),
					       gebr_geoxml_document_get_title(document));
		} else {
			GString *tmp = g_string_new(_("empty"));
			glong n = gebr_geoxml_line_get_flows_number(GEBR_GEOXML_LINE(document));
			quantity_selected++;

			if (n > 0)
				g_string_printf(tmp, _("including %ld flow(s)"), n);

			GtkTreeSelection *tree_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_project_line->view));
			
			if (gtk_tree_selection_iter_is_selected(tree_selection, &parent))
				g_string_append(delete_list, "  ");

			g_string_append_printf(delete_list, _("Line '%s' (%s).\n"),
					       gebr_geoxml_document_get_title(document), tmp->str);
			g_string_free(tmp, TRUE);
		}
	}
	gtk_text_buffer_insert_at_cursor(text_buffer, delete_list->str, delete_list->len);

	/* now asks the user for confirmation */
	if (!can_delete)
		goto out;
	if (quantity_selected > 1){
		GtkWidget *table = gtk_table_new(3, 2, FALSE);
		GtkWidget * image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
		GtkWidget * title = gtk_label_new("");
		GtkWidget * message = gtk_label_new(_("The following documents are about to be deleted.\nThis operation can't be undone! Are you sure?\n"));
		guint row = 0;
		char *markup = g_markup_printf_escaped("<span size='large'><b>%s</b></span>", _("Confirm multiple deletion"));
		GtkWidget *scrolled_window;
		scrolled_window = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
					       GTK_POLICY_AUTOMATIC);
		gtk_widget_set_size_request(scrolled_window, 400, 200);

		dialog = gtk_dialog_new_with_buttons(_("Confirm multiple deletion"), NULL,
						     (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
						     GTK_STOCK_NO,
						     GTK_RESPONSE_NO,
						     GTK_STOCK_YES,
						     GTK_RESPONSE_YES ,
						     NULL);
		gtk_label_set_markup (GTK_LABEL (title), markup);
		g_free (markup);
		gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);
		gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), table, TRUE, TRUE, 0);
		gtk_table_attach(GTK_TABLE(table), image, 0, 1, row, row + 1, (GtkAttachOptions)GTK_FILL,
				 (GtkAttachOptions)GTK_FILL, 3, 3);
		gtk_table_attach(GTK_TABLE(table), title, 1, 2, row, row + 1, (GtkAttachOptions)GTK_FILL,
				 (GtkAttachOptions)GTK_FILL, 3, 3), row++;

		gtk_table_attach(GTK_TABLE(table), message, 1, 2, row, row + 1, (GtkAttachOptions)GTK_FILL,
				 (GtkAttachOptions)GTK_FILL, 3, 3), row++;

		gtk_table_attach(GTK_TABLE(table), scrolled_window, 1, 2, row, row + 1, (GtkAttachOptions)GTK_FILL,
				 (GtkAttachOptions)GTK_EXPAND
				 , 3, 3), row++;

		gtk_widget_show_all(gtk_dialog_get_content_area(GTK_DIALOG(dialog)));
		ret = gtk_dialog_run(GTK_DIALOG(dialog));
		can_delete = (ret == GTK_RESPONSE_YES) ? TRUE : FALSE;

		gtk_widget_destroy(dialog);

	} else {
		GtkWidget *table = gtk_table_new(3, 2, FALSE);
		GtkWidget * image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
		GtkWidget * title = gtk_label_new("");
		GtkWidget * message = gtk_label_new(_("The following document is about to be deleted.\nThis operation can't be undone! Are you sure?\n"));
		guint row = 0;
		char *markup = g_markup_printf_escaped("<span size='large'><b>%s</b></span>", _("Confirm deletion"));

		dialog = gtk_dialog_new_with_buttons(_("Confirm deletion"), NULL,
						     (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
						     GTK_STOCK_NO,
						     GTK_RESPONSE_NO,
						     GTK_STOCK_YES,
						     GTK_RESPONSE_YES ,
						     NULL);
		gtk_label_set_markup (GTK_LABEL (title), markup);
		g_free (markup);

		gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), table, TRUE, TRUE, 0);
		gtk_table_attach(GTK_TABLE(table), image, 0, 1, row, row + 1, (GtkAttachOptions)GTK_FILL,
				 (GtkAttachOptions)GTK_FILL, 3, 3);
		gtk_table_attach(GTK_TABLE(table), title, 1, 2, row, row + 1, (GtkAttachOptions)GTK_FILL,
				 (GtkAttachOptions)GTK_FILL, 3, 3), row++;

		gtk_table_attach(GTK_TABLE(table), message, 1, 2, row, row + 1, (GtkAttachOptions)GTK_FILL,
				 (GtkAttachOptions)GTK_FILL, 3, 3), row++;

		gtk_table_attach(GTK_TABLE(table), text_view, 1, 2, row, row + 1, (GtkAttachOptions)GTK_FILL,
				 (GtkAttachOptions)GTK_FILL, 3, 3), row++;
		gtk_widget_show_all(gtk_dialog_get_content_area(GTK_DIALOG(dialog)));
		ret = gtk_dialog_run(GTK_DIALOG(dialog));
		can_delete = (ret == GTK_RESPONSE_YES) ? TRUE : FALSE;

		gtk_widget_destroy(dialog);
	}
	if (!can_delete)
		goto out;

	/* delete lines first, removing them from list */
	for (GList *i = selected; i != NULL; ) {
		GtkTreeIter * iter = (GtkTreeIter*)i->data;
		GtkTreeIter parent;
		gboolean is_line = gtk_tree_model_iter_parent(GTK_TREE_MODEL(gebr.ui_project_line->store), &parent, iter);

		if (is_line) {
			line_delete(iter, TRUE);

			GList *tmp = i;
			i = g_list_next(i);
			gtk_tree_iter_free(iter);
			selected = g_list_remove_link(selected, tmp);
		} else
			i = g_list_next(i);
	}
	/* now delete the remaining empty projects */
	for (GList *i = selected; i != NULL; i = g_list_next(i)) {
		GtkTreeIter * iter = (GtkTreeIter*)i->data;
		project_delete(iter, TRUE);
	}

out:
	/* frees */
	g_string_free(delete_list, TRUE);
	g_list_foreach(selected, (GFunc) gtk_tree_iter_free, NULL);
	g_list_free(selected);
}

void project_line_free(void)
{
	gebr.project_line = NULL;
	gebr.project = NULL;
	gebr.line = NULL;

	GtkTreeModel *model = GTK_TREE_MODEL(gebr.ui_flow_browse->store);
	gtk_tree_view_set_model(GTK_TREE_VIEW(gebr.ui_flow_browse->view), NULL);
	gtk_tree_store_clear(gebr.ui_flow_browse->store);
	gtk_tree_view_set_model(GTK_TREE_VIEW(gebr.ui_flow_browse->view), model);
	flow_free();

	project_line_info_update();
}

static void
on_maestro_state_change(GebrMaestroController *mc,
                        GebrMaestroServer *maestro,
                        GebrUiProjectLine *upl)
{
	GebrGeoXmlLine *line;
	GtkTreeIter parent, iter;
	GtkTreeModel *model = GTK_TREE_MODEL(upl->store);

	gebr_gui_gtk_tree_model_foreach(parent, model) {
		gboolean valid = gtk_tree_model_iter_children(model, &iter, &parent);
		while (valid) {
			gtk_tree_model_get(model, &iter, PL_XMLPOINTER, &line, -1);

			gboolean sensitive;
			GebrMaestroServer *mg = gebr_maestro_controller_get_maestro(mc);

			const gchar *line_nfsid = gebr_geoxml_line_get_maestro(line);
			if (!line_nfsid || !*line_nfsid) {
				if (gebr_maestro_server_get_state(mg) == SERVER_STATE_LOGGED) {
					const gchar *nfsid = gebr_maestro_server_get_nfsid(maestro);
					if (nfsid) {
						gebr_geoxml_line_set_maestro(line, nfsid);
						document_save(GEBR_GEOXML_DOCUMENT(line), TRUE, FALSE);
					}
				}
			}

			GebrMaestroServer *m = gebr_maestro_controller_get_maestro_for_line(mc, line);

			if (m && gebr_maestro_server_get_state(m) == SERVER_STATE_LOGGED)
				sensitive = TRUE;
			else
				sensitive = FALSE;

			if (sensitive) {
				gchar *base_dir = gebr_geoxml_line_get_path_by_name(line, "BASE");

				if (!gebr_maestro_server_has_home_dir(m)) {
					gebr_geoxml_line_set_path_by_name(line, "HOME", gebr_maestro_server_get_home_dir(maestro));
					gebr_ui_document_send_paths_to_maestro(m, GEBR_COMM_PROTOCOL_PATH_CREATE,
					                                       NULL, base_dir);
				} else
					gebr_geoxml_line_set_path_by_name(line, "HOME", gebr_maestro_server_get_home_dir(maestro));

				g_free(base_dir);
			}

			gtk_tree_store_set(upl->store, &iter, PL_SENSITIVE, sensitive, -1);
			valid = gtk_tree_model_iter_next(model, &iter);

			if(mg && gebr_maestro_server_get_state(mg) == SERVER_STATE_LOGGED)
				sensitive = TRUE;
			else
				sensitive = FALSE;

			gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_project_line, "project_line_import"), sensitive);
			gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_general, "help_demos_su"), sensitive);

		}
	}
}

static void
update_control_sensitive(GebrUiProjectLine *upl)
{
	gboolean has_maestro = TRUE;
	GtkTreeModel *model;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(upl->view));
	GList *rows = gtk_tree_selection_get_selected_rows(selection, &model);
	gboolean is_multiple, multiple_projs;
	guint n_projs = 0;

	guint len = g_list_length(rows);

	is_multiple = len > 1;
	if (!rows)
		return;

	for (GList *i = rows; i; i = i->next) {
		gint depth = gtk_tree_path_get_depth(i->data);
		if (depth == 2) {
			GtkTreeIter iter;
			GebrGeoXmlLine *line;
			gtk_tree_model_get_iter(model, &iter, i->data);
			gtk_tree_model_get(model, &iter, PL_XMLPOINTER, &line, -1);
			GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, line);
			if (!maestro || gebr_maestro_server_get_state(maestro) != SERVER_STATE_LOGGED) {
				has_maestro = FALSE;
				break;
			}
		} else {
			n_projs++;
		}
	}

	g_list_foreach(rows, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(rows);

	if (n_projs > 1)
		multiple_projs = TRUE;
	else
		multiple_projs = FALSE;

	// Set sensitive for tab Projects and Lines
	if (has_maestro) {
		gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_project_line, "project_line_new_line"), !multiple_projs);
		gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_project_line, "project_line_delete"), has_maestro);
		gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_project_line, "project_line_properties"), !is_multiple);
		gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_project_line, "project_line_dict_edit"), !is_multiple);
		gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_project_line, "project_line_export"), has_maestro);
		gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_project_line, "project_line_view"), !is_multiple);
		gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_project_line, "project_line_edit"), !is_multiple);
	} else {
		gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_project_line, "project_line_delete"), FALSE);
		gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_project_line, "project_line_properties"), FALSE);
		gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_project_line, "project_line_dict_edit"), FALSE);
		gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_project_line, "project_line_export"), FALSE);
		gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_project_line, "project_line_view"), FALSE);
		gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_project_line, "project_line_edit"), FALSE);
	}
}

/*
 * Load the selected project or line from file.
 */
static void project_line_load(void)
{
	GtkTreeIter iter;
	GtkTreeIter child;

	gboolean is_line;
	gboolean multiple_selection = FALSE;

	project_line_free();
	if (!project_line_get_selected(&iter, DontWarnUnselection)) {
		flow_browse_set_run_widgets_sensitiveness(gebr.ui_flow_browse, FALSE, FALSE);
		return;
	}

	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_project_line->view));
	GList *rows = gtk_tree_selection_get_selected_rows(selection, NULL);

	if (g_list_length(rows) >= 2)
		multiple_selection = TRUE;

	if (!multiple_selection) {
		GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(gebr.ui_project_line->store), &iter);
		is_line = gtk_tree_path_get_depth(path) == 2 ? TRUE : FALSE;
		gtk_tree_path_free(path);

		if (is_line) {
			child = iter;
			gtk_tree_model_iter_parent(GTK_TREE_MODEL(gebr.ui_project_line->store), &iter, &child);
		}

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->store), &iter,
		                   PL_XMLPOINTER, &gebr.project, -1);

		if (is_line) {
			gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->store), &child,
			                   PL_XMLPOINTER, &gebr.line, -1);
			gebr.project_line = GEBR_GEOXML_DOC(gebr.line);

			GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line);
			if (maestro) {
				if (gebr_maestro_server_get_state(maestro) == SERVER_STATE_LOGGED) {
					gtk_widget_show(gebr.ui_flow_browse->view);
					line_load_flows();
					if (gebr_geoxml_line_get_flows_number(gebr.line) < 1)
						flow_browse_reload_selected();
				}
				else {
					gtk_widget_hide(gebr.ui_flow_browse->view);
				}
			}
		} else {
			gebr.project_line = GEBR_GEOXML_DOC(gebr.project);
			gebr.line = NULL;
		}
	}

	GebrMaestroServer *maestro =
			gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller,
			                                             gebr.line);
	gebr_flow_browse_update_server(gebr.ui_flow_browse, maestro);

	project_line_info_update();

	g_list_foreach(rows, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(rows);
}

static void pl_change_selection_update_validator(GtkTreeSelection *selection)
{
	if(gebr.validator)
		gebr_validator_update(gebr.validator);
}

/**
 * \internal
 */
static void project_line_on_row_activated(GtkTreeView * tree_view, GtkTreePath * path,
					  GtkTreeViewColumn * column, struct ui_project_line *ui)
{
	GtkTreeIter iter;

	project_line_get_selected(&iter, DontWarnUnselection);

	if (gtk_tree_store_iter_depth(ui->store, &iter) == 0) {
		GtkTreePath *path;
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(ui->store), &iter);

		if (gtk_tree_view_row_expanded(GTK_TREE_VIEW(ui->view), path)) {
			gtk_tree_view_collapse_row(GTK_TREE_VIEW(ui->view), path);
		} else {
			gtk_tree_view_expand_row(GTK_TREE_VIEW(ui->view), path, FALSE);
		}
		gtk_tree_path_free(path);
		return;
	}

	gebr.last_notebook = gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook));
	gebr.config.current_notebook = 1;
	gtk_notebook_set_current_page(GTK_NOTEBOOK(gebr.notebook), gebr.config.current_notebook);
}

/**
 * \internal
 */
static GtkMenu *project_line_popup_menu(GtkWidget * widget, struct ui_project_line *ui_project_line)
{
	GtkWidget *menu;
	GtkWidget *menu_item;

	menu = gtk_menu_new();

	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_project_line->view));
	gboolean multiple_selection = gtk_tree_selection_count_selected_rows(selection) > 1;

	/* new project */
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group_project_line, "project_line_new_project")));

	if (!project_line_get_selected(NULL, DontWarnUnselection)) {
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

		menu_item = gtk_menu_item_new_with_label(_("Collapse all"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_collapse_all), ui_project_line->view);

		menu_item = gtk_menu_item_new_with_label(_("Expand all"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_expand_all), ui_project_line->view);
		goto out;
	}

	/* new line */
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group_project_line, "project_line_new_line")));

	/* properties */
	if (!multiple_selection)
		gtk_container_add(GTK_CONTAINER(menu),
		                  gtk_action_create_menu_item(gtk_action_group_get_action
		                                              (gebr.action_group_project_line, "project_line_properties")));
	/* delete */
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group_project_line, "project_line_delete")));

	/* view report */
	if (!multiple_selection)
		gtk_container_add(GTK_CONTAINER(menu),
		                  gtk_action_create_menu_item(gtk_action_group_get_action
		                                              (gebr.action_group_project_line, "project_line_view")));

	/* edit report */
	if (!multiple_selection)
		gtk_container_add(GTK_CONTAINER(menu),
		                  gtk_action_create_menu_item(gtk_action_group_get_action
		                                              (gebr.action_group_project_line, "project_line_edit")));

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

	menu_item = gtk_menu_item_new_with_label(_("Collapse all"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_collapse_all), ui_project_line->view);

	menu_item = gtk_menu_item_new_with_label(_("Expand all"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_expand_all), ui_project_line->view);

 out:	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}


/**
 * \internal
 * 
 * Lines and projects reordering callback.
 */
static gboolean
line_reorder(GtkTreeView *tree_view, GtkTreeIter *source_iter, GtkTreeIter *target_iter,
	     GtkTreeViewDropPosition drop_position)
{
	GtkTreeIter source_iter_parent, target_iter_parent;
	GtkTreeIter new_iter;
	gboolean source_is_line, target_is_line;
	gchar *source_line_filename = NULL, *target_line_filename = NULL;
	gchar *source_project_filename = NULL, *target_project_filename = NULL;
	
	GtkTreeModel *model = GTK_TREE_MODEL(gebr.ui_project_line->store);

	/* If the iters have parents, they refer to lines. Otherwise, they refer to projects. */
	source_is_line = gtk_tree_model_iter_parent(model, &source_iter_parent, source_iter);
	target_is_line = gtk_tree_model_iter_parent(model, &target_iter_parent, target_iter);

	/* Get all projects and lines filenames (sources and targets). */
	if (source_is_line) {
		gtk_tree_model_get(model, source_iter, PL_FILENAME, &source_line_filename, -1);
		gtk_tree_model_get(model, &source_iter_parent, PL_FILENAME, &source_project_filename, -1);
	}
	else {
		/* Source iter is a project. Thus, we get its filename only (line is unknown in this case). */
		gtk_tree_model_get(model, source_iter, PL_FILENAME, &source_line_filename, -1);
	}

	if (target_is_line) {
		gtk_tree_model_get(model, target_iter, PL_FILENAME, &target_line_filename, -1);
		gtk_tree_model_get(model, &target_iter_parent, PL_FILENAME, &target_project_filename, -1);
	}
	else {
		/* Target iter is a project. Thus, we get its filename only (line is unknown in this case). */
		gtk_tree_model_get(model, target_iter, PL_FILENAME, &target_project_filename, -1);
	}

	/* Drop cases: */
	if (!source_is_line && !target_is_line){ /* Source and target are Projects.*/
		gboolean drop_before;
		drop_before = (drop_position == GTK_TREE_VIEW_DROP_BEFORE);

		if (drop_before) {
			gtk_tree_store_move_before(gebr.ui_project_line->store, source_iter, target_iter);
		}
		else { /* GTK_TREE_VIEW_DROP_AFTER */
			gtk_tree_store_move_after(gebr.ui_project_line->store, source_iter, target_iter);
		}
		return TRUE;
	}

	if (source_is_line && target_is_line) {
		gboolean drop_before;
		drop_before = (drop_position == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE || drop_position == GTK_TREE_VIEW_DROP_BEFORE);

		if (drop_before) {
			gtk_tree_store_insert_before(gebr.ui_project_line->store, &new_iter, NULL, target_iter);
		}
		else { /* GTK_TREE_VIEW_DROP_INTO_OR_AFTER || GTK_TREE_VIEW_DROP_AFTER */
			gtk_tree_store_insert_after(gebr.ui_project_line->store, &new_iter, NULL, target_iter);
		}
		gebr_gui_gtk_tree_model_iter_copy_values(model, &new_iter, source_iter);
		gtk_tree_store_remove(gebr.ui_project_line->store, source_iter);
		
		project_line_move(source_project_filename, source_line_filename, target_project_filename, target_line_filename, drop_before);
		project_line_select_iter(&new_iter);

		return TRUE;
	}

	if (source_is_line && !target_is_line) { /* Target is a project. */
		gtk_tree_store_append(gebr.ui_project_line->store, &new_iter, target_iter);
		gebr_gui_gtk_tree_model_iter_copy_values(model, &new_iter, source_iter);
		gtk_tree_store_remove(gebr.ui_project_line->store, source_iter);

		project_line_move(source_project_filename, source_line_filename, target_project_filename, NULL, FALSE);
		project_line_select_iter(&new_iter);

		return TRUE;
	}

	return FALSE;
}


/**
 * \internal
 *
 * Lines and projects reordering acceptance callback.
 */
static gboolean
line_can_reorder(GtkTreeView *tree_view, GtkTreeIter *source_iter, GtkTreeIter *target_iter,
		 GtkTreeViewDropPosition drop_position)
{
	GtkTreeIter source_iter_parent, target_iter_parent;
	gboolean source_is_line, target_is_line;

	/* If the iters have parents, they refer to lines. Otherwise, they refer to projects. */
	source_is_line = gtk_tree_model_iter_parent(GTK_TREE_MODEL(gebr.ui_project_line->store), &source_iter_parent, source_iter);
	target_is_line = gtk_tree_model_iter_parent(GTK_TREE_MODEL(gebr.ui_project_line->store), &target_iter_parent, target_iter);

	if (source_is_line) {
		gboolean sensitive;
		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->store), source_iter, PL_SENSITIVE, &sensitive, -1);
		if (!sensitive)
			return FALSE;
	}

	if (source_is_line && target_is_line) /* Source and target are lines. */
		return TRUE;

	if (!source_is_line && target_is_line) /* Source is a project. */
		return FALSE;

	if (source_is_line && !target_is_line) /* Target is a project. */
		return drop_position == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE || drop_position == GTK_TREE_VIEW_DROP_INTO_OR_AFTER;

	if (!source_is_line && !target_is_line) /* Source and target are projects. */
		return drop_position == GTK_TREE_VIEW_DROP_BEFORE || drop_position == GTK_TREE_VIEW_DROP_AFTER;

	return FALSE;
}

/**
 */
void project_line_show_help(void)
{
	gebr_help_show(GEBR_GEOXML_OBJECT (gebr.project_line), FALSE);
}

void project_line_edit_help(void)
{
	if (gebr.project_line == NULL) {
		project_line_get_selected(NULL, ProjectLineSelection); //show a message to the user
		return;
	}
	gebr_help_edit_document(GEBR_GEOXML_DOC(gebr.project_line));
	document_save(GEBR_GEOXML_DOCUMENT(gebr.project_line), TRUE, FALSE);
}

#if 0
gboolean servers_filter_visible_func (GtkTreeModel *filter,
				      GtkTreeIter *iter,
				      gpointer data)
{
	gboolean is_fs;
	const gchar *group;
	GebrDaemonServer *daemon;

	if (!gebr.line)
		return FALSE;

	group = gebr_geoxml_line_get_group (gebr.line, &is_fs);

	gtk_tree_model_get (filter, iter, 0, &daemon, -1);

	if (!server)
		return TRUE;

	return TRUE; // gebr_server_is_in_group(server, group, is_fs);
}
#endif

#if 0
gint servers_sort_func (GtkTreeModel *model,
			GtkTreeIter *a,
			GtkTreeIter *b,
			gpointer data)
{
	GebrServer *sa, *sb;
	gboolean ca, cb;
	gboolean is_auto_choose = FALSE;

	gtk_tree_model_get(GTK_TREE_MODEL(model), a, SERVER_IS_AUTO_CHOOSE, &is_auto_choose, -1);

	if (is_auto_choose)
		return -1;

	gtk_tree_model_get(GTK_TREE_MODEL(model), b, SERVER_IS_AUTO_CHOOSE, &is_auto_choose, -1);

	if (is_auto_choose)
		return 1;

	gtk_tree_model_get (model, a, SERVER_POINTER, &sa, -1);
	gtk_tree_model_get (model, b, SERVER_POINTER, &sb, -1);

	if (!sa || !sb)
		return 0;

	if (!sa->comm || !sb->comm)
		return 0;

	ca = sa->comm->socket->protocol->logged;
	cb = sb->comm->socket->protocol->logged;

	// Order by Connected state first
	if (ca != cb)
		// Return values are:
		//   1 if server 'a' is connected
		//  -1 if server 'b' is connected
		return ca ? -1:1;

	// If states are equal, order alfabetically
	return g_strcmp0 (sa->comm->address->str, sb->comm->address->str);
}
#endif
