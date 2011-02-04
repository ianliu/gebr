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

#include <string.h>
#include <unistd.h>

#include <glib/gi18n.h>
#include <libgebr/gui/gebr-gui-utils.h>
#include <libgebr/utils.h>
#include <libgebr/date.h>

#include "job.h"
#include "gebr.h"
#include "ui_job_control.h"

struct job *job_find(GString * address, GString * id, gboolean jid)
{
	struct job *job = NULL;

	gboolean job_find_foreach_func(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter)
	{
		struct job *i;
		gboolean is_job;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_job_control->store), iter, JC_STRUCT, &i, JC_IS_JOB, &is_job,
				   -1);
		if (!is_job || strcmp(i->server->comm->address->str, address->str))
			return FALSE;
		if ((jid && !strcmp(i->jid->str, id->str)) || (!jid && !strcmp(i->run_id->str, id->str))) {
			job = i;
			return TRUE;	
		}
		return FALSE;
	}

	gebr_gui_gtk_tree_model_foreach_recursive(GTK_TREE_MODEL(gebr.ui_job_control->store),
						  (GtkTreeModelForeachFunc)job_find_foreach_func, NULL); 

	return job;
}

static GtkTreeIter job_add_jc_queue_iter(struct job * job)
{
	GtkTreeIter queue_jc_iter;

	server_queue_find_at_job_control(job->server, job->queue->str, &queue_jc_iter);
	gtk_tree_store_set(gebr.ui_job_control->store, &queue_jc_iter, JC_STRUCT, job, -1);

	return queue_jc_iter;
}

static struct job *job_new(struct server *server, GString * title, GString *queue)
{
	struct job *job = g_new(struct job, 1);
	job->server = server;
	job->status = JOB_STATUS_UNKNOWN; 
	job->waiting_server_details = FALSE;
	gchar local_hostname[100];
	gethostname(local_hostname, 100);
	job->hostname = g_string_new(local_hostname != NULL ? local_hostname : "");
	job->title = g_string_new(title->str);
	job->queue = g_string_new(queue->str); 

	job->run_id = g_string_new("");
	job->jid = g_string_new("");
	job->start_date = g_string_new("");
	job->finish_date = g_string_new("");
	job->issues = g_string_new("");
	job->cmd_line = g_string_new("");
	job->output = g_string_new("");
	job->moab_jid = g_string_new("");

	/* Add iterators 
	 */
	/* Add queue on the job control list */
	GtkTreeIter queue_jc_iter = job_add_jc_queue_iter(job);
	/* Add job on the job control list */
	GtkTreeIter iter;
	gtk_tree_store_append(gebr.ui_job_control->store, &iter, &queue_jc_iter); 
	gtk_tree_store_set(gebr.ui_job_control->store, &iter,
			   JC_SERVER_ADDRESS, job->server->comm->address->str,
			   JC_QUEUE_NAME, job->queue->str,
			   JC_TITLE, job->title->str,
			   JC_STRUCT, job,
			   JC_IS_JOB, TRUE,
			   -1);
	job->iter = iter;
	/* Add queue on the server queue list model (only if server is regular) */
	GtkTreeIter queue_iter;
	gboolean queue_exists = server_queue_find(job->server, job->queue->str, &queue_iter);
	if (!queue_exists && job->server->type == GEBR_COMM_SERVER_TYPE_REGULAR && job->queue->str[0] == 'q') {
		GString *string = g_string_new(NULL);
		g_string_printf(string, _("At '%s'"), job->queue->str+1);
		gtk_list_store_append(job->server->queues_model, &queue_iter);
		gtk_list_store_set(job->server->queues_model, &queue_iter,
				   SERVER_QUEUE_TITLE, string->str,
				   SERVER_QUEUE_ID, job->queue->str, 
				   SERVER_QUEUE_LAST_RUNNING_JOB, NULL, -1);
		g_string_free(string, TRUE);
	}

	return job;
}

struct job *job_new_from_flow(struct server *server, GebrGeoXmlFlow * flow, GString *queue)
{
	GString *title = g_string_new(gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(flow)));
	struct job *job = job_new(server, title, queue);
	g_string_free(title, TRUE);

	job->waiting_server_details = TRUE;

	return job;
}

