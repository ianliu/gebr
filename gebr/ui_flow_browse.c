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
#include "menu.h"

/*
 * Prototypes
 */

static void flow_browse_load(void);

static gboolean flow_browse_static_info_update(void);

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
                                       GtkWidget *container);

static void flow_browse_menu_add(void);

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
gebr_flow_browse_update_programs_view(GebrUiFlowBrowse *fb)
{
	gint nrows = gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(fb->view)));
	GtkWidget *prog_view = gebr_flow_edition_get_programs_view(gebr.ui_flow_edition);

	if (nrows == 1) {
		gtk_widget_reparent(prog_view, fb->prog_frame);
		gtk_widget_show_all(fb->prog_window);
	} else {
		gtk_widget_hide(prog_view);
		gtk_widget_hide(fb->prog_window);
	}
}

static void
on_context_button_toggled(GtkToggleButton *button,
                          GebrUiFlowBrowse *fb)
{
	gboolean active = gtk_toggle_button_get_active(button);

	g_signal_handlers_block_by_func(fb->properties_ctx_button, on_context_button_toggled, fb);
	g_signal_handlers_block_by_func(fb->snapshots_ctx_button, on_context_button_toggled, fb);

	if (!active) {
		active = TRUE;
		gtk_toggle_button_set_active(button, active);
	}

	if (button == fb->properties_ctx_button) {
		gtk_toggle_button_set_active(fb->snapshots_ctx_button, !active);

		gtk_widget_show(fb->properties_ctx_box);
		gtk_widget_hide(fb->snapshots_ctx_box);

		gebr_flow_browse_load_parameters_review(gebr.flow, fb);

		if (gebr_geoxml_flow_get_revisions_number(gebr.flow) > 1) {
			GString *cmd = g_string_new("unselect-all\bNone\n");
			gebr_comm_process_write_stdin_string(fb->graph_process, cmd);
			g_string_free(cmd, TRUE);
		}
	}
	else if (button == fb->snapshots_ctx_button) {
		gtk_toggle_button_set_active(fb->properties_ctx_button, !active);

		gtk_widget_hide(fb->properties_ctx_box);
		gtk_widget_show(fb->snapshots_ctx_box);
	}

	GList *childs = gtk_container_get_children(GTK_CONTAINER(gebr.ui_flow_browse->jobs_status_box));
	for (GList *i = childs; i; i = i->next)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(i->data), FALSE);


	gtk_widget_hide(fb->jobs_ctx_box);
	gtk_widget_hide(fb->menu_window);
	g_signal_handlers_unblock_by_func(fb->properties_ctx_button, on_context_button_toggled, fb);
	g_signal_handlers_unblock_by_func(fb->snapshots_ctx_button, on_context_button_toggled, fb);
}

static void
on_output_job_clicked(GtkToggleButton *button,
                      GebrJob *job)
{
	gboolean active = gtk_toggle_button_get_active(button);
	GebrCommJobStatus status = gebr_job_get_status(job);

	if (status == JOB_STATUS_QUEUED || status == JOB_STATUS_INITIAL) {
		if (active)
			gtk_toggle_button_set_active(button, FALSE);
		return;
	}

	if (active) {
		GList *childs = gtk_container_get_children(GTK_CONTAINER(gebr.ui_flow_browse->jobs_status_box));
		for (GList *i = childs; i; i = i->next) {
			if (button == i->data)
				continue;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(i->data), !active);
		}

		gtk_widget_hide(gebr.ui_flow_browse->properties_ctx_box);
		gtk_widget_hide(gebr.ui_flow_browse->snapshots_ctx_box);
		gtk_widget_hide(gebr.ui_flow_browse->menu_window);
		if(job) {
			gtk_widget_show(gebr.ui_flow_browse->jobs_ctx_box);
			gebr_job_control_select_job(gebr.job_control, job);
		} else {
			gtk_widget_hide(gebr.ui_flow_browse->jobs_ctx_box);
		}
	} else {
		gtk_widget_hide(gebr.ui_flow_browse->jobs_ctx_box);

		if (gtk_toggle_button_get_active(gebr.ui_flow_browse->properties_ctx_button))
			gtk_widget_show(gebr.ui_flow_browse->properties_ctx_box);
		else
			gtk_widget_show(gebr.ui_flow_browse->snapshots_ctx_box);
	}
}

static void
on_dismiss_clicked(GtkButton *dismiss,
                   GebrUiFlowBrowse *fb)
{
	GList *childs = gtk_container_get_children(GTK_CONTAINER(fb->jobs_status_box));
	for (GList *i = childs; i; i = i->next) {
		GList *child = gtk_container_get_children(GTK_CONTAINER(i->data));

		GebrJob *job;
		g_object_get(child->data, "user-data", &job, NULL);
		g_signal_handlers_disconnect_by_func(job, on_job_info_status_changed, child->data);
		g_signal_handlers_disconnect_by_func(i->data, on_output_job_clicked, job);
		gtk_container_remove(GTK_CONTAINER(fb->jobs_status_box), GTK_WIDGET(i->data));
	}

	if (dismiss)
		gebr_flow_browse_reset_jobs_from_flow(gebr.flow, fb);

	/* Hide Info bar*/
	gtk_widget_hide(fb->info_jobs);

	/* Restore last context */
	if (gtk_toggle_button_get_active(fb->properties_ctx_button))
		gtk_toggle_button_set_active(fb->properties_ctx_button, TRUE);
	else
		gtk_toggle_button_set_active(fb->snapshots_ctx_button, TRUE);
}

