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

GebrJob *job_find(GString * address, GString * id, gboolean jid)
{
	GebrJob *job = NULL;

	gboolean job_find_foreach_func(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter)
	{
		GebrJob *i;
		gboolean is_job;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_job_control->store), iter, JC_STRUCT, &i, JC_IS_JOB, &is_job,
				   -1);
		if (!is_job || strcmp(i->server->comm->address->str, address->str))
			return FALSE;
		if ((jid && !strcmp(i->parent.jid->str, id->str)) || (!jid && !strcmp(i->parent.run_id->str, id->str))) {
			job = i;
			return TRUE;	
		}
		return FALSE;
	}

	gebr_gui_gtk_tree_model_foreach_recursive(GTK_TREE_MODEL(gebr.ui_job_control->store),
						  (GtkTreeModelForeachFunc)job_find_foreach_func, NULL); 

	return job;
}


/* GOBJECT STUFF */
enum {
	LAST_PROPERTY
};
enum {
	LAST_SIGNAL
};
// static guint object_signals[LAST_SIGNAL];
G_DEFINE_TYPE(GebrJob, gebr_job, GEBR_COMM_JOB_TYPE)
static void gebr_job_init(GebrJob * job)
{
}
static void gebr_job_class_init(GebrJobClass * klass)
{
}

static GtkTreeIter job_add_jc_queue_iter(GebrJob * job)
{
	GtkTreeIter queue_jc_iter;

	server_queue_find_at_job_control(job->server, job->parent.queue_id->str, &queue_jc_iter);
	gtk_tree_store_set(gebr.ui_job_control->store, &queue_jc_iter, JC_STRUCT, job, -1);

	return queue_jc_iter;
}

static GebrJob *job_new(GebrServer *server, GString * title, GString *queue)
{
	GebrJob *job = GEBR_JOB(g_object_new(GEBR_JOB_TYPE, NULL, NULL));
	job->server = server;

	gchar local_hostname[100];
	gethostname(local_hostname, 100);

	job->parent.client_hostname = g_string_new(local_hostname != NULL ? local_hostname : "");
	job->parent.title = g_string_new(title->str);
	job->parent.queue_id = g_string_new(queue->str); 
	job->parent.status = JOB_STATUS_INITIAL; 

	/* Add iterators 
	 */
	/* Add queue on the job control list */
	GtkTreeIter queue_jc_iter = job_add_jc_queue_iter(job);
	/* Add job on the job control list */
	GtkTreeIter iter;
	gtk_tree_store_append(gebr.ui_job_control->store, &iter, &queue_jc_iter); 
	gtk_tree_store_set(gebr.ui_job_control->store, &iter,
			   JC_SERVER_ADDRESS, job->server->comm->address->str,
			   JC_QUEUE_NAME, job->parent.queue_id->str,
			   JC_TITLE, job->parent.title->str,
			   JC_STRUCT, job,
			   JC_IS_JOB, TRUE,
			   -1);
	job->iter = iter;
	/* Add queue on the server queue list model (only if server is regular) */
	GtkTreeIter queue_iter;
	gboolean queue_exists = server_queue_find(job->server, job->parent.queue_id->str, &queue_iter);
	if (!queue_exists && job->server->type == GEBR_COMM_SERVER_TYPE_REGULAR && job->parent.queue_id->str[0] == 'q') {
		GString *string = g_string_new(NULL);
		g_string_printf(string, _("At '%s'"), job->parent.queue_id->str+1);
		gtk_list_store_append(job->server->queues_model, &queue_iter);
		gtk_list_store_set(job->server->queues_model, &queue_iter,
				   SERVER_QUEUE_TITLE, string->str,
				   SERVER_QUEUE_ID, job->parent.queue_id->str, 
				   SERVER_QUEUE_LAST_RUNNING_JOB, NULL, -1);
		g_string_free(string, TRUE);
	}

	return job;
}

GebrJob *job_new_from_flow(GebrServer *server, GebrGeoXmlFlow * flow, GString *queue)
{
	GString *title = g_string_new(gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(flow)));
	GebrJob *job = job_new(server, title, queue);
	g_string_free(title, TRUE);

	return job;
}

