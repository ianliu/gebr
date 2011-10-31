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

#define DELIM ", "

typedef struct {
	GebrJob *job;
	guint sig_output;
	guint sig_status;
	guint sig_issued;
	guint sig_cmd_line;
	guint sig_button;
} LastSelection;

struct _GebrJobControlPriv {
	guint timeout_source_id;
	GtkWidget *widget;
	GtkBuilder *builder;
	LastSelection last_selection;
};

enum {
	JC_STRUCT,
	JC_N_COLUMN
};

#define BLOCK_SELECTION_CHANGED_SIGNAL(jc) \
	g_signal_handlers_block_by_func(gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->view)), \
					job_control_on_cursor_changed, jc);

#define UNBLOCK_SELECTION_CHANGED_SIGNAL(jc) \
	g_signal_handlers_unblock_by_func(gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->view)), \
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

static void on_text_view_populate_popup(GtkTextView * text_view, GtkMenu * menu);


static GtkMenu *job_control_popup_menu(GtkWidget * widget, GebrJobControl *job_control);

static void job_control_fill_servers_info(GebrJobControl *jc);

/* Private methods {{{1 */
static GebrJob *
get_selected_job(GebrJobControl *jc)
{
	GebrJob *job = NULL;
	GtkTreeModel *model;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->view));
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
jobs_visible_func(GtkTreeModel *model,
		  GtkTreeIter *iter,
		  GebrJobControl *jc)
{
	return TRUE;
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

static gboolean
on_popup_focus_out(GtkWidget *widget,
		   GdkEventFocus *focus,
		   GtkToggleButton *button)
{
	gtk_widget_hide(widget);
	return TRUE;
}

static void
on_info_button_clicked(GtkButton *button,
		       GebrJobControl *jc)
{
	static GtkWidget *popup = NULL;

	GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(button));
	GdkWindow *window = gtk_widget_get_window(toplevel);
	GtkAllocation a;
	gint x, y;

	gdk_window_get_origin(window, &x, &y);
	gtk_widget_get_allocation(GTK_WIDGET(button), &a);

	if (!popup) {
		popup = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		g_signal_connect(popup, "focus-out-event",
				 G_CALLBACK(on_popup_focus_out), button);
		gtk_window_set_decorated(GTK_WINDOW(popup), FALSE);
		GtkWidget *widget = GTK_WIDGET(gtk_builder_get_object(jc->priv->builder, "servers_widget"));
		gtk_container_add(GTK_CONTAINER(popup), widget);
	} else
		gtk_widget_show(popup);

	gtk_window_move(GTK_WINDOW(popup), x+a.x, y+a.y+a.height);
	gtk_widget_show_all(popup);
}

static void
on_job_output(GebrJob *job,
	      GebrTask *task,
	      const gchar *output,
	      GebrJobControl *jc)
{
	GtkTextIter end_iter;
	GtkTextMark *mark;

	gtk_text_buffer_get_end_iter(jc->text_buffer, &end_iter);
	gtk_text_buffer_insert(jc->text_buffer, &end_iter, output, strlen(output));
	if (gebr.config.job_log_auto_scroll) {
		mark = gtk_text_buffer_get_mark(jc->text_buffer, "end");
		gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(jc->text_view), mark);
	}
}