void
gebr_flow_browse_info_job(GebrUiFlowBrowse *fb,
                          const gchar *job_id)
{
	GebrJob *job = gebr_job_control_find(gebr.job_control, job_id);

	if (!job)
		return;

	const gchar *title = gebr_job_get_description(job);

	GtkWidget *job_box = gtk_hbox_new(FALSE, 5);

	GtkWidget *img = gtk_image_new();
	GtkWidget *label = gtk_label_new(title);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

	gtk_box_pack_start(GTK_BOX(job_box), img, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(job_box), label, TRUE, TRUE, 5);

	GtkWidget *output_button = gtk_toggle_button_new();
	gtk_button_set_relief(GTK_BUTTON(output_button), GTK_RELIEF_NONE);
	gtk_button_set_focus_on_click(GTK_BUTTON(output_button), FALSE);

	gtk_container_add(GTK_CONTAINER(output_button), job_box);
	g_signal_connect(output_button, "toggled", G_CALLBACK(on_output_job_clicked), job);

	/* Update status */
	GebrCommJobStatus status = gebr_job_get_status(job);
	on_job_info_status_changed(job, 0, status, NULL, job_box);

	if (status != JOB_STATUS_FINISHED ||
	    status != JOB_STATUS_CANCELED ||
	    status != JOB_STATUS_FAILED ) {
		g_signal_connect(job, "status-change", G_CALLBACK(on_job_info_status_changed), job_box);
		g_object_set(job_box, "user-data", job, NULL);
	}
	gtk_box_pack_start(GTK_BOX(fb->jobs_status_box), output_button, TRUE, TRUE, 0);

	gtk_widget_show_all(fb->jobs_status_box);

	if (!gtk_widget_get_visible(fb->info_jobs))
		gtk_widget_show_all(fb->info_jobs);
}

static void
on_queue_set_text(GtkCellLayout   *cell_layout,
                  GtkCellRenderer *cell,
                  GtkTreeModel    *tree_model,
                  GtkTreeIter     *iter,
                  gpointer         data)
{
	GebrJob *job;
	gchar *name_queue;

	gtk_tree_model_get(tree_model, iter, 0, &job, -1);

	if (!job)
		name_queue = g_strdup(_("Immediately"));
	else
		name_queue = g_strdup_printf(_("After %s"), gebr_job_get_title(job));

	g_object_set(cell, "text", name_queue, NULL);
	g_free(name_queue);
}

static gboolean
flow_browse_find_by_group(GebrUiFlowBrowse *fb,
                          GebrMaestroServerGroupType type,
                          const gchar *name,
                          GtkTreeIter *iter)
{
	GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(fb->server_combobox));
	if (!model)
		return FALSE;

	gboolean valid = gtk_tree_model_get_iter_first(model, iter);

	while (valid)
	{
		GebrMaestroServerGroupType ttype;
		gchar *tname;

		gtk_tree_model_get(model, iter,
				   MAESTRO_SERVER_TYPE, &ttype,
				   MAESTRO_SERVER_NAME, &tname,
				   -1);

		if (g_strcmp0(tname, name) == 0 && ttype == type) {
			g_free(tname);
			return TRUE;
		}

		g_free(tname);
		valid = gtk_tree_model_iter_next(model, iter);
	}

	return gtk_tree_model_get_iter_first(model, iter);
}

static gboolean
flow_browse_find_flow_server(GebrUiFlowBrowse *fb,
                             GebrGeoXmlFlow *flow,
                             GtkTreeModel   *model,
                             GtkTreeIter    *iter)
{
	if (!flow)
		return FALSE;

	gboolean ret;
	gchar *tmp, *name;
	GebrMaestroServerGroupType type;

	gebr_geoxml_flow_server_get_group(flow, &tmp, &name);
	type = gebr_maestro_server_group_str_to_enum(tmp);
	ret = flow_browse_find_by_group(fb, type, name, iter);

	g_free(tmp);
	g_free(name);

	return ret;
}

void
gebr_flow_browse_select_queue(GebrUiFlowBrowse *self)
{
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(self->queue_combobox)) == -1)
		gtk_combo_box_set_active(GTK_COMBO_BOX(self->queue_combobox), 0);
}

void
gebr_flow_browse_select_group_for_flow(GebrUiFlowBrowse *fb,
					GebrGeoXmlFlow *flow)
{
	GtkTreeIter iter;
	GtkComboBox *cb = GTK_COMBO_BOX(fb->server_combobox);
	GtkTreeModel *model = gtk_combo_box_get_model(cb);

	if (model) {
		if (flow_browse_find_flow_server(fb, flow, model, &iter))
			gtk_combo_box_set_active_iter(cb, &iter);
		gebr_flow_browse_select_queue(fb);
	}
}

static void
on_groups_combobox_changed(GtkComboBox *combobox,
			   GebrUiFlowBrowse *fb)
{
	GtkTreeIter iter;
	GtkTreeIter flow_iter;

	if (!flow_browse_get_selected(&flow_iter, TRUE))
		return;

	if (!gtk_combo_box_get_active_iter(combobox, &iter))
		return;

	GtkTreeModel *model = gtk_combo_box_get_model(combobox);

	GebrMaestroServer *maestro =
		gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller,
							     gebr.line);

	if (!maestro)
		return;

	gchar *name;
	GebrMaestroServerGroupType type;
	gtk_tree_model_get(model, &iter,
			   MAESTRO_SERVER_TYPE, &type,
			   MAESTRO_SERVER_NAME, &name,
			   -1);

	if (fb->name)
		g_free(fb->name);
	fb->name = g_strdup(name);
	fb->type = type;

	flow_edition_set_io();
	flow_browse_info_update();
}

static void
on_server_disconnected_set_row_insensitive(GtkCellLayout   *cell_layout,
					   GtkCellRenderer *cell,
					   GtkTreeModel    *tree_model,
					   GtkTreeIter     *iter,
					   gpointer         data)
{
	gchar *name;
	GebrMaestroServerGroupType type;

	gtk_tree_model_get(tree_model, iter,
			   MAESTRO_SERVER_TYPE, &type,
			   MAESTRO_SERVER_NAME, &name,
			   -1);

	GebrDaemonServer *daemon = NULL;
	GebrMaestroServer *maestro;
	gboolean is_connected = TRUE;

	maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller,
							       gebr.line);

	if (!maestro) {
		if (GTK_IS_CELL_RENDERER_TEXT(cell))
			g_object_set(cell, "text", "", NULL);
		else
			g_object_set(cell, "stock-id", NULL, NULL);
		return;
	}

	if (type == MAESTRO_SERVER_TYPE_DAEMON) {
		daemon = gebr_maestro_server_get_daemon(maestro, name);
		if (!daemon)
			return;
		is_connected = gebr_daemon_server_get_state(daemon) == SERVER_STATE_LOGGED;
	}

	if (GTK_IS_CELL_RENDERER_TEXT(cell)) {
		const gchar *txt;

		if (type == MAESTRO_SERVER_TYPE_GROUP) {
			if (name && *name)
				txt = name;
			else
				txt = gebr_maestro_server_get_display_address(maestro);
		} else  {
			txt = gebr_daemon_server_get_hostname(daemon);
			if (!txt || !*txt)
				txt = gebr_daemon_server_get_address(daemon);
		}
		g_object_set(cell, "text", txt, NULL);
	} else {
		const gchar *stock_id;
		if (type == MAESTRO_SERVER_TYPE_GROUP)
			stock_id = "group";
		else {
			if (is_connected)
				stock_id = GTK_STOCK_CONNECT;
			else
				stock_id = GTK_STOCK_DISCONNECT;
		}

		g_object_set(cell, "stock-id", stock_id, NULL);
	}

	g_object_set(cell, "sensitive", is_connected, NULL);
}

