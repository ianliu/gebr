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

typedef struct {
	GebrCommRunner *runner;
	GebrJob *child;
} RunnerAndJob;

struct _GebrJobPriv {
	gchar *runid;
	GebrCommJobStatus status;

	gint exec_speed;
	gchar *title;
	gchar *queue;
	gchar *hostname;
	gchar *server_group;
	gchar *server_group_type;
	gchar *input_file;
	gchar *output_file;
	gchar *log_file;
	gchar *submit_date;
	gchar *start_date;
	gchar *finish_date;
	gchar *issues;
	gchar *nice;
	gchar *nprocs;

	/* Interface properties */
	GtkTreeIter iter;
	GtkTreeModel *model;

	/* Task info */
	GebrJobTask *tasks;
	gint n_servers;
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
	g_free(job->priv->input_file);
	g_free(job->priv->output_file);
	g_free(job->priv->log_file);

	G_OBJECT_CLASS(gebr_job_parent_class)->finalize(object);
}

static void
gebr_job_status_change_real(GebrJob *job,
			    gint old_status,
			    gint new_status,
			    const gchar *parameter,
			    gpointer user_data)
{
	if (job->priv->model) {
		GtkTreePath *path = gtk_tree_model_get_path(job->priv->model, &job->priv->iter);
		gtk_tree_model_row_changed(job->priv->model, path, &job->priv->iter);
		gtk_tree_path_free(path);
	}
}

static void
gebr_job_init(GebrJob *job)
{
	job->priv = G_TYPE_INSTANCE_GET_PRIVATE(job,
						GEBR_TYPE_JOB,
						GebrJobPriv);
	job->priv->status = JOB_STATUS_INITIAL;
}

static void
gebr_job_class_init(GebrJobClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize = gebr_job_finalize;

	klass->status_change = gebr_job_status_change_real;

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
			     gebr_cclosure_marshal_VOID__INT_STRING,
			     G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_STRING);

	signals[OUTPUT] =
		g_signal_new("output",
			     G_OBJECT_CLASS_TYPE(gobject_class),
			     G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			     G_STRUCT_OFFSET(GebrJobClass, output),
			     NULL, NULL,
			     gebr_cclosure_marshal_VOID__INT_STRING,
			     G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_STRING);

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
	return job->priv->status == JOB_STATUS_FINISHED
		|| job->priv->status == JOB_STATUS_FAILED
		|| job->priv->status == JOB_STATUS_CANCELED;
}

gboolean
gebr_job_can_kill(GebrJob *job)
{
	return job->priv->status == JOB_STATUS_QUEUED
		|| job->priv->status == JOB_STATUS_RUNNING;
}

/* Public methods {{{1 */
GebrJob *
gebr_job_new(const gchar *queue)
{
	static gint id = 0;

	gchar *rid = g_strdup_printf("%d:%s", id++, gebr_get_session_id());
	GebrJob *job = gebr_job_new_with_id(rid, queue);
	return job;
}

GebrJob *
gebr_job_new_with_id(const gchar *rid,
		     const gchar *queue)
{
	GebrJob *job = g_object_new(GEBR_TYPE_JOB, NULL);

	job->priv->queue = g_strdup(queue);
	job->priv->runid = g_strdup(rid);
	job->priv->n_servers = 0;

	return job;
}

void
gebr_job_set_servers(GebrJob *job,
		     const gchar *servers)
{
	gchar **split = g_strsplit(servers, ",", 0);

	if (!split)
		return;

	while (split[job->priv->n_servers])
		job->priv->n_servers++;
	job->priv->n_servers /= 2;

	job->priv->tasks = g_new0(GebrJobTask, job->priv->n_servers);

	for (int i = 0; i < job->priv->n_servers; i++) {

		job->priv->tasks[i].server = split[i*2];
		job->priv->tasks[i].percentage = g_strtod(split[i*2 + 1], NULL);
		job->priv->tasks[i].cmd_line = NULL;
		job->priv->tasks[i].frac = i+1;
		job->priv->tasks[i].output = g_string_new("");
	}
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

GebrCommJobStatus
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
	g_return_val_if_fail(n!= NULL, NULL);
	GebrJobTask *tasks = gebr_job_get_tasks(job, n);
	gchar **servers = g_new0(gchar*, *n+1);
	for (int i=0; i < *n; i++){
		g_debug("getting server '%s'", tasks[i].server);
		servers[i] = g_strdup(tasks[i].server);
	}
	servers[*n] = NULL;
	return servers;
}

GebrJobTask *
gebr_job_get_tasks(GebrJob *job, gint *n)
{
	if (n)
		*n = job->priv->n_servers;
	return job->priv->tasks;
}

gdouble *
gebr_job_get_percentages(GebrJob *job, gint *n)
{
	g_return_val_if_fail(n!= NULL, NULL);
	GebrJobTask *tasks = gebr_job_get_tasks(job, n);
	gdouble *percs = g_new(gdouble, *n);
	for (int i=0; i < *n; i++){
		percs[i] = tasks[i].percentage;
	}
	return percs;
}

gchar *
gebr_job_get_command_line(GebrJob *job)
{
	g_warning("TODO: Implement %s", __func__);
	return g_strdup("");
}

