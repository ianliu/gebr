/*   GÍBR - An environment for seismic processing.
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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

#include <unistd.h>

#include "cb_job.h"
#include "gebr.h"

/*
 * Internal stuff
 */

void
__job_close(struct job * job)
{
	if (job->status == JOB_STATUS_RUNNING) {
		log_message(ACTION, "Can't close running job...", TRUE);
		return;
	}
	if (g_ascii_strcasecmp(job->jid->str, "0"))
		protocol_send_data(job->server->protocol, job->server->tcp_socket,
			protocol_defs.clr_def, 1, job->jid->str);

	job_delete(job);
}

void
__job_clear_or_select_first(void)
{
	GtkTreeIter		iter;
	GtkTreeSelection *	selection;

	/* select the first job */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(W.job_view));
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(W.job_store), &iter) == TRUE) {
		gtk_tree_selection_select_iter(selection, &iter);
		job_clicked();
	} else {
		gtk_label_set_text(GTK_LABEL(W.job_ctrl.job_label), "");
		gtk_text_buffer_set_text(W.job_ctrl.text_buffer, "", 0);
	}
}

/*
 * Public functions
 */

struct job *
job_add(struct server * server, GString * jid,
	GString * _status, GString * title,
	GString * start_date, GString * finish_date,
	GString * hostname, GString * issues,
	GString * cmd_line, GString * output,
	gboolean go_to)
{
	GtkTreeSelection *	selection;
	GtkTreeIter		iter;
	struct job *		job;
	enum JobStatus		status;
	gchar			local_hostname[100];

	gethostname(local_hostname, 100);
	status = job_translate_status(_status);
	job = g_malloc(sizeof(struct job));
	*job = (struct job) {
		.status = status,
		.server = server,
		.title = g_string_new(title->str),
		.jid = g_string_new(jid->str),
		.start_date = g_string_new(start_date->str),
		.finish_date = g_string_new(finish_date == NULL ? "" : finish_date->str),
		.hostname = g_string_new(hostname == NULL ? local_hostname : hostname->str),
		.issues = g_string_new(issues->str),
		.cmd_line = g_string_new(cmd_line->str),
		.output = g_string_new(output->str)
	};

	/* append to the store and select it */
	gtk_list_store_append(W.job_store, &iter);
	gtk_list_store_set(W.job_store, &iter,
			JC_TITLE, job->title->str,
			JC_STRUCT, job,
			-1);
	job->iter = iter;
	job_update_status(job);

	if (go_to) {
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(W.job_view));
		gtk_tree_selection_select_iter(selection, &iter);
		job_clicked();

		/* go to jobs pages */
		gtk_notebook_set_current_page(GTK_NOTEBOOK(W.notebook), 3);
	}

	return job;
}

void
job_delete(struct job * job)
{
	gtk_list_store_remove(W.job_store, &job->iter);
	g_string_free(job->title, TRUE);
	g_string_free(job->jid, TRUE);
	g_string_free(job->hostname, TRUE);
	g_string_free(job->start_date, TRUE);
	g_string_free(job->finish_date, TRUE);
	g_string_free(job->cmd_line, TRUE);
	g_string_free(job->issues, TRUE);
	g_string_free(job->output, TRUE);
	g_free(job);

	__job_clear_or_select_first();
}

struct job *
job_find(GString * address, GString * jid)
{
	GtkTreeIter	iter;
	struct job *	job;
	gboolean	valid;

	job = NULL;
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(W.job_store), &iter);
	while (valid) {
		struct job *	i;

		gtk_tree_model_get(GTK_TREE_MODEL(W.job_store), &iter,
				JC_STRUCT, &i,
				-1);

		if (!g_ascii_strcasecmp(i->server->address->str, address->str) &&
			!g_ascii_strcasecmp(i->jid->str, jid->str)) {
			job = i;
			break;
		}

		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(W.job_store), &iter);
	}

	return job;
}

void
job_cancel(void)
{
	GtkTreeIter		iter;
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	struct job *	        job;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(W.job_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE)
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(W.job_store), &iter,
			JC_STRUCT, &job,
			-1);

	if (job->status != JOB_STATUS_RUNNING) {
		log_message(SERVER, "Job is not running", TRUE);
		return;
	}

	log_message(SERVER, "Telling server to terminate job...", TRUE);

	protocol_send_data(job->server->protocol, job->server->tcp_socket,
		protocol_defs.end_def, 1, job->jid->str);
}

void
job_close(void)
{
	GtkTreeIter		iter;
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	struct job *		job;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(W.job_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE)
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(W.job_store), &iter,
			JC_STRUCT, &job,
			-1);

	__job_close(job);
	__job_clear_or_select_first();
}

void
job_clear(void)
{
	GtkTreeIter		iter;
	gboolean		valid;

	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(W.job_store), &iter);
	while (valid) {
		struct job *	job;
		GtkTreeIter	this;

		gtk_tree_model_get(GTK_TREE_MODEL(W.job_store), &iter,
				JC_STRUCT, &job,
				-1);
		/* go to next before the possible deletion */
		this = iter;
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(W.job_store), &iter);

		__job_close(job);
	}
	__job_clear_or_select_first();
}

