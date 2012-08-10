/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or *(at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see
 *   <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <gdk/gdkkeysyms.h>

#include <glib/gi18n.h>
#include <libgebr/date.h>
#include <libgebr/gui/gebr-gui-utils.h>

#include "ui_flow_browse.h"
#include "gebr.h"
#include "document.h"
#include "line.h"
#include "flow.h"
#include "ui_flow.h"
#include "ui_help.h"
#include "callbacks.h"
#include "ui_parameters.h"

/*
 * Prototypes
 */

static void flow_browse_load(void);

static void flow_browse_on_row_activated(GtkTreeView * tree_view, GtkTreePath * path,
					 GtkTreeViewColumn * column, GebrUiFlowBrowse *ui_flow_browse);

static GtkMenu *flow_browse_popup_menu(GtkWidget * widget, GebrUiFlowBrowse *ui_flow_browse);

static void gebr_flow_browse_revision_revert(const gchar *rev_id);

static void gebr_flow_browse_revision_delete(const gchar *rev_id);

static void flow_browse_on_flow_move(void);

static void update_speed_slider_sensitiveness(GebrUiFlowBrowse *ufb);

static void flow_browse_add_revisions_graph(GebrGeoXmlFlow *flow,
                                            GebrUiFlowBrowse *fb,
                                            gboolean keep_selection);

static void on_job_info_status_changed(GebrJob *job,
                                       GebrCommJobStatus old_status,
                                       GebrCommJobStatus new_status,
                                       const gchar *parameter,
                                       GtkWidget *image);

static void
on_job_button_clicked(GtkButton *button,
                      GebrUiFlowBrowse *fb)
{
	GebrJob *job = gebr_job_control_get_recent_job_from_flow(GEBR_GEOXML_DOCUMENT(gebr.flow), gebr.job_control);
	gebr_job_control_apply_flow_filter(gebr.flow, gebr.job_control);
	gebr_job_control_select_job(gebr.job_control, job);
	gebr_interface_change_tab(NOTEBOOK_PAGE_JOB_CONTROL);
}

static void
on_context_button_toggled(GtkToggleButton *button,
                          GebrUiFlowBrowse *fb)
{
	gboolean active = gtk_toggle_button_get_active(button);

	g_signal_handlers_block_by_func(fb->properties_ctx_button, on_context_button_toggled, fb);
	g_signal_handlers_block_by_func(fb->snapshots_ctx_button, on_context_button_toggled, fb);
	g_signal_handlers_block_by_func(fb->jobs_ctx_button, on_context_button_toggled, fb);

	if (!active) {
		gtk_toggle_button_set_active(button, TRUE);
		goto out;
	}

	if (button == fb->properties_ctx_button) {
		gtk_toggle_button_set_active(fb->snapshots_ctx_button, !active);
		gtk_toggle_button_set_active(fb->jobs_ctx_button, !active);

		gtk_widget_show(fb->properties_ctx_box);
		gtk_widget_hide(fb->snapshots_ctx_box);
		gtk_widget_hide(fb->jobs_ctx_box);

		gebr_flow_browse_load_parameters_review(gebr.flow, fb);
	}
	else if (button == fb->snapshots_ctx_button) {
		gtk_toggle_button_set_active(fb->properties_ctx_button, !active);
		gtk_toggle_button_set_active(fb->jobs_ctx_button, !active);

		gtk_widget_hide(fb->properties_ctx_box);
		gtk_widget_show(fb->snapshots_ctx_box);
		gtk_widget_hide(fb->jobs_ctx_box);
	}
	else if (button == fb->jobs_ctx_button) {
		gtk_toggle_button_set_active(fb->snapshots_ctx_button, !active);
		gtk_toggle_button_set_active(fb->properties_ctx_button, !active);

		gtk_widget_hide(fb->properties_ctx_box);
		gtk_widget_hide(fb->snapshots_ctx_box);
		gtk_widget_show(fb->jobs_ctx_box);

		GebrJob *job = gebr_job_control_get_recent_job_from_flow(GEBR_GEOXML_DOCUMENT(gebr.flow), gebr.job_control);
		if(job){
			gtk_widget_hide(fb->info.job_no_output);
			gtk_widget_show(fb->info.job_has_output);
		} else {
			gtk_widget_show(fb->info.job_no_output);
			gtk_widget_hide(fb->info.job_has_output);
		}
		gebr_job_control_select_job(gebr.job_control, job);
	}

out:
	g_signal_handlers_unblock_by_func(fb->properties_ctx_button, on_context_button_toggled, fb);
	g_signal_handlers_unblock_by_func(fb->snapshots_ctx_button, on_context_button_toggled, fb);
	g_signal_handlers_unblock_by_func(fb->jobs_ctx_button, on_context_button_toggled, fb);
}

void
gebr_flow_browse_info_job(GebrUiFlowBrowse *fb,
                          const gchar *job_id)
{
	GebrJob *job = gebr_job_control_find(gebr.job_control, job_id);

	const gchar *title = gebr_job_get_description(job);

	GtkWidget *job_box = gtk_hbox_new(FALSE, 5);

	GtkWidget *img = gtk_image_new();
	GtkWidget *label = gtk_label_new(title);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

	g_signal_connect(job, "status-change", G_CALLBACK(on_job_info_status_changed), img);

	gtk_box_pack_start(GTK_BOX(job_box), img, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(job_box), label, TRUE, TRUE, 5);

	gtk_box_pack_start(GTK_BOX(fb->jobs_status_box), job_box, TRUE, TRUE, 0);

	gtk_widget_show_all(fb->jobs_status_box);

	if (!gtk_widget_get_visible(fb->info_jobs))
		gtk_widget_show_all(fb->info_jobs);
}

void
gebr_flow_browse_select_job(GebrUiFlowBrowse *fb)
{
	flow_browse_load();
	gtk_toggle_button_set_active(fb->jobs_ctx_button, TRUE);
}

static void
on_dismiss_clicked(GtkButton *dismiss,
                   GebrUiFlowBrowse *fb)
{
	GList *childs = gtk_container_get_children(GTK_CONTAINER(fb->info_jobs));
	for (GList *i = childs; i; i = i->next)
		gtk_container_remove(GTK_CONTAINER(fb->info_jobs), GTK_WIDGET(i->data));

	gtk_widget_hide(fb->info_jobs);
}

GebrUiFlowBrowse *flow_browse_setup_ui()
{
	GebrUiFlowBrowse *ui_flow_browse;

	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	GtkWidget *page;
	GtkWidget *hpanel;
	GtkWidget *scrolled_window;
	GtkWidget *scrolled_window_info;
	GtkWidget *infopage;

	/* alloc */
	ui_flow_browse = g_new(GebrUiFlowBrowse, 1);

	ui_flow_browse->graph_process = NULL;
	ui_flow_browse->select_flows = NULL;

	/*
	 * Create flow browse page
	 */
	page = gtk_vbox_new(FALSE, 0);
	ui_flow_browse->widget = page;
	hpanel = gtk_hpaned_new();
	gtk_container_add(GTK_CONTAINER(page), hpanel);

	/*
	 * Left side: flow list
	 */
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_paned_pack1(GTK_PANED(hpanel), scrolled_window, FALSE, FALSE);
	gtk_widget_set_size_request(scrolled_window, 300, -1);

	ui_flow_browse->store = gtk_list_store_new(FB_N_COLUMN,
						   G_TYPE_STRING,	/* Name (title for libgeoxml) */
						   G_TYPE_STRING,	/* Filename */
						   G_TYPE_POINTER,	/* GebrGeoXmlFlow pointer */
						   G_TYPE_POINTER,	/* GebrGeoXmlLineFlow pointer */
						   G_TYPE_STRING,	/* Last snapshot modification*/
						   G_TYPE_POINTER);	/* Last queue hash table */

	ui_flow_browse->view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ui_flow_browse->store));
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(ui_flow_browse->view), TRUE);
	gtk_container_add(GTK_CONTAINER(scrolled_window), ui_flow_browse->view);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_flow_browse->view)),
				    GTK_SELECTION_MULTIPLE);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ui_flow_browse->view), FALSE);
	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(ui_flow_browse->view),
						  (GebrGuiGtkPopupCallback) flow_browse_popup_menu, ui_flow_browse);
	gebr_gui_gtk_tree_view_set_gebr_geoxml_sequence_moveable(GTK_TREE_VIEW(ui_flow_browse->view),
								 FB_LINE_FLOW_POINTER,
								 (GebrGuiGtkTreeViewMoveSequenceCallback)
								 flow_browse_on_flow_move, NULL);
	g_signal_connect(ui_flow_browse->view, "row-activated", G_CALLBACK(flow_browse_on_row_activated),
			 ui_flow_browse);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_flow_browse->view));
	g_signal_connect(selection, "changed", G_CALLBACK(flow_browse_load), NULL);
	g_signal_connect_swapped(selection, "changed", G_CALLBACK(update_speed_slider_sensitiveness), ui_flow_browse);

	/* Text column */
	ui_flow_browse->text_renderer = renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_flow_browse->view), col);
	gtk_tree_view_column_set_expand(col, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", FB_TITLE);

	/* Icon column */
	ui_flow_browse->snap_renderer = renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_flow_browse->view), col);
	gtk_tree_view_column_set_cell_data_func(col, renderer, gebr_flow_browse_snapshot_icon, NULL, NULL);

	g_signal_connect(ui_flow_browse->view, "cursor-changed",
	                 G_CALLBACK(gebr_flow_browse_select_snapshot_column), ui_flow_browse);

	/*
	 * Right side: flow info tab
	 */
	scrolled_window_info = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window_info),
	                               GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	/* Get glade file */
	ui_flow_browse->info.builder_flow = gtk_builder_new();
	gtk_builder_add_from_file(ui_flow_browse->info.builder_flow, GEBR_GLADE_DIR "/flow-properties.glade", NULL);

	infopage = gtk_vbox_new(FALSE, 0);
	GtkWidget *infopage_flow = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "main"));
	ui_flow_browse->info_window = infopage_flow;

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);

	if (maestro && gebr_maestro_server_get_state(maestro) == SERVER_STATE_LOGGED && !gebr.line) {
		ui_flow_browse->warn_window = gtk_label_new(_("No Line is selected\n"));
	} else
		ui_flow_browse->warn_window = gtk_label_new(_("The Maestro of this Line is disconnected,\nthen you cannot edit flows.\n"
							      "Try changing its maestro or connecting it."));

	gtk_widget_set_sensitive(ui_flow_browse->warn_window, FALSE);
	gtk_box_pack_start(GTK_BOX(infopage), ui_flow_browse->warn_window, TRUE, TRUE, 0);

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window_info), infopage);

	gtk_paned_pack2(GTK_PANED(hpanel), scrolled_window_info, TRUE, FALSE);

	/* Flow Icon Status */
	ui_flow_browse->info.status = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "flow_status"));

	/* Description */
	ui_flow_browse->info.description = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "flow_description"));

	/* Dates */
	ui_flow_browse->info.modified = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "flow_modified"));

	/* Last execution */
	ui_flow_browse->info.lastrun = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "flow_jobs_label"));
	ui_flow_browse->info.job_status = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "flow_jobs_status"));
	gtk_widget_hide(ui_flow_browse->info.job_status);

	ui_flow_browse->info.job_button = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "flow_jobs_button"));
	g_signal_connect(ui_flow_browse->info.job_button, "clicked", G_CALLBACK(on_job_button_clicked), ui_flow_browse);
	gtk_widget_hide(ui_flow_browse->info.job_button);

	/*
	 * Set button and box context
	 */
	ui_flow_browse->properties_ctx_button = GTK_TOGGLE_BUTTON(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "context_button_flow"));
	ui_flow_browse->properties_ctx_box = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "properties_scroll"));
	g_signal_connect(ui_flow_browse->properties_ctx_button, "toggled", G_CALLBACK(on_context_button_toggled), ui_flow_browse);
	gtk_widget_set_tooltip_text(GTK_WIDGET(ui_flow_browse->properties_ctx_button), "Review of parameters");

	ui_flow_browse->snapshots_ctx_button = GTK_TOGGLE_BUTTON(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "context_button_snaps"));
	ui_flow_browse->snapshots_ctx_box = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "snapshots_box"));
	g_signal_connect(ui_flow_browse->snapshots_ctx_button, "toggled", G_CALLBACK(on_context_button_toggled), ui_flow_browse);
	gtk_widget_set_tooltip_text(GTK_WIDGET(ui_flow_browse->snapshots_ctx_button), "Snapshots");

	ui_flow_browse->jobs_ctx_button = GTK_TOGGLE_BUTTON(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "context_button_jobs"));
	ui_flow_browse->jobs_ctx_box = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "jobs_box"));
	g_signal_connect(ui_flow_browse->jobs_ctx_button, "toggled", G_CALLBACK(on_context_button_toggled), ui_flow_browse);
	gtk_widget_set_tooltip_text(GTK_WIDGET(ui_flow_browse->jobs_ctx_button), "Output of last execution");

	/* Info Bar for Jobs */
	ui_flow_browse->info_jobs = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "info_jobs_box"));
	ui_flow_browse->jobs_status_box = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "job_status_box"));

	GtkButton *dismiss_button = GTK_BUTTON(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "dismiss_button"));
	g_signal_connect(dismiss_button, "clicked", G_CALLBACK(on_dismiss_clicked), ui_flow_browse);

	GtkButton *job_control_button = GTK_BUTTON(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "job_control_button"));
	g_signal_connect(job_control_button, "clicked", G_CALLBACK(on_job_button_clicked), ui_flow_browse);

	gtk_widget_hide(ui_flow_browse->info_jobs);

	/*
	 * Review of parameters Context
	 */
	GtkWidget *properties_box = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "properties_box"));
	GtkWidget *rev_params = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "main_rev_params"));
	gtk_container_add(GTK_CONTAINER(properties_box), rev_params);

	/*
	 * Snapshots Context
	 */
	GtkWidget *rev_main = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "main_rev"));

	ui_flow_browse->revpage_main = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "revisions_main"));

	ui_flow_browse->revpage_warn = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "revisions_warn"));

	ui_flow_browse->revpage_warn_label = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "revpage_warn_label"));

	gtk_widget_show(ui_flow_browse->revpage_main);
	gtk_widget_hide(ui_flow_browse->revpage_warn);

	gtk_container_add(GTK_CONTAINER(ui_flow_browse->snapshots_ctx_box), rev_main);

	/*
	 * Jobs Context
	 */
	GtkWidget *output_view = gebr_job_control_get_output_view(gebr.job_control);
	GtkWidget *output_box = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "jobs_output_box"));
	gtk_widget_reparent(output_view, output_box);
	ui_flow_browse->info.job_has_output = output_box;

	GtkWidget *job_no_output = gtk_label_new("This flow has never been executed.\n\n"
						"GêBR can execute it using (Ctrl+R).");
	gtk_widget_set_sensitive(job_no_output, FALSE);
	ui_flow_browse->info.job_no_output = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "jobs_no_output_box"));
	gtk_box_pack_start(GTK_BOX(ui_flow_browse->info.job_no_output), job_no_output, TRUE, TRUE, 0);

	gtk_widget_show(ui_flow_browse->info.job_no_output);
	gtk_widget_hide(ui_flow_browse->info.job_has_output);

	/*
	 * Add Flow Page on GêBR window
	 */
	gtk_box_pack_start(GTK_BOX(infopage), infopage_flow, TRUE, TRUE, 0);

	return ui_flow_browse;
}

