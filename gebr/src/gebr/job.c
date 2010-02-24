/*   GeBR - An environment for seismic processing.
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

/*
 * File: job.c
 * Job callbacks
 */

#include <string.h>
#include <unistd.h>

#include <libgebr/intl.h>
#include <libgebr/gui/utils.h>
#include <libgebr/utils.h>
#include <libgebr/date.h>

#include "job.h"
#include "gebr.h"
#include "ui_job_control.h"

struct job *job_add(struct server *server, GString * jid,
		    GString * _status, GString * title,
		    GString * start_date, GString * finish_date,
		    GString * hostname, GString * issues, GString * cmd_line, GString * output, GString * queue, GString * moab_jid)
{
	GtkTreeIter iter;

	struct job *job;
	enum JobStatus status;
	gchar local_hostname[100];

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
		.output = g_string_new(NULL),
		.queue = g_string_new(queue->str), 
		.moab_jid = g_string_new(moab_jid->str)
	};

	/* append to the store and select it */
	gtk_list_store_append(gebr.ui_job_control->store, &iter);
	gtk_list_store_set(gebr.ui_job_control->store, &iter, JC_TITLE, job->title->str, JC_STRUCT, job, -1);
	job->iter = iter;
	job_update_status(job);
	job_set_active(job);

	return job;
}

void job_free(struct job *job)
{
	g_string_free(job->title, TRUE);
	g_string_free(job->jid, TRUE);
	g_string_free(job->hostname, TRUE);
	g_string_free(job->start_date, TRUE);
	g_string_free(job->finish_date, TRUE);
	g_string_free(job->cmd_line, TRUE);
	g_string_free(job->issues, TRUE);
	g_string_free(job->output, TRUE);
	g_string_free(job->queue, TRUE);
	g_string_free(job->moab_jid, TRUE);
	g_free(job);
}

void job_delete(struct job *job)
{
	gtk_list_store_remove(gebr.ui_job_control->store, &job->iter);
	job_free(job);

	job_control_clear_or_select_first();
}

void job_close(struct job *job)
{
	if (job->status == JOB_STATUS_RUNNING) {
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Can't close running job"));
		return;
	}
	if (gebr_comm_server_is_logged(job->server->comm) == FALSE) {
		/* TODO */
	} else if (strcmp(job->jid->str, "0"))
		gebr_comm_protocol_send_data(job->server->comm->protocol, job->server->comm->stream_socket,
					     gebr_comm_protocol_defs.clr_def, 1, job->jid->str);

	job_delete(job);
}

struct job *job_find(GString * address, GString * jid)
{
	GtkTreeIter iter;
	struct job *job;

	job = NULL;
	gebr_gui_gtk_tree_model_foreach(iter, GTK_TREE_MODEL(gebr.ui_job_control->store)) {
		struct job *i;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter, JC_STRUCT, &i, -1);

		if (!strcmp(i->server->comm->address->str, address->str) && !strcmp(i->jid->str, jid->str)) {
			job = i;
			break;
		}
	}

	return job;
}

void job_set_active(struct job *job)
{
 	gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(gebr.ui_job_control->view), &job->iter);
}

/*
 * Function: job_is_active
 * *Fill me in!*
 */
gboolean job_is_active(struct job *job)
{
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_job_control->view));

	return gtk_tree_selection_iter_is_selected(selection, &job->iter);
}

/*
 * Function: job_append_output
 * *Fill me in!*
 */
void job_append_output(struct job *job, GString * output)
{
	GtkTextIter iter;
	GString *text;
	GtkTextMark *mark;

	if (!output->len)
		return;
	if (!job->output->len) {
		g_string_printf(job->output, "\n%s\n%s", _("Output:"), output->str);
		text = job->output;
	} else {
		g_string_append(job->output, output->str);
		text = output;
	}
	if (job_is_active(job) == TRUE) {
		gtk_text_buffer_get_end_iter(gebr.ui_job_control->text_buffer, &iter);
		gtk_text_buffer_insert(gebr.ui_job_control->text_buffer, &iter, text->str, text->len);
		if (gebr.config.job_log_auto_scroll) {
			mark = gtk_text_buffer_get_mark(gebr.ui_job_control->text_buffer, "end");
			gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(gebr.ui_job_control->text_view), mark);
		}
	}
}