static void
on_job_status(GebrJob *job,
	      enum JobStatus old_status,
	      enum JobStatus new_status,
	      const gchar *parameter,
	      GebrJobControl *jc)
{
	gebr_jc_update_status_and_time(jc, job, new_status);
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

	gtk_text_buffer_get_end_iter(jc->cmd_buffer, &end_iter);
	gtk_text_buffer_insert(jc->cmd_buffer, &end_iter, cmdline, strlen(cmdline));
	if (gebr.config.job_log_auto_scroll) {
		mark = gtk_text_buffer_get_mark(jc->cmd_buffer, "end");
		gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(jc->cmd_view), mark);
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
	gint n_servers = 0, i;
	gchar **servers;

	servers = g_strsplit(gebr_job_get_servers(job), ",", -1);

	while (servers[n_servers])
		n_servers++;

	gebr_job_get_resources(job, &nprocs, &niceness);

	if (!nprocs || !niceness)
		g_string_printf(resources, _("Waiting for server(s) details"));
	else {
		g_string_append_printf(resources, _("Executed with %s processor(s) on %d server(s)\n"), nprocs, n_servers);
		if (g_strcmp0(niceness, "0"))
			g_string_append_printf(resources, _("Using all machines resources\n"));
		else
			g_string_append_printf(resources, _("Using the machines idle time\n"));
		g_free(nprocs);
		g_free(niceness);
	}
	gtk_label_set_text(res_label, resources->str);

	g_string_free(resources, TRUE);

	GString *servers_list = g_string_new(NULL);
	GString *servers_info = g_string_new(NULL);
	GtkLabel *servers_label = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "servers_text"));

	gint cmpfun(gconstpointer a, gconstpointer b) {
		const gchar *aa = *(gchar * const *)a;
		const gchar *bb = *(gchar * const *)b;

		if (g_strcmp0(aa, "127.0.0.1") == 0)
			return -1;

		if (g_strcmp0(bb, "127.0.0.1") == 0)
			return 1;

		return g_strcmp0(aa, bb);
	}

	qsort(servers, n_servers, sizeof(servers[0]), cmpfun);

	for (i = 0; servers[i]; i++) {
		const gchar *server;
		if (!g_strcmp0(servers[i], "127.0.0.1")) {
			 server = server_get_name_from_address(servers[i]);
		} else
			server = servers[i];
		g_string_append_printf(servers_list, "%s\n", server);
	}

	g_string_append(servers_info, servers_list->str);

	gtk_label_set_text(servers_label, servers_info->str);

	g_free(servers);
	g_string_free(servers_info, TRUE);
	g_string_free(servers_list, TRUE);
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
		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(jc->store), &iter, (GtkTreePath*)i->data)) {
			gtk_tree_model_get(GTK_TREE_MODEL(jc->store), &iter, JC_STRUCT, &job, -1);
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
	GList *rows = gtk_tree_selection_get_selected_rows(selection, NULL);
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
		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(jc->store), &iter, (GtkTreePath*)rows->data))
			gtk_tree_model_get(GTK_TREE_MODEL(jc->store), &iter, JC_STRUCT, &job, -1);
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

	if (gebr_job_get_status(job) == JOB_STATUS_QUEUED) {
		g_object_set(cell, "text", "Waiting", NULL);
		return;
	}

	start_time_str = gebr_job_get_start_date(job);
	if (!start_time_str)
		return;
	g_time_val_from_iso8601(start_time_str, &start_time);
	g_get_current_time(&curr_time);


	relative_time_msg = gebr_calculate_relative_time(&start_time, &curr_time);
	g_object_set(cell, "text", relative_time_msg, NULL);
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
		gtk_image_set_from_stock(img, GTK_STOCK_APPLY, GTK_ICON_SIZE_DIALOG);
		gtk_label_set_text(subheader, finish->str);
		gtk_label_set_text(details_start_date, start->str);
		gtk_widget_show(GTK_WIDGET(details_start_date));
	}

	else if (status == JOB_STATUS_RUNNING) {
		gtk_image_set_from_stock(img, GTK_STOCK_EXECUTE, GTK_ICON_SIZE_DIALOG);
		gtk_label_set_text(subheader, start->str);
		gtk_widget_hide(GTK_WIDGET(details_start_date));
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

	g_string_free(start, FALSE);
	g_string_free(finish, FALSE);
}