static void
gebr_ui_flow_browse_set_job_status(GebrJob *job,
                                   GebrCommJobStatus status,
                                   GebrUiFlowBrowse *fb)
{
	GtkImage *img = GTK_IMAGE(fb->info.job_status);
	gchar *last_text;
	const gchar *icon, *job_state, *date;

	switch(status) {
	case JOB_STATUS_FINISHED:
		icon = GTK_STOCK_APPLY;
		job_state = _("finished");
		date = gebr_job_get_finish_date(job);
		break;
	case JOB_STATUS_RUNNING:
		icon = GTK_STOCK_EXECUTE;
		job_state = _("started");
		date = gebr_job_get_start_date(job);
		break;
	case JOB_STATUS_CANCELED:
		icon = GTK_STOCK_CANCEL;
		job_state = _("canceled");
		date = gebr_job_get_finish_date(job);
		break;
	case JOB_STATUS_FAILED:
		icon = GTK_STOCK_CANCEL;
		job_state = _("failed");
		date = gebr_job_get_finish_date(job);
		break;
	case JOB_STATUS_QUEUED:
		icon = "chronometer";
		job_state = _("submitted");
		date = gebr_job_get_last_run_date(job);
		break;
	case JOB_STATUS_INITIAL:
	default:
		icon = GTK_STOCK_NETWORK;
		job_state = _("submitted");
		date = gebr_job_get_last_run_date(job);
		break;
	}

	const gchar *iso = gebr_localized_date(date);
	last_text = g_markup_printf_escaped(_("Last execution %s on %s"), job_state, iso);
	gtk_image_set_from_stock(img, icon, GTK_ICON_SIZE_BUTTON);
	gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.lastrun), last_text);

	g_free(last_text);
}