void job_init_details(struct job *job, GString * _status, GString * title, GString * start_date, GString * finish_date,
		      GString * hostname, GString * issues, GString * cmd_line, GString * output, GString * queue,
		      GString * moab_jid)
{
	job->status = job_translate_status(_status); 
	job->waiting_server_details = FALSE;
	/* TITLE CHANGE! Shouldn't ever happen (maybe different XML parsing from client to server) */
	if (strcmp(job->title->str, title->str)) {
		gtk_tree_store_set(gebr.ui_job_control->store, &job->iter,
				   JC_TITLE, title->str, -1);
		gebr_message(GEBR_LOG_DEBUG, TRUE, TRUE, _("The title of job '%s' changed to '%s' according to the server."), job->title->str, title->str);
	}
	g_string_assign(job->title, title->str);
	if (hostname != NULL) {
		if (job->hostname->len && strcmp(job->hostname->str, hostname->str))
			gebr_message(GEBR_LOG_WARNING, FALSE, TRUE, _("The hostname sent for job '%s' differs from this host ('%s')."), hostname->str, job->hostname->str);
		g_string_assign(job->hostname, hostname->str);
	}
	g_string_assign(job->start_date, start_date->str);
	if (finish_date != NULL)
		g_string_assign(job->finish_date, finish_date->str);
	g_string_assign(job->issues, issues->str);
	g_string_assign(job->cmd_line, cmd_line->str);
	g_string_assign(job->moab_jid, moab_jid->str);
	/* QUEUE CHANGE!! Shouldn't ever happen */
	if (job->queue->str[0] != 'j' && strcmp(job->queue->str, queue->str))
		gebr_message(GEBR_LOG_DEBUG, FALSE, FALSE, _("The queue of job '%s' changed to '%s' when received from server."), job->title->str, job->queue->str);
	/* necessary for the new job queue name */
	gtk_tree_store_set(gebr.ui_job_control->store, &job->iter,
			   JC_QUEUE_NAME, queue->str, -1);
	g_string_assign(job->queue, queue->str); 

	job_append_output(job, output);
	job_status_show(job);
	job_update(job);
}

struct job *job_new_from_jid(struct server *server, GString * jid, GString * _status, GString * title,
			     GString * start_date, GString * finish_date, GString * hostname, GString * issues,
			     GString * cmd_line, GString * output, GString * queue, GString * moab_jid)
{
	struct job *job = job_new(server, title, queue);
	g_string_assign(job->jid, jid->str);
	job_init_details(job, _status, title, start_date, finish_date, hostname, issues, cmd_line, output, queue, moab_jid);
	return job;
}

void job_free(struct job *job)
{
	/* UI */
	if (gtk_tree_store_remove(gebr.ui_job_control->store, &job->iter))
		gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(gebr.ui_job_control->view), &job->iter);
	else {
		gtk_text_buffer_set_text(gebr.ui_job_control->text_buffer, "", -1);
		gtk_label_set_text(GTK_LABEL(gebr.ui_job_control->label), "");
	}
	/* struct */
	g_string_free(job->title, TRUE);
	g_string_free(job->run_id, TRUE);
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
	job_free(job);
}

void job_close(struct job *job, gboolean force, gboolean verbose)
{
	/* Checking if passed job pointer is valid */
	if (job == NULL)
		return;

	if (force) {
		job_delete(job);
		return;
	}
	/* NOTE: changes here must reflect changes in job_clear at gebrd */
	if (job->status == JOB_STATUS_RUNNING || job->status == JOB_STATUS_QUEUED) {
		if (verbose) {
			if (job->status == JOB_STATUS_RUNNING)
				gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Can't close running job '%s'"), job->title->str);
			else if (job->status == JOB_STATUS_QUEUED)
				gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Can't close queued job '%s'"), job->title->str);
		}
		return;
	}

	if (gebr_comm_server_is_logged(job->server->comm))
		gebr_comm_protocol_send_data(job->server->comm->protocol, job->server->comm->stream_socket,
					     gebr_comm_protocol_defs.clr_def, 1, job->jid->str);

	job_delete(job);
}