/*
 * Function: job_update
 * *Fill me in!*
 */
void job_update(struct job *job)
{
	if (job_is_active(job) == FALSE)
		return;
	job_set_active(job);
}

/*
 * Function: job_update_label
 * *Fill me in!*
 */
void job_update_label(struct job *job)
{
	GString *label;

	/* initialization */
	label = g_string_new(NULL);

	g_string_printf(label, "job at %s: %s", job->hostname->str, gebr_localized_date(job->start_date->str));
	if (job->status != JOB_STATUS_RUNNING) {
		g_string_append(label, " - ");
		g_string_append(label, gebr_localized_date(job->finish_date->str));
	}
	gtk_label_set_text(GTK_LABEL(gebr.ui_job_control->label), label->str);

	/* free */
	g_string_free(label, TRUE);
}

/*
 * Function: job_translate_status
 * *Fill me in!*
 */
enum JobStatus job_translate_status(GString * status)
{
	enum JobStatus translated_status;

	if (!strcmp(status->str, "unknown"))
		translated_status = JOB_STATUS_UNKNOWN;
	else if (!strcmp(status->str, "queued"))
		translated_status = JOB_STATUS_QUEUED;
	else if (!strcmp(status->str, "failed"))
		translated_status = JOB_STATUS_FAILED;
	else if (!strcmp(status->str, "running"))
		translated_status = JOB_STATUS_RUNNING;
	else if (!strcmp(status->str, "finished"))
		translated_status = JOB_STATUS_FINISHED;
	else if (!strcmp(status->str, "canceled"))
		translated_status = JOB_STATUS_CANCELED;
	else
		translated_status = JOB_STATUS_UNKNOWN;

	return translated_status;
}

void job_update_status(struct job *job)
{
	GdkPixbuf *pixbuf;
	GtkTextIter iter;
	GtkTextMark *mark;

	/* Select and set icon */
	switch (job->status) {
	case JOB_STATUS_RUNNING:
		pixbuf = gebr.pixmaps.stock_execute;
		break;
	case JOB_STATUS_QUEUED:
		pixbuf = gebr.pixmaps.chronometer;
		break;
	case JOB_STATUS_FINISHED:
		pixbuf = gebr.pixmaps.stock_apply;
		break;
	case JOB_STATUS_FAILED:
	case JOB_STATUS_CANCELED:
		pixbuf = gebr.pixmaps.stock_cancel;
		break;
	default:
		pixbuf = NULL;
		return;
	}
	gtk_list_store_set(gebr.ui_job_control->store, &job->iter, JC_ICON, pixbuf, -1);
	if (job_is_active(job) == FALSE) 
		return;
	
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group, "job_control_stop"),
				 job->status != JOB_STATUS_QUEUED);

	/* job label */
	job_update_label(job);
	/* job info */
	if (job->status == JOB_STATUS_FINISHED) {
		GString *finish_date;

		finish_date = g_string_new(NULL);

		g_string_printf(finish_date, "\n%s %s", _("Finish date:"), gebr_localized_date(job->finish_date->str));
		gtk_text_buffer_get_end_iter(gebr.ui_job_control->text_buffer, &iter);
		gtk_text_buffer_insert(gebr.ui_job_control->text_buffer, &iter, finish_date->str, finish_date->len);

		g_string_free(finish_date, TRUE);
	}

	if (gebr.config.job_log_auto_scroll) {
		mark = gtk_text_buffer_get_mark(gebr.ui_job_control->text_buffer, "end");
		gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(gebr.ui_job_control->text_view), mark);
	}
}
