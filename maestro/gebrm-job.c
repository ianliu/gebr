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

#include "gebrm-job.h"

#include <glib/gi18n.h>
#include <stdlib.h>
#include <libgebr/utils.h>

#include "gebrm-marshal.h"

typedef struct {
	GebrCommRunner *runner;
	GebrmJob *child;
} RunnerAndJob;

struct _GebrmJobPriv {
	GebrmJobInfo info;
	GList *tasks;
	gchar *servers;
	gchar *nprocs;
	gint total;
	GebrCommJobStatus status;
	gboolean has_issued;
	GList *children; // A list of GebrCommRunner
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

static void gebrm_job_append_task_output(GebrmTask *task,
					 const gchar *output,
					 GebrmJob *job);

static void gebrm_job_change_task_status(GebrmTask *task,
					 gint old_status,
					 gint new_status,
					 const gchar *parameter,
					 GebrmJob *job);

static void on_task_destroy(GebrmJob *job,
			    GebrmTask *task);

G_DEFINE_TYPE(GebrmJob, gebrm_job, G_TYPE_OBJECT);

/* Private methods {{{1 */
static void
gebrm_job_finalize(GObject *object)
{
	GebrmJob *job = GEBRM_JOB(object);

	g_free(job->priv->info.id);
	g_free(job->priv->info.title);
	g_free(job->priv->info.hostname);
	g_free(job->priv->info.parent_id);
	g_free(job->priv->info.nice);
	g_free(job->priv->info.input);
	g_free(job->priv->info.output);
	g_free(job->priv->info.error);
	g_free(job->priv->info.submit_date);
	g_free(job->priv->info.group);
	g_free(job->priv->info.speed);
	g_free(job->priv->servers);
	g_free(job->priv->nprocs);
	g_list_foreach(job->priv->tasks, (GFunc)g_object_unref, NULL);
	g_list_free(job->priv->tasks);

	G_OBJECT_CLASS(gebrm_job_parent_class)->finalize(object);
}

static void
gebrm_job_status_change_real(GebrmJob *job,
			     gint old_status,
			     gint new_status,
			     const gchar *parameter,
			     gpointer user_data)
{
	switch (new_status) {
	case JOB_STATUS_FINISHED:
	case JOB_STATUS_FAILED:
	case JOB_STATUS_CANCELED: {
		for (GList *i = job->priv->children; i; i = i->next) {
			RunnerAndJob *raj = i->data;
			gebr_comm_runner_run_async(raj->runner, gebrm_job_get_id(raj->child));
		}
		g_list_free(job->priv->children);
		job->priv->children = NULL;
		break;
	}
	default:
		break;
	}
}

static void
gebrm_job_init(GebrmJob *job)
{
	job->priv = G_TYPE_INSTANCE_GET_PRIVATE(job,
						GEBRM_TYPE_JOB,
						GebrmJobPriv);
	job->priv->status = JOB_STATUS_INITIAL;
	job->priv->has_issued = FALSE;
}

static void
gebrm_job_class_init(GebrmJobClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize = gebrm_job_finalize;

	klass->status_change = gebrm_job_status_change_real;

	signals[STATUS_CHANGE] =
		g_signal_new("status-change",
			     G_OBJECT_CLASS_TYPE(gobject_class),
			     G_SIGNAL_RUN_FIRST,
			     G_STRUCT_OFFSET(GebrmJobClass, status_change),
			     NULL, NULL,
			     gebrm_cclosure_marshal_VOID__INT_INT_STRING,
			     G_TYPE_NONE, 3, G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING);

	signals[ISSUED] =
		g_signal_new("issued",
			     G_OBJECT_CLASS_TYPE(gobject_class),
			     G_SIGNAL_RUN_FIRST,
			     G_STRUCT_OFFSET(GebrmJobClass, issued),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__STRING,
			     G_TYPE_NONE, 1, G_TYPE_STRING);

	signals[CMD_LINE_RECEIVED] =
		g_signal_new("cmd-line-received",
			     G_OBJECT_CLASS_TYPE(gobject_class),
			     G_SIGNAL_RUN_FIRST,
			     G_STRUCT_OFFSET(GebrmJobClass, cmd_line_received),
			     NULL, NULL,
			     gebrm_cclosure_marshal_VOID__OBJECT_STRING,
			     G_TYPE_NONE, 2, GEBRM_TYPE_TASK, G_TYPE_STRING);

	signals[OUTPUT] =
		g_signal_new("output",
			     G_OBJECT_CLASS_TYPE(gobject_class),
			     G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			     G_STRUCT_OFFSET(GebrmJobClass, output),
			     NULL, NULL,
			     gebrm_cclosure_marshal_VOID__OBJECT_STRING,
			     G_TYPE_NONE, 2, GEBRM_TYPE_TASK, G_TYPE_STRING);

	signals[DISCONNECT] =
		g_signal_new("disconnect",
			     G_OBJECT_CLASS_TYPE(gobject_class),
			     G_SIGNAL_RUN_FIRST,
			     G_STRUCT_OFFSET(GebrmJobClass, disconnect),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__VOID,
			     G_TYPE_NONE, 0);

	g_type_class_add_private(klass, sizeof(GebrmJobPriv));
}

static void
gebrm_job_append_task_output(GebrmTask *task,
			     const gchar *output,
			     GebrmJob *job)
{
	if (!*output)
		return;

	g_signal_emit(job, signals[OUTPUT], 0, task, output);
}

gboolean
gebrm_job_is_stopped(GebrmJob *job)
{
	return job->priv->status == JOB_STATUS_FINISHED
		|| job->priv->status == JOB_STATUS_FAILED
		|| job->priv->status == JOB_STATUS_CANCELED;
}

gboolean
gebrm_job_is_queueable(GebrmJob *job)
{
	return job->priv->status == JOB_STATUS_QUEUED
		|| job->priv->status == JOB_STATUS_RUNNING;
}

gboolean
gebrm_job_can_close(GebrmJob *job)
{
	GebrCommJobStatus status = gebrm_job_get_partial_status(job);

	return status == JOB_STATUS_FINISHED
		|| status == JOB_STATUS_FAILED
		|| status == JOB_STATUS_CANCELED;
}

gboolean
gebrm_job_can_kill(GebrmJob *job)
{
	GebrCommJobStatus status = gebrm_job_get_partial_status(job);

	return status == JOB_STATUS_QUEUED
		|| status == JOB_STATUS_RUNNING;
}

static void
gebrm_job_change_task_status(GebrmTask *task,
			     gint old_status,
			     gint new_status,
			     const gchar *parameter,
			     GebrmJob *job)
{
	GList *i;
	GebrCommJobStatus old = job->priv->status;
	gint frac, total, ntasks;

	frac = gebrm_task_get_fraction(task);
	total = job->priv->total;
	ntasks = g_list_length(job->priv->tasks);

	if (gebrm_job_is_stopped(job)) {
		gebrm_task_kill(task);
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
			if (gebrm_task_get_status(i->data) != JOB_STATUS_FINISHED)
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
}

static void
on_task_destroy(GebrmJob *job,
		GebrmTask *finalized_task)
{
	GebrCommJobStatus old_status = job->priv->status;
	job->priv->status = JOB_STATUS_INITIAL;
	job->priv->tasks = g_list_remove(job->priv->tasks, finalized_task);

	if (!job->priv->tasks)
		g_signal_emit(job, signals[DISCONNECT], 0);
	else if (old_status != job->priv->status)
		g_signal_emit(job, signals[STATUS_CHANGE], 0,
			      old_status, job->priv->status, "");
}

/* Public methods {{{1 */
GebrmJob *
gebrm_job_new(void)
{
	static gint id = 0;
	gchar *rid = g_strdup_printf("%d", id++);
	GebrmJob *job = g_object_new(GEBRM_TYPE_JOB, NULL);
	job->priv->info.id = rid;
	return job;
}

void
gebrm_job_init_details(GebrmJob *job, GebrmJobInfo *info)
{
	if (!job->priv->info.id)
		job->priv->info.id = g_strdup(info->id);

	job->priv->info.title = g_strdup(info->title);
	g_debug("THIS IS THE TITLE: %s", job->priv->info.title);
	job->priv->info.hostname = g_strdup(info->hostname);
	job->priv->info.parent_id = g_strdup(info->parent_id);
	job->priv->info.nice = g_strdup(info->nice);
	job->priv->info.input = g_strdup(info->input);
	job->priv->info.output = g_strdup(info->output);
	job->priv->info.error = g_strdup(info->error);
	job->priv->info.submit_date = g_strdup(info->submit_date);
	job->priv->info.group = g_strdup(info->group);
	job->priv->info.speed = g_strdup(info->speed);
}

const gchar *
gebrm_job_get_hostname(GebrmJob *job)
{
	return job->priv->info.hostname;
}

const gchar *
gebrm_job_get_queue(GebrmJob *job)
{
	return job->priv->info.parent_id;
}

const gchar *
gebrm_job_get_title(GebrmJob *job)
{
	return job->priv->info.title;
}

void
gebrm_job_append_task(GebrmJob *job, GebrmTask *task)
{
	job->priv->tasks = g_list_prepend(job->priv->tasks, task);

	gint frac, total;

	total = job->priv->total;
	frac = gebrm_task_get_fraction(task);
	const gchar *issues = gebrm_task_get_issues(task);
	GebrCommJobStatus status = gebrm_task_get_status(task);

	g_signal_connect(task, "status-change", G_CALLBACK(gebrm_job_change_task_status), job);
	g_signal_connect(task, "output", G_CALLBACK(gebrm_job_append_task_output), job);
	g_object_weak_ref(G_OBJECT(task), (GWeakNotify)on_task_destroy, job);

	g_signal_emit(job, signals[CMD_LINE_RECEIVED], 0, task, gebrm_task_get_cmd_line(task));
	g_signal_emit(job, signals[OUTPUT], 0, task, gebrm_task_get_output(task));

	if (strlen(issues))
		gebrm_job_change_task_status(task, job->priv->status,
					     JOB_STATUS_ISSUED, issues, job);

	switch(status)
	{
	case JOB_STATUS_FAILED:
		gebrm_job_change_task_status(task, job->priv->status, status, "", job);
		break;
	case JOB_STATUS_FINISHED:
	case JOB_STATUS_CANCELED:
		gebrm_job_change_task_status(task, job->priv->status, status, gebrm_task_get_finish_date(task), job);
		break;
	case JOB_STATUS_QUEUED:
		gebrm_job_change_task_status(task, job->priv->status, status, gebrm_job_get_queue(job), job);
		break;
	case JOB_STATUS_RUNNING:
		gebrm_job_change_task_status(task, job->priv->status, status, gebrm_task_get_start_date(task), job);
		break;
	default:
		g_warn_if_reached();
		break;
	}
}

GebrCommJobStatus
gebrm_job_get_status(GebrmJob *job)
{
	return job->priv->status;
}

const gchar *
gebrm_job_get_id(GebrmJob *job)
{
	return job->priv->info.id;
}

gchar *
gebrm_job_get_command_line(GebrmJob *job)
{
	GString *buf = g_string_new(NULL);

	for (GList *i = job->priv->tasks; i; i = i->next) {
		gint frac, total;
		GebrmTask *task = i->data;

		total = job->priv->total;
		frac = gebrm_task_get_fraction(task);

		if (total != 1) {
			g_string_append_printf(buf, _("Command line for task %d of %d "), frac, total);
			g_string_append_printf(buf, _(" \(Server: %s)\n"),
					       gebrm_daemon_get_address(gebrm_task_get_daemon(task)));
		}
		g_string_append_printf(buf, "%s\n\n", gebrm_task_get_cmd_line(task));
	}

	return g_string_free(buf, FALSE);
}

gchar *
gebrm_job_get_output(GebrmJob *job)
{
	GString *output = g_string_new(NULL);

	for (GList *i = job->priv->tasks; i; i = i->next)
		g_string_append(output, gebrm_task_get_output(i->data));

	return g_string_free(output, FALSE);
}

const gchar *
gebrm_job_get_submit_date(GebrmJob *job)
{
	return job->priv->info.submit_date;
}

const gchar *
gebrm_job_get_start_date(GebrmJob *job)
{
	const gchar *start_date = NULL;

	for (GList *i = job->priv->tasks; i; i = i->next) {
		GebrmTask *task = i->data;
		GTimeVal new_time, old_time;
		const gchar *start_task_date = gebrm_task_get_start_date(task);

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
gebrm_job_get_finish_date(GebrmJob *job)
{
	const gchar *finish_date = NULL;

	for (GList *i = job->priv->tasks; i; i = i->next) {
		GebrmTask *task = i->data;
		GTimeVal new_time, old_time;
		const gchar *finish_task_date = gebrm_task_get_finish_date(task);

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
gebrm_job_get_issues(GebrmJob *job)
{
	return g_strdup(gebrm_task_get_issues(job->priv->tasks->data));
}

gboolean
gebrm_job_has_issues(GebrmJob *job)
{
	if (!job->priv->tasks)
		return FALSE;

	return gebrm_task_get_issues(job->priv->tasks->data)[0] == '\0' ? FALSE : TRUE;
}

gboolean
gebrm_job_close(GebrmJob *job)
{
	if (!gebrm_job_can_close(job))
		return FALSE;

	for (GList *i = job->priv->tasks; i; i = i->next)
		gebrm_task_close(i->data, job->priv->info.id);

	return TRUE;
}

void
gebrm_job_kill(GebrmJob *job)
{
	if (!gebrm_job_can_kill(job))
		return;

	for (GList *i = job->priv->tasks; i; i = i->next)
		gebrm_task_kill(i->data);
}

void
gebrm_job_set_io(GebrmJob *job,
		 const gchar *input_file,
		 const gchar *output_file,
		 const gchar *log_file)
{
	if (job->priv->info.input)
		g_free(job->priv->info.input);

	if (job->priv->info.output)
		g_free(job->priv->info.output);

	if (job->priv->info.error)
		g_free(job->priv->info.error);

	job->priv->info.input = g_strdup(input_file);
	job->priv->info.output = g_strdup(output_file);
	job->priv->info.error = g_strdup(log_file);
}

void
gebrm_job_get_io(GebrmJob *job,
		 gchar **input_file,
		 gchar **output_file,
		 gchar **log_file)
{
	if (input_file) {
		if (!job->priv->info.input || strlen(job->priv->info.input) == 0)
			*input_file = NULL;
		else
			*input_file = g_strdup(job->priv->info.input);
	}

	if (output_file) {
		if (!job->priv->info.output || strlen(job->priv->info.output) == 0)
			*output_file = NULL;
		else
			*output_file = g_strdup(job->priv->info.output);
	}

	if (log_file) {
		if (!job->priv->info.error || strlen(job->priv->info.error) == 0)
			*log_file = NULL;
		else
			*log_file = g_strdup(job->priv->info.error);
	}
}

const gchar *
gebrm_job_get_nice(GebrmJob *job)
{
	return job->priv->info.nice;
}

gchar *
gebrm_job_get_running_time(GebrmJob *job, const gchar *start_date)
{
	GTimeVal start_time, current_time;
	g_time_val_from_iso8601(start_date, &start_time);
	g_get_current_time(&current_time);

	return gebr_calculate_detailed_relative_time(&start_time, &current_time);
}
gchar *
gebrm_job_get_elapsed_time(GebrmJob *job)
{
	const gchar *start_time = gebrm_job_get_start_date(job);
	const gchar *finish_time = gebrm_job_get_finish_date(job);
	GTimeVal finish, start;
	g_time_val_from_iso8601(finish_time, &finish);
	g_time_val_from_iso8601(start_time, &start);

	return gebr_calculate_detailed_relative_time(&start, &finish);
}

const gchar *
gebrm_job_get_server_group(GebrmJob *job)
{
	return job->priv->info.group ? job->priv->info.group : "";
}

const gchar *
gebrm_job_get_exec_speed(GebrmJob *job)
{
	return job->priv->info.speed;
}

GebrmTask *
gebrm_job_get_task_from_server(GebrmJob *job,
			       const gchar *server)
{
	for (GList *i = job->priv->tasks; i; i = i->next)
		if (g_strcmp0(gebrm_daemon_get_address(gebrm_task_get_daemon(i->data)), server) == 0)
			return i->data;
	return NULL;
}

GList *
gebrm_job_get_list_of_tasks(GebrmJob *job)
{
	return job->priv->tasks;
}

GebrCommJobStatus
gebrm_job_get_partial_status(GebrmJob *job)
{
	if (!job->priv->tasks)
		return JOB_STATUS_INITIAL;

	gint total, ntasks;

	total = job->priv->total;
	ntasks = g_list_length(job->priv->tasks);

	if (ntasks == total)
		return gebrm_job_get_status(job);

	gint running = 0;
	for (GList *i = job->priv->tasks; i; i = i->next) {
		GebrCommJobStatus sta = gebrm_task_get_status(i->data);
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

void
gebrm_job_append_child(GebrmJob *job,
		       GebrCommRunner *runner,
		       GebrmJob *child)
{
	g_return_if_fail(gebrm_job_can_kill(job));

	RunnerAndJob *raj = g_new(RunnerAndJob, 1);
	raj->runner = runner;
	raj->child = child;
	job->priv->children = g_list_prepend(job->priv->children, raj);
}

void
gebrm_job_set_servers_list(GebrmJob *job,
			   const gchar *servers)
{
	if (job->priv->servers)
		g_free(job->priv->servers);
	job->priv->servers = g_strdup(servers);
}

void
gebrm_job_set_nprocs(GebrmJob *job,
		     const gchar *nprocs)
{
	if (job->priv->nprocs)
		g_free(job->priv->nprocs);
	job->priv->nprocs = g_strdup(nprocs);
}

const gchar *
gebrm_job_get_nprocs(GebrmJob *job)
{
	return job->priv->nprocs;
}

const gchar *
gebrm_job_get_servers_list(GebrmJob *job)
{
	return job->priv->servers;
}

void
gebrm_job_set_total_tasks(GebrmJob *job, gint total)
{
	job->priv->total = total;
}