void job_set_active(struct job *job)
{
 	gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(gebr.ui_job_control->view), &job->iter);
}

gboolean job_is_active(struct job *job)
{
	return gtk_tree_selection_iter_is_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_job_control->view)),
						   &job->iter);
}

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

void job_update(struct job *job)
{
	if (job_is_active(job) == FALSE)
		return;
	job_set_active(job);
}

void job_update_label(struct job *job)
{
	if (job_is_active(job) == FALSE) 
		return;

	GString *label = g_string_new("");
	if (job->waiting_server_details)
		g_string_printf(label, _("job waiting for server details"));
	else if (job->status == JOB_STATUS_QUEUED && job->queue->str[0] != 'j')
		g_string_printf(label, _("job queued (%s) at %s"), job->queue->str+1, job->hostname->str);
	else if (job->start_date->len) {
		g_string_printf(label, _("job at %s: %s"), job->hostname->str, gebr_localized_date(job->start_date->str));
		if (job->finish_date->len) {
			g_string_append(label, _(" - "));
			g_string_append(label, gebr_localized_date(job->finish_date->str));
		}
	} else
		g_string_printf(label, _("job at %s"), job->hostname->str);

	gtk_label_set_text(GTK_LABEL(gebr.ui_job_control->label), label->str);
	g_string_free(label, TRUE);
}

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
	else if (!strcmp(status->str, "requeued"))
		translated_status = JOB_STATUS_REQUEUED;
	else if (!strcmp(status->str, "issued"))
		translated_status = JOB_STATUS_ISSUED;
	else
		translated_status = JOB_STATUS_UNKNOWN;

	return translated_status;
}

void job_status_show(struct job *job)
{
	GdkPixbuf *pixbuf;
	
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
	gtk_tree_store_set(gebr.ui_job_control->store, &job->iter, JC_ICON, pixbuf, -1);

	if (job_is_active(job) == FALSE) 
		return;

	job_update_label(job);
	if (gebr.config.job_log_auto_scroll) {
		GtkTextMark *mark;

		mark = gtk_text_buffer_get_mark(gebr.ui_job_control->text_buffer, "end");
		gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(gebr.ui_job_control->text_view), mark);
	}
}

