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

#include <string.h>
#include <stdlib.h>

#include <glib/gi18n.h>
#include <libgebr/utils.h>
#include <libgebr/date.h>
#include <libgebr/gui/gebr-gui-utils.h>
#include <libgebr/gui/gebr-gui-save-dialog.h>

#include "gebr-job-control.h"
#include "gebr.h"
#include "gebr-job.h"

typedef struct {
	GebrJob *job;
	guint sig_output;
	guint sig_status;
	guint sig_issued;
	guint sig_cmd_line;
	guint sig_button;
} LastSelection;

struct _GebrJobControlPriv {
	GtkBuilder *builder;
	GtkComboBox *group_combo;
	GtkComboBox *server_combo;
	GtkComboBox *status_combo;
	GtkListStore *server_filter;
	GtkListStore *status_model;
	GtkListStore *store;
	GtkTextBuffer *cmd_buffer;
	GtkTextBuffer *text_buffer;
	GtkListStore *group_filter;
	GtkWidget *cmd_view;
	GtkWidget *label;
	GtkWidget *text_view;
	GtkWidget *view;
	GtkWidget *widget;
	GtkWidget *info_button;
	LastSelection last_selection;
	guint timeout_source_id;
};

enum {
	JC_STRUCT,
	JC_N_COLUMN
};

enum {
	ST_ICON,
	ST_TEXT,
	ST_STATUS,
	ST_N_COLUMN
};

#define BLOCK_SELECTION_CHANGED_SIGNAL(jc) \
	g_signal_handlers_block_by_func(gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->priv->view)), \
					job_control_on_cursor_changed, jc);

#define UNBLOCK_SELECTION_CHANGED_SIGNAL(jc) \
	g_signal_handlers_unblock_by_func(gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->priv->view)), \
					  job_control_on_cursor_changed, jc);

/* Prototypes {{{1 */
static void icon_column_data_func(GtkTreeViewColumn *tree_column,
				  GtkCellRenderer *cell,
				  GtkTreeModel *tree_model,
				  GtkTreeIter *iter,
				  gpointer data);

static void title_column_data_func(GtkTreeViewColumn *tree_column,
				   GtkCellRenderer *cell,
				   GtkTreeModel *tree_model,
				   GtkTreeIter *iter,
				   gpointer data);

static void time_column_data_func(GtkTreeViewColumn *tree_column,
                                  GtkCellRenderer *cell,
                                  GtkTreeModel *tree_model,
                                  GtkTreeIter *iter,
                                  gpointer data);

static void gebr_job_control_load_details(GebrJobControl *jc,
					  GebrJob *job);

static void job_control_on_cursor_changed(GtkTreeSelection *selection,
					  GebrJobControl *jc);

static void on_toggled_more_details(GtkToggleButton *button,
                                    GtkBuilder *builder);

static void gebr_jc_update_status_and_time(GebrJobControl *jc,
                                           GebrJob 	  *job,
                                           enum JobStatus status);

static void on_text_view_populate_popup(GtkTextView * text_view, GtkMenu * menu, GebrJobControl *jc);


static GtkMenu *job_control_popup_menu(GtkWidget * widget, GebrJobControl *job_control);

static void job_control_fill_servers_info(GebrJobControl *jc);


/* Private methods {{{1 */
static gboolean
get_server_group_iter(GebrJobControl *jc, const gchar *group, GtkTreeIter *iter)
{
	GtkTreeIter i;
	GtkTreeModel *model = GTK_TREE_MODEL(jc->priv->group_filter);
	gchar *g;
	gboolean valid = gtk_tree_model_get_iter_first(model, &i);

	if (valid)
		valid = gtk_tree_model_iter_next(model, &i);

	while (valid) {
		gtk_tree_model_get(model, &i, 1, &g, -1);
		if (g_strcmp0(g, group) == 0) {
			if (iter)
				*iter = i;
			g_free(g);
			return TRUE;
		}
		g_free(g);
		valid = gtk_tree_model_iter_next(model, &i);
	}

	return FALSE;
}

static GebrJob *
get_selected_job(GebrJobControl *jc)
{
	GebrJob *job = NULL;
	GtkTreeModel *model;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->priv->view));
	GList *rows = gtk_tree_selection_get_selected_rows(selection, &model);

	if (!rows || rows->next)
		goto free_and_return;

	GtkTreeIter iter;

	if (!gtk_tree_model_get_iter(model, &iter, rows->data))
		g_return_val_if_reached(NULL);

	gtk_tree_model_get(model, &iter, JC_STRUCT, &job, -1);

free_and_return:

	g_list_foreach(rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free(rows);

	return job;
}

static gboolean
jobs_visible_for_group(GtkTreeModel *model,
                       GtkTreeIter *iter,
                       GebrJobControl *jc)
{
	GtkTreeIter active;
	gchar *combo_group;
	gboolean visible = FALSE;

	if (!gtk_combo_box_get_active_iter (jc->priv->group_combo, &active))
		return TRUE;

	gtk_tree_model_get(GTK_TREE_MODEL(jc->priv->group_filter), &active,
			   1, &combo_group, -1);

	gchar *tmp = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(jc->priv->group_filter),
							 &active);
	gint index = atoi(tmp);
	g_free(tmp);

	if (index == 0) // Any
		return TRUE;

	GebrJob *job;
	const gchar *group;

	gtk_tree_model_get(model, iter, JC_STRUCT, &job, -1);

	if (!job)
		return FALSE;

	group = gebr_job_get_server_group(job);

	if (!g_strcmp0(combo_group, group))
		visible = TRUE;

	g_free(combo_group);
	return visible;
}

static gboolean
jobs_visible_for_servers(GtkTreeModel *model,
                         GtkTreeIter *iter,
                         GebrJobControl *jc)
{
	gboolean visible = FALSE;
	GtkTreeIter active;
	GebrServer *combo_server;

	if (!gtk_combo_box_get_active_iter (jc->priv->server_combo, &active))
		return TRUE;

	gtk_tree_model_get(GTK_TREE_MODEL(jc->priv->server_filter), &active,
	                   1, &combo_server, -1);

	if (!combo_server)
		return TRUE;

	GebrJob *job;
	gchar **servers;
	gint n_servers;

	gtk_tree_model_get(model, iter, JC_STRUCT, &job, -1);

	if (!job)
		return FALSE;

	servers = gebr_job_get_servers(job, &n_servers);

	for (gint i = 0; i < n_servers; i++) {
		if (!g_strcmp0(servers[i], combo_server->comm->address->str))
			visible = TRUE;
	}

	return visible;
}

static gboolean
jobs_visible_for_status(GtkTreeModel *model,
                        GtkTreeIter *iter,
                        GebrJobControl *jc)
{
	GtkTreeIter active;
	enum JobStatus combo_status;

	if (!gtk_combo_box_get_active_iter (jc->priv->status_combo, &active))
		return TRUE;

	gtk_tree_model_get(GTK_TREE_MODEL(jc->priv->status_model), &active,
	                   ST_STATUS, &combo_status, -1);

	if (combo_status == -1)
		return TRUE;

	GebrJob *job;

	gtk_tree_model_get(model, iter, JC_STRUCT, &job, -1);

	if (!job)
		return FALSE;

	enum JobStatus status = gebr_job_get_status(job);

	if (status == combo_status)
		return TRUE;
	else if (status == JOB_STATUS_FAILED && combo_status == JOB_STATUS_CANCELED)
		return TRUE;

	return FALSE;

}

static gboolean
jobs_visible_func(GtkTreeModel *model,
		  GtkTreeIter *iter,
		  GebrJobControl *jc)
{
	if (!jobs_visible_for_status(model, iter, jc))
		return FALSE;

	if (!jobs_visible_for_servers(model, iter, jc))
		return FALSE;

	if (!jobs_visible_for_group(model, iter, jc))
		return FALSE;

	return TRUE;
}