static void
gebr_job_control_load_details(GebrJobControl *jc,
			      GebrJob *job)
{
	g_return_if_fail(job != NULL);

	GString *info = g_string_new("");
	GString *info_cmd = g_string_new("");
	GtkTextIter end_iter;
	GtkTextIter end_iter_cmd;
	GtkLabel *input_file, *output_file, *log_file, *job_group;
	gchar *input_file_str, *output_file_str, *log_file_str;
	enum JobStatus status = gebr_job_get_status(job);

	input_file = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "input_label"));
	output_file = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "output_label"));
	log_file = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "error_label"));
	job_group = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "job_group"));


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

	gchar *msg = g_strdup(gebr_job_get_group(job));
	if (msg == NULL) 
		msg = g_strdup("All servers");
	gtk_label_set_markup (job_group, msg);

	gebr_jc_update_status_and_time(jc, job, status);

	GtkLabel *label = GTK_LABEL(gtk_builder_get_object(jc->priv->builder, "header_label"));
	const gchar *title = gebr_job_get_title(job);
	markup = g_markup_printf_escaped ("<span size=\"large\"><b>%s</b></span>", title);
	gtk_label_set_markup (label, markup);
	g_free (markup);

	gtk_text_buffer_set_text(jc->text_buffer, "", 0);
	gtk_text_buffer_set_text(jc->cmd_buffer, "", 0);

	/* command-line */
	gchar *cmdline = gebr_job_get_command_line(job);
	g_string_append_printf(info_cmd, "\n%s\n%s\n", _("Command line:"), cmdline);
	g_free(cmdline);

	gtk_text_buffer_get_end_iter(jc->cmd_buffer, &end_iter_cmd);
	gtk_text_buffer_insert(jc->cmd_buffer, &end_iter_cmd, info_cmd->str, info_cmd->len);

	/* output */
	g_string_append(info, gebr_job_get_output(job));

	gtk_text_buffer_get_end_iter(jc->text_buffer, &end_iter);
	gtk_text_buffer_insert(jc->text_buffer, &end_iter, info->str, info->len);

	g_string_free(info, TRUE);
	g_string_free(info_cmd, TRUE);
}

static gboolean
update_tree_view(gpointer data)
{
	GtkTreeIter iter;
	GebrJobControl *jc = data;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(jc->view));
	gboolean valid = gtk_tree_model_get_iter_first(model, &iter);

	while (valid)
	{
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_model_row_changed(GTK_TREE_MODEL(jc->store), path, &iter);
		gtk_tree_path_free(path);
		valid = gtk_tree_model_iter_next(model, &iter);
	}

	return TRUE;
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

	jc->store = gtk_list_store_new(JC_N_COLUMN, G_TYPE_POINTER);

	GtkTreeModel *filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(jc->store), NULL);

	GtkTreeView *treeview;
	treeview = GTK_TREE_VIEW(gtk_builder_get_object(jc->priv->builder, "treeview_jobs"));
	gtk_tree_view_set_model(treeview, filter);

	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filter),
					       (GtkTreeModelFilterVisibleFunc)jobs_visible_func,
					       jc, NULL);
	g_object_unref(filter);
	
	jc->view = GTK_WIDGET(treeview);

	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->view)),
				    GTK_SELECTION_MULTIPLE);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(jc->view), FALSE);
	g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->view)), "changed",
			 G_CALLBACK(job_control_on_cursor_changed), jc);

	col = GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(jc->priv->builder, "tv_column"));

	renderer = GTK_CELL_RENDERER(gtk_builder_get_object(jc->priv->builder, "tv_icon_cell"));
	gtk_tree_view_column_set_cell_data_func(col, renderer, icon_column_data_func, NULL, NULL);

	renderer = GTK_CELL_RENDERER(gtk_builder_get_object(jc->priv->builder, "tv_title_cell"));
	gtk_tree_view_column_set_cell_data_func(col, renderer, title_column_data_func, NULL, NULL);

	renderer = GTK_CELL_RENDERER(gtk_builder_get_object(jc->priv->builder, "tv_time_cell"));
	gtk_tree_view_column_set_cell_data_func(col, renderer, time_column_data_func, NULL, NULL);

	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(jc->view),
	                                          (GebrGuiGtkPopupCallback) job_control_popup_menu, jc);

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

	GtkButton *info_button;
	info_button = GTK_BUTTON(gtk_builder_get_object(jc->priv->builder, "info_button"));
	g_signal_connect(info_button, "clicked", G_CALLBACK(on_info_button_clicked), jc);

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
	jc->text_buffer = gtk_text_buffer_new(NULL);
	gtk_text_buffer_get_end_iter(jc->text_buffer, &iter_end);
	gtk_text_buffer_create_mark(jc->text_buffer, "end", &iter_end, FALSE);

	text_view = GTK_WIDGET(gtk_builder_get_object(jc->priv->builder, "textview_output"));
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(text_view), jc->text_buffer);

	g_object_set(G_OBJECT(text_view), "editable", FALSE, "cursor-visible", FALSE, NULL);
	{
		PangoFontDescription *font;

		font = pango_font_description_new();
		pango_font_description_set_family(font, "monospace");
		gtk_widget_modify_font(text_view, font);

		pango_font_description_free(font);
	}

	jc->text_view = text_view;
	g_signal_connect(jc->text_view, "populate-popup", G_CALLBACK(on_text_view_populate_popup), jc);

	/* Text view of command line */
	jc->cmd_buffer = gtk_text_buffer_new(NULL);
	gtk_text_buffer_get_end_iter(jc->cmd_buffer, &iter_end);
	gtk_text_buffer_create_mark(jc->cmd_buffer, "end", &iter_end, FALSE);

	text_view = GTK_WIDGET(gtk_builder_get_object(jc->priv->builder, "textview_command_line"));
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(text_view), jc->cmd_buffer);

	g_object_set(G_OBJECT(text_view), "editable", FALSE, "cursor-visible", FALSE, NULL);
	{
		PangoFontDescription *font;

		font = pango_font_description_new();
		pango_font_description_set_family(font, "monospace");
		gtk_widget_modify_font(text_view, font);

		pango_font_description_free(font);
	}

	jc->cmd_view = text_view;
	g_signal_connect(jc->cmd_view, "populate-popup", G_CALLBACK(on_text_view_populate_popup), jc);

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
	gtk_list_store_append(jc->store, gebr_job_get_iter(job));
	gtk_list_store_set(jc->store, gebr_job_get_iter(job), JC_STRUCT, job, -1);
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

	gtk_tree_model_foreach(GTK_TREE_MODEL(jc->store), job_find_foreach_func, NULL);

	return job;
}