static void on_queue_combobox_changed (GtkComboBox *combo,
                                       GebrUiFlowBrowse *fb)
{
	gint index;
	GebrJob *job;
	GtkTreeIter iter;
	GtkTreeIter server_iter;
	GtkTreeModel *server_model;

	if (!flow_browse_get_selected (&iter, FALSE))
		return;

	if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX(fb->server_combobox), &server_iter))
		return;

	server_model = gtk_combo_box_get_model (GTK_COMBO_BOX(fb->server_combobox));

	gtk_tree_model_get (server_model, &server_iter,
			    0, &job,
			    -1);

	index = gtk_combo_box_get_active (combo);

	if (index < 0)
		index = 0;

	gtk_combo_box_set_active(combo, index);
}


void
gebr_flow_browse_update_server(GebrUiFlowBrowse *fb,
                               GebrMaestroServer *maestro)
{
	gboolean sensitive = maestro != NULL;

	if (maestro) {
		const gchar *type, *msg;

		gebr_maestro_server_get_error(maestro, &type, &msg);

		if (g_strcmp0(type, "error:none") == 0) {
			/* Set groups/servers model for Maestro */
			GtkTreeModel *model = gebr_maestro_server_get_groups_model(maestro);
			gtk_combo_box_set_model(GTK_COMBO_BOX(fb->server_combobox), model);

			/* Set queues model for Maestro */
			GtkTreeModel *queue_model = gebr_maestro_server_get_queues_model(maestro);
			gtk_combo_box_set_model(GTK_COMBO_BOX(fb->queue_combobox), queue_model);

			if (gebr.flow)
				gebr_flow_browse_select_group_for_flow(fb, gebr.flow);
		}
		else
			sensitive = FALSE;
	}
	if (gebr_geoxml_line_get_flows_number(gebr.line) > 0)
		flow_browse_set_run_widgets_sensitiveness(fb, sensitive, TRUE);
	else
		flow_browse_set_run_widgets_sensitiveness(fb, FALSE, FALSE);
}

static void
on_daemons_changed(GebrMaestroController *mc,
                   GebrUiFlowBrowse *fb)
{
	gebr_flow_browse_select_group_for_flow(fb, gebr.flow);
}

static void
on_controller_maestro_state_changed(GebrMaestroController *mc,
				    GebrMaestroServer *maestro,
				    GebrUiFlowBrowse *fb)
{
	if (!gebr.line)
		return;

	gchar *addr1 = gebr_geoxml_line_get_maestro(gebr.line);
	const gchar *addr2 = gebr_maestro_server_get_address(maestro);

	if (g_strcmp0(addr1, addr2) != 0)
		goto out;

	switch (gebr_maestro_server_get_state(maestro)) {
	case SERVER_STATE_DISCONNECTED:
		flow_browse_set_run_widgets_sensitiveness(fb, FALSE, TRUE);
		break;
	case SERVER_STATE_LOGGED:
		gebr_flow_browse_update_server(fb, maestro);
		break;
	default:
		break;
	}

out:
	g_free(addr1);


}

static void
restore_last_selection(GebrUiFlowBrowse *fb)
{
	GtkTreeIter iter;

	if (flow_browse_find_by_group(fb, fb->type, fb->name, &iter))
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(fb->server_combobox), &iter);
}

static void
on_group_changed(GebrMaestroController *mc,
		 GebrMaestroServer *maestro,
		 GebrUiFlowBrowse *fb)
{
	GtkTreeIter iter;
	GtkComboBox *cb = GTK_COMBO_BOX(fb->server_combobox);
	GtkTreeModel *model = gtk_combo_box_get_model(cb);

	if (!gebr.line)
		return;

	if (fb->name) {
		restore_last_selection(fb);
		return;
	}

	if (gebr_maestro_controller_get_maestro_for_line(mc, gebr.line)) {
		if (flow_browse_find_flow_server(fb, gebr.flow, model, &iter)) {
			gchar *name;
			gtk_tree_model_get(model, &iter, MAESTRO_SERVER_NAME, &name, -1);
			gtk_combo_box_set_active_iter(cb, &iter);
		}
		gebr_flow_browse_select_queue(fb);
	}
}

static gboolean
menu_search_func(GtkTreeModel *model,
		 gint column,
		 const gchar *key,
		 GtkTreeIter *iter,
		 gpointer data)
{
	gchar *title, *desc;
	gchar *lt, *ld, *lk; // Lower case strings
	gboolean match;

	if (!key)
		return FALSE;

	gtk_tree_model_get(model, iter,
			   MENU_TITLE_COLUMN, &title,
			   MENU_DESC_COLUMN, &desc,
			   -1);

	lt = title ? g_utf8_strdown(title, -1) : g_strdup("");
	ld = desc ?  g_utf8_strdown(desc, -1)  : g_strdup("");
	lk = g_utf8_strdown(key, -1);

	match = gebr_utf8_strstr(lt, lk) || gebr_utf8_strstr(ld, lk);

	g_free(title);
	g_free(desc);
	g_free(lt);
	g_free(ld);
	g_free(lk);

	return !match;
}

static gboolean flow_browse_get_selected_menu(GtkTreeIter * iter, gboolean warn_unselected)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->menu_view));
	if (gtk_tree_selection_get_selected(selection, &model, iter) == FALSE) {
		if (warn_unselected)
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("No menu selected."));
		return FALSE;
	}
	if (gtk_tree_model_iter_has_child(GTK_TREE_MODEL(gebr.ui_flow_browse->menu_store), iter)) {
		if (warn_unselected)
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Select a menu instead of a category."));
		return FALSE;
	}

	return TRUE;
}