static void
on_cb_changed(GtkComboBox *combo,
	      GebrJobControl *jc)
{
	GtkTreeModel *sort = gtk_tree_view_get_model(GTK_TREE_VIEW(jc->priv->view));
	GtkTreeModel *filter = gtk_tree_model_sort_get_model(GTK_TREE_MODEL_SORT(sort));
	gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(filter));
}

static void
build_servers_filter_list(GebrJobControl *jc)
{
	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(gebr.ui_server_list->common.store);

	gtk_list_store_clear(jc->priv->server_filter);

	GtkTreeIter it;
	gtk_list_store_append(jc->priv->server_filter, &it);
	gtk_list_store_set(jc->priv->server_filter, &it, 0, _("Any"), -1);

	gebr_gui_gtk_tree_model_foreach(iter, model) {
		gboolean is_autochoose;
		GebrServer *server;

		gtk_tree_model_get(model, &iter,
				   SERVER_IS_AUTO_CHOOSE, &is_autochoose,
				   SERVER_POINTER, &server,
				   -1);

		if (!server || !gebr_comm_server_is_logged(server->comm) || is_autochoose)
			continue;

		gtk_list_store_append(jc->priv->server_filter, &it);
		gtk_list_store_set(jc->priv->server_filter, &it,
				   0, server->comm->address->str,
				   1, server,
				   -1);
	}

	gtk_combo_box_set_active(jc->priv->server_combo, 0);
}

static const gchar *
get_issues_image_for_status(GebrJob *job)
{
	return GTK_STOCK_DIALOG_WARNING;
}

static void
fill_issues_properties(GebrJobControl *jc,
                       const gchar *issues,
                       GebrJob *job)
{
	const gchar *stock_id;
	GtkInfoBar *info = GTK_INFO_BAR(gtk_builder_get_object(jc->priv->builder, "issues_info_bar"));
	GtkLabel *label = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "issues_info_bar_label"));
	GtkImage *image = GTK_IMAGE(gtk_builder_get_object(jc->priv->builder, "issues_info_bar_image"));

	gtk_widget_show(GTK_WIDGET(info));
	GtkButton *show_issues = GTK_BUTTON(gtk_builder_get_object(jc->priv->builder, "show_issues_button"));
	gtk_widget_hide(GTK_WIDGET(show_issues));

	stock_id = get_issues_image_for_status(job);

	if (g_strcmp0(stock_id, GTK_STOCK_DIALOG_ERROR) == 0)
		gtk_info_bar_set_message_type(info, GTK_MESSAGE_ERROR);
	else
		gtk_info_bar_set_message_type(info, GTK_MESSAGE_WARNING);

	gtk_label_set_text(label, issues);
	gtk_image_set_from_stock(image, stock_id, GTK_ICON_SIZE_DIALOG);
}

static void
on_dismiss_clicked(GtkButton *dismiss, GebrJobControl *jc)
{
	GtkInfoBar *info = GTK_INFO_BAR(gtk_builder_get_object(jc->priv->builder, "issues_info_bar"));
	gtk_widget_hide(GTK_WIDGET(info));

	GtkButton *show_issues = GTK_BUTTON(gtk_builder_get_object(jc->priv->builder, "show_issues_button"));
	gtk_widget_show(GTK_WIDGET(show_issues));
}

static void
on_show_issues_clicked(GtkButton *show_issues, GebrJobControl *jc)
{
	GebrJob *job = get_selected_job(jc);
	fill_issues_properties(jc, gebr_job_get_issues(job), job);
}

static void
on_job_wait_button(GtkButton *button,
                   GebrJobControl *jc)
{
	GebrJob *job, *parent;

	job = get_selected_job(jc);
	parent = gebr_job_control_find(jc, gebr_job_get_queue(job));
	gebr_job_control_select_job(jc, parent);
}

static void
on_job_output(GebrJob *job,
	      GebrTask *task,
	      const gchar *output,
	      GebrJobControl *jc)
{
	GtkTextIter end_iter;
	GtkTextMark *mark;

	gtk_text_buffer_get_end_iter(jc->priv->text_buffer, &end_iter);
	gtk_text_buffer_insert(jc->priv->text_buffer, &end_iter, output, strlen(output));
	if (gebr.config.job_log_auto_scroll) {
		mark = gtk_text_buffer_get_mark(jc->priv->text_buffer, "end");
		gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(jc->priv->text_view), mark);
	}
}

static void
on_job_status(GebrJob *job,
	      enum JobStatus old_status,
	      enum JobStatus new_status,
	      const gchar *parameter,
	      GebrJobControl *jc)
{
	gebr_job_control_load_details(jc, job);
}

static void
on_job_issued(GebrJob *job,
	      const gchar *issues,
	      GebrJobControl *jc)
{
	fill_issues_properties(jc, issues, job);
}

static void
on_job_cmd_line_received(GebrJob *job,
			 GebrJobControl *jc)
{
	GtkTextIter end_iter;
	GtkTextMark *mark;
	const gchar *cmdline = gebr_job_get_command_line(job);

	if (jc->priv->cmd_view) {
		gtk_text_buffer_get_end_iter(jc->priv->cmd_buffer, &end_iter);
		gtk_text_buffer_insert(jc->priv->cmd_buffer, &end_iter, cmdline, strlen(cmdline));
		if (gebr.config.job_log_auto_scroll) {
			mark = gtk_text_buffer_get_mark(jc->priv->cmd_buffer, "end");
			gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(jc->priv->cmd_view), mark);
		}
	}
}

static void
gebr_job_control_info_set_visible(GebrJobControl *jc,
				  gboolean visible,
				  const gchar *txt)
{
	GtkWidget *job_info = GTK_WIDGET(gtk_builder_get_object(jc->priv->builder, "job_info_widget"));
	GtkLabel *empty_label = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "empty_job_selection_label"));

	gtk_widget_set_visible(job_info, visible);
	gtk_widget_set_visible(GTK_WIDGET(empty_label), !visible);

	if (!visible)
		gtk_label_set_text(empty_label, txt);
}

static void
job_control_fill_servers_info(GebrJobControl *jc)
{
	GebrJob *job = get_selected_job(jc);
	GString *resources = g_string_new(NULL);
	GtkLabel *res_label = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "resources_text"));
	gchar *nprocs, *niceness;
	gint n_servers, i;
	gchar **servers;

	servers = gebr_job_get_servers(job, &n_servers);

	gebr_job_get_resources(job, &nprocs, &niceness);

	if (!nprocs || !niceness)
		g_string_printf(resources, _("Waiting for server(s) details"));
	else {
		const gchar *groups = gebr_job_get_server_group(job);
		gchar *markup;

		if (g_strcmp0(groups, "") == 0)
			groups = _("All Servers");

		markup = g_markup_printf_escaped (_("Job submitted by <b>%s</b> to group <b>%s</b> and executed\n"
						  "using <b>%s</b> processor(s) distributed on <b>%d</b> servers.\n"),
						  gebr_job_get_hostname(job), groups, nprocs, n_servers);
		g_string_append(resources, markup);

		if (!g_strcmp0(niceness, "0"))
			g_string_append_printf(resources, _("Using all machines resources\n"));
		else
			g_string_append_printf(resources, _("Using the machines idle time\n"));

		g_free(nprocs);
		g_free(niceness);
		g_free(markup);
	}
	gtk_label_set_markup (res_label, resources->str);

	g_string_free(resources, TRUE);

	GtkSizeGroup *sgroup = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	GtkBox *servers_box = GTK_BOX(gtk_builder_get_object(jc->priv->builder, "servers_box"));
	gtk_container_foreach(GTK_CONTAINER(servers_box), (GtkCallback)gtk_widget_destroy, NULL);

	for (i = 0; servers[i]; i++) {
		const gchar *server;
		if (!g_strcmp0(servers[i], "127.0.0.1"))
			 server = server_get_name_from_address(servers[i]);
		else
			server = servers[i];
		GtkWidget *hbox = gtk_hbox_new(FALSE, 5);
		GtkWidget *pbar = gtk_progress_bar_new();
		GtkWidget *label = gtk_label_new(server);

		GebrTask *task = gebr_job_get_task_from_server(job, servers[i]);

		gtk_size_group_add_widget(sgroup, label);
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pbar),
					      task? gebr_task_get_percentage(task) : 0);
		gtk_widget_set_size_request(pbar, -1, 5);

		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(hbox), pbar, FALSE, TRUE, 0);
		gtk_box_pack_start(servers_box, hbox, FALSE, TRUE, 0);
		gtk_widget_show_all(hbox);
	}
}