void job_init_details(GebrJob *job, GString * _status, GString * title, GString * start_date, GString * finish_date,
		      GString * hostname, GString * issues, GString * cmd_line, GString * output, GString * queue,
		      GString * moab_jid)
{
	job->parent.status = job_translate_status(_status); 
	/* TITLE CHANGE! Shouldn't ever happen (maybe different XML parsing from client to server) */
	if (strcmp(job->parent.title->str, title->str)) {
		gtk_tree_store_set(gebr.ui_job_control->store, &job->iter,
				   JC_TITLE, title->str, -1);
		gebr_message(GEBR_LOG_DEBUG, TRUE, TRUE, _("The title of job '%s' changed to '%s' according to the server."),
			     job->parent.title->str, title->str);
	}
	g_string_assign(job->parent.title, title->str);
	if (hostname != NULL) {
		if (job->parent.client_hostname->len && strcmp(job->parent.client_hostname->str, hostname->str))
			gebr_message(GEBR_LOG_WARNING, FALSE, TRUE, _("The hostname sent for job '%s' differs from this host ('%s')."),
				     hostname->str, job->parent.client_hostname->str);
		g_string_assign(job->parent.client_hostname, hostname->str);
	}
	g_string_assign(job->parent.start_date, start_date->str);
	if (finish_date != NULL)
		g_string_assign(job->parent.finish_date, finish_date->str);
	g_string_assign(job->parent.issues, issues->str);
	g_string_assign(job->parent.cmd_line, cmd_line->str);
	g_string_assign(job->parent.moab_jid, moab_jid->str);
	/* QUEUE CHANGE!! Shouldn't ever happen */
	if (job->parent.queue_id->str[0] != 'j' && strcmp(job->parent.queue_id->str, queue->str))
		gebr_message(GEBR_LOG_DEBUG, FALSE, FALSE, _("The queue of job '%s' changed to '%s' when received from server."),
			     job->parent.title->str, job->parent.queue_id->str);
	/* necessary for the new job queue name */
	gtk_tree_store_set(gebr.ui_job_control->store, &job->iter,
			   JC_QUEUE_NAME, queue->str, -1);
	g_string_assign(job->parent.queue_id, queue->str); 

	job_append_output(job, output);
	job_status_show(job);
	job_update(job);
}

GebrJob *job_new_from_jid(GebrServer *server, GString * jid, GString * _status, GString * title,
			     GString * start_date, GString * finish_date, GString * hostname, GString * issues,
			     GString * cmd_line, GString * output, GString * queue, GString * moab_jid)
{
	GebrJob *job = job_new(server, title, queue);
	g_string_assign(job->parent.jid, jid->str);
	job_init_details(job, _status, title, start_date, finish_date, hostname, issues, cmd_line, output, queue, moab_jid);
	return job;
}

void job_free(GebrJob *job)
{
	/* UI */
	if (gtk_tree_store_remove(gebr.ui_job_control->store, &job->iter))
		gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(gebr.ui_job_control->view), &job->iter);
	else {
		gtk_text_buffer_set_text(gebr.ui_job_control->text_buffer, "", -1);
		gtk_label_set_text(GTK_LABEL(gebr.ui_job_control->label), "");
	}
	g_object_unref(job);
}

void job_delete(GebrJob *job)
{
	job_free(job);
}

const gchar *job_get_queue_name(GebrJob *job)
{
	const gchar *queue;
	if (job->server->type == GEBR_COMM_SERVER_TYPE_REGULAR) {
		if (job->parent.queue_id->str[0] == 'q')
			queue = job->parent.queue_id->str+1;
		else
			return NULL;
	} else
		queue = job->parent.queue_id->str;
	return queue;
}

void job_close(GebrJob *job, gboolean force, gboolean verbose)
{
	/* Checking if passed job pointer is valid */
	if (job == NULL)
		return;

	if (force) {
		job_delete(job);
		return;
	}
	/* NOTE: changes here must reflect changes in job_clear at gebrd */
	if (job->parent.status == JOB_STATUS_RUNNING || job->parent.status == JOB_STATUS_QUEUED) {
		if (verbose) {
			if (job->parent.status == JOB_STATUS_RUNNING)
				gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Can't close running job '%s'"), job->parent.title->str);
			else if (job->parent.status == JOB_STATUS_QUEUED)
				gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Can't close queued job '%s'"), job->parent.title->str);
		}
		return;
	}

	if (gebr_comm_server_is_logged(job->server->comm))
		gebr_comm_protocol_socket_oldmsg_send(job->server->comm->socket, FALSE,
						      gebr_comm_protocol_defs.clr_def, 1,
						      job->parent.jid->str);

	job_delete(job);
}

void job_set_active(GebrJob *job)
{
 	gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(gebr.ui_job_control->view), &job->iter);
}

gboolean job_is_active(GebrJob *job)
{
	return gtk_tree_selection_iter_is_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_job_control->view)),
						   &job->iter);
}