static void flow_browse_menu_add(void)
{
	GtkTreeIter iter;
	gchar *name;
	gchar *filename;
	GebrGeoXmlFlow *menu;
	GebrGeoXmlSequence *program;
	GebrGeoXmlSequence *menu_programs;
	gint menu_programs_index;

	if (!flow_browse_get_selected(NULL, TRUE))
		return;

	if (!flow_browse_get_selected_menu(&iter, FALSE)) {
		GtkTreePath *path;
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(gebr.ui_flow_browse->menu_store), &iter);
		if (gtk_tree_view_row_expanded(GTK_TREE_VIEW(gebr.ui_flow_browse->menu_view), path))
			gtk_tree_view_collapse_row(GTK_TREE_VIEW(gebr.ui_flow_browse->menu_view), path);
		else
			gtk_tree_view_expand_row(GTK_TREE_VIEW(gebr.ui_flow_browse->menu_view), path, FALSE);
		gtk_tree_path_free(path);
		return;
	}

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->menu_store), &iter,
			   MENU_TITLE_COLUMN, &name, MENU_FILEPATH_COLUMN, &filename, -1);
	menu = menu_load_path(filename);
	if (menu == NULL)
		goto out;

	/* set parameters' values of menus' programs to default
	 * note that menu changes aren't saved to disk
	 */
	GebrGeoXmlProgramControl c1, c2;
	GebrGeoXmlProgram *first_prog;

	gtk_tree_model_get_iter_first (GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter);
	gtk_tree_model_get (GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter,
			    FSEQ_GEBR_GEOXML_POINTER, &first_prog, -1);

	gebr_geoxml_flow_get_program(menu, &program, 0);

	c1 = gebr_geoxml_program_get_control (GEBR_GEOXML_PROGRAM (program));
	c2 = gebr_geoxml_program_get_control (first_prog);

	if (c1 != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY && c2 != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY) {
		document_free(GEBR_GEOXML_DOC(menu));
		gebr_message (GEBR_LOG_ERROR, TRUE, TRUE, _("This Flow already contains a loop"));
		goto out;
	}

	for (; program != NULL; gebr_geoxml_sequence_next(&program))
		gebr_geoxml_parameters_reset_to_default(gebr_geoxml_program_get_parameters(GEBR_GEOXML_PROGRAM(program)));

	menu_programs_index = gebr_geoxml_flow_get_programs_number(gebr.flow);
	/* add it to the file */
	gebr_geoxml_flow_add_flow(gebr.flow, menu);
	if (c1 == GEBR_GEOXML_PROGRAM_CONTROL_FOR) {
		GebrGeoXmlParameter *dict_iter = GEBR_GEOXML_PARAMETER(gebr_geoxml_document_get_dict_parameter(GEBR_GEOXML_DOCUMENT(gebr.flow)));
		gebr_validator_insert(gebr.validator, dict_iter, NULL, NULL);
	}
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);

	gebr_flow_set_toolbar_sensitive();
	flow_browse_set_run_widgets_sensitiveness(gebr.ui_flow_browse, TRUE, FALSE);
	gebr_flow_edition_update_speed_slider_sensitiveness(gebr.ui_flow_edition);

	/* and to the GUI */
	gebr_geoxml_flow_get_program(gebr.flow, &menu_programs, menu_programs_index);
	flow_add_program_sequence_to_view(menu_programs, TRUE, TRUE);

	document_free(GEBR_GEOXML_DOC(menu));
 out:	g_free(name);
	g_free(filename);
}

static void flow_browse_menu_show_help(void)
{
	GtkTreeIter iter;
	gchar *menu_filename;
	GebrGeoXmlFlow *menu;

	if (!flow_browse_get_selected_menu(&iter, TRUE))
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->menu_store), &iter,
			   MENU_FILEPATH_COLUMN, &menu_filename, -1);

	menu = menu_load_path(menu_filename);
	if (menu == NULL)
		goto out;
	gebr_help_show(GEBR_GEOXML_OBJECT(menu), TRUE);

out:	g_free(menu_filename);
}

static GtkMenu *flow_browse_menu_popup_menu(GtkWidget * widget, GebrUiFlowBrowse *fb)
{
	GtkTreeIter iter;
	GtkWidget *menu;
	GtkWidget *menu_item;

	menu = gtk_menu_new();
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group_flow_edition, "flow_edition_refresh")));

	if (!flow_browse_get_selected_menu(&iter, FALSE)) {
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
		menu_item = gtk_menu_item_new_with_label(_("Collapse all"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_collapse_all), fb->menu_view);
		menu_item = gtk_menu_item_new_with_label(_("Expand all"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_expand_all), fb->menu_view);
		goto out;
	}

	/* add */
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(GTK_OBJECT(menu_item), "activate", G_CALLBACK(flow_browse_menu_add), NULL);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
	menu_item = gtk_menu_item_new_with_label(_("Collapse all"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_collapse_all), fb->menu_view);
	menu_item = gtk_menu_item_new_with_label(_("Expand all"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_expand_all), fb->menu_view);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

	/* help */
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_HELP, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(GTK_OBJECT(menu_item), "activate", G_CALLBACK(flow_browse_menu_show_help), NULL);
	gchar *menu_filename;
	gtk_tree_model_get(GTK_TREE_MODEL(fb->menu_store), &iter,
			   MENU_FILEPATH_COLUMN, &menu_filename, -1);
	GebrGeoXmlDocument *xml = GEBR_GEOXML_DOCUMENT(menu_load_path(menu_filename));
	gchar *tmp_help = gebr_geoxml_document_get_help(xml);
	if (xml != NULL && strlen(tmp_help) <= 1)
		gtk_widget_set_sensitive(menu_item, FALSE);
	if (xml)
		document_free(xml);
	g_free(menu_filename);
	g_free(tmp_help);

 out:	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}

void
on_size_request(GtkWidget      *widget,
                GtkAllocation *allocation,
                GebrUiFlowBrowse *fb)
{
	allocation->width -= 100;
	gtk_widget_size_allocate(fb->jobs_status_box, allocation);
}

void
gebr_flow_browse_select_job(GebrUiFlowBrowse *fb)
{
	flow_browse_static_info_update();
}