static void
job_control_disconnect_signals(GebrJobControl *jc,
                               gboolean finished,
                               gboolean running,
                               gboolean saved)
{
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_job_control, "job_control_close"), finished);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_job_control, "job_control_stop"), running);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_job_control, "job_control_save"), saved);

	if (jc->priv->last_selection.job) {
		g_signal_handler_disconnect(jc->priv->last_selection.job,
		                            jc->priv->last_selection.sig_output);
		g_signal_handler_disconnect(jc->priv->last_selection.job,
		                            jc->priv->last_selection.sig_status);
		g_signal_handler_disconnect(jc->priv->last_selection.job,
		                            jc->priv->last_selection.sig_issued);
		g_signal_handler_disconnect(jc->priv->last_selection.job,
		                            jc->priv->last_selection.sig_cmd_line);
	}
}

static void
gebr_jc_get_jobs_state(GebrJobControl *jc,
                        GList *jobs,
                        gboolean *finished,
                        gboolean *running)
{
	*finished = FALSE;
	*running = FALSE;
	GtkTreeIter iter;
	GebrJob *job;

	for (GList *i = jobs; i; i = i->next) {
		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(jc->priv->store), &iter, (GtkTreePath*)i->data)) {
			gtk_tree_model_get(GTK_TREE_MODEL(jc->priv->store), &iter, JC_STRUCT, &job, -1);
			if (!(*finished) && gebr_job_is_stopped(job))
				*finished = TRUE;
			if (!(*running) && !gebr_job_is_stopped(job))
				*running = TRUE;
		}
	}
}