void job_append_output(GebrJob *job, GString * output)
{
	GtkTextIter iter;
	GString *text;
	GtkTextMark *mark;

	if (!output->len)
		return;
	if (!job->parent.output->len) {
		g_string_printf(job->parent.output, "\n%s\n%s", _("Output:"), output->str);
		text = job->parent.output;
	} else {
		g_string_append(job->parent.output, output->str);
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

void job_update(GebrJob *job)
{
	if (job_is_active(job) == FALSE)
		return;
	job_set_active(job);
}

void job_update_label(GebrJob *job)
{
	if (job_is_active(job) == FALSE) 
		return;

	GString *label = g_string_new("");
	if (job->parent.status == JOB_STATUS_INITIAL)
		g_string_printf(label, _("Job waiting for server details"));
	else {
		/* who and where, same at job_update_text_buffer */
		GString *queue_info = g_string_new(NULL); 
		const gchar *queue = job_get_queue_name(job);
		if (queue == NULL)
			g_string_assign(queue_info, _("unqueued"));
		else
			g_string_printf(queue_info, "on %s", queue);
		g_string_append_printf(label, _("Job submitted at '%s' ('%s') by %s\n"),
				       server_get_name(job->server), queue_info->str, job->parent.client_hostname->str);
		g_string_free(queue_info, TRUE);

		if (job->parent.start_date->len) {
			g_string_append(label, _(": "));
			g_string_append(label, gebr_localized_date(job->parent.start_date->str));
			if (job->parent.finish_date->len) {
				g_string_append(label, _(" - "));
				g_string_append(label, gebr_localized_date(job->parent.finish_date->str));
			} else
				g_string_append(label, _(" (running)"));
		}
		g_string_append(label, _("."));
	}

	gtk_label_set_text(GTK_LABEL(gebr.ui_job_control->label), label->str);
	g_string_free(label, TRUE);
}

enum JobStatus job_translate_status(GString * status)
{
	enum JobStatus translated_status;

	if (!strcmp(status->str, "unknown"))
		translated_status = JOB_STATUS_INITIAL;
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
		translated_status = JOB_STATUS_INITIAL;

	return translated_status;
}

void job_status_show(GebrJob *job)
{
	GdkPixbuf *pixbuf;
	
	switch (job->parent.status) {
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
	case JOB_STATUS_INITIAL:
		pixbuf = gtk_widget_render_icon(gebr.invisible, GTK_STOCK_NETWORK, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
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
		GtkTextMark *mark = gtk_text_buffer_get_mark(gebr.ui_job_control->text_buffer, "end");
		gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(gebr.ui_job_control->text_view), mark);
	}
}

void job_status_update(GebrJob *job, enum JobStatus status, const gchar *parameter)
{
	GtkTreeIter queue_iter;
	gboolean queue_exists = server_queue_find(job->server, job->parent.queue_id->str, &queue_iter);

	/* false status */
	if (status == JOB_STATUS_REQUEUED) {
		/* 'parameter' is the new name for the job's queue. */
		g_string_assign(job->parent.queue_id, parameter);

		GtkTreeIter iter;
		GtkTreeIter parent = job_add_jc_queue_iter(job);
		gtk_tree_store_append(gebr.ui_job_control->store, &iter, &parent); 
		gebr_gui_gtk_tree_model_iter_copy_values(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter, &job->iter);

		gboolean was_selected = job_is_active(job);
		gtk_tree_store_remove(gebr.ui_job_control->store, &job->iter);
		job->iter = iter;
		job_status_show(job);

		/* We suppose the requeue always happen to a true queue */
		if (job->parent.status == JOB_STATUS_RUNNING) {
			const gchar *queue_title = job->server->type == GEBR_COMM_SERVER_TYPE_REGULAR 
				? job->parent.queue_id->str+1 /* jump q identifier */ : job->parent.queue_id->str;
			GString *string = g_string_new(NULL);
			g_string_printf(string, _("After '%s' at '%s'"), job->parent.title->str, queue_title);
			gtk_list_store_set(job->server->queues_model, &queue_iter, 
					   SERVER_QUEUE_TITLE, string->str,
					   SERVER_QUEUE_ID, job->parent.queue_id->str, 
					   SERVER_QUEUE_LAST_RUNNING_JOB, job, -1);
			g_string_free(string, TRUE);
		}

		if (was_selected)
			job_set_active(job);

		return;
	}
	/* false status */
	if (status == JOB_STATUS_ISSUED) {
		g_string_append(job->parent.issues, parameter);

		if (job_is_active(job) == FALSE) 
			return;
		g_object_set(gebr.ui_job_control->issues_title_tag, "invisible", FALSE, NULL);
		GtkTextMark * mark = gtk_text_buffer_get_mark(gebr.ui_job_control->text_buffer, "issue");
		if (mark != NULL) {
			GtkTextIter iter;
			gtk_text_buffer_get_iter_at_mark(gebr.ui_job_control->text_buffer, &iter, mark);
			GString *parameter_gstring = g_string_new(parameter);
			gtk_text_buffer_insert(gebr.ui_job_control->text_buffer, &iter, parameter_gstring->str, parameter_gstring->len);
			g_string_free(parameter_gstring, TRUE);
		} else
			g_warning("Can't find mark \"issue\"");
		return;
	}
	
	enum JobStatus old_status = job->parent.status;
	job->parent.status = status;
	job_status_show(job);

	/* Update on the server queue list model */
	if (old_status == JOB_STATUS_RUNNING && queue_exists) {
		if (job->server->type == GEBR_COMM_SERVER_TYPE_REGULAR && job->parent.queue_id->str[0] == 'j') {
			/* If the job is not running anymore, then it is not an option to start a queue.
			 * Thus, it should not be in the model. */
			gtk_list_store_remove(job->server->queues_model, &queue_iter);
			gtk_combo_box_set_active(GTK_COMBO_BOX(gebr.ui_flow_edition->queue_combobox), 0);
		} else {
			/* The last job of the queue is not running anymore.
			 * Rename queue. */
			GebrJob *last_job;
			gtk_tree_model_get(GTK_TREE_MODEL(job->server->queues_model), &queue_iter, 
					   SERVER_QUEUE_LAST_RUNNING_JOB, &last_job, -1);
			if (job == last_job) {
				GString *string = g_string_new(NULL);
				g_string_printf(string, _("At '%s'"), job->server->type == GEBR_COMM_SERVER_TYPE_REGULAR
						? job->parent.queue_id->str+1 /* jump q identifier */ : job->parent.queue_id->str);
				gtk_list_store_set(job->server->queues_model, &queue_iter, 
						   SERVER_QUEUE_TITLE, string->str, 
						   SERVER_QUEUE_ID, job->parent.queue_id->str, 
						   SERVER_QUEUE_LAST_RUNNING_JOB, NULL, -1);
				g_string_free(string, TRUE);
			}
		}
	}
	if (job->parent.status == JOB_STATUS_RUNNING) {
		g_string_assign(job->parent.start_date, parameter);
		job_update(job);

		/* Update on the server queue list model */
		GString *string = g_string_new(NULL);
		if (job->server->type == GEBR_COMM_SERVER_TYPE_REGULAR && job->parent.queue_id->str[0] == 'j') {
			g_string_printf(string, _("After '%s'"), job->parent.title->str);
		} else {
			const gchar *queue_title = job->server->type == GEBR_COMM_SERVER_TYPE_REGULAR 
				? job->parent.queue_id->str+1 /* jump q identifier */ : job->parent.queue_id->str;
			g_string_printf(string, _("After '%s' at '%s'"), job->parent.title->str, queue_title);
		}
		if (!queue_exists)
			gtk_list_store_append(job->server->queues_model, &queue_iter);
		gtk_list_store_set(job->server->queues_model, &queue_iter, 
				   SERVER_QUEUE_TITLE, string->str,
				   SERVER_QUEUE_ID, job->parent.queue_id->str, 
				   SERVER_QUEUE_LAST_RUNNING_JOB, job, -1);
		g_string_free(string, TRUE);

	}
	if ((job->parent.status == JOB_STATUS_FINISHED) || job->parent.status == JOB_STATUS_CANCELED) {
		g_string_assign(job->parent.finish_date, parameter);
		job_update_label(job);

		if (job_is_active(job) == FALSE) 
			return;

		/* Add to the text buffer */
		GString *finish_date = g_string_new(NULL);
		g_string_printf(finish_date, "\n%s %s",
			       	job->parent.status == JOB_STATUS_FINISHED ? _("Finish date:") : _("Cancel date:"),
			       	gebr_localized_date(job->parent.finish_date->str));
		GtkTextIter iter;
		gtk_text_buffer_get_end_iter(gebr.ui_job_control->text_buffer, &iter);
		gtk_text_buffer_insert(gebr.ui_job_control->text_buffer, &iter, finish_date->str, finish_date->len);
		g_string_free(finish_date, TRUE);
	} 
}