GebrUiFlowBrowse *flow_browse_setup_ui()
{
	GebrUiFlowBrowse *ui_flow_browse;

	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	GtkWidget *page;
	GtkWidget *hpanel;
	GtkWidget *scrolled_window;
	GtkWidget *infopage;

	/* alloc */
	ui_flow_browse = g_new(GebrUiFlowBrowse, 1);

	ui_flow_browse->graph_process = NULL;
	ui_flow_browse->select_flows = NULL;

	g_signal_connect_after(gebr.maestro_controller, "maestro-state-changed",
	                       G_CALLBACK(on_controller_maestro_state_changed), ui_flow_browse);
	g_signal_connect_after(gebr.maestro_controller, "group-changed",
	                       G_CALLBACK(on_group_changed), ui_flow_browse);
	g_signal_connect(gebr.maestro_controller, "daemons-changed",
	                 G_CALLBACK(on_daemons_changed), ui_flow_browse);

	ui_flow_browse->name = NULL;
	ui_flow_browse->type = 0;

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

	/*
	 * Creates the QUEUE combobox
	 */
	ui_flow_browse->queue_combobox = gtk_combo_box_new();
	g_signal_connect(ui_flow_browse->queue_combobox, "changed",
	                 G_CALLBACK(on_queue_combobox_changed), ui_flow_browse);
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(ui_flow_browse->queue_combobox), renderer, TRUE);
	gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(ui_flow_browse->queue_combobox), renderer,
	                                   on_queue_set_text, NULL, NULL);
	gtk_widget_show(ui_flow_browse->queue_combobox);

	/*
	 * Creates the SERVER combobox
	 */
	ui_flow_browse->server_combobox = gtk_combo_box_new();
	g_signal_connect(ui_flow_browse->server_combobox, "changed",
	                 G_CALLBACK(on_groups_combobox_changed), ui_flow_browse);
	renderer = gtk_cell_renderer_pixbuf_new();
	g_object_set(renderer, "stock-size", GTK_ICON_SIZE_MENU, NULL);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(ui_flow_browse->server_combobox), renderer, FALSE);
	gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(ui_flow_browse->server_combobox), renderer,
	                                   on_server_disconnected_set_row_insensitive, NULL, NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(ui_flow_browse->server_combobox), renderer, TRUE);
	gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(ui_flow_browse->server_combobox), renderer,
	                                   on_server_disconnected_set_row_insensitive, NULL, NULL);

	/* View with combobox, flows and programs */
	GtkWidget* left_size = gtk_vbox_new(FALSE, 0);

	/* Add Queues combobox and Groups combobox on view */
	GtkWidget *frame = gtk_frame_new(NULL);
	GtkWidget *vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	GtkWidget *label = gtk_label_new_with_mnemonic(_("Run"));
	GtkWidget *alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 4, 5, 5);
	gtk_frame_set_label_widget(GTK_FRAME(frame), label);
	gtk_container_add(GTK_CONTAINER(vbox), alignment);
	gtk_container_add(GTK_CONTAINER(vbox), ui_flow_browse->queue_combobox);
	gtk_container_add(GTK_CONTAINER(vbox), ui_flow_browse->server_combobox);

	gtk_box_pack_start(GTK_BOX(left_size), frame, FALSE, TRUE, 0);

	GtkWidget *list = gtk_vpaned_new();

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request(scrolled_window, 250, -1);
	gtk_paned_pack1(GTK_PANED(list), scrolled_window, FALSE, FALSE);

	ui_flow_browse->prog_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ui_flow_browse->prog_window), GTK_POLICY_AUTOMATIC,
	                               GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request(ui_flow_browse->prog_window, 250, 50);

	ui_flow_browse->prog_frame = gtk_frame_new(NULL);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(ui_flow_browse->prog_window), ui_flow_browse->prog_frame);

	/* Set programs list */
	GtkWidget *prog_view = gebr_flow_edition_get_programs_view(gebr.ui_flow_edition);
	gtk_widget_reparent(prog_view, ui_flow_browse->prog_frame);
	gtk_frame_set_label(GTK_FRAME(ui_flow_browse->prog_frame), _("Flow sequence"));

	gtk_paned_pack2(GTK_PANED(list), ui_flow_browse->prog_window, FALSE, FALSE);

	gtk_box_pack_start(GTK_BOX(left_size), list, TRUE, TRUE, 0);
	gtk_paned_pack1(GTK_PANED(hpanel), left_size, FALSE, FALSE);

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

	/* Icon column */
	ui_flow_browse->icon_renderer = renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_flow_browse->view), col);
	gtk_tree_view_column_set_cell_data_func(col, renderer, gebr_flow_browse_status_icon, NULL, NULL);

	/* Text column */
	ui_flow_browse->text_renderer = renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_flow_browse->view), col);
	gtk_tree_view_column_set_expand(col, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", FB_TITLE);

	/* Snap Icon column */
	ui_flow_browse->snap_renderer = renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_flow_browse->view), col);
	gtk_tree_view_column_set_cell_data_func(col, renderer, gebr_flow_browse_snapshot_icon, NULL, NULL);

	g_signal_connect(ui_flow_browse->view, "cursor-changed",
	                 G_CALLBACK(gebr_flow_browse_select_snapshot_column), ui_flow_browse);

	/*
	 * Right side: flow info tab
	 */
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

	gtk_paned_pack2(GTK_PANED(hpanel), infopage, TRUE, FALSE);

	/* Flow Icon Status */
	ui_flow_browse->info.status = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "flow_status"));

	/* Description */
	ui_flow_browse->info.description = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "flow_description"));

	/* Dates */
	ui_flow_browse->info.modified = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "flow_modified"));

	/* Last execution */
	ui_flow_browse->info.lastrun = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "flow_jobs_label"));

	/*
	 * Set button and box context
	 */
	ui_flow_browse->properties_ctx_button = GTK_TOGGLE_BUTTON(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "context_button_flow"));
	ui_flow_browse->properties_ctx_box = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "properties_scroll"));
	g_signal_connect(ui_flow_browse->properties_ctx_button, "toggled", G_CALLBACK(on_context_button_toggled), ui_flow_browse);
	gtk_widget_set_tooltip_text(GTK_WIDGET(ui_flow_browse->properties_ctx_button), _("Review of parameters"));

	ui_flow_browse->snapshots_ctx_button = GTK_TOGGLE_BUTTON(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "context_button_snaps"));
	ui_flow_browse->snapshots_ctx_box = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "snapshots_box"));
	g_signal_connect(ui_flow_browse->snapshots_ctx_button, "toggled", G_CALLBACK(on_context_button_toggled), ui_flow_browse);
	gtk_widget_set_tooltip_text(GTK_WIDGET(ui_flow_browse->snapshots_ctx_button), "Snapshots");

	ui_flow_browse->jobs_ctx_box = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "jobs_output_box"));

	/* Info Bar for Jobs */
	ui_flow_browse->info_jobs = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "info_jobs_box"));
	ui_flow_browse->jobs_status_box = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "job_status_box"));

	g_signal_connect(ui_flow_browse->info_jobs, "size-allocate", G_CALLBACK(on_size_request), ui_flow_browse);

	GtkButton *dismiss_button = GTK_BUTTON(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "dismiss_button"));
	g_signal_connect(dismiss_button, "clicked", G_CALLBACK(on_dismiss_clicked), ui_flow_browse);

	GtkButton *job_control_button = GTK_BUTTON(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "job_control_button"));
	g_signal_connect(job_control_button, "clicked", G_CALLBACK(on_job_button_clicked), ui_flow_browse);

	gtk_widget_hide(ui_flow_browse->info_jobs);

	/*
	 * Review of parameters Context
	 */
	ui_flow_browse->html_parameters = gebr_gui_html_viewer_widget_new();

	GtkWidget *properties_box = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "properties_box"));
	gtk_container_add(GTK_CONTAINER(properties_box), GTK_WIDGET(ui_flow_browse->html_parameters));

	gtk_widget_show_all(ui_flow_browse->html_parameters);

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
	gtk_widget_reparent(output_view, ui_flow_browse->jobs_ctx_box);

	/*
	 * Menu list context
	 */
	ui_flow_browse->menu_window = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "menu_box"));

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
	                               GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(ui_flow_browse->menu_window), scrolled_window);

	ui_flow_browse->menu_store = gtk_tree_store_new(MENU_N_COLUMN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(ui_flow_browse->menu_store), MENU_TITLE_COLUMN, GTK_SORT_ASCENDING);

	ui_flow_browse->menu_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ui_flow_browse->menu_store));
	gtk_container_add(GTK_CONTAINER(scrolled_window), ui_flow_browse->menu_view);

	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(ui_flow_browse->menu_view),
	                                          (GebrGuiGtkPopupCallback) flow_browse_menu_popup_menu,
	                                          ui_flow_browse);
	g_signal_connect(ui_flow_browse->menu_view, "key-press-event",
	                 G_CALLBACK(flow_edition_component_key_pressed), ui_flow_browse);
	g_signal_connect(GTK_OBJECT(ui_flow_browse->menu_view), "row-activated",
	                 G_CALLBACK(flow_browse_menu_add), ui_flow_browse);
	gebr_gui_gtk_tree_view_fancy_search(GTK_TREE_VIEW(ui_flow_browse->menu_view), MENU_TITLE_COLUMN);
	gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(ui_flow_browse->menu_view),
	                                    menu_search_func, NULL, NULL);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Title"), renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_flow_browse->menu_view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "markup", MENU_TITLE_COLUMN);
	gtk_tree_view_column_set_sort_column_id(col, MENU_TITLE_COLUMN);
	gtk_tree_view_column_set_sort_indicator(col, TRUE);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Description"), renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_flow_browse->menu_view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", MENU_DESC_COLUMN);

	/*
	 * Add Flow Page on GêBR window
	 */
	gtk_box_pack_start(GTK_BOX(infopage), infopage_flow, TRUE, TRUE, 0);

	/* Create Hash Table */
	ui_flow_browse->flow_jobs = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	return ui_flow_browse;
}