static void
on_job_info_status_changed(GebrJob *job,
                           GebrCommJobStatus old_status,
                           GebrCommJobStatus new_status,
                           const gchar *parameter,
                           GtkWidget *image)
{
	gchar *icon;

	switch(new_status) {
	case JOB_STATUS_FINISHED:
		icon = GTK_STOCK_APPLY;
//		job_state = _("finished");
//		date = gebr_job_get_finish_date(job);
		break;
	case JOB_STATUS_RUNNING:
		icon = GTK_STOCK_EXECUTE;
//		job_state = _("started");
//		date = gebr_job_get_start_date(job);
		break;
	case JOB_STATUS_CANCELED:
		icon = GTK_STOCK_CANCEL;
//		job_state = _("canceled");
//		date = gebr_job_get_finish_date(job);
		break;
	case JOB_STATUS_FAILED:
		icon = GTK_STOCK_CANCEL;
//		job_state = _("failed");
//		date = gebr_job_get_finish_date(job);
		break;
	case JOB_STATUS_QUEUED:
		icon = "chronometer";
//		job_state = _("submitted");
//		date = gebr_job_get_last_run_date(job);
		break;
	case JOB_STATUS_INITIAL:
	default:
		icon = GTK_STOCK_NETWORK;
//		job_state = _("submitted");
//		date = gebr_job_get_last_run_date(job);
		break;
	}

	gtk_image_set_from_stock(GTK_IMAGE(image), icon, GTK_ICON_SIZE_BUTTON);
}

static void
on_job_status_changed(GebrJob *job,
                      GebrCommJobStatus old_status,
                      GebrCommJobStatus new_status,
                      const gchar *parameter,
                      GebrUiFlowBrowse *fb)
{
	gebr_ui_flow_browse_set_job_status(job, new_status, fb);
}

void flow_browse_info_update(void)
{
	if (gebr.flow == NULL) {
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.description), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.modified), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.lastrun), "");

		/* Update parameters properties context */
		GtkWidget *box = GTK_WIDGET(gtk_builder_get_object(gebr.ui_flow_browse->info.builder_flow, "parameters_box"));
		GList *childs = gtk_container_get_children(GTK_CONTAINER(box));
		for (GList *i = childs; i; i = i->next)
			gtk_container_remove(GTK_CONTAINER(box), GTK_WIDGET(i->data));

		/* Update snapshots context */
		gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->revpage_warn_label), _("No Flow selected."));
		gtk_widget_hide(gebr.ui_flow_browse->revpage_main);
		gtk_widget_show(gebr.ui_flow_browse->revpage_warn);

		navigation_bar_update();
		return;
	}

	/* Status Icon */
	gchar *flow_icon_path = g_build_filename(LIBGEBR_ICONS_DIR, "gebr-theme", "48x48", "stock", "flow-icon.png", NULL);
	GIcon *flow_icon = g_icon_new_for_string(flow_icon_path, NULL);

	gchar *status_icon_path;

	GError *error = NULL;
	gebr_geoxml_flow_validate(gebr.flow, gebr.validator, &error);
	if (error) {
		status_icon_path = g_build_filename(LIBGEBR_ICONS_DIR, "gebr-theme", "22x22", "stock", "dialog-warning.png", NULL);
		gtk_widget_set_tooltip_text(gebr.ui_flow_browse->info.status, error->message);
		g_clear_error(&error);
	} else {
		status_icon_path = g_build_filename(LIBGEBR_ICONS_DIR, "gebr-theme", "22x22", "stock", "gtk-apply.png", NULL);
		gtk_widget_set_tooltip_text(gebr.ui_flow_browse->info.status, _("Ready to execute"));
	}
	GIcon *status_icon = g_icon_new_for_string(status_icon_path, NULL);
	GEmblem *status_emblem = g_emblem_new(status_icon);

	GIcon *icon = g_emblemed_icon_new(flow_icon, status_emblem);

	gtk_image_set_from_gicon(GTK_IMAGE(gebr.ui_flow_browse->info.status), icon, GTK_ICON_SIZE_DIALOG);

	g_free(flow_icon_path);
	g_free(status_icon_path);

	gchar *markup;

	/* Description */
	gchar *description = gebr_geoxml_document_get_description(GEBR_GEOXML_DOC(gebr.flow));
	markup = g_markup_printf_escaped("<span size='x-large'>%s</span>", description);
	gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.description), markup);
	g_free(markup);
	g_free(description);

	/* Modified date */
	gchar *modified = gebr_geoxml_document_get_date_modified(GEBR_GEOXML_DOC(gebr.flow));
	gchar *mod_date = g_markup_printf_escaped(_("Modified on %s"),
	                                          gebr_localized_date(modified));
	gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.modified), mod_date);
	g_free(mod_date);
	g_free(modified);

	gchar *last_text = NULL;

	const gchar *last_run = gebr_localized_date(gebr_geoxml_flow_get_date_last_run(gebr.flow));
        if (!last_run || !*last_run) {
        	last_text = g_strdup(_("This flow was never executed"));

        	gtk_widget_hide(gebr.ui_flow_browse->info.job_button);
        	gtk_widget_hide(gebr.ui_flow_browse->info.job_status);
        	gtk_widget_hide(gebr.ui_flow_browse->info.job_has_output);
        	gtk_widget_show(gebr.ui_flow_browse->info.job_no_output);
        } else {
        	/* Update job button */
        	GebrJob *job = gebr_job_control_get_recent_job_from_flow(GEBR_GEOXML_DOCUMENT(gebr.flow), gebr.job_control);
        	if (job) {
        		gebr_ui_flow_browse_set_job_status(job, gebr_job_get_status(job), gebr.ui_flow_browse);

        		g_signal_connect(job, "status-change", G_CALLBACK(on_job_status_changed), gebr.ui_flow_browse);

        		gtk_widget_show(gebr.ui_flow_browse->info.job_button);
        		gtk_widget_show(gebr.ui_flow_browse->info.job_status);
        		gtk_widget_show(gebr.ui_flow_browse->info.job_has_output);
        		gtk_widget_hide(gebr.ui_flow_browse->info.job_no_output);
        	} else {
        		last_text = g_markup_printf_escaped(_("Last execution at %s"), last_run);

			gtk_widget_hide(gebr.ui_flow_browse->info.job_button);
        		gtk_widget_hide(gebr.ui_flow_browse->info.job_status);
        		gtk_widget_hide(gebr.ui_flow_browse->info.job_has_output);
        		gtk_widget_show(gebr.ui_flow_browse->info.job_no_output);
        	}
        }
        if (last_text) {
        	gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.lastrun), last_text);
        	g_free(last_text);
        }

	navigation_bar_update();
}

