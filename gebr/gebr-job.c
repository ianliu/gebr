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
#include <stdlib.h>
#include <libgebr/utils.h>

#include "gebr.h"
#include "gebr-marshal.h"

struct _GebrJobPriv {
	GList *tasks;
	gchar *title;
	gchar *runid;
	gchar *queue;
	gchar *hostname;
	gchar **servers;
	gchar *server_group;
	gint exec_speed;
	gint n_servers;
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
	DISCONNECT,
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

static void on_task_destroy(GebrJob *job,
			    GebrTask *task);

G_DEFINE_TYPE(GebrJob, gebr_job, G_TYPE_OBJECT);

/* Private methods {{{1 */
static void
gebr_job_finalize(GObject *object)
{
	GebrJob *job = GEBR_JOB(object);
	g_free(job->priv->title);
	g_free(job->priv->hostname);
	g_free(job->priv->runid);
	g_free(job->priv->queue);
	g_strfreev(job->priv->servers);
	g_free(job->priv->input_file);
	g_free(job->priv->output_file);
	g_free(job->priv->log_file);
	g_list_foreach(job->priv->tasks, (GFunc)g_object_unref, NULL);
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

	signals[DISCONNECT] =
		g_signal_new("disconnect",
			     G_OBJECT_CLASS_TYPE(gobject_class),
			     G_SIGNAL_RUN_FIRST,
			     G_STRUCT_OFFSET(GebrJobClass, disconnect),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__VOID,
			     G_TYPE_NONE, 0);

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

gboolean
gebr_job_is_stopped(GebrJob *job)
{
	return job->priv->status == JOB_STATUS_FINISHED
		|| job->priv->status == JOB_STATUS_FAILED
		|| job->priv->status == JOB_STATUS_CANCELED;
}

gboolean
gebr_job_is_queueable(GebrJob *job)
{
	return job->priv->status == JOB_STATUS_QUEUED
		|| job->priv->status == JOB_STATUS_RUNNING;
}

gboolean
gebr_job_can_close(GebrJob *job)
{
	enum JobStatus status = gebr_job_get_partial_status(job);

	return status == JOB_STATUS_FINISHED
		|| status == JOB_STATUS_FAILED
		|| status == JOB_STATUS_CANCELED;
}

gboolean
gebr_job_can_kill(GebrJob *job)
{
	enum JobStatus status = gebr_job_get_partial_status(job);

	return status == JOB_STATUS_QUEUED
		|| status == JOB_STATUS_RUNNING;
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

	if (gebr_job_is_stopped(job)) {
		gebr_task_kill(task);
		return;
	}

	/* Do not change the status if the job isn't complete.
	 * But if the new status is Failed or Canceled, let this
	 * change pass.
	 */
	if (ntasks != total &&
	    (new_status != JOB_STATUS_CANCELED &&
	     new_status != JOB_STATUS_FAILED))
		return;

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

static void
on_task_destroy(GebrJob *job,
		GebrTask *finalized_task)
{
	enum JobStatus old_status = job->priv->status;
	job->priv->status = JOB_STATUS_INITIAL;
	job->priv->tasks = g_list_remove(job->priv->tasks, finalized_task);

	g_debug("JOB: Task %p disconnected", finalized_task);

	if (!job->priv->tasks)
		g_signal_emit(job, signals[DISCONNECT], 0);
	else if (old_status != job->priv->status)
		g_signal_emit(job, signals[STATUS_CHANGE], 0,
			      old_status, job->priv->status, "");
}

static gint
compare_func(gconstpointer a, gconstpointer b)
{
	const gchar *aa = *(gchar * const *)a;
	const gchar *bb = *(gchar * const *)b;

	if (g_strcmp0(aa, "127.0.0.1") == 0)
		return -1;

	if (g_strcmp0(bb, "127.0.0.1") == 0)
		return 1;

	return g_strcmp0(aa, bb);
}

/* Public methods {{{1 */
GebrJob *
gebr_job_new_with_id(const gchar *rid,
		     const gchar *queue,
		     const gchar *servers)
{
	g_return_val_if_fail(servers != NULL, NULL);
	g_return_val_if_fail(strlen(servers) > 0, NULL);

	GebrJob *job = g_object_new(GEBR_TYPE_JOB, NULL);

	job->priv->queue = g_strdup(queue);
	job->priv->servers = g_strsplit(servers, ",", 0);
	job->priv->runid = g_strdup(rid);
	job->priv->n_servers = 0;

	while (job->priv->servers[job->priv->n_servers])
		job->priv->n_servers++;

	qsort(job->priv->servers, job->priv->n_servers,
	      sizeof(job->priv->servers[0]), compare_func);

	return job;
}

void
gebr_job_set_hostname(GebrJob *job,
		      const gchar *hostname)
{
	if (job->priv->hostname)
		g_free(job->priv->hostname);
	job->priv->hostname = g_strdup(hostname);
}

const gchar *
gebr_job_get_hostname(GebrJob *job)
{
	return job->priv->hostname;
}

const gchar *
gebr_job_get_queue(GebrJob *job)
{
	return gebr_task_get_queue(job->priv->tasks->data);
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
	g_object_weak_ref(G_OBJECT(task), (GWeakNotify)on_task_destroy, job);

	g_signal_emit(job, signals[CMD_LINE_RECEIVED], 0);
	g_signal_emit(job, signals[OUTPUT], 0, task, gebr_task_get_output(task));

	if (strlen(issues))
		gebr_job_change_task_status(task, job->priv->status,
					    JOB_STATUS_ISSUED, issues, job);

	switch(status)
	{
	case JOB_STATUS_FAILED:
		gebr_job_change_task_status(task, job->priv->status, status, "", job);
		break;
	case JOB_STATUS_FINISHED:
	case JOB_STATUS_CANCELED:
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

gchar **
gebr_job_get_servers(GebrJob *job, gint *n)
{
	if (n)
		*n = job->priv->n_servers;
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
		if (total != 1) {
			g_string_append_printf(buf, _("Command line for task %d of %d "), frac, total);
			g_string_append_printf(buf, _(" \(Server: %s)\n"), (gebr_task_get_server(task))->comm->address->str);
		}
		g_string_append_printf(buf, "%s\n\n", gebr_task_get_cmd_line(task));
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
gebr_job_get_last_run_date(GebrJob *job)
{
	const gchar *last_run_date = NULL;

	for (GList *i = job->priv->tasks; i; i = i->next) {
		GebrTask *task = i->data;
		GTimeVal new_time, old_time;
		const gchar *last_run_task_date = gebr_task_get_last_run_date(task);

		if (!last_run_date) {
			last_run_date = last_run_task_date;
			continue;
		}

		g_time_val_from_iso8601(last_run_task_date, &new_time);
		g_time_val_from_iso8601(last_run_date, &old_time);

		if (new_time.tv_sec < old_time.tv_sec)
			last_run_date = last_run_task_date;
	}

	return last_run_date;
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
	return g_strdup(gebr_task_get_issues(job->priv->tasks->data));
}

gboolean
gebr_job_has_issues(GebrJob *job)
{
	if (!job->priv->tasks)
		return FALSE;

	return gebr_task_get_issues(job->priv->tasks->data)[0] == '\0' ? FALSE : TRUE;
}

gboolean
gebr_job_close(GebrJob *job)
{
	if (!gebr_job_can_close(job))
		return FALSE;

	for (GList *i = job->priv->tasks; i; i = i->next)
		gebr_task_close(i->data, job->priv->runid);

	return TRUE;
}

void
gebr_job_kill(GebrJob *job)
{
	if (!gebr_job_can_kill(job))
		return;

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
gebr_job_set_io(GebrJob *job,
		const gchar *input_file,
		const gchar *output_file,
		const gchar *log_file)
{
	if (job->priv->input_file)
		g_free(job->priv->input_file);

	if (job->priv->output_file)
		g_free(job->priv->output_file);

	if (job->priv->log_file)
		g_free(job->priv->log_file);

	job->priv->input_file = g_strdup(input_file);
	job->priv->output_file = g_strdup(output_file);
	job->priv->log_file = g_strdup(log_file);
}

void
gebr_job_get_io(GebrJob *job,
		gchar **input_file,
		gchar **output_file,
		gchar **log_file)
{
	if (input_file) {
		if (!job->priv->input_file || strlen(job->priv->input_file) == 0)
			*input_file = NULL;
		else
			*input_file = g_strdup(job->priv->input_file);
	}

	if (output_file) {
		if (!job->priv->output_file || strlen(job->priv->output_file) == 0)
			*output_file = NULL;
		else
			*output_file = g_strdup(job->priv->output_file);
	}

	if (log_file) {
		if (!job->priv->log_file || strlen(job->priv->log_file) == 0)
			*log_file = NULL;
		else
			*log_file = g_strdup(job->priv->log_file);
	}
}

void
gebr_job_get_resources(GebrJob *job,
                       gchar **nprocs,
                       gchar **niceness)
{
	gint total_procs = 0;

	if (!job->priv->tasks) {
		*nprocs = NULL;
		*niceness = NULL;
		return;
	}

	for (GList *i = job->priv->tasks; i; i = i->next) {
		GebrTask *task = i->data;
		gint n = atoi(gebr_task_get_nprocs(task));
		total_procs += n;
	}

	*nprocs = g_strdup_printf("%d", total_procs);
	*niceness = g_strdup(gebr_task_get_niceness(job->priv->tasks->data));
}

gchar *
gebr_job_get_remaining_servers(GebrJob *job)
{
	gint total, ntasks;
	GString *remainings = g_string_new(NULL);

	if (!job->priv->tasks)
		return NULL;

	gebr_task_get_fraction(job->priv->tasks->data, NULL, &total);
	ntasks = g_list_length(job->priv->tasks);

	if (ntasks != total) {
		gchar **servers = job->priv->servers;

		for (gint j = 0; servers[j]; j++) {
			gboolean has_server = FALSE;

			for (GList *i = job->priv->tasks; i; i = i->next) {
				GebrServer *server = gebr_task_get_server(i->data);
				if (!g_strcmp0(servers[j], server->comm->address->str)) {
					has_server = TRUE;
					break;
				}
			}
			if (!has_server) {
				if (remainings->len == 0)
					g_string_printf(remainings, "%s", servers[j]);
				else
					g_string_append_printf(remainings, ", %s", servers[j]);
			}
		}
	}

	return g_string_free(remainings, FALSE);
}

gchar *
gebr_job_get_running_time(GebrJob *job, const gchar *start_date)
{
	GTimeVal start_time, current_time;
	g_time_val_from_iso8601(start_date, &start_time);
	g_get_current_time(&current_time);

	return gebr_calculate_detailed_relative_time(&start_time, &current_time);
}
gchar *
gebr_job_get_elapsed_time(GebrJob *job)
{
	const gchar *start_time = gebr_job_get_start_date(job);
	const gchar *finish_time = gebr_job_get_finish_date(job);
	GTimeVal finish, start;
	g_time_val_from_iso8601(finish_time, &finish);
	g_time_val_from_iso8601(start_time, &start);

	return gebr_calculate_detailed_relative_time(&start, &finish);
}

void
gebr_job_set_server_group(GebrJob *job, const gchar *server_group)
{
	if (job->priv->server_group)
		g_free(job->priv->server_group);
	job->priv->server_group = g_strdup(server_group ? server_group : "");
}

const gchar *
gebr_job_get_server_group(GebrJob *job)
{
	return job->priv->server_group ? job->priv->server_group : "";
}

gint
gebr_job_get_exec_speed(GebrJob *job)
{
	return job->priv->exec_speed;
}

void
gebr_job_set_exec_speed(GebrJob *job, gint exec_speed)
{
	job->priv->exec_speed = exec_speed;
}

GebrTask *
gebr_job_get_task_from_server(GebrJob *job,
			      const gchar *server)
{
	for (GList *i = job->priv->tasks; i; i = i->next)
		if (g_strcmp0(gebr_task_get_server(i->data)->comm->address->str, server) == 0)
			return i->data;
	return NULL;
}

GList *
gebr_job_get_list_of_tasks(GebrJob *job)
{
	return job->priv->tasks;
}

enum JobStatus
gebr_job_get_partial_status(GebrJob *job)
{
	if (!job->priv->tasks)
		return JOB_STATUS_INITIAL;

	gint total, ntasks;
	GebrTask *task = job->priv->tasks->data;

	gebr_task_get_fraction(task, NULL, &total);
	ntasks = g_list_length(job->priv->tasks);

	if (ntasks == total)
		return gebr_job_get_status(job);

	gint running = 0;
	for (GList *i = job->priv->tasks; i; i = i->next) {
		enum JobStatus sta = gebr_task_get_status(i->data);
		switch (sta) {
		case JOB_STATUS_FAILED:
		case JOB_STATUS_FINISHED:
		case JOB_STATUS_CANCELED:
		case JOB_STATUS_QUEUED:
			return sta;
		case JOB_STATUS_REQUEUED:
		case JOB_STATUS_ISSUED:
		case JOB_STATUS_INITIAL:
			g_return_val_if_reached(sta);
		case JOB_STATUS_RUNNING:
			running++;
			break;
		}
	}

	if (running == ntasks)
		return JOB_STATUS_RUNNING;

	g_return_val_if_reached(JOB_STATUS_RUNNING);
}