static void
job_control_on_cursor_changed(GtkTreeSelection *selection,
			      GebrJobControl *jc)
{
	GtkTreeModel *model;
	GList *rows = gtk_tree_selection_get_selected_rows(selection, &model);
	gboolean finished = FALSE, running = FALSE;

	if (!rows) {
		job_control_disconnect_signals(jc, finished, running, FALSE);
		jc->priv->last_selection.job = NULL;
		gebr_job_control_info_set_visible(jc, FALSE, _("Please, select a job at the list on the left"));
		return;
	}

	GebrJob *job = NULL;

	if (!rows->next) {
		GtkTreeIter iter;
		if (gtk_tree_model_get_iter(model, &iter, (GtkTreePath*)rows->data))
			gtk_tree_model_get(model, &iter, JC_STRUCT, &job, -1);
		else
			g_warn_if_reached();
	} else
		gebr_jc_get_jobs_state(jc, rows, &finished, &running);

	gboolean has_job = (job != NULL);

	if (!has_job) {
		job_control_disconnect_signals(jc, finished, running, TRUE);
		jc->priv->last_selection.job = NULL;
		gebr_job_control_info_set_visible(jc, FALSE, _("Multiple jobs selected"));
		return;
	}

	gebr_job_control_info_set_visible(jc, TRUE, NULL);
	GebrJob *old_job = jc->priv->last_selection.job;

	if (has_job) {
		finished = gebr_job_is_stopped(job);
		job_control_disconnect_signals(jc, finished, !finished, TRUE);

		jc->priv->last_selection.job = job;
		jc->priv->last_selection.sig_output =
				g_signal_connect(job, "output", G_CALLBACK(on_job_output), jc);
		jc->priv->last_selection.sig_status =
				g_signal_connect(job, "status-change", G_CALLBACK(on_job_status), jc);
		jc->priv->last_selection.sig_issued =
				g_signal_connect(job, "issued", G_CALLBACK(on_job_issued), jc);
		jc->priv->last_selection.sig_cmd_line =
				g_signal_connect(job, "cmd-line-received", G_CALLBACK(on_job_cmd_line_received), jc);

		gebr_job_control_load_details(jc, job);
	}

	if (job == old_job)
		return;

	GtkButton *button = GTK_BUTTON(gtk_builder_get_object(jc->priv->builder, "show_issues_button"));
	GtkInfoBar *info = GTK_INFO_BAR(gtk_builder_get_object(jc->priv->builder, "issues_info_bar"));

	gtk_widget_hide(GTK_WIDGET(info));

	if (has_job && gebr_job_has_issues(job)) {
		GtkImage *image = GTK_IMAGE(gtk_builder_get_object(jc->priv->builder, "show_issues_img"));
		gtk_image_set_from_stock(image, get_issues_image_for_status(job), GTK_ICON_SIZE_DND);
		gtk_widget_show(GTK_WIDGET(button));
	} else
		gtk_widget_hide(GTK_WIDGET(button));


	g_list_foreach(rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free(rows);
}

static void
on_toggled_more_details(GtkToggleButton *button,
                        GtkBuilder *builder)
{
	GtkVBox *details = GTK_VBOX(gtk_builder_get_object(builder, "details_widget"));

	if (!gtk_toggle_button_get_active(button)) {
		gtk_widget_set_visible(GTK_WIDGET(details), FALSE);
		return;
	}
	gtk_widget_set_visible(GTK_WIDGET(details), TRUE);
}

static const gchar *
job_control_get_icon_for_job(GebrJob *job)
{
	const gchar *stock_id;

	switch (gebr_job_get_status(job))
	{
	case JOB_STATUS_CANCELED:
	case JOB_STATUS_FAILED:
		stock_id = GTK_STOCK_CANCEL;
		break;
	case JOB_STATUS_FINISHED:
		stock_id = GTK_STOCK_APPLY;
		break;
	case JOB_STATUS_INITIAL:
		stock_id = GTK_STOCK_NETWORK;
		break;
	case JOB_STATUS_QUEUED:
		stock_id = "chronometer";
		break;
	case JOB_STATUS_RUNNING:
		stock_id = GTK_STOCK_EXECUTE;
		break;
	case JOB_STATUS_ISSUED:
	case JOB_STATUS_REQUEUED:
		break;
	}
	return stock_id;
}

static void
icon_column_data_func(GtkTreeViewColumn *tree_column,
		      GtkCellRenderer *cell,
		      GtkTreeModel *tree_model,
		      GtkTreeIter *iter,
		      gpointer data)
{
	GebrJob *job;
	const gchar *stock_id;

	gtk_tree_model_get(tree_model, iter, JC_STRUCT, &job, -1);

	stock_id = job_control_get_icon_for_job(job);

	g_object_set(cell, "stock-id", stock_id, NULL);
}

static void
title_column_data_func(GtkTreeViewColumn *tree_column,
		       GtkCellRenderer *cell,
		       GtkTreeModel *tree_model,
		       GtkTreeIter *iter,
		       gpointer data)
{
	GebrJob *job;

	gtk_tree_model_get(tree_model, iter, JC_STRUCT, &job, -1);
	g_object_set(cell, "text", gebr_job_get_title(job), NULL);
}

static void
time_column_data_func(GtkTreeViewColumn *tree_column,
		      GtkCellRenderer *cell,
		      GtkTreeModel *tree_model,
		      GtkTreeIter *iter,
		      gpointer data)
{
	GebrJob *job;
	const gchar *start_time_str;
	gchar *relative_time_msg;

	GTimeVal start_time, curr_time;

	gtk_tree_model_get(tree_model, iter, JC_STRUCT, &job, -1);

	start_time_str = gebr_job_get_last_run_date(job);
	if (!start_time_str)
		return;
	g_time_val_from_iso8601(start_time_str, &start_time);
	g_get_current_time(&curr_time);


	gchar *str_aux = gebr_calculate_relative_time(&start_time, &curr_time);
	relative_time_msg = g_strconcat( str_aux, _(" ago"), NULL );

	g_object_set(cell, "text", relative_time_msg, NULL);
	g_free(str_aux);
	g_free(relative_time_msg);

	return;
}

static void
gebr_jc_update_status_and_time(GebrJobControl *jc,
                               GebrJob 	      *job,
                               enum JobStatus status)
{
	const gchar *start_date = gebr_job_get_start_date(job);
	const gchar *finish_date = gebr_job_get_finish_date(job);
	GString *start = g_string_new(NULL);
	GString *finish = g_string_new(NULL);

	/* start date (may have failed, never started) */
	if (start_date && strlen(start_date))
		g_string_append_printf(start, "%s %s", _("Started at"),
		                       gebr_localized_date(start_date));

	/* finish date */
	if (finish_date && strlen(finish_date))
		g_string_append_printf(finish, "%s %s", status == JOB_STATUS_FINISHED ?
				       _("Finished at") : _("Canceled at"),
				       gebr_localized_date(finish_date));

	GtkImage *img = GTK_IMAGE(gtk_builder_get_object(jc->priv->builder, "status_image"));
	GtkLabel *subheader = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "subheader_label"));
	GtkButton *queued_button = GTK_BUTTON(gtk_builder_get_object(jc->priv->builder, "subheader_button"));
	GtkLabel *details_start_date = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "detail_start_date"));
	gtk_widget_hide(GTK_WIDGET(queued_button));

	job_control_fill_servers_info(jc);

	if (status == JOB_STATUS_FINISHED) {
		gchar *elapsed_time = g_strdup_printf("%s\nElapsed time: %s", finish->str, gebr_job_get_elapsed_time(job));
		gtk_image_set_from_stock(img, GTK_STOCK_APPLY, GTK_ICON_SIZE_DIALOG);
		gtk_label_set_text(subheader, elapsed_time);
		gtk_label_set_text(details_start_date, start->str);
		gtk_widget_show(GTK_WIDGET(details_start_date));
		g_free(elapsed_time);
	}

	else if (status == JOB_STATUS_RUNNING) {
		gchar *running = g_strdup_printf("%s\nElapsed time: %s", start->str, gebr_job_get_running_time(job, start_date));
		gtk_image_set_from_stock(img, GTK_STOCK_EXECUTE, GTK_ICON_SIZE_DIALOG);
		gtk_label_set_text(subheader, running);
		gtk_widget_hide(GTK_WIDGET(details_start_date));
		g_free(running);
	}

	else if (status == JOB_STATUS_CANCELED) {
		gtk_image_set_from_stock(img, GTK_STOCK_CANCEL, GTK_ICON_SIZE_DIALOG);
		gtk_label_set_text(subheader, finish->str);
		gtk_label_set_text(details_start_date, start->str);
	}

	else if (status == JOB_STATUS_FAILED) {
		gtk_image_set_from_stock(img, GTK_STOCK_CANCEL, GTK_ICON_SIZE_DIALOG);
		gtk_label_set_text(subheader, _("Job failed"));
		gtk_label_set_text(details_start_date, start->str);
	}

	else if (status == JOB_STATUS_QUEUED) {
		GebrJob *parent;
		GtkImage *wait_img = GTK_IMAGE(gtk_builder_get_object(jc->priv->builder, "subheader_button_img"));
		GtkLabel *wait_label = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "subheader_button_lbl"));

		parent = gebr_job_control_find(jc, gebr_job_get_queue(job));

		if (parent) {
			gtk_image_set_from_stock(img, "chronometer", GTK_ICON_SIZE_DIALOG);
			gtk_label_set_text(subheader, "Waiting for job");

			gtk_image_set_from_stock(wait_img, job_control_get_icon_for_job(parent), GTK_ICON_SIZE_BUTTON);
			gtk_label_set_text(wait_label, gebr_job_get_title(parent));

			gtk_widget_show(GTK_WIDGET(queued_button));
		}
	}
	else if (status == JOB_STATUS_INITIAL) {
		gchar *remainings = gebr_job_get_remaining_servers(job);
		gchar *wait_text = g_strdup_printf(_("Waiting for servers: %s"), remainings? remainings : "");
		gtk_image_set_from_stock(img, GTK_STOCK_NETWORK, GTK_ICON_SIZE_DIALOG);
		gtk_label_set_text(subheader, wait_text);
		gtk_widget_hide(GTK_WIDGET(details_start_date));
		if (remainings)
			g_free(remainings);
		g_free(wait_text);
	}

	g_string_free(start, FALSE);
	g_string_free(finish, FALSE);
}

static void
gebr_job_control_include_cmd_line(GebrJobControl *jc,
                                  GebrJob *job)
{
	GString *info_cmd = g_string_new("");
	GtkTextIter end_iter_cmd;
	GtkScrolledWindow *scroll = GTK_SCROLLED_WINDOW(gtk_builder_get_object(jc->priv->builder, "scrolledwindow2"));
	GtkWidget *text = gtk_bin_get_child(GTK_BIN(scroll));

	if (text) {
		GList *child = gtk_container_get_children(GTK_CONTAINER(text));
		if (child)
			g_list_free(child);
		gtk_widget_destroy(text);
	}

	/* command-line */
	GList *tasks = gebr_job_get_list_of_tasks(job);
	if (tasks && g_list_length(tasks) > 1) {
		jc->priv->cmd_view = NULL;
		GtkWidget *vbox = gtk_vbox_new(FALSE, 8);

		GtkAdjustment *hadj = gtk_scrolled_window_get_hadjustment(scroll);
		GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(scroll);
		GtkWidget *viewport = gtk_viewport_new(hadj, vadj);

		for (GList *i = tasks; i; i = i->next) {
			gint frac, total;
			GebrTask *task = i->data;
			gebr_task_get_fraction(task, &frac, &total);
			GtkWidget *expander;

			GString *new_buf = g_string_new(NULL);
			g_string_append_printf(new_buf, _("Command line for task %d of %d "), frac, total);
			g_string_append_printf(new_buf, _(" \(Server: %s)"), (gebr_task_get_server(task))->comm->address->str);

			expander = gtk_expander_new(new_buf->str);
			gtk_expander_set_expanded(GTK_EXPANDER(expander), FALSE);

			GtkTextIter task_iter;
			GtkTextBuffer *task_buf = gtk_text_buffer_new(NULL);
			gchar *task_line = g_strdup_printf("%s\n", gebr_task_get_cmd_line(task));
			gtk_text_buffer_get_end_iter(task_buf, &task_iter);
			gtk_text_buffer_insert(task_buf, &task_iter, task_line, strlen(task_line));

			GtkWidget *task_view = gtk_text_view_new_with_buffer(task_buf);

			PangoFontDescription *font;
			font = pango_font_description_new();
			pango_font_description_set_family(font, "monospace");
			gtk_widget_modify_font(task_view, font);
			pango_font_description_free(font);

			g_object_set(task_view, "wrap-mode", GTK_WRAP_WORD, NULL);
			gtk_container_add (GTK_CONTAINER (expander), task_view);
			gtk_box_pack_start(GTK_BOX(vbox), expander, FALSE, FALSE, 0);

			g_string_free(new_buf, TRUE);
			g_free(task_line);
		}
		gtk_container_add(GTK_CONTAINER(viewport), vbox);
		gtk_container_add(GTK_CONTAINER(scroll), viewport);
		gtk_widget_show_all(viewport);
	} else {
		GtkTextIter iter_end;
		jc->priv->cmd_buffer = gtk_text_buffer_new(NULL);
		gtk_text_buffer_get_end_iter(jc->priv->cmd_buffer, &iter_end);
		gtk_text_buffer_create_mark(jc->priv->cmd_buffer, "end", &iter_end, FALSE);
		jc->priv->cmd_view = gtk_text_view_new_with_buffer(jc->priv->cmd_buffer);
		g_object_set(jc->priv->cmd_view, "wrap-mode", gebr.config.job_log_word_wrap ? GTK_WRAP_WORD : GTK_WRAP_NONE, NULL);
		g_object_set(G_OBJECT(jc->priv->cmd_view), "editable", FALSE, "cursor-visible", FALSE, NULL);

		PangoFontDescription *font;
		font = pango_font_description_new();
		pango_font_description_set_family(font, "monospace");
		gtk_widget_modify_font(jc->priv->cmd_view, font);
		pango_font_description_free(font);

		g_signal_connect(jc->priv->cmd_view, "populate-popup", G_CALLBACK(on_text_view_populate_popup), jc);

		gtk_container_add(GTK_CONTAINER(scroll), jc->priv->cmd_view);
		gtk_widget_show_all(jc->priv->cmd_view);

		gchar *cmdline = gebr_job_get_command_line(job);
		g_string_append_printf(info_cmd, "%s\n", cmdline);
		g_free(cmdline);

		gtk_text_buffer_get_end_iter(jc->priv->cmd_buffer, &end_iter_cmd);
		gtk_text_buffer_insert(jc->priv->cmd_buffer, &end_iter_cmd, info_cmd->str, info_cmd->len);
	}
	g_string_free(info_cmd, TRUE);
}