gboolean flow_browse_get_selected(GtkTreeIter * iter, gboolean warn_unselected)
{
	if (!gebr_gui_gtk_tree_view_get_selected(GTK_TREE_VIEW(gebr.ui_flow_browse->view), iter)) {
		if (warn_unselected)
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("No Flow selected"));
		return FALSE;
	}
	return TRUE;
}

void flow_browse_reload_selected(void)
{
	flow_browse_load();
}

void flow_browse_select_iter(GtkTreeIter * iter)
{
	gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(gebr.ui_flow_browse->view), iter);
}

void flow_browse_single_selection(void)
{
	gebr_gui_gtk_tree_view_turn_to_single_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
}

static void
graph_process_read_stderr(GebrCommProcess * process,
                          GebrUiFlowBrowse *fb)
{
	GString *output;
	output = gebr_comm_process_read_stderr_string_all(process);

//	g_debug("ERROR OF PYTHON: %s", output->str);

	gchar **action = g_strsplit(output->str, ":", -1);

	if (!g_strcmp0(action[0], "revert")) {
		gebr_flow_browse_revision_revert(action[1]);
	}
	else if (!g_strcmp0(action[0], "delete")) {
		gebr_flow_browse_revision_delete(action[1]);
	}
	else if (!g_strcmp0(action[0], "snapshot")) {
		gdk_threads_enter();
		flow_revision_save();
		gdk_threads_leave();
	}
	else if (!g_strcmp0(action[0], "select")) {
		const gchar *id = gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(gebr.flow));
		GList *find = g_list_find_custom(fb->select_flows, id, (GCompareFunc)g_strcmp0);
		if (!find)
			fb->select_flows = g_list_append(fb->select_flows, g_strdup(id));
	}
	else if (!g_strcmp0(action[0], "unselect")) {
		const gchar *id = gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(gebr.flow));
		GList *find = g_list_find_custom(fb->select_flows, id, (GCompareFunc)g_strcmp0);
		if (find)
			fb->select_flows = g_list_remove_link(fb->select_flows, find);
	}
	else if (!g_strcmp0(action[0], "run")) {
		gboolean is_parallel = FALSE;

		if (!g_strcmp0(action[1],"parallel"))
			is_parallel = TRUE;

		gebr_ui_flow_run_snapshots(gebr.flow, action[2], is_parallel);
	}

	g_string_free(output, TRUE);
	g_strfreev(action);
}

static void
graph_process_finished(GebrCommProcess *process)
{
	gebr_comm_process_free(process);
}

static void
flow_browse_add_revisions_graph(GebrGeoXmlFlow *flow,
                                GebrUiFlowBrowse *fb,
                                gboolean keep_selection)
{
	GHashTable *revs = gebr_flow_revisions_hash_create(flow);

	if (fb->update_graph) {
		gchar *dotfile;
		if (gebr_flow_revisions_create_graph(flow, revs, &dotfile)) {
			fb->update_graph = FALSE;
			GString *file = g_string_new(dotfile);
			gchar *flow_filename = g_strdup(gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(flow)));

			gchar *command = g_strdup_printf("draw\b%s\b%s\b", flow_filename, keep_selection? "yes" : "no");
			g_string_prepend(file, command);

			if (gebr_comm_process_write_stdin_string(fb->graph_process, file) == 0)
				g_debug("Can't create dotfile.");

			g_string_free(file, TRUE);
			g_free(flow_filename);
			g_free(command);
		}
		g_free(dotfile);
	}

//	gebr_flow_revisions_hash_free(revs);
}

static void
gebr_flow_browse_create_graph(GebrUiFlowBrowse *fb)
{
	if (fb->graph_process)
		return;

	/*
	 * Graph methods
	 */

	fb->update_graph = TRUE;
	fb->graph_process = gebr_comm_process_new();

	GtkWidget *box = GTK_WIDGET(gtk_builder_get_object(fb->info.builder_flow, "graph_box"));

	GtkWidget *socket = gtk_socket_new();
	gtk_box_pack_start(GTK_BOX(box), socket, TRUE, TRUE, 0);
	GdkNativeWindow socket_id = gtk_socket_get_id(GTK_SOCKET(socket));

	g_debug("SOCKET ID %d", socket_id);

	gchar *cmd_line = g_strdup_printf("python %s/gebr-xdot-graph.py %d %s", GEBR_PYTHON_DIR, socket_id, PACKAGE_LOCALE_DIR);
	GString *cmd = g_string_new(cmd_line);

	g_signal_connect(fb->graph_process, "ready-read-stderr", G_CALLBACK(graph_process_read_stderr), fb);
	g_signal_connect(fb->graph_process, "finished", G_CALLBACK(graph_process_finished), NULL);

	if (!gebr_comm_process_start(fb->graph_process, cmd))
		g_debug("FAIL");

	gtk_widget_show_all(socket);

	g_free(cmd_line);
	g_string_free(cmd, TRUE);

	/*
	 * End graph methods
	 */
}

/**
 * \internal
 * Load a selected flow from file when selected in "Flow Browser".
 */