gchar *
gebr_job_get_output(GebrJob *job)
{
	GString *output = g_string_new("");
	for (int i = 0; i < job->priv->n_servers; i++)
		g_string_append(output, job->priv->tasks[i].output->str);
	return g_string_free(output, FALSE);
}

const gchar *
gebr_job_get_last_run_date(GebrJob *job)
{
	return job->priv->submit_date;
}

const gchar *
gebr_job_get_start_date(GebrJob *job)
{
	return job->priv->start_date;
}

const gchar *
gebr_job_get_finish_date(GebrJob *job)
{
	return job->priv->finish_date;
}

gchar *
gebr_job_get_issues(GebrJob *job)
{
	return job->priv->issues;
}

gboolean
gebr_job_has_issues(GebrJob *job)
{
	if (!job->priv->issues)
		return FALSE;
	return job->priv->issues[0] == '\0' ? FALSE : TRUE;
}

gboolean
gebr_job_close(GebrJob *job)
{
	if (!gebr_job_can_close(job))
		return FALSE;

	gchar *url = g_strdup_printf("/close?id=%s", gebr_job_get_id(job));

	GList *m = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller,
								gebr.line);
	GebrCommServer *server = gebr_maestro_server_get_server(m->data);
	gebr_comm_protocol_socket_send_request(server->socket,
	                                       GEBR_COMM_HTTP_METHOD_PUT, url, NULL);
	return TRUE;
}

void
gebr_job_kill(GebrJob *job)
{
	if (!gebr_job_can_kill(job))
		return;

	gchar *url = g_strdup_printf("/kill?id=%s", gebr_job_get_id(job));

	GList *m = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller,
								gebr.line);
	GebrCommServer *server = gebr_maestro_server_get_server(m->data);
	gebr_comm_protocol_socket_send_request(server->socket,
	                                       GEBR_COMM_HTTP_METHOD_PUT, url, NULL);
}

void
gebr_job_set_runid (GebrJob *job,
		    gchar *id)
{
	if (job->priv->runid)
		g_free(job->priv->runid);
	job->priv->runid = g_strdup(id);
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
                       const gchar **nprocs,
                       const gchar **nice)
{
	if (nprocs)
		*nprocs = job->priv->nprocs;
	if (nice)
		*nice = job->priv->nice;
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

void
gebr_job_set_server_group_type(GebrJob *job, const gchar *group_type){
	job->priv->server_group_type = g_strdup(group_type);
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

void
gebr_job_set_static_status(GebrJob *job, GebrCommJobStatus status)
{
	job->priv->status = status;
}

void
gebr_job_set_status(GebrJob *job, GebrCommJobStatus status, const gchar *parameter)
{
	GebrCommJobStatus old = job->priv->status;
	job->priv->status = status;

	switch(status) {
	case JOB_STATUS_RUNNING:
		gebr_job_set_start_date(job, parameter);
		break;
	case JOB_STATUS_CANCELED:
	case JOB_STATUS_FAILED:
	case JOB_STATUS_FINISHED:
		gebr_job_set_finish_date(job, parameter);
		break;
	default:
		break;
	}

	g_signal_emit(job, signals[STATUS_CHANGE], 0, old, status, parameter);
}

void
gebr_job_set_start_date(GebrJob *job, const gchar *start_date)
{
	if (job->priv->start_date)
		g_free(job->priv->start_date);
	job->priv->start_date = g_strdup(start_date);
}

void
gebr_job_set_finish_date(GebrJob *job, const gchar *finish_date)
{
	if (job->priv->finish_date)
		g_free(job->priv->finish_date);
	job->priv->finish_date = g_strdup(finish_date);
}

void
gebr_job_set_nprocs(GebrJob *job,
		    const gchar *nprocs)
{
	if (job->priv->nprocs)
		g_free(job->priv->nprocs);
	job->priv->nprocs = g_strdup(nprocs);
}

void
gebr_job_set_queue(GebrJob *job,
		    const gchar *queue)
{
	if (job->priv->queue)
		g_free(job->priv->queue);
	job->priv->queue= g_strdup(queue);
}
void
gebr_job_set_nice(GebrJob *job,
		  const gchar *nice)
{
	if (job->priv->nice)
		g_free(job->priv->nice);
	job->priv->nice = g_strdup(nice);
}

void
gebr_job_set_submit_date(GebrJob *job,
			 const gchar *submit_date)
{
	if (job->priv->submit_date)
		g_free(job->priv->submit_date);
	job->priv->submit_date = g_strdup(submit_date);
}

void
gebr_job_set_cmd_line(GebrJob *job, gint frac, const gchar *cmd_line)
{
	if (job->priv->tasks[frac].cmd_line)
		g_free(job->priv->tasks[frac].cmd_line);
	job->priv->tasks[frac].cmd_line = g_strdup(cmd_line);
	g_signal_emit(job, signals[CMD_LINE_RECEIVED], 0, frac, cmd_line);
}

void
gebr_job_set_issues(GebrJob *job, const gchar *issues)
{
	if (job->priv->issues)
		g_free(job->priv->issues);
	job->priv->issues = g_strdup(issues);
	g_signal_emit(job, signals[ISSUED], 0, issues);
}

void
gebr_job_append_output(GebrJob *job, gint frac,
		       const gchar *output)
{
	g_string_append(job->priv->tasks[frac].output, output);
	g_signal_emit(job, signals[OUTPUT], 0, frac, output);
}