static void
on_job_info_status_changed(GebrJob *job,
                           GebrCommJobStatus old_status,
                           GebrCommJobStatus new_status,
                           const gchar *parameter,
                           GtkWidget *container)
{
	gchar *icon, *job_state;
	const gchar *date;
	gchar *title;
	const gchar *snap_id = gebr_job_get_snapshot_id(job);

	gchar *aux_title = g_markup_printf_escaped("%s <span>#%s</span>",
	                                           gebr_job_get_title(job),
	                                           gebr_job_get_job_counter(job));

	if(snap_id && *snap_id)
		title = g_strdup_printf("%s (%s)", aux_title,
				gebr_job_get_snapshot_title(job));
	else
		title = g_strdup(aux_title);

	gchar *tooltip_beginning = g_markup_printf_escaped(_("Output of <span font_style='italic'>"));
	gchar *tooltip_end = g_markup_printf_escaped("</span>");
	gchar *tooltip = g_strconcat(tooltip_beginning, aux_title, tooltip_end, NULL);

	switch(new_status) {
	case JOB_STATUS_FINISHED:
		icon = GTK_STOCK_APPLY;
		job_state = g_strdup(_("finished"));
		date = gebr_localized_date(gebr_job_get_finish_date(job));
		break;
	case JOB_STATUS_RUNNING:
		icon = GTK_STOCK_EXECUTE;
		job_state = g_strdup(_("started"));
		date = gebr_localized_date(gebr_job_get_start_date(job));
		gtk_widget_set_tooltip_markup(container, tooltip);
		break;
	case JOB_STATUS_CANCELED:
		icon = GTK_STOCK_CANCEL;
		job_state = g_strdup(_("canceled"));
		date = gebr_localized_date(gebr_job_get_finish_date(job));
		break;
	case JOB_STATUS_FAILED:
		icon = GTK_STOCK_CANCEL;
		job_state = g_strdup(_("failed"));
		date = gebr_localized_date(gebr_job_get_finish_date(job));
		break;
	case JOB_STATUS_QUEUED:
	case JOB_STATUS_INITIAL:
	default:
		icon = "chronometer";
		job_state = g_strdup(_("submitted"));
		date = gebr_localized_date(gebr_job_get_last_run_date(job));
		gtk_widget_set_tooltip_text(container, _("This job doesn't have output yet"));
		break;
	}

	GList *children = gtk_container_get_children(GTK_CONTAINER(container));
	GtkWidget *image = GTK_WIDGET(g_list_nth_data(children, 0));
	gtk_image_set_from_stock(GTK_IMAGE(image), icon, GTK_ICON_SIZE_BUTTON);
	GtkWidget *label = GTK_WIDGET(g_list_nth_data(children, 1));

	gtk_label_set_markup(GTK_LABEL(label), title);

	g_free(aux_title);
	g_free(title);
	g_free(tooltip);
	g_free(tooltip_beginning);
	g_free(tooltip_end);
	g_free(job_state);
}