#if 0
gboolean job_control_save(void)
{
	GtkTreeIter iter;
	GtkWidget *chooser_dialog;
	GtkFileFilter *filefilter;
	GtkTreeModel *model;

	gchar *fname;
	FILE *fp;

	GtkTextIter start_iter;
	GtkTextIter end_iter;
	gchar *text;
	gchar * title;

	GebrJob *job;

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

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(gebr.job_control->view));

	gebr_gui_gtk_tree_view_foreach_selected(&iter, gebr.job_control->view) {
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		if (gtk_tree_path_get_depth(path) == 1) {
			gtk_tree_path_free(path);
			continue;
		}
		gtk_tree_path_free(path);

		gtk_tree_model_get(model, &iter, JC_STRUCT, &job, -1);
		job_load_details(job);
		
		title = g_strdup_printf("---------- %s ---------\n", gebr_job_get_title(job));
		fputs(title, fp);
		g_free(title);

		gtk_text_buffer_get_start_iter(gebr.job_control->text_buffer, &start_iter);
		gtk_text_buffer_get_end_iter(gebr.job_control->text_buffer, &end_iter);
		text = gtk_text_buffer_get_text(gebr.job_control->text_buffer, &start_iter, &end_iter, FALSE);
		text = g_strdup_printf("%s\n\n", text);
		fputs(text, fp);

		gebr_message(GEBR_LOG_INFO, TRUE, TRUE, _("Saved Job information at \"%s\"."), fname);
		
		g_free(text);
	}
	
	fclose(fp);
	g_free(fname);
	return TRUE;
}
#endif