static void
gebr_job_control_load_details(GebrJobControl *jc,
			      GebrJob *job)
{
	g_return_if_fail(job != NULL);

	GString *info = g_string_new("");
	GtkTextIter end_iter;
	GtkImage *info_button_image;
	GtkLabel *input_file, *output_file, *log_file, *job_group;
	gchar *input_file_str, *output_file_str, *log_file_str;
	enum JobStatus status = gebr_job_get_status(job);

	input_file = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "input_label"));
	output_file = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "output_label"));
	log_file = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "error_label"));
	job_group = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "job_group"));
	info_button_image = GTK_IMAGE(gtk_bin_get_child(GTK_BIN(jc->priv->info_button)));

	gebr_job_get_io(job, &input_file_str, &output_file_str, &log_file_str);

	gchar *markup;

	markup = g_markup_printf_escaped ("<b>Input file</b>: %s", input_file_str ? input_file_str : _("None"));
	gtk_label_set_markup (input_file, markup);
	g_free(markup);

	markup = g_markup_printf_escaped ("<b>Output file</b>: %s", output_file_str ? output_file_str : _("None"));
	gtk_label_set_markup (output_file, markup);
	g_free(markup);

	markup = g_markup_printf_escaped ("<b>Log File</b>: %s", log_file_str ? log_file_str : _("None"));
	gtk_label_set_markup (log_file, markup);
	g_free(markup);

	gchar *msg = g_strdup(gebr_job_get_server_group(job));
	if (!g_strcmp0(msg, ""))
		msg = g_strdup("All servers");
	gtk_label_set_markup (job_group, msg);

	switch (gebr_job_get_exec_speed(job))
	{
	case 1:
		gtk_image_set_from_stock(info_button_image, "gebr-speed-low", GTK_ICON_SIZE_LARGE_TOOLBAR);
		break;
	case 2: case 3: case 4:
		gtk_image_set_from_stock(info_button_image, "gebr-speed-medium", GTK_ICON_SIZE_LARGE_TOOLBAR);
		break;
	case 5:
		gtk_image_set_from_stock(info_button_image, "gebr-speed-high", GTK_ICON_SIZE_LARGE_TOOLBAR);
		break;
	default:
		g_warn_if_reached();
	}

	gebr_jc_update_status_and_time(jc, job, status);

	GtkLabel *label = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "header_label"));
	const gchar *title = gebr_job_get_title(job);
	markup = g_markup_printf_escaped ("<span size=\"large\"><b>%s</b></span>", title);
	gtk_label_set_markup (label, markup);
	g_free (markup);

	gtk_text_buffer_set_text(jc->priv->text_buffer, "", 0);

	gebr_job_control_include_cmd_line(jc, job);

	/* output */
	g_string_append(info, gebr_job_get_output(job));

	gtk_text_buffer_get_end_iter(jc->priv->text_buffer, &end_iter);
	gtk_text_buffer_insert(jc->priv->text_buffer, &end_iter, info->str, info->len);

	g_string_free(info, TRUE);
}

static gboolean
update_tree_view(gpointer data)
{
	GtkTreeIter iter;
	GebrJobControl *jc = data;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(jc->priv->view));
	gboolean valid = gtk_tree_model_get_iter_first(model, &iter);

	while (valid)
	{
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_model_row_changed(model, path, &iter);
		gtk_tree_path_free(path);
		valid = gtk_tree_model_iter_next(model, &iter);
	}

	GebrJob *job = get_selected_job(jc);
	if (job) {
		enum JobStatus status = gebr_job_get_status(job);
		if (status == JOB_STATUS_RUNNING)
			gebr_jc_update_status_and_time(jc, job, status);
	}
	return TRUE;
}

static gint
tree_sort_func(GtkTreeModel *model,
	       GtkTreeIter *a,
	       GtkTreeIter *b,
	       gpointer user_data)
{
	GebrJob *ja, *jb;

	gtk_tree_model_get(model, a, JC_STRUCT, &ja, -1);
	gtk_tree_model_get(model, b, JC_STRUCT, &jb, -1);

	if (!ja && !jb)
		return 0;

	if (!ja)
		return 1;

	if (!jb)
		return -1;

	const gchar *ta = gebr_job_get_last_run_date(ja);
	const gchar *tb = gebr_job_get_last_run_date(jb);

	if (!ta && !tb)
		return 0;

	if (!ta)
		return 1;

	if (!tb)
		return -1;

	GTimeVal tva;
	GTimeVal tvb;
	g_time_val_from_iso8601(ta, &tva);
	g_time_val_from_iso8601(tb, &tvb);

	return tva.tv_sec - tvb.tv_sec;
}

static gboolean
server_group_separator_func(GtkTreeModel *model,
			    GtkTreeIter *iter,
			    gpointer data)
{
	gboolean is_sep;
	gtk_tree_model_get(model, iter, TAG_SEP, &is_sep, -1);
	return is_sep;
}