static void flow_browse_load(void)
{
	GtkTreeIter iter;

	gchar *filename;
	gchar *title;

	gebr_flow_browse_create_graph(gebr.ui_flow_browse);

	flow_free();

	gebr_flow_set_toolbar_sensitive();

	if (!flow_browse_get_selected(&iter, FALSE))
		return;

	gint nrows = gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view)));
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_flow, "flow_change_revision"), nrows > 1? FALSE : TRUE);

	/* load its filename and title */
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter,
			   FB_FILENAME, &filename,
			   FB_TITLE, &title,
			   FB_XMLPOINTER, &gebr.flow,
			   -1);

	if (gebr.validator)
		gebr_validator_update(gebr.validator);

	/* free previous flow and load it */
	flow_edition_load_components();

	/* check if has revisions */
	gboolean has_revision = gebr_geoxml_flow_get_revisions_number(gebr.flow) > 0;

	/* Create model for Revisions */
	if (has_revision && nrows == 1) {
		gtk_widget_show(gebr.ui_flow_browse->revpage_main);
		gtk_widget_hide(gebr.ui_flow_browse->revpage_warn);

		gebr.ui_flow_browse->update_graph = TRUE;
		flow_browse_add_revisions_graph(gebr.flow,
		                                gebr.ui_flow_browse,
		                                FALSE);
	} else if (nrows > 1) {
		gchar *multiple_selection_msg = g_strdup_printf(_("%d Flows selected.\n\n"
								  "GêBR can take a snapshot\n"
								  "of the current state for\n"
								  "each of the selected Flows.\n"
								  "To do it, just click on the\n"
								  "camera icon."
								  ),
								nrows);
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->revpage_warn_label),
				   multiple_selection_msg);
		g_free(multiple_selection_msg);

		gtk_widget_hide(gebr.ui_flow_browse->revpage_main);
		gtk_widget_show(gebr.ui_flow_browse->revpage_warn);

	} else {
		const gchar *no_snapshots_msg = _("There are no snapshots.\n\n"
						  "A snapshot stores the settings of "
						  "your flow so you can restore it at any "
						  "moment. To take a snapshot, just click "
						  "on the camera icon and give a non-empty "
						  "description.");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->revpage_warn_label),
				   no_snapshots_msg);

		GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter);
		gtk_tree_model_row_changed(GTK_TREE_MODEL(gebr.ui_flow_browse->store), path, &iter);
		gtk_widget_hide(gebr.ui_flow_browse->revpage_main);
		gtk_widget_show(gebr.ui_flow_browse->revpage_warn);
	}


	gebr_flow_edition_select_group_for_flow(gebr.ui_flow_edition,
						gebr.flow);

	flow_browse_info_update();

	if (!gtk_toggle_button_get_active(gebr.ui_flow_browse->properties_ctx_button))
		gtk_toggle_button_set_active(gebr.ui_flow_browse->properties_ctx_button, TRUE);
	else
		gebr_flow_browse_load_parameters_review(gebr.flow, gebr.ui_flow_browse);

	g_free(filename);
	g_free(title);
}

static void
update_speed_slider_sensitiveness(GebrUiFlowBrowse *ufb)
{
	gboolean sensitive = FALSE;
	GtkTreeModel *model;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ufb->view));
	GList *rows = gtk_tree_selection_get_selected_rows(selection, &model);

	for (GList *i = rows; i; i = i->next) {
		GtkTreeIter iter;
		GebrGeoXmlFlow *flow;
		gtk_tree_model_get_iter(model, &iter, i->data);
		gtk_tree_model_get(model, &iter, FB_XMLPOINTER, &flow, -1);
		gebr_validator_set_document(gebr.validator, (GebrGeoXmlDocument**) &flow, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, FALSE);
		gboolean parallel = gebr_geoxml_flow_is_parallelizable(flow, gebr.validator);
		gebr_validator_set_document(gebr.validator, (GebrGeoXmlDocument**) &gebr.flow, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, FALSE);
		GebrGeoXmlProgram *prog = gebr_geoxml_flow_get_first_mpi_program(flow);
		gboolean has_mpi = (gebr_geoxml_program_get_status(prog) == GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED);
		gebr_geoxml_object_unref(prog);

		if (parallel || has_mpi) {
			sensitive = TRUE;
			break;
		}

	}
	if (gebr.line)
		gebr_flow_set_toolbar_sensitive();

	g_list_foreach(rows, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(rows);

	gtk_widget_set_sensitive(ufb->speed_slider, sensitive);
	gebr_interface_update_speed_sensitiveness(ufb->speed_button,
						  ufb->speed_slider,
						  ufb->ruler,
						  sensitive);
}

/**
 */
void flow_browse_show_help(void)
{
	if (!gebr.flow)
		return;
	gebr_help_show(GEBR_GEOXML_OBJECT(gebr.flow), FALSE);
}

/**
 */
void flow_browse_edit_help(void)
{
	if (!gebr.flow)
		return;
	gebr_help_edit_document(GEBR_GEOXML_DOC(gebr.flow));
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);
}

/**
 * \internal
 * Go to flow components tab
 */
static void
flow_browse_on_row_activated(GtkTreeView * tree_view, GtkTreePath * path,
			     GtkTreeViewColumn * column, GebrUiFlowBrowse *ui_flow_browse)
{
	gebr.config.current_notebook = 2;
	gtk_notebook_set_current_page(GTK_NOTEBOOK(gebr.notebook), gebr.config.current_notebook);
}

static void
gebr_flow_browse_revision_delete(const gchar *rev_id)
{
	gboolean response;

	gchar **snaps = g_strsplit(rev_id, ",", -1);

	gdk_threads_enter();
	if (!snaps[1])
		response = gebr_gui_confirm_action_dialog(_("Remove this snapshot permanently?"),
		                                          _("If you choose to remove this snapshot "
							    "you will not be able to recover it later."));
	else
		response = gebr_gui_confirm_action_dialog(_("Remove snapshots permanently?"),
		                                          _("If you choose to remove these snapshots "
							    "you will not be able to recover them later."));
	gdk_threads_leave();

	if (response) {
		for (gint i = 0; snaps[i]; i++) {
			GebrGeoXmlRevision *revision;

			revision = gebr_geoxml_flow_get_revision_by_id(gebr.flow, snaps[i]);

			if (!revision)
				continue;

			gchar *id;
			gchar *flow_xml;
			GebrGeoXmlDocument *revdoc;

			gebr_geoxml_flow_get_revision_data(revision, &flow_xml, NULL, NULL, &id);

			if (gebr_geoxml_document_load_buffer(&revdoc, flow_xml) != GEBR_GEOXML_RETV_SUCCESS) {
				g_free(flow_xml);
				g_free(id);
				return;
			}
			g_free(flow_xml);

			gchar *parent_id = gebr_geoxml_document_get_parent_id(revdoc);
			gchar *head_parent = gebr_geoxml_document_get_parent_id(GEBR_GEOXML_DOCUMENT(gebr.flow));

			gebr_geoxml_document_free(revdoc);

			GHashTable *hash_rev = gebr_flow_revisions_hash_create(gebr.flow);

			gboolean change_head_parent = flow_revision_remove(gebr.flow, id, head_parent, hash_rev);

			if (change_head_parent)
				gebr_geoxml_document_set_parent_id(GEBR_GEOXML_DOCUMENT(gebr.flow), parent_id);

			document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, FALSE);
			gebr_flow_revisions_hash_free(hash_rev);
		}

		flow_browse_load();
		gtk_toggle_button_set_active(gebr.ui_flow_browse->snapshots_ctx_button, TRUE);
	}

}

/**
 * \internal
 * Build popup menu
 */