void
gebr_job_control_stop_selected(GebrJobControl *jc)
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	GebrJob *job;
	
	gboolean asked = FALSE;
	gint selected_rows = 0;
	selected_rows =	gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.job_control->view)));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(gebr.job_control->view));

	gebr_gui_gtk_tree_view_foreach_selected(&iter, gebr.job_control->view) {
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
		gebr_message(GEBR_LOG_INFO, TRUE, FALSE, _("Asking server to cancel Job."));
		gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Asking server(s) \"%s\" to cancel Job \"%s\"."),
			     gebr_job_get_servers(job), gebr_job_get_title(job));

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

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->view));
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

	for (GList *i = rowrefs; i; i = i->next) {
		GtkTreePath *path = gtk_tree_row_reference_get_path(i->data);

		if (!gtk_tree_model_get_iter(model, &iter, path)) {
			gtk_tree_path_free(path);
			g_warn_if_reached();
			continue;
		}
		gtk_tree_model_get(model, &iter, JC_STRUCT, &job, -1);
		if (gebr_job_close(job))
			gtk_list_store_remove(jc->store, gebr_job_get_iter(job));
		gtk_tree_path_free(path);
	}

	g_list_foreach(rowrefs, (GFunc)gtk_tree_row_reference_free, NULL);
	g_list_free(rowrefs);

free_rows:
	g_list_foreach(rows, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(rows);
}


/**
 * \internal
 */
static void wordwrap_toggled(GtkCheckMenuItem * check_menu_item, GtkTextView * text_view)
{
	gebr.config.job_log_word_wrap = gtk_check_menu_item_get_active(check_menu_item);
	g_object_set(G_OBJECT(text_view), "wrap-mode",
		     gebr.config.job_log_word_wrap ? GTK_WRAP_WORD : GTK_WRAP_NONE, NULL);
}

/**
 * \internal
 */
static void autoscroll_toggled(GtkCheckMenuItem * check_menu_item)
{
	gebr.config.job_log_auto_scroll = gtk_check_menu_item_get_active(check_menu_item);
}

/**
 * \internal
 */
static void on_text_view_populate_popup(GtkTextView * text_view, GtkMenu * menu)
{
	GtkWidget *menu_item;

	menu_item = gtk_separator_menu_item_new();
	gtk_widget_show(menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = gtk_check_menu_item_new_with_label(_("Word-wrap"));
	gtk_widget_show(menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(menu_item, "toggled", G_CALLBACK(wordwrap_toggled), text_view);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), gebr.config.job_log_word_wrap);

	menu_item = gtk_check_menu_item_new_with_label(_("Auto-scroll"));
	gtk_widget_show(menu_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(menu_item, "toggled", G_CALLBACK(autoscroll_toggled), NULL);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), gebr.config.job_log_auto_scroll);
}

#if 0
/*
 * Queue Actions
 */
void job_control_queue_save(void)
{
	job_control_queue_by_func(job_control_save);
}

#endif

/**
 * \internal
 * Build popup menu
 */
static GtkMenu *job_control_popup_menu(GtkWidget * widget, GebrJobControl *job_control)
{
	GtkWidget *menu;
	GtkTreeModel *model;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(job_control->view));
	GList *rows = gtk_tree_selection_get_selected_rows(selection, &model);

	menu = gtk_menu_new();

	if(rows) {

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

void
gebr_job_control_select_job_by_rid(GebrJobControl *jc, const gchar *rid)
{
	gebr_job_control_select_job(jc, gebr_job_control_find(jc, rid));
}

void
gebr_job_control_select_job(GebrJobControl *jc, GebrJob *job)
{
	if (job) {
		GtkTreeModel *filter = gtk_tree_view_get_model(GTK_TREE_VIEW(jc->view));
		GtkTreeIter *iter = gebr_job_get_iter(job), filter_iter;

		if (!gtk_tree_model_filter_convert_child_iter_to_iter(GTK_TREE_MODEL_FILTER(filter), &filter_iter, iter))
			return;

		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(jc->view));
		gtk_tree_selection_unselect_all(selection);
		gtk_tree_selection_select_iter(selection, &filter_iter);
	}
}

void
gebr_job_control_show(GebrJobControl *jc)
{
	jc->priv->timeout_source_id = g_timeout_add(5000, update_tree_view, jc);
}

void
gebr_job_control_hide(GebrJobControl *jc)
{
	if (jc->priv->timeout_source_id)
		g_source_remove(jc->priv->timeout_source_id);
}