static void
gebr_jc_populate_status_cb(GebrJobControl *jc)
{
	GtkTreeIter iter;
	GtkListStore *store = jc->priv->status_model;

	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(jc->priv->status_model), &iter);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
	                   ST_TEXT, _("Any"),
	                   ST_STATUS, -1,
	                   -1);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
	                   ST_ICON, GTK_STOCK_APPLY,
	                   ST_TEXT, _("Finished"),
	                   ST_STATUS, JOB_STATUS_FINISHED,
	                   -1);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
	                   ST_ICON, GTK_STOCK_CANCEL,
	                   ST_TEXT, _("Canceled/Failed"),
	                   ST_STATUS, JOB_STATUS_CANCELED,
	                   -1);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
	                   ST_ICON, GTK_STOCK_EXECUTE,
	                   ST_TEXT, _("Running"),
	                   ST_STATUS, JOB_STATUS_RUNNING,
	                   -1);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
	                   ST_ICON, "chronometer",
	                   ST_TEXT, _("Queued"),
	                   ST_STATUS, JOB_STATUS_QUEUED,
	                   -1);
}

static void
on_job_disconnected(GebrJob *job, GebrJobControl *jc)
{
	BLOCK_SELECTION_CHANGED_SIGNAL(jc);
	gebr_job_control_remove(jc, job);
	UNBLOCK_SELECTION_CHANGED_SIGNAL(jc);
}

static void
wordwrap_toggled(GtkCheckMenuItem * check_menu_item, GebrJobControl * jc)
{
	gebr.config.job_log_word_wrap = gtk_check_menu_item_get_active(check_menu_item);
	g_object_set(G_OBJECT(jc->priv->text_view), "wrap-mode",
		     gebr.config.job_log_word_wrap ? GTK_WRAP_WORD : GTK_WRAP_NONE, NULL);
	g_object_set(G_OBJECT(jc->priv->cmd_view), "wrap-mode",
			     gebr.config.job_log_word_wrap ? GTK_WRAP_WORD : GTK_WRAP_NONE, NULL);
}

static void
autoscroll_toggled(GtkCheckMenuItem * check_menu_item)
{
	gebr.config.job_log_auto_scroll = gtk_check_menu_item_get_active(check_menu_item);
}

static void
on_text_view_populate_popup(GtkTextView * text_view, GtkMenu * menu, GebrJobControl *jc)
{
	GtkWidget *menu_item;

	menu_item = gtk_separator_menu_item_new();
	gtk_widget_show(menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_check_menu_item_new_with_label(_("Word-wrap"));
	gtk_widget_show(menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(menu_item, "toggled", G_CALLBACK(wordwrap_toggled), jc);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), gebr.config.job_log_word_wrap);

	menu_item = gtk_check_menu_item_new_with_label(_("Auto-scroll"));
	gtk_widget_show(menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(menu_item, "toggled", G_CALLBACK(autoscroll_toggled), NULL);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), gebr.config.job_log_auto_scroll);
}

static GtkMenu *
job_control_popup_menu(GtkWidget * widget, GebrJobControl *jc)
{
	GtkWidget *menu;
	GtkTreeModel *model;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->priv->view));
	GList *rows = gtk_tree_selection_get_selected_rows(selection, &model);

	menu = gtk_menu_new();

	if (rows) {

		gtk_container_add(GTK_CONTAINER(menu),
		                  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_job_control, "job_control_save")));
		gtk_container_add(GTK_CONTAINER(menu),
		                  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_job_control, "job_control_close")));
		gtk_container_add(GTK_CONTAINER(menu),
		                  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_job_control, "job_control_stop")));

		gtk_widget_show_all(menu);

		g_list_foreach(rows, (GFunc)gtk_tree_path_free, NULL);
		g_list_free(rows);

		return GTK_MENU(menu);
	}
	return NULL;
}

static void
on_filter_show_unselect_jobs(GtkToggleButton *button,
			     GebrJobControl *jc)
{
	if (gtk_toggle_button_get_active(button))
		gebr_job_control_select_job(jc, NULL);
}