void job_status_update(struct job *job, enum JobStatus status, const gchar *parameter)
{
	GtkTreeIter queue_iter;
	gboolean queue_exists = server_queue_find(job->server, job->queue->str, &queue_iter);

	/* false status */
	if (status == JOB_STATUS_REQUEUED) {
		/* 'parameter' is the new name for the job's queue. */
		g_string_assign(job->queue, parameter);

		GtkTreeIter iter;
		GtkTreeIter parent = job_add_jc_queue_iter(job);
		gtk_tree_store_append(gebr.ui_job_control->store, &iter, &parent); 
		gebr_gui_gtk_tree_model_iter_copy_values(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter, &job->iter);

		gboolean was_selected = job_is_active(job);
		gtk_tree_store_remove(gebr.ui_job_control->store, &job->iter);
		job->iter = iter;
		job_status_show(job);

		/* We suppose the requeue always happen to a true queue */
		if (job->status == JOB_STATUS_RUNNING) {
			puts("here");
			const gchar *queue_title = job->server->type == GEBR_COMM_SERVER_TYPE_REGULAR 
				? job->queue->str+1 /* jump q identifier */ : job->queue->str;
			GString *string = g_string_new(NULL);
			g_string_printf(string, _("After '%s' at '%s'"), job->title->str, queue_title);
			gtk_list_store_set(job->server->queues_model, &queue_iter, 
					   SERVER_QUEUE_TITLE, string->str,
					   SERVER_QUEUE_ID, job->queue->str, 
					   SERVER_QUEUE_LAST_RUNNING_JOB, job, -1);
			g_string_free(string, TRUE);
		}

		if (was_selected)
			job_set_active(job);

		return;
	}
	/* false status */
	if (status == JOB_STATUS_ISSUED) {
		g_string_append(job->issues, parameter);

		if (job_is_active(job) == FALSE) 
			return;
		g_object_set(gebr.ui_job_control->issues_title_tag, "invisible", FALSE, NULL);
		GtkTextMark * mark = gtk_text_buffer_get_mark(gebr.ui_job_control->text_buffer, "issue");
		if (mark != NULL) {
			GtkTextIter iter;
			gtk_text_buffer_get_iter_at_mark(gebr.ui_job_control->text_buffer, &iter, mark);
			gtk_text_buffer_insert(gebr.ui_job_control->text_buffer, &iter, job->issues->str, job->issues->len);
		} else
			g_warning("Can't find mark \"issue\"");
		return;
	}
	
	enum JobStatus old_status = job->status;
	job->status = status;
	job_status_show(job);

	/* Update on the server queue list model */
	if (old_status == JOB_STATUS_RUNNING && queue_exists) {
		if (job->server->type == GEBR_COMM_SERVER_TYPE_REGULAR && job->queue->str[0] == 'j') {
			/* If the job is not running anymore, then it is not an option to start a queue.
			 * Thus, it should not be in the model. */
			gtk_list_store_remove(job->server->queues_model, &queue_iter);
			gtk_combo_box_set_active(GTK_COMBO_BOX(gebr.ui_flow_edition->queue_combobox), 0);
		} else {
			/* The last job of the queue is not running anymore.
			 * Rename queue. */
			struct job *last_job;
			gtk_tree_model_get(GTK_TREE_MODEL(job->server->queues_model), &queue_iter, 
					   SERVER_QUEUE_LAST_RUNNING_JOB, &last_job, -1);
			if (job == last_job) {
				GString *string = g_string_new(NULL);
				g_string_printf(string, _("At '%s'"), job->server->type == GEBR_COMM_SERVER_TYPE_REGULAR
						? job->queue->str+1 /* jump q identifier */ : job->queue->str);
				gtk_list_store_set(job->server->queues_model, &queue_iter, 
						   SERVER_QUEUE_TITLE, string->str, 
						   SERVER_QUEUE_ID, job->queue->str, 
						   SERVER_QUEUE_LAST_RUNNING_JOB, NULL, -1);
				g_string_free(string, TRUE);
			}
		}
	}
	if (job->status == JOB_STATUS_RUNNING) {
		g_string_assign(job->start_date, parameter);
		job_update(job);

		/* Update on the server queue list model */
		GString *string = g_string_new(NULL);
		if (job->server->type == GEBR_COMM_SERVER_TYPE_REGULAR && job->queue->str[0] == 'j') {
			g_string_printf(string, _("After '%s'"), job->title->str);
		} else {
			const gchar *queue_title = job->server->type == GEBR_COMM_SERVER_TYPE_REGULAR 
				? job->queue->str+1 /* jump q identifier */ : job->queue->str;
			g_string_printf(string, _("After '%s' at '%s'"), job->title->str, queue_title);
		}
		if (!queue_exists)
			gtk_list_store_append(job->server->queues_model, &queue_iter);
		gtk_list_store_set(job->server->queues_model, &queue_iter, 
				   SERVER_QUEUE_TITLE, string->str,
				   SERVER_QUEUE_ID, job->queue->str, 
				   SERVER_QUEUE_LAST_RUNNING_JOB, job, -1);
		g_string_free(string, TRUE);

	}
	if ((job->status == JOB_STATUS_FINISHED) || job->status == JOB_STATUS_CANCELED) {
		g_string_assign(job->finish_date, parameter);
		job_update_label(job);

		if (job_is_active(job) == FALSE) 
			return;

		/* Add to the text buffer */
		GString *finish_date = g_string_new(NULL);
		g_string_printf(finish_date, "\n%s %s",
			       	job->status == JOB_STATUS_FINISHED ? _("Finish date:") : _("Cancel date:"),
			       	gebr_localized_date(job->finish_date->str));
		GtkTextIter iter;
		gtk_text_buffer_get_end_iter(gebr.ui_job_control->text_buffer, &iter);
		gtk_text_buffer_insert(gebr.ui_job_control->text_buffer, &iter, finish_date->str, finish_date->len);
		g_string_free(finish_date, TRUE);
	} 
}