static GtkMenu *flow_browse_popup_menu(GtkWidget * widget, GebrUiFlowBrowse *ui_flow_browse)
{
	GtkWidget *menu;
	GtkWidget *menu_item;

	GtkTreeIter iter;

	/* no line, no new flow possible */
	if (gebr.line == NULL)
		return NULL;

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line);
	if (!maestro || gebr_maestro_server_get_state(maestro) != SERVER_STATE_LOGGED)
		return NULL;

	menu = gtk_menu_new();

	if (!flow_browse_get_selected(&iter, FALSE)) {
		gtk_container_add(GTK_CONTAINER(menu),
				  gtk_action_create_menu_item(gtk_action_group_get_action
							      (gebr.action_group_flow, "flow_new")));
		gtk_container_add(GTK_CONTAINER(menu),
				  gtk_action_create_menu_item(gtk_action_group_get_action
							      (gebr.action_group_flow, "flow_paste")));
		goto out;
	}

	/* Move top */
	if (gebr_gui_gtk_list_store_can_move_up(ui_flow_browse->store, &iter) == TRUE) {
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_GOTO_TOP, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect(menu_item, "activate", G_CALLBACK(line_move_flow_top), NULL);
	}
	/* Move bottom */
	if (gebr_gui_gtk_list_store_can_move_down(ui_flow_browse->store, &iter) == TRUE) {
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_GOTO_BOTTOM, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect(menu_item, "activate", G_CALLBACK(line_move_flow_bottom), NULL);
	}
	/* separator */
	if (gebr_gui_gtk_list_store_can_move_up(ui_flow_browse->store, &iter) == TRUE ||
	    gebr_gui_gtk_list_store_can_move_down(ui_flow_browse->store, &iter) == TRUE)
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_new")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_copy")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_paste")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_delete")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_properties")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_view")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_edit")));

	menu_item = gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_change_revision"));
	gtk_container_add(GTK_CONTAINER(menu), menu_item);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_execute")));

 out:	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}
void
on_snapshot_save_clicked (GtkWidget *widget,
			  GtkWidget *dialog)
{
	gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_YES);
}

void
on_snapshot_discard_changes_clicked(GtkWidget *widget,
				    GtkWidget *dialog)
{
	gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_NO);
}

void
on_snapshot_cancel(GtkWidget *widget,
		   GtkWidget *dialog)
{
	gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);
}

void
on_snapshot_help_button_clicked(GtkWidget *widget,
		   GtkWidget *dialog)
{
	const gchar *section = "flows_browser_save_state_flow";
	gchar *error;

	gebr_gui_help_button_clicked(section, &error);

	if (error) {
		gebr_message (GEBR_LOG_ERROR, TRUE, TRUE, error);
		g_free(error);
	}
}

/*Show dialog just if snapshot_save_default is not set*/
static gint
gebr_flow_browse_confirm_revert(void)
{
	/* Get glade file */
	GtkBuilder *builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, GEBR_GLADE_DIR "/snapshot-revert.glade", NULL);
	GtkWidget *dialog = GTK_WIDGET(gtk_builder_get_object(builder, "main"));

	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gebr.window));
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);

	GObject *snapshot_button = gtk_builder_get_object(builder, "take_snapshot_button");
	GObject *discard_button = gtk_builder_get_object(builder, "discard_changes_button");
	GObject *cancel_button = gtk_builder_get_object(builder, "cancel_button");
	GObject *help_button = gtk_builder_get_object(builder, "help_button");

	g_signal_connect(snapshot_button, "clicked", G_CALLBACK(on_snapshot_save_clicked), dialog);
	g_signal_connect(discard_button, "clicked", G_CALLBACK(on_snapshot_discard_changes_clicked), dialog);
	g_signal_connect(cancel_button, "clicked", G_CALLBACK(on_snapshot_cancel), dialog);
	g_signal_connect(help_button, "clicked", G_CALLBACK(on_snapshot_help_button_clicked), dialog);

	gtk_widget_show(GTK_WIDGET(dialog));

	gdk_threads_enter();
	gint ret = gtk_dialog_run(GTK_DIALOG(dialog));
	gdk_threads_leave();
	gtk_widget_destroy(GTK_WIDGET(dialog));
	g_object_unref(builder);

	return ret;
}

static void
gebr_flow_browse_revision_revert(const gchar *rev_id)
{
	gint confirm_revert = GTK_RESPONSE_NONE;
	gboolean confirm_save  = FALSE;
	gchar *flow_title = gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(gebr.flow));
	gchar *flow_last_modified_date = NULL;
	gchar *snapshot_last_modified_date = NULL;
	GtkTreeIter iter;

	GebrGeoXmlRevision *revision = gebr_geoxml_flow_get_revision_by_id(gebr.flow, rev_id);

	gebr_gui_gtk_tree_model_find_by_column(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter, FB_TITLE,flow_title);
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter, FB_SNP_LAST_MODIF, &snapshot_last_modified_date, -1);
	flow_last_modified_date = gebr_geoxml_document_get_date_modified(GEBR_GEOXML_DOCUMENT(gebr.flow));

	GTimeVal flow_time = gebr_iso_date_to_g_time_val(flow_last_modified_date);
	GTimeVal snapshot_time;
	if (snapshot_last_modified_date)
		snapshot_time = gebr_iso_date_to_g_time_val(snapshot_last_modified_date);

	if (!snapshot_last_modified_date || (snapshot_time.tv_sec < flow_time.tv_sec)) {
		confirm_revert = gebr_flow_browse_confirm_revert();
	}

	if (confirm_revert == GTK_RESPONSE_CANCEL) {
		return;
	} else if (confirm_revert == GTK_RESPONSE_YES) {
		confirm_save = flow_revision_save();
		gdk_threads_leave();
		if(!confirm_save)
			return;
	}

	if (!gebr_geoxml_flow_change_to_revision(gebr.flow, revision)) {
		document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, FALSE);
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Could not revert to snapshot with ID %s"), rev_id);
		return;
	}
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, FALSE);

	flow_browse_load();

	gtk_toggle_button_set_active(gebr.ui_flow_browse->snapshots_ctx_button, TRUE);

	gebr_validator_force_update(gebr.validator);
	flow_browse_info_update();
	update_speed_slider_sensitiveness(gebr.ui_flow_browse);
	gchar *last_date = gebr_geoxml_document_get_date_modified(GEBR_GEOXML_DOCUMENT(gebr.flow));
	gebr_flow_set_snapshot_last_modify_date(last_date);
	g_free(snapshot_last_modified_date);
	g_free(flow_last_modified_date);
	g_free(last_date);
}

/**
 * \internal
 * Saves the current selected line.
 */
static void flow_browse_on_flow_move(void)
{
	document_save(GEBR_GEOXML_DOC(gebr.line), TRUE, TRUE);
}

void
gebr_flow_browse_hide(GebrUiFlowBrowse *self)
{
	return;
}

void
gebr_flow_browse_show(GebrUiFlowBrowse *self)
{
	if (gebr.line)
		gebr_flow_set_toolbar_sensitive();

	update_speed_slider_sensitiveness(self);

	if (gebr.config.niceness == 0)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->nice_button_high), TRUE);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->nice_button_low), TRUE);

	flow_browse_info_update();

	GtkWidget *output_view = gebr_job_control_get_output_view(gebr.job_control);
	GtkWidget *output_box = GTK_WIDGET(gtk_builder_get_object(self->info.builder_flow, "jobs_output_box"));
	gtk_widget_reparent(output_view, output_box);

	gtk_widget_show(self->info.job_no_output);
	gtk_widget_hide(self->info.job_has_output);

	/* Set default on properties flow */
	if (!gtk_toggle_button_get_active(gebr.ui_flow_browse->properties_ctx_button))
		gtk_toggle_button_set_active(gebr.ui_flow_browse->properties_ctx_button, TRUE);
	else
		gebr_flow_browse_load_parameters_review(gebr.flow, self);
}