/* Public methods {{{1 */
GebrJobControl *
gebr_job_control_new(void)
{
	GebrJobControl *jc;

	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	jc = g_new(GebrJobControl, 1);
	jc->priv = g_new0(GebrJobControlPriv, 1);

	jc->priv->builder = gtk_builder_new();
	gtk_builder_add_from_file(jc->priv->builder, GEBR_GLADE_DIR"/gebr-job-control.glade", NULL);

	jc->priv->widget = GTK_WIDGET(gtk_builder_get_object(jc->priv->builder, "top-level-widget"));

	/*
	 * Left side
	 */

	jc->priv->store = gtk_list_store_new(JC_N_COLUMN, G_TYPE_POINTER);

	GtkTreeModel *filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(jc->priv->store), NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filter),
					       (GtkTreeModelFilterVisibleFunc)jobs_visible_func,
					       jc, NULL);

	GtkTreeModel *sort = gtk_tree_model_sort_new_with_model(filter);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(sort), 0, tree_sort_func, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(sort), 0, GTK_SORT_DESCENDING);

	GtkTreeView *treeview;
	treeview = GTK_TREE_VIEW(gtk_builder_get_object(jc->priv->builder, "treeview_jobs"));
	gtk_tree_view_set_model(treeview, sort);
	g_object_unref(filter);
	g_object_unref(sort);

	gtk_widget_set_size_request(GTK_WIDGET(treeview), 150, -1);

	jc->priv->view = GTK_WIDGET(treeview);

	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->priv->view)),
				    GTK_SELECTION_MULTIPLE);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(jc->priv->view), FALSE);
	g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->priv->view)), "changed",
			 G_CALLBACK(job_control_on_cursor_changed), jc);

	col = GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(jc->priv->builder, "tv_column"));

	renderer = GTK_CELL_RENDERER(gtk_builder_get_object(jc->priv->builder, "tv_icon_cell"));
	gtk_tree_view_column_set_cell_data_func(col, renderer, icon_column_data_func, NULL, NULL);

	renderer = GTK_CELL_RENDERER(gtk_builder_get_object(jc->priv->builder, "tv_title_cell"));
	gtk_tree_view_column_set_cell_data_func(col, renderer, title_column_data_func, NULL, NULL);

	renderer = GTK_CELL_RENDERER(gtk_builder_get_object(jc->priv->builder, "tv_time_cell"));
	gtk_tree_view_column_set_cell_data_func(col, renderer, time_column_data_func, NULL, NULL);

	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(jc->priv->view),
	                                          (GebrGuiGtkPopupCallback) job_control_popup_menu, jc);

	/*
	 * Filter
	 */

	GtkCellRenderer *cell;

	GtkComboBox *group_cb = GTK_COMBO_BOX(gtk_builder_get_object(jc->priv->builder, "filter-servers-group-cb"));
	cell = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(group_cb), cell, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(group_cb), cell, "text", 0);
	jc->priv->group_combo = group_cb;

	GtkComboBox *server_cb = GTK_COMBO_BOX(gtk_builder_get_object(jc->priv->builder, "filter-servers-cb"));
	cell = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(server_cb), cell, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(server_cb), cell, "text", 0);
	jc->priv->server_combo = server_cb;

	GtkComboBox *status_cb = GTK_COMBO_BOX(gtk_builder_get_object(jc->priv->builder, "filter-status-cb"));
	cell = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(status_cb), cell, FALSE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(status_cb), cell, "stock-id", ST_ICON);
	cell = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(status_cb), cell, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(status_cb), cell, "text", ST_TEXT);
	jc->priv->status_combo = status_cb;

	/* by Servers */
	jc->priv->server_filter = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
	gtk_combo_box_set_model(server_cb, GTK_TREE_MODEL(jc->priv->server_filter));
	g_signal_connect(jc->priv->server_combo, "changed", G_CALLBACK(on_cb_changed), jc);
	//g_signal_connect(gebr.ui_server_list->common.store, "row-changed", G_CALLBACK(build_servers_filter_list), jc);

	/* by Group of servers */
	jc->priv->group_filter = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	GtkTreeIter iter;
	gtk_list_store_append(jc->priv->group_filter, &iter);
	gtk_list_store_set(jc->priv->group_filter, &iter, 0, _("Any"), -1);
	gtk_combo_box_set_model(group_cb, GTK_TREE_MODEL(jc->priv->group_filter));
	g_signal_connect(jc->priv->group_combo, "changed", G_CALLBACK(on_cb_changed), jc);
	gtk_combo_box_set_active(jc->priv->group_combo, 0);

	/* by Status of job */
	GtkListStore *store = gtk_list_store_new(ST_N_COLUMN,
	                                         G_TYPE_STRING,
	                                         G_TYPE_STRING,
	                                         G_TYPE_INT);
	jc->priv->status_model = store;
	gebr_jc_populate_status_cb(jc);
	gtk_combo_box_set_model(status_cb, GTK_TREE_MODEL(jc->priv->status_model));
	g_signal_connect(jc->priv->status_combo, "changed", G_CALLBACK(on_cb_changed), jc);
	gtk_combo_box_set_active(jc->priv->status_combo, 0);


	/*
	 * Right side
	 */
	GtkButton *dismiss;
	dismiss = GTK_BUTTON(gtk_builder_get_object(jc->priv->builder, "issues_info_bar_dismiss"));
	g_signal_connect(dismiss, "clicked", G_CALLBACK(on_dismiss_clicked), jc);

	GtkButton *show_issues;
	show_issues = GTK_BUTTON(gtk_builder_get_object(jc->priv->builder, "show_issues_button"));
	g_signal_connect(show_issues, "clicked", G_CALLBACK(on_show_issues_clicked), jc);

	GtkButton *wait_button;
	wait_button = GTK_BUTTON(gtk_builder_get_object(jc->priv->builder, "subheader_button"));
	g_signal_connect(wait_button, "clicked", G_CALLBACK(on_job_wait_button), jc);

	jc->priv->info_button = gebr_gui_tool_button_new();
	gtk_button_set_relief(GTK_BUTTON(jc->priv->info_button), GTK_RELIEF_NONE);
	gtk_container_add(GTK_CONTAINER(jc->priv->info_button), gtk_image_new());
	gtk_widget_show_all(jc->priv->info_button);
	GtkBox *box = GTK_BOX(gtk_builder_get_object(jc->priv->builder, "info_box"));
	GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object(jc->priv->builder, "servers_widget"));
	gebr_gui_tool_button_add(GEBR_GUI_TOOL_BUTTON(jc->priv->info_button), widget);
	gtk_box_pack_start(box, jc->priv->info_button, FALSE, TRUE, 0);

	GtkInfoBar *info = GTK_INFO_BAR(gtk_builder_get_object(jc->priv->builder, "issues_info_bar"));
	if (!gtk_widget_get_visible(GTK_WIDGET(info)))
		gtk_widget_show(GTK_WIDGET(show_issues));
	else
		gtk_widget_hide(GTK_WIDGET(show_issues));

	GtkWidget *text_view;
	GtkTextIter iter_end;
	
	GtkToggleButton *details = GTK_TOGGLE_BUTTON(gtk_builder_get_object(jc->priv->builder, "more_details"));
	g_signal_connect(details, "toggled", G_CALLBACK(on_toggled_more_details), jc->priv->builder);
	gtk_toggle_button_set_active(details, FALSE);

	/* Text view of output*/
	jc->priv->text_buffer = gtk_text_buffer_new(NULL);
	gtk_text_buffer_get_end_iter(jc->priv->text_buffer, &iter_end);
	gtk_text_buffer_create_mark(jc->priv->text_buffer, "end", &iter_end, FALSE);

	text_view = GTK_WIDGET(gtk_builder_get_object(jc->priv->builder, "textview_output"));
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(text_view), jc->priv->text_buffer);
	g_object_set(text_view, "wrap-mode", gebr.config.job_log_word_wrap ? GTK_WRAP_WORD : GTK_WRAP_NONE, NULL);

	g_object_set(G_OBJECT(text_view), "editable", FALSE, "cursor-visible", FALSE, NULL);
	PangoFontDescription *font;

	font = pango_font_description_new();
	pango_font_description_set_family(font, "monospace");
	gtk_widget_modify_font(text_view, font);

	pango_font_description_free(font);

	jc->priv->text_view = text_view;
	g_signal_connect(jc->priv->text_view, "populate-popup", G_CALLBACK(on_text_view_populate_popup), jc);

	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_job_control, "job_control_close"), FALSE);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_job_control, "job_control_stop"), FALSE);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_job_control, "job_control_save"), FALSE);

	return jc;
}

void
gebr_job_control_free(GebrJobControl *jc)
{
	g_object_unref(jc->priv->builder);
	g_free(jc->priv);
	g_free(jc);
}

GtkWidget *
gebr_job_control_get_widget(GebrJobControl *jc)
{
	return jc->priv->widget;
}

void
gebr_job_control_add(GebrJobControl *jc, GebrJob *job)
{
	gtk_list_store_append(jc->priv->store, gebr_job_get_iter(job));
	gtk_list_store_set(jc->priv->store, gebr_job_get_iter(job), JC_STRUCT, job, -1);
	g_signal_connect(job, "disconnect", G_CALLBACK(on_job_disconnected), jc);

	const gchar *group = gebr_job_get_group(job);
	if (!group)
		group = "";

	if (!get_server_group_iter(jc, group, NULL)) {
		GtkTreeIter iter;
		const gchar *tmp;

		if (g_strcmp0(group, "") == 0)
			tmp  = _("All Servers");
		else
			tmp = group;

		gtk_list_store_append(jc->priv->group_filter, &iter);
		gtk_list_store_set(jc->priv->group_filter, &iter,
				   0, tmp,
				   1, group, -1);
	}
}

GebrJob *
gebr_job_control_find(GebrJobControl *jc, const gchar *rid)
{
	GebrJob *job = NULL;

	g_return_val_if_fail(rid != NULL, NULL);

	gboolean job_find_foreach_func(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
	{
		GebrJob *i;

		gtk_tree_model_get(model, iter, JC_STRUCT, &i, -1);

		if (!i)
			return FALSE;

		if (g_strcmp0(gebr_job_get_id(i), rid) == 0) {
			job = i;
			return TRUE;
		}
		return FALSE;
	}

	gtk_tree_model_foreach(GTK_TREE_MODEL(jc->priv->store), job_find_foreach_func, NULL);

	return job;
}

gboolean
gebr_job_control_save_selected(GebrJobControl *jc)
{
	GtkWidget *chooser_dialog;
	GtkFileFilter *filefilter;
	gchar *fname;
	FILE *fp;

	/* run file chooser */
	chooser_dialog = gebr_gui_save_dialog_new(_("Choose filename to save"), GTK_WINDOW(gebr.window));
	gebr_gui_save_dialog_set_default_extension(GEBR_GUI_SAVE_DIALOG(chooser_dialog), ".txt");

	filefilter = gtk_file_filter_new();
	gtk_file_filter_set_name(filefilter, _("Text (*.txt)"));
	gtk_file_filter_add_pattern(filefilter, "*.txt");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_dialog), filefilter);

	/* show file chooser */
	fname = gebr_gui_save_dialog_run(GEBR_GUI_SAVE_DIALOG(chooser_dialog));
	if (!fname)
		return TRUE;

	/* save to file */
	fp = fopen(fname, "w");
	if (fp == NULL) {
		gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Could not write file."));
		g_free(fname);
		return TRUE;
	}

	GtkTreeIter iter;
	GtkTreeModel *model;
	GebrJob *job;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(jc->priv->view));

	gebr_gui_gtk_tree_view_foreach_selected(&iter, jc->priv->view) {

		gtk_tree_model_get(model, &iter, JC_STRUCT, &job, -1);
		
		gchar * title;
		title = g_strdup_printf("---------- %s ---------\n", gebr_job_get_title(job));
		fputs(title, fp);
		g_free(title);

		/* Start and Finish dates */
		const gchar *start_date = gebr_job_get_start_date(job);
		const gchar *finish_date = gebr_job_get_finish_date(job);
		gchar *dates;
		dates = g_strdup_printf("\nStart date: %s\nFinish date: %s\n", start_date? gebr_localized_date(start_date): "(None)",
					finish_date? gebr_localized_date(finish_date) : "(None)");
		fputs(dates, fp);
		g_free(dates);

		/* Issues */
		gchar *issues;
		gchar *job_issue = gebr_job_get_issues(job);
		issues = g_strdup_printf("\nIssues:\n%s", strlen(job_issue)? job_issue : "(None)\n");
		fputs(issues, fp);
		g_free(issues);
		g_free(job_issue);

		/* Input, output and log files */
		gchar *io, *input, *output, *log;
		gebr_job_get_io(job, &input, &output, &log);
		io = g_strdup_printf(_("\nInput File: %s\nOutput File: %s\nLog File: %s\n"), input? input : "(None)",
				     output? output : "(None)", log? log : "(None)");
		fputs(io, fp);
		g_free(io);
		g_free(input);
		g_free(output);
		g_free(log);

		/* Command line */
		gchar *cmd_line;
		gchar *command = gebr_job_get_command_line(job);
		cmd_line = g_strdup_printf(_("\nCommand Line:\n%s\n"), strlen(command)? command : "(None)");
		fputs(cmd_line, fp);
		g_free(cmd_line);
		g_free(command);

		/* Output */
		gchar *text;
		gchar *output_text = gebr_job_get_output(job);
		text = g_strdup_printf(_("Output:\n%s\n\n"), strlen(output_text)? output_text : "(None)");
		fputs(text, fp);
		g_free(text);
		g_free(output_text);

		gebr_message(GEBR_LOG_INFO, TRUE, TRUE, _("Saved Job information at \"%s\"."), fname);
	}
	
	fclose(fp);
	g_free(fname);
	return TRUE;
}