static gboolean
flow_browse_static_info_update(void)
{
	if (gebr.flow == NULL) {
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.description), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.modified), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.lastrun), "");

		/* Update snapshots context */
		gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->revpage_warn_label), _("No Flow selected."));
		gtk_widget_hide(gebr.ui_flow_browse->revpage_main);
		gtk_widget_show(gebr.ui_flow_browse->revpage_warn);

		navigation_bar_update();
		return FALSE;
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
	if (!description || !*description){
		markup = g_markup_printf_escaped(_("<span size='x-large'>No description available</span>"));
		gtk_widget_set_sensitive(gebr.ui_flow_browse->info.description, FALSE);
	}
	else {
		markup = g_markup_printf_escaped("<span size='x-large'>%s</span>",description);
		gtk_widget_set_sensitive(gebr.ui_flow_browse->info.description, TRUE);
	}
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
	gchar *last_run_date = gebr_geoxml_flow_get_date_last_run(gebr.flow);
	if (!last_run_date || !*last_run_date) {
		last_text = g_strdup(_("This flow was never executed"));
	} else {
		const gchar *last_run = gebr_localized_date(last_run_date);
		last_text = g_markup_printf_escaped(_("Submitted on %s"), last_run);
	}
	g_free(last_run_date);

	if (last_text) {
		gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.lastrun), last_text);
		g_free(last_text);
	}

	navigation_bar_update();

	return TRUE;
}

void flow_browse_info_update(void)
{
	if (!flow_browse_static_info_update())
		return;

        /* Update flow list */
        GtkTreeIter iter;
        if (flow_browse_get_selected(&iter, FALSE)) {
        	GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter);
        	gtk_tree_model_row_changed(GTK_TREE_MODEL(gebr.ui_flow_browse->store), path, &iter);
        }

        /* Clean job list view */
        on_dismiss_clicked(NULL, gebr.ui_flow_browse);

        gint nrows = gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view)));
        /* Get the list and rebuild Info Bar */
        GList *jobs = gebr_flow_browse_get_jobs_from_flow(gebr.flow, gebr.ui_flow_browse);
        if (jobs) {
        	if (nrows == 1)
        		for (GList *i = jobs; i; i = i->next)
        			gebr_flow_browse_info_job(gebr.ui_flow_browse, i->data);
        }
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
	flow_browse_set_run_widgets_sensitiveness(gebr.ui_flow_browse, TRUE, FALSE);

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

	gebr_flow_browse_update_programs_view(gebr.ui_flow_browse);

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

		gtk_widget_hide(gebr.ui_flow_browse->revpage_main);
		gtk_widget_show(gebr.ui_flow_browse->revpage_warn);
	}

	gebr_flow_browse_select_group_for_flow(gebr.ui_flow_browse,
	                                       gebr.flow);

	flow_browse_info_update();

	if (gebr_geoxml_flow_get_programs_number(gebr.flow) == 0) {
		gebr_flow_browse_show_menu_list(gebr.ui_flow_browse);
	} else {
		if (!gtk_toggle_button_get_active(gebr.ui_flow_browse->properties_ctx_button))
			gtk_toggle_button_set_active(gebr.ui_flow_browse->properties_ctx_button, TRUE);
		else
			gebr_flow_browse_load_parameters_review(gebr.flow, gebr.ui_flow_browse);
		if (gtk_widget_get_visible(gebr.ui_flow_browse->jobs_ctx_box))
			gtk_widget_hide(gebr.ui_flow_browse->jobs_ctx_box);
		gtk_widget_show(gebr.ui_flow_browse->properties_ctx_box);
	}

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
	gebr_flow_browse_show_menu_list(ui_flow_browse);
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
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_new_program")));
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

	gebr_flow_browse_update_programs_view(self);

	GtkWidget *output_view = gebr_job_control_get_output_view(gebr.job_control);
	gtk_widget_reparent(output_view, self->jobs_ctx_box);
	gtk_widget_hide(gebr.ui_flow_browse->menu_window);


	/* Set default on properties flow */
	if (!gtk_toggle_button_get_active(gebr.ui_flow_browse->properties_ctx_button))
		gtk_toggle_button_set_active(gebr.ui_flow_browse->properties_ctx_button, TRUE);
	else
		gebr_flow_browse_load_parameters_review(gebr.flow, self);
}

void
gebr_flow_browse_status_icon(GtkTreeViewColumn *tree_column,
                             GtkCellRenderer *cell,
                             GtkTreeModel *model,
                             GtkTreeIter *iter,
                             gpointer data)
{
	GebrGeoXmlFlow * flow;

	gtk_tree_model_get(model, iter,
	                   FB_XMLPOINTER, &flow,
	                   -1);

	if (gebr_geoxml_flow_validate(flow, gebr.validator, NULL))
		g_object_set(cell, "stock-id", "flow-icon", NULL);
	else
		g_object_set(cell, "stock-id", GTK_STOCK_DIALOG_WARNING, NULL);
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

void
gebr_flow_browse_append_job_on_flow(GebrGeoXmlFlow *flow,
                                    const gchar *job_id,
                                    GebrUiFlowBrowse *fb)
{
	const gchar *flow_id = gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(flow));

	GList *jobs = NULL;

	jobs = g_hash_table_lookup(fb->flow_jobs, flow_id);

	if (g_list_length(jobs) == 5) {
		GList *last_job = g_list_last(jobs);
		jobs = g_list_remove_link(jobs, last_job);
	}
	jobs = g_list_prepend(jobs, (gchar*)job_id);

	g_hash_table_insert(fb->flow_jobs, g_strdup(flow_id), g_list_copy(jobs));

	g_list_free(jobs);
}

GList *
gebr_flow_browse_get_jobs_from_flow(GebrGeoXmlFlow *flow,
                                    GebrUiFlowBrowse *fb)
{
	const gchar *flow_id = gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(flow));
	GList *jobs;

	jobs = g_hash_table_lookup(fb->flow_jobs, flow_id);

	return jobs;
}

void
gebr_flow_browse_update_jobs_info(GebrGeoXmlFlow *flow,
                                  GebrUiFlowBrowse *fb)
{
	on_dismiss_clicked(NULL, fb);

	GList *jobs = gebr_flow_browse_get_jobs_from_flow(flow, fb);
	for (GList *job = jobs; job; job = job->next)
		gebr_flow_browse_info_job(gebr.ui_flow_browse, job->data);
}

