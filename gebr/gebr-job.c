/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2011 GeBR core team (http://www.gebrproject.com/)
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

#include "gebr-job.h"

#include <glib/gi18n.h>

#include "gebr.h"
#include "gebr-marshal.h"

struct _GebrJobPriv {
	GList *tasks;
	gchar *title;
	gchar *runid;
	gchar *queue;
	gchar *servers;
	GtkTreeIter iter;
	enum JobStatus status;
	gboolean has_issued;
	GtkTreeModel *model;
	gchar *input_file;
	gchar *output_file;
	gchar *log_file;
};

enum {
	STATUS_CHANGE,
	ISSUED,
	CMD_LINE_RECEIVED,
	OUTPUT,
	N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0, };

static void gebr_job_append_task_output(GebrTask *task,
					const gchar *output,
					GebrJob *job);

static void gebr_job_change_task_status(GebrTask *task,
                                        gint old_status,
                                        gint new_status,
                                        const gchar *parameter,
                                        GebrJob *job);

G_DEFINE_TYPE(GebrJob, gebr_job, G_TYPE_OBJECT);

/* Private methods {{{1 */
static void
gebr_job_finalize(GObject *object)
{
	GebrJob *job = GEBR_JOB(object);
	g_free(job->priv->title);
	g_free(job->priv->runid);
	g_free(job->priv->queue);
	g_free(job->priv->servers);
	g_free(job->priv->input_file);
	g_free(job->priv->output_file);
	g_free(job->priv->log_file);
	g_list_free(job->priv->tasks);

	G_OBJECT_CLASS(gebr_job_parent_class)->finalize(object);
}

static void
gebr_job_init(GebrJob *job)
{
	job->priv = G_TYPE_INSTANCE_GET_PRIVATE(job,
						GEBR_TYPE_JOB,
						GebrJobPriv);
	job->priv->status = JOB_STATUS_INITIAL;
	job->priv->has_issued = FALSE;
}

static void
gebr_job_class_init(GebrJobClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize = gebr_job_finalize;

	signals[STATUS_CHANGE] =
		g_signal_new("status-change",
			     G_OBJECT_CLASS_TYPE(gobject_class),
			     G_SIGNAL_RUN_FIRST,
			     G_STRUCT_OFFSET(GebrJobClass, status_change),
			     NULL, NULL,
			     gebr_cclosure_marshal_VOID__INT_INT_STRING,
			     G_TYPE_NONE, 3, G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING);

	signals[ISSUED] =
		g_signal_new("issued",
			     G_OBJECT_CLASS_TYPE(gobject_class),
			     G_SIGNAL_RUN_FIRST,
			     G_STRUCT_OFFSET(GebrJobClass, issued),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__STRING,
			     G_TYPE_NONE, 1, G_TYPE_STRING);

	signals[CMD_LINE_RECEIVED] =
		g_signal_new("cmd-line-received",
			     G_OBJECT_CLASS_TYPE(gobject_class),
			     G_SIGNAL_RUN_FIRST,
			     G_STRUCT_OFFSET(GebrJobClass, cmd_line_received),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__VOID,
			     G_TYPE_NONE, 0);

	signals[OUTPUT] =
		g_signal_new("output",
			     G_OBJECT_CLASS_TYPE(gobject_class),
			     G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			     G_STRUCT_OFFSET(GebrJobClass, output),
			     NULL, NULL,
			     gebr_cclosure_marshal_VOID__OBJECT_STRING,
			     G_TYPE_NONE, 2, GEBR_TYPE_TASK, G_TYPE_STRING);

	g_type_class_add_private(klass, sizeof(GebrJobPriv));
}

static void
gebr_job_append_task_output(GebrTask *task,
                            const gchar *output,
                            GebrJob *job)
{
	if (!*output)
		return;

	g_signal_emit(job, signals[OUTPUT], 0, task, output);
}

static gboolean
job_is_stopped(GebrJob *job)
{
	return job->priv->status == JOB_STATUS_FINISHED
		|| job->priv->status == JOB_STATUS_FAILED
		|| job->priv->status == JOB_STATUS_CANCELED;
}

static void
gebr_job_change_task_status(GebrTask *task,
                            gint old_status,
                            gint new_status,
                            const gchar *parameter,
                            GebrJob *job)
{
	GList *i;
	enum JobStatus old = job->priv->status;
	gint frac, total, ntasks;

	gebr_task_get_fraction(task, &frac, &total);
	ntasks = g_list_length(job->priv->tasks);

	if (job_is_stopped(job)) {
		gebr_task_kill(task);
		return;
	}

	switch (new_status)
	{
	case JOB_STATUS_ISSUED:
		if (!job->priv->has_issued) {
			job->priv->has_issued = TRUE;
			g_signal_emit(job, signals[ISSUED], 0, parameter);
		}
		break;
	case JOB_STATUS_RUNNING:
		job->priv->status = JOB_STATUS_RUNNING;
		g_signal_emit(job, signals[STATUS_CHANGE], 0,
			      old, job->priv->status, parameter);
		break;
	case JOB_STATUS_REQUEUED:
		g_debug("The task %d:%d was requeued to %s",
			frac, total, parameter);
		break;
	case JOB_STATUS_QUEUED:
		if (total != 1)
			g_critical("A job with multiple tasks can't be queued");
		else {
			job->priv->status = JOB_STATUS_QUEUED;
			g_signal_emit(job, signals[STATUS_CHANGE], 0,
				      old, job->priv->status, parameter);
		}
		break;
	case JOB_STATUS_CANCELED:
	case JOB_STATUS_FAILED:
		job->priv->status = new_status;
		g_signal_emit(job, signals[STATUS_CHANGE], 0,
			      old, job->priv->status, parameter);
		break;
	case JOB_STATUS_FINISHED:
		for (i = job->priv->tasks; i; i = i->next)
			if (gebr_task_get_status(i->data) != JOB_STATUS_FINISHED)
				break;
		if (ntasks == total && i == NULL) { // If i == NULL, all tasks are finished
			job->priv->status = JOB_STATUS_FINISHED;
			g_signal_emit(job, signals[STATUS_CHANGE], 0,
				      old, job->priv->status, parameter);
		}
		break;
	default:
		g_return_if_reached();
	}
	if (job->priv->model) {
		GtkTreePath *path = gtk_tree_model_get_path(job->priv->model, &job->priv->iter);
		gtk_tree_model_row_changed(job->priv->model, path, &job->priv->iter);
		gtk_tree_path_free(path);
	}
}


/* Public methods {{{1 */
GebrJob *
gebr_job_new_with_id(const gchar *rid,
		     const gchar *queue,
		     const gchar *servers)
{
	GebrJob *job = g_object_new(GEBR_TYPE_JOB, NULL);

	job->priv->queue = g_strdup(queue);
	job->priv->servers = g_strdup(servers);
	job->priv->runid = g_strdup(rid);

	return job;
}

const gchar *
gebr_job_get_group(GebrJob *job)
{
	return job->priv->servers;
}

const gchar *
gebr_job_get_queue(GebrJob *job)
{
	return job->priv->queue;
}

void
gebr_job_set_title(GebrJob *job, const gchar *title)
{
	if (job->priv->title)
		g_free(job->priv->title);
	job->priv->title = g_strdup(title);
}

const gchar *
gebr_job_get_title(GebrJob *job)
{
	return job->priv->title;
}

void
gebr_job_append_task(GebrJob *job, GebrTask *task)
{
	job->priv->tasks = g_list_prepend(job->priv->tasks, task);

	gint frac, total;
	gebr_task_get_fraction(task, &frac, &total);
	const gchar *issues = gebr_task_get_issues(task);
	enum JobStatus status = gebr_task_get_status(task);

	g_signal_connect(task, "status-change", G_CALLBACK(gebr_job_change_task_status), job);
	g_signal_connect(task, "output", G_CALLBACK(gebr_job_append_task_output), job);

	g_signal_emit(job, signals[CMD_LINE_RECEIVED], 0);
	g_signal_emit(job, signals[OUTPUT], 0, task, gebr_task_get_output(task));

	if (strlen(issues))
		gebr_job_change_task_status(task, job->priv->status, status, issues, job);

	switch(status)
	{
	case JOB_STATUS_FINISHED:
	case JOB_STATUS_CANCELED:
	case JOB_STATUS_FAILED:
		gebr_job_change_task_status(task, job->priv->status, status, gebr_task_get_finish_date(task), job);
		break;
	case JOB_STATUS_QUEUED:
		gebr_job_change_task_status(task, job->priv->status, status, gebr_job_get_queue(job), job);
		break;
	case JOB_STATUS_RUNNING:
		gebr_job_change_task_status(task, job->priv->status, status, gebr_task_get_start_date(task), job);
		break;
	default:
		g_warn_if_reached();
		break;
	}
}

enum JobStatus
gebr_job_get_status(GebrJob *job)
{
	return job->priv->status;
}

const gchar *
gebr_job_get_id(GebrJob *job)
{
	return job->priv->runid;
}

GtkTreeIter *
gebr_job_get_iter(GebrJob *job)
{
	return &job->priv->iter;
}

const gchar *
gebr_job_get_servers(GebrJob *job)
{
	return job->priv->servers;
}

gchar *
gebr_job_get_command_line(GebrJob *job)
{
	GString *buf = g_string_new(NULL);

	for (GList *i = job->priv->tasks; i; i = i->next) {
		gint frac, total;
		GebrTask *task = i->data;

		gebr_task_get_fraction(task, &frac, &total);
		g_string_append_printf(buf, _("Command line for task %d of %d "), frac, total);
		g_string_append_printf(buf, _(" \(Server: %s)\n"), (gebr_task_get_server(task))->comm->address->str);
		g_string_append_printf(buf, "%s\n", gebr_task_get_cmd_line(task));
	}

	return g_string_free(buf, FALSE);
}

gchar *
gebr_job_get_output(GebrJob *job)
{
	GString *output = g_string_new(NULL);

	for (GList *i = job->priv->tasks; i; i = i->next)
		g_string_append(output, gebr_task_get_output(i->data));

	return g_string_free(output, FALSE);
}

const gchar *
gebr_job_get_start_date(GebrJob *job)
{
	const gchar *start_date = NULL;

	for (GList *i = job->priv->tasks; i; i = i->next) {
		GebrTask *task = i->data;
		GTimeVal new_time, old_time;
		const gchar *start_task_date = gebr_task_get_start_date(task);

		if (!start_date) {
			start_date = start_task_date;
			continue;
		}

		g_time_val_from_iso8601(start_task_date, &new_time);
		g_time_val_from_iso8601(start_date, &old_time);

		if (new_time.tv_sec < old_time.tv_sec)
			start_date = start_task_date;
	}

	return start_date;
}

const gchar *
gebr_job_get_finish_date(GebrJob *job)
{
	const gchar *finish_date = NULL;

	for (GList *i = job->priv->tasks; i; i = i->next) {
		GebrTask *task = i->data;
		GTimeVal new_time, old_time;
		const gchar *finish_task_date = gebr_task_get_finish_date(task);

		if (!finish_date) {
			finish_date = finish_task_date;
			continue;
		}

		g_time_val_from_iso8601(finish_task_date, &new_time);
		g_time_val_from_iso8601(finish_date, &old_time);

		if (new_time.tv_sec > old_time.tv_sec)
			finish_date = finish_task_date;
	}

	return finish_date;
}

gchar *
gebr_job_get_issues(GebrJob *job)
{
	GString *buf = g_string_new(NULL);
	for (GList *i = job->priv->tasks; i; i = i->next)
		g_string_append_printf(buf, "%s\n", gebr_task_get_issues(i->data));
	return g_string_free(buf, FALSE);
}

void
gebr_job_close(GebrJob *job)
{
	if (job->priv->status != JOB_STATUS_FINISHED &&
	    job->priv->status != JOB_STATUS_CANCELED &&
	    job->priv->status != JOB_STATUS_FAILED)
		return;

	for (GList *i = job->priv->tasks; i; i = i->next)
		gebr_task_close(i->data, job->priv->runid);

	g_object_unref(job);
}

void
gebr_job_kill(GebrJob *job)
{
	for (GList *i = job->priv->tasks; i; i = i->next)
		gebr_task_kill(i->data);
}

void
gebr_job_set_model(GebrJob *job,
                   GtkTreeModel *model)
{
	job->priv->model = model;
}

void
gebr_job_set_io(GebrJob *job, gchar *input_file, gchar *output_file,
		gchar *log_file)
{
	job->priv->input_file = g_strdup(input_file);
	job->priv->output_file = g_strdup(output_file);
	job->priv->log_file = g_strdup(log_file);
}

void
gebr_job_get_io(GebrJob *job, gchar **input_file, gchar **output_file,
		gchar **log_file)
{
	(*input_file) = g_strdup(job->priv->input_file);
	(*output_file) = g_strdup(job->priv->output_file);
	(*log_file) = g_strdup(job->priv->log_file);
}