void
gebr_job_control_stop_selected(GebrJobControl *jc)
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	GebrJob *job;
	
	gboolean asked = FALSE;
	gint selected_rows = 0;
	selected_rows =	gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->priv->view)));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(jc->priv->view));

	gebr_gui_gtk_tree_view_foreach_selected(&iter, jc->priv->view) {
		gtk_tree_model_get(model, &iter, JC_STRUCT, &job, -1);
		
		if (selected_rows == 1) {
			if (gebr_gui_confirm_action_dialog(_("Cancel Job"),
							   _("Are you sure you want to cancel Job \"%s\"?"),
							   gebr_job_get_title(job)) == FALSE)
				return;
		} else if (!asked) {
			if (gebr_gui_confirm_action_dialog(_("Cancel Job"),
							   _("Are you sure you want to cancel the selected Jobs?")) == FALSE)
				return;
			asked = TRUE;
		}

		gchar *servers = g_strjoinv(", ", gebr_job_get_servers(job, NULL));

		gebr_message(GEBR_LOG_INFO, TRUE, FALSE, _("Asking server to cancel Job."));
		gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Asking server(s) \"%s\" to cancel Job \"%s\"."),
			     servers, gebr_job_get_title(job));
		g_free(servers);

		gebr_job_kill(job);
	}
}

void
gebr_job_control_close_selected(GebrJobControl *jc)
{
	GebrJob *job;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GList *rows;
	GList *rowrefs = NULL;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->priv->view));
	rows = gtk_tree_selection_get_selected_rows(selection, &model);

	if (!rows->next) {
		gtk_tree_model_get_iter(model, &iter, rows->data);
		gtk_tree_model_get(model, &iter, JC_STRUCT, &job, -1);
		if (!gebr_gui_confirm_action_dialog(_("Clear Job"),
		                                    _("Are you sure you want to clear Job \"%s\"?"), gebr_job_get_title(job)))
			goto free_rows;
	} else {
		if (!gebr_gui_confirm_action_dialog(_("Clear Job"),
		                                    _("Are you sure you want to clear the selected Jobs?")))
			goto free_rows;
	}

	for (GList *i = rows; i; i = i->next) {
		GtkTreeRowReference *rowref = gtk_tree_row_reference_new(model, i->data);
		rowrefs = g_list_prepend(rowrefs, rowref);
	}

	jc->priv->last_selection.job = NULL;

	BLOCK_SELECTION_CHANGED_SIGNAL(jc);
	for (GList *i = rowrefs; i; i = i->next) {
		GtkTreePath *path = gtk_tree_row_reference_get_path(i->data);

		if (!gtk_tree_model_get_iter(model, &iter, path)) {
			gtk_tree_path_free(path);
			g_warn_if_reached();
			continue;
		}
		gtk_tree_model_get(model, &iter, JC_STRUCT, &job, -1);
		if (gebr_job_close(job))
			gebr_job_control_remove(jc, job);
		gtk_tree_path_free(path);
	}
	UNBLOCK_SELECTION_CHANGED_SIGNAL(jc);
	job_control_on_cursor_changed(gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->priv->view)), jc);

	g_list_foreach(rowrefs, (GFunc)gtk_tree_row_reference_free, NULL);
	g_list_free(rowrefs);

free_rows:
	g_list_foreach(rows, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(rows);
}

void
gebr_job_control_select_job_by_rid(GebrJobControl *jc, const gchar *rid)
{
	gebr_job_control_select_job(jc, gebr_job_control_find(jc, rid));
}

void
gebr_job_control_select_job(GebrJobControl *jc, GebrJob *job)
{
	if (job) {
		GtkTreeModel *sort = gtk_tree_view_get_model(GTK_TREE_VIEW(jc->priv->view));
		GtkTreeModel *filter = gtk_tree_model_sort_get_model(GTK_TREE_MODEL_SORT(sort));
		GtkTreeIter *iter = gebr_job_get_iter(job), filter_iter, sort_iter;

		if (!gtk_tree_model_filter_convert_child_iter_to_iter(GTK_TREE_MODEL_FILTER(filter), &filter_iter, iter))
			return;

		if (!gtk_tree_model_sort_convert_child_iter_to_iter(GTK_TREE_MODEL_SORT(sort), &sort_iter, &filter_iter))
			return;

		GtkTreePath *path = gtk_tree_model_get_path(sort, &sort_iter);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(jc->priv->view), path, NULL, FALSE);
		gtk_tree_path_free(path);
	} else {
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->priv->view));
		gtk_tree_selection_unselect_all(selection);
	}
}

void
gebr_job_control_show(GebrJobControl *jc)
{
	jc->priv->timeout_source_id = g_timeout_add(5000, update_tree_view, jc);
	build_servers_filter_list(jc);
}

void
gebr_job_control_hide(GebrJobControl *jc)
{
	if (jc->priv->timeout_source_id)
		g_source_remove(jc->priv->timeout_source_id);
}

void
gebr_job_control_setup_filter_button(GebrJobControl *jc,
				     GebrGuiToolButton *tool_button)
{
	GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object(jc->priv->builder, "tl-filter"));
	g_signal_connect(tool_button, "toggled", G_CALLBACK(on_filter_show_unselect_jobs), jc);
	gebr_gui_tool_button_add(tool_button, widget);
}

void
gebr_job_control_remove(GebrJobControl *jc,
			GebrJob *job)
{
	gtk_list_store_remove(jc->priv->store, gebr_job_get_iter(job));
	g_object_unref(job);
}

GtkTreeModel *
gebr_job_control_get_model(GebrJobControl *jc)
{
	return GTK_TREE_MODEL(jc->priv->store);
}