void
gebr_flow_browse_reset_jobs_from_flow(GebrGeoXmlFlow *flow,
                                      GebrUiFlowBrowse *fb)
{
	const gchar *flow_id = gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(flow));

	g_hash_table_insert(fb->flow_jobs, g_strdup(flow_id), NULL);

	on_dismiss_clicked(NULL, fb);
}

void
gebr_flow_browse_load_parameters_review(GebrGeoXmlFlow *flow,
                                        GebrUiFlowBrowse *fb)
{
	if (!flow)
		return;

	if (gtk_widget_get_visible(fb->menu_window)) {
		gtk_widget_hide(fb->menu_window);
		gtk_widget_show(fb->properties_ctx_box);
	}

	GString *prog_content = g_string_new("");
	g_string_append_printf(prog_content, "<html>\n"
					     "  <head>\n"
					     "    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n");
	g_string_append_printf(prog_content, "    <link rel=\"stylesheet\" type=\"text/css\" href=\"file://%s/gebr-report.css\" />",
						  LIBGEBR_STYLES_DIR);
	g_string_append_printf(prog_content, "    <style type=\"text/css\"/>\n"
					     "     .programs .title {"
					     "	     font-size: 14px;"
					     "	     padding-left: 4px;"
					     "	   }"
					     "     .programs .description {"
					     "	     font-size: 12px;"
					     "	     padding-left: 2px;"
					     " 	     padding-bottom: 2px;"
					     "     }"
					     "     .parameters {"
					     "       font-size: 12px;"
					     "       margin-top: 10px;"
					     "       margin-right: 10px;"
					     "       margin-left: 10px;"
					     "     }"
					     "     .parameters caption {"
					     "       display:none;"
					     "     }"
					     "     .io:before {"
					     "       font-size: 12px;"
					     "	     font-weight: bold;"
					     "       content: \"I/O Table\";"
					     "     }"
					     "     .io table {"
					     "       font-size: 12px;"
					     "       text-align: left;"
					     "	     margin-top: 15px;"
					     "       margin-right: 10px;"
					     "       margin-left: 15px;"
					     "     }"
					     "     .io caption {"
					     "       display:none;"
					     "     }"
					     "    </style>");

	g_string_append_printf(prog_content, "  </head>\n"
					     "  <body>\n");

	gebr_flow_generate_io_table(flow, prog_content);
	gebr_flow_generate_parameter_value_table(flow, prog_content, NULL, TRUE);

	g_string_append_printf(prog_content, "  </body>\n"
					     "</html>");
	gebr_gui_html_viewer_widget_show_html(GEBR_GUI_HTML_VIEWER_WIDGET(fb->html_parameters), prog_content->str);

	g_string_free(prog_content, TRUE);
}

const gchar *
gebr_flow_browse_get_selected_queue(GebrUiFlowBrowse *fb)
{
	GtkTreeIter iter;
	GtkComboBox *combo = GTK_COMBO_BOX(fb->queue_combobox);

	if (!gtk_combo_box_get_active_iter(combo, &iter))
		return "";
	else {
		GebrJob *job;
		GtkTreeModel *model = gtk_combo_box_get_model(combo);
		gtk_tree_model_get(model, &iter, 0, &job, -1);
		return job ? gebr_job_get_id(job) : "";
	}
}

void
gebr_flow_browse_get_current_group(GebrUiFlowBrowse *fb,
                                   GebrMaestroServerGroupType *type,
                                   gchar **name)
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(fb->server_combobox));

	if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(fb->server_combobox), &iter))
		gtk_tree_model_get_iter_first(model, &iter);

	gtk_tree_model_get(model, &iter,
			   MAESTRO_SERVER_TYPE, type,
			   MAESTRO_SERVER_NAME, name,
			   -1);
}

void
gebr_flow_browse_get_server_hostname(GebrUiFlowBrowse *fb,
                                     gchar **host)
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(fb->server_combobox));

	if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(fb->server_combobox), &iter))
		gtk_tree_model_get_iter_first(model, &iter);

	gtk_tree_model_get(model, &iter,
			   MAESTRO_SERVER_HOST, host,
			   -1);
}

void
flow_browse_set_run_widgets_sensitiveness(GebrUiFlowBrowse *fb,
                                          gboolean sensitive,
                                          gboolean maestro_err)
{
	if (gebr_geoxml_line_get_flows_number(gebr.line) == 0 && sensitive)
		sensitive = FALSE;

	const gchar *tooltip_disconn;
	const gchar *tooltip_execute;

	if (!gebr.line) {
		if (!gebr.project)
			tooltip_disconn = _("Select a line to execute a flow");
		else
			tooltip_disconn = _("Select a line of this project to execute a flow");
	} else {
		GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line);
		if (!maestro || gebr_maestro_server_get_state(maestro) != SERVER_STATE_LOGGED)
			tooltip_disconn = _("The Maestro of this line is disconnected.\nConnecting it to execute a flow.");
		else if (gebr_geoxml_line_get_flows_number(gebr.line) == 0)
			tooltip_disconn = _("This line does not contain flows\nCreate a flow to execute this line");
		else if (gebr_geoxml_flow_get_programs_number(gebr.flow) == 0)
			tooltip_disconn = _("This flow does not contain programs\nAdd at least one to execute this flow");
		else
			tooltip_disconn = _("Execute");
	}
	tooltip_execute = _("Execute");

	gtk_widget_set_sensitive(fb->queue_combobox, sensitive);
	gtk_widget_set_sensitive(fb->server_combobox, sensitive);

	GtkAction *action = gtk_action_group_get_action(gebr.action_group_flow, "flow_execute");
	const gchar *tooltip = sensitive ? tooltip_execute : tooltip_disconn;

	gtk_action_set_stock_id(action, "gtk-execute");
	gtk_action_set_sensitive(action, sensitive);
	gtk_action_set_tooltip(action, tooltip);

	action = gtk_action_group_get_action(gebr.action_group_flow_edition, "flow_edition_execute");
	gtk_action_set_stock_id(action, "gtk-execute");
	gtk_action_set_sensitive(action, sensitive);
	gtk_action_set_tooltip(action, tooltip);
}

void
gebr_flow_browse_show_menu_list(GebrUiFlowBrowse *fb)
{
	gtk_widget_show(fb->menu_window);
	gtk_widget_hide(fb->properties_ctx_box);
	gtk_widget_hide(fb->snapshots_ctx_box);
	gtk_widget_hide(fb->jobs_ctx_box);
}