void
gebr_flow_browse_snapshot_icon (GtkTreeViewColumn *tree_column,
                      GtkCellRenderer *cell,
                      GtkTreeModel *model,
                      GtkTreeIter *iter,
                      gpointer data)
{
	GebrGeoXmlFlow * flow;

	gtk_tree_model_get(model, iter,
	                   FB_XMLPOINTER, &flow,
	                   -1);

	if (gebr_geoxml_flow_get_revisions_number(flow) > 0)
		g_object_set(cell, "stock-id", "photos", NULL);
	else
		g_object_set(cell, "stock-id", NULL, NULL);

}

void
gebr_flow_browse_select_snapshot_column(GtkTreeView *tree_view,
                                        GebrUiFlowBrowse *ui_flow_browse)
{
	gint nrows = gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view)));
	if (nrows > 1)
		return;

	GtkTreeIter iter;
	gebr_gui_gtk_tree_view_turn_to_single_selection(tree_view);
	if (!flow_browse_get_selected(&iter, TRUE))
		return;

	gebr_gui_gtk_tree_view_set_drag_source_dest(tree_view);

	GebrGeoXmlFlow *flow;
	gtk_tree_model_get(GTK_TREE_MODEL(ui_flow_browse->store), &iter,
	                   FB_XMLPOINTER, &flow, -1);

	if (!gebr_geoxml_flow_get_revisions_number(flow))
		return;

	GtkTreePath *path;
	GtkTreeViewColumn *column;
	gtk_tree_view_get_cursor(tree_view, &path, &column);

	if (!column || !path)
		return;

	gint pos, wid;
	if(!gtk_tree_view_column_cell_get_position(column, ui_flow_browse->snap_renderer, &pos, &wid))
		return;

	gchar *path_str = gtk_tree_path_to_string(path);

	gtk_tree_view_unset_rows_drag_source(tree_view);

	gtk_toggle_button_set_active(ui_flow_browse->snapshots_ctx_button, TRUE);

	g_free(path_str);
	gtk_tree_path_free(path);
}

static void
parameters_review_set_io(GebrGeoXmlFlow *flow,
                         GebrUiFlowBrowse *fb)
{
	gchar *input = gebr_geoxml_flow_io_get_input(flow);
	gchar *output = gebr_geoxml_flow_io_get_output(flow);
	gchar *error = gebr_geoxml_flow_io_get_error(flow);

	GtkLabel *input_label = GTK_LABEL(gtk_builder_get_object(fb->info.builder_flow, "input_label"));
	if (input && *input) {
		gtk_widget_set_sensitive(GTK_WIDGET(input_label), TRUE);
		gtk_label_set_text(input_label, input);
	} else {
		gtk_label_set_text(input_label, _("Input file"));
		gtk_widget_set_sensitive(GTK_WIDGET(input_label), FALSE);
	}

	GtkLabel *output_label = GTK_LABEL(gtk_builder_get_object(fb->info.builder_flow, "output_label"));
	GtkImage *output_image = GTK_IMAGE(gtk_builder_get_object(fb->info.builder_flow, "output_image"));
	if (output && *output) {
		gboolean is_append = gebr_geoxml_flow_io_get_output_append(flow);
		gtk_widget_set_sensitive(GTK_WIDGET(output_label), TRUE);
		gtk_label_set_text(output_label, output);
		gtk_image_set_from_stock(output_image, is_append ? "gebr-append-stdout":"gebr-stdout", GTK_ICON_SIZE_BUTTON);
	} else {
		gtk_label_set_text(output_label, _("Output file"));
		gtk_widget_set_sensitive(GTK_WIDGET(output_label), FALSE);
	}

	GtkLabel *error_label = GTK_LABEL(gtk_builder_get_object(fb->info.builder_flow, "error_label"));
	GtkImage *error_image = GTK_IMAGE(gtk_builder_get_object(fb->info.builder_flow, "error_image"));
	if (error && *error) {
		gboolean is_append = gebr_geoxml_flow_io_get_error_append(flow);
		gtk_widget_set_sensitive(GTK_WIDGET(error_label), TRUE);
		gtk_label_set_text(error_label, error);
		gtk_image_set_from_stock(error_image, is_append ? "gebr-append-stderr":"gebr-stderr", GTK_ICON_SIZE_BUTTON);
	} else {
		gtk_label_set_text(error_label, _("Error file"));
		gtk_widget_set_sensitive(GTK_WIDGET(error_label), FALSE);
	}

	g_free(input);
	g_free(output);
	g_free(error);
}

static gboolean
parameters_review_create_row(GebrGeoXmlParameter *parameter,
                             GtkWidget *box,
                             const gchar *group_label,
                             gboolean insert_header)
{
	gint i, n_instances;
	GebrGeoXmlSequence * param;
	GebrGeoXmlSequence * instance;
	GebrGeoXmlParameters * parameters;
	gboolean changed = FALSE;

	if (gebr_geoxml_parameter_get_is_program_parameter(parameter)) {
		GString * str_value;
		GString * default_value;
		GebrGeoXmlProgramParameter * program;
		GtkWidget *params_info = gtk_hbox_new(FALSE, 0);
		GtkWidget *param_title, *param_value;

		program = GEBR_GEOXML_PROGRAM_PARAMETER(parameter);
		str_value = gebr_geoxml_program_parameter_get_string_value(program, FALSE);
		default_value = gebr_geoxml_program_parameter_get_string_value(program, TRUE);

		if (g_strcmp0(str_value->str, default_value->str) != 0)
		{
			changed = TRUE;
			/* Translating enum values to labels */
			GebrGeoXmlSequence *enum_option = NULL;

			gebr_geoxml_program_parameter_get_enum_option(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), &enum_option, 0);

			for (; enum_option; gebr_geoxml_sequence_next(&enum_option))
			{
				gchar *enum_value = gebr_geoxml_enum_option_get_value(GEBR_GEOXML_ENUM_OPTION(enum_option));
				if (g_strcmp0(str_value->str, enum_value) == 0)
				{
					gchar *label = gebr_geoxml_enum_option_get_label(GEBR_GEOXML_ENUM_OPTION(enum_option));
					g_string_printf(str_value, "%s", label);
					g_free(enum_value);
					g_free(label);
					gebr_geoxml_object_unref(enum_option);
					break;
				}
				g_free(enum_value);
			}
			gchar *label = gebr_geoxml_parameter_get_label(parameter);
			str_value->str = g_markup_printf_escaped("<i>%s</i>",str_value->str);

			if (insert_header && gebr_geoxml_parameter_get_is_in_group(parameter) && group_label) {
				gchar *label = g_strdup_printf("<b>%s</b>", group_label);

				GtkWidget *group_title = gtk_label_new(NULL);
				gtk_label_set_markup(GTK_LABEL(group_title), label);
				gtk_misc_set_alignment(GTK_MISC(group_title), 0.0, 0.5);
				gtk_box_pack_start(GTK_BOX(box), group_title, FALSE, FALSE, 2);

				g_free(label);
			}

			/* Add parameter Title and Value on box */
			gchar *title = g_strdup_printf("%s:", label);
			param_title = gtk_label_new(title);
			g_free(title);

			param_value = gtk_label_new(NULL);
			gtk_label_set_markup(GTK_LABEL(param_value), str_value->str);

			if (gebr_geoxml_parameter_get_is_in_group(parameter))
				gtk_misc_set_padding(GTK_MISC(param_title), 20, 0);
			else
				gtk_box_set_spacing(GTK_BOX(params_info), 20);

			gtk_box_pack_start(GTK_BOX(params_info), GTK_WIDGET(param_title), FALSE, FALSE, 0);
			gtk_box_pack_start(GTK_BOX(params_info), GTK_WIDGET(param_value), FALSE, FALSE, 0);

			gtk_box_pack_start(GTK_BOX(box), params_info, FALSE, FALSE, 0);

			g_free(label);
		}
		g_string_free(str_value, TRUE);
		g_string_free(default_value, TRUE);
	} else {
		/* Group Parameters */
		gchar *group_label;
		gchar *label = gebr_geoxml_parameter_get_label(parameter);

		gebr_geoxml_parameter_group_get_instance(GEBR_GEOXML_PARAMETER_GROUP(parameter), &instance, 0);
		n_instances = gebr_geoxml_parameter_group_get_instances_number(GEBR_GEOXML_PARAMETER_GROUP(parameter));

		i = 1;
		while (instance) {
			if (n_instances > 1) {
				gchar *instance_label = g_strdup_printf(_("Instance %d"), i);
				group_label = g_strdup_printf("%s (%s)", label, instance_label);
			} else {
				group_label = g_strdup(label);
			}

			parameters = GEBR_GEOXML_PARAMETERS(instance);
			gebr_geoxml_parameters_get_parameter(parameters, &param, 0);

			gboolean insert = TRUE;
			while (param) {
				if (parameters_review_create_row(GEBR_GEOXML_PARAMETER(param), box, group_label, insert))
					insert = FALSE;
				gebr_geoxml_sequence_next(&param);
			}

			i++;
			gebr_geoxml_sequence_next(&instance);
		}
		g_free(label);
		g_free(group_label);
	}

	return changed;
}