void
job_stop(void)
{
	GtkTreeIter		iter;
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	struct job *	        job;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(W.job_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE)
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(W.job_store), &iter,
			JC_STRUCT, &job,
			-1);

	if (job->status != JOB_STATUS_RUNNING) {
		log_message(SERVER, "Job is not running", TRUE);
		return;
	}

	log_message(SERVER, "Telling server to kill job...", TRUE);
	protocol_send_data(job->server->protocol, job->server->tcp_socket,
		protocol_defs.kil_def, 0);
}

void
job_clicked(void)
{
	GtkTreeIter		iter;
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(W.job_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE)
		return;

	struct job *		job;
	GString *		info;

	gtk_tree_model_get(GTK_TREE_MODEL(W.job_store), &iter,
			JC_STRUCT, &job,
			-1);
	info = g_string_new(NULL);

	/* fill job label */
	job_update_label(job);

	/*
	 * Fill job information
	 */
	/* who and where */
	g_string_append_printf(info, "Job executed at %s by %s\n",
		job->server->protocol->hostname->str, job->hostname->str);
	/* start date */
	g_string_append_printf(info, "Start date: %s\n", job->start_date->str);
	/* issues */
	if (job->issues->len)
		g_string_append_printf(info, "Issues:\n%s\n", job->issues->str);
	/* command line */
	if (job->cmd_line->len)
		g_string_append_printf(info, "Command line:\n%s\n\n", job->cmd_line->str);
	/* output */
	if (job->output->len)
		g_string_append_printf(info, "Output:\n%s\n", job->output->str);
	/* finish date*/
	if (job->finish_date->len)
		g_string_append_printf(info, "Finish date: %s", job->finish_date->str);
	/* to view */
	gtk_text_buffer_set_text(W.job_ctrl.text_buffer, info->str, info->len);

	g_string_free(info, TRUE);
}

gboolean
job_is_active(struct job * job)
{
	GtkTreeSelection *	selection;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(W.job_view));

	return gtk_tree_selection_iter_is_selected(selection, &job->iter);
}

void
job_append_output(struct job * job, GString * output)
{
	if (job_is_active(job) == FALSE)
		return;

	GtkTextIter	iter;

	if (!job->output->len)
		g_string_printf(job->output, "Output:\n%s", output->str);
	else
		g_string_append(job->output, output->str);

	gtk_text_buffer_get_end_iter(W.job_ctrl.text_buffer, &iter);
	gtk_text_buffer_insert(W.job_ctrl.text_buffer, &iter, output->str, output->len);
}

void
job_update(struct job * job)
{
	if (job_is_active(job) == FALSE)
		return;

	job_clicked();
}

void
job_update_label(struct job * job)
{
	GString *	label;

	/* initialization */
	label = g_string_new(NULL);

	g_string_printf(label, "job at %s: %s", job->hostname->str, job->start_date->str);
	if (job->status != JOB_STATUS_RUNNING) {
		g_string_append(label, " - ");
		g_string_append(label, job->finish_date->str);
	}
	gtk_label_set_text(GTK_LABEL(W.job_ctrl.job_label), label->str);

	/* free */
	g_string_free(label, TRUE);
}

enum JobStatus
job_translate_status(GString * status)
{
	enum JobStatus	translated_status;

	if (!g_ascii_strcasecmp(status->str, "running"))
		translated_status = JOB_STATUS_RUNNING;
	else if (!g_ascii_strcasecmp(status->str, "finished"))
		translated_status = JOB_STATUS_FINISHED;
	else if (!g_ascii_strcasecmp(status->str, "canceled"))
		translated_status = JOB_STATUS_CANCELED;
	else
		translated_status = JOB_STATUS_CANCELED;

	return translated_status;
}

void
job_update_status(struct job * job)
{
	GdkPixbuf *	pixbuf;

	switch (job->status) {
	case JOB_STATUS_RUNNING:
		pixbuf = W.pixmaps.running_icon;
		break;
	case JOB_STATUS_FINISHED: {
		GtkTextIter	iter;
		GString *	finish_date;

		pixbuf = W.pixmaps.configured_icon;
		if (job_is_active(job) == FALSE)
			break;

		/* initialization */
		finish_date = g_string_new(NULL);

		/* job label */
		job_update_label(job);
		/* job info */
		g_string_printf(finish_date, "\nFinish date: %s", job->finish_date->str);
		gtk_text_buffer_get_end_iter(W.job_ctrl.text_buffer, &iter);
		gtk_text_buffer_insert(W.job_ctrl.text_buffer, &iter, finish_date->str, finish_date->len);

		/* frees */
		g_string_free(finish_date, TRUE);
		break;
	}
	case JOB_STATUS_FAILED:
	case JOB_STATUS_CANCELED:
		pixbuf = W.pixmaps.disabled_icon;
		break;
	default:
		pixbuf = NULL;
		break;
	}

	gtk_list_store_set(W.job_store, &job->iter,
			JC_ICON, pixbuf,
			-1);
}