static void
open_properties_program(GtkButton *button,
                        GebrGeoXmlProgram *prog)
{
	GtkTreeIter iter;
	gebr_flow_edition_get_iter_for_program(prog, &iter);

	flow_edition_select_component_iter(&iter);
	parameters_configure_setup_ui();
}

static GtkWidget *
parameters_review_set_params(GebrGeoXmlFlow *flow,
                             GebrUiFlowBrowse *fb)
{
	gboolean has_programs = FALSE;
	GtkWidget *vbox = gtk_vbox_new(FALSE, 5);

	GtkWidget *prog_label;
	GtkWidget *prog_img;
	gboolean changed = FALSE;

	GebrGeoXmlSequence * prog_seq;
	gebr_geoxml_flow_get_program(flow, &prog_seq, 0);
	while (prog_seq) {
		GebrGeoXmlProgram * prog = GEBR_GEOXML_PROGRAM(prog_seq);
		GebrGeoXmlParameters *parameters;
		GebrGeoXmlSequence *sequence;

		/* Create programs info */
		GtkWidget *prog_props = gtk_hbox_new(FALSE, 5);

		/* Set image of program */
		const gchar *icon = gebr_gui_get_program_icon(GEBR_GEOXML_PROGRAM(prog));
		prog_img = gtk_image_new_from_stock(icon, GTK_ICON_SIZE_BUTTON);

		/* Set tooltip for program */
		const gchar *tooltip = NULL;

		GebrIExprError errorid;
		if (gebr_geoxml_program_get_error_id(prog, &errorid))
			tooltip = gebr_flow_get_error_tooltip_from_id(errorid);

		gtk_widget_set_tooltip_text(prog_img, tooltip);


		/* Set name of program */
		gchar *title = g_markup_printf_escaped("<b>%s</b>", gebr_geoxml_program_get_title(prog));
		prog_label = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(prog_label), title);
		g_free(title);

		GtkWidget *prog_button = gtk_button_new();
		GtkWidget *image = gtk_image_new_from_stock("kontact_todo", GTK_ICON_SIZE_BUTTON);
		gtk_button_set_relief(GTK_BUTTON(prog_button), GTK_RELIEF_NONE);
		gtk_button_set_image(GTK_BUTTON(prog_button), image);
		g_signal_connect(prog_button, "clicked", G_CALLBACK(open_properties_program), prog);

		gtk_box_pack_start(GTK_BOX(prog_props), prog_img, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(prog_props), prog_label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(prog_props), prog_button, FALSE, FALSE, 0);

		GtkWidget *expander = gtk_expander_new(NULL);
		gtk_expander_set_label_widget(GTK_EXPANDER(expander), prog_props);
		gebr_gui_gtk_expander_hacked_define(expander, prog_props);

		/* Set program on Flows box */
		GtkWidget *prog_box = gtk_hbox_new(FALSE, 5);
		gtk_box_pack_start(GTK_BOX(prog_box), expander, FALSE, FALSE, 0);

		gtk_box_pack_start(GTK_BOX(vbox), prog_box, FALSE, FALSE, 0);

		if (gebr_geoxml_flow_has_control_program(flow) &&
		    gebr_geoxml_flow_get_control_program(flow) == prog)
			gtk_box_reorder_child(GTK_BOX(vbox), prog_box, 0);

		/* Create box of parameters */
		GtkWidget *param_align = gtk_alignment_new(0.5, 0.5, 1.0, 1.0);
		gtk_alignment_set_padding(GTK_ALIGNMENT(param_align), 5, 5, 30, 0);

		GtkWidget *params_box = gtk_vbox_new(FALSE, 5);
		gtk_container_add(GTK_CONTAINER(param_align), params_box);

		gtk_container_add(GTK_CONTAINER(expander), param_align);

		if (gebr_geoxml_program_get_status(prog) == GEBR_GEOXML_PROGRAM_STATUS_DISABLED)
			gtk_expander_set_expanded(GTK_EXPANDER(expander), FALSE);
		else
			gtk_expander_set_expanded(GTK_EXPANDER(expander), TRUE);

		parameters = gebr_geoxml_program_get_parameters (prog);
		gebr_geoxml_parameters_get_parameter (parameters, &sequence, 0);
		gebr_geoxml_object_unref(parameters);

		changed = FALSE;
		while (sequence) {
			if (parameters_review_create_row(GEBR_GEOXML_PARAMETER(sequence), params_box, NULL, FALSE))
				changed = TRUE;
			gebr_geoxml_sequence_next (&sequence);
		}

		if (!changed) {
			GtkWidget *label = gtk_label_new(_("This program has only default parameters"));
			gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
			gtk_box_pack_start(GTK_BOX(params_box), label, FALSE, FALSE, 0);
			gtk_widget_set_sensitive(label, FALSE);
		}

		has_programs = TRUE;
		gebr_geoxml_sequence_next(&prog_seq);
	}
	if (!has_programs) {
		prog_label = gtk_label_new(_("This flow has no programs"));
		gtk_box_pack_start(GTK_BOX(vbox), prog_label, FALSE, FALSE, 0);
		gtk_widget_set_sensitive(prog_label, FALSE);
		gtk_misc_set_alignment(GTK_MISC(prog_label), 0.04, 0.5);
	}

	return vbox;
}

void
gebr_flow_browse_load_parameters_review(GebrGeoXmlFlow *flow,
                                        GebrUiFlowBrowse *fb)
{
	if (!flow)
		return;

	GtkWidget *parameters;

	parameters_review_set_io(flow, fb);
	parameters = parameters_review_set_params(flow, fb);
	gtk_widget_show_all(parameters);

	/* Clean parameters_box before adding a new content */
	GtkWidget *box = GTK_WIDGET(gtk_builder_get_object(fb->info.builder_flow, "parameters_box"));
	GList *childs = gtk_container_get_children(GTK_CONTAINER(box));
	for (GList *i = childs; i; i = i->next)
		gtk_container_remove(GTK_CONTAINER(box), GTK_WIDGET(i->data));

	gtk_container_add(GTK_CONTAINER(box), parameters);
}
