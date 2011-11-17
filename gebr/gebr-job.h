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

#ifndef __GEBR_JOB_H__
#define __GEBR_JOB_H__

#include <libgebr/comm/gebr-comm.h>

#include "gebr-task.h"

#define GEBR_TYPE_JOB			(gebr_job_get_type())
#define GEBR_JOB(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_TYPE_JOB, GebrJob))
#define GEBR_JOB_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_TYPE_JOB, GebrJobClass))
#define GEBR_IS_JOB(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_TYPE_JOB))
#define GEBR_IS_JOB_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_TYPE_JOB))
#define GEBR_JOB_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_TYPE_JOB, GebrJobClass))

G_BEGIN_DECLS

typedef struct _GebrJob GebrJob;
typedef struct _GebrJobPriv GebrJobPriv;
typedef struct _GebrJobClass GebrJobClass;

struct _GebrJob {
	GObject parent;
	GebrJobPriv *priv;
};

struct _GebrJobClass {
	GObjectClass parent_class;

	void (*status_change) (GebrJob *job,
			       gint old_status,
			       gint new_status,
			       const gchar *parameter,
			       gpointer user_data);

	void (*issued) (GebrJob     *job,
			const gchar *issues);

	void (*cmd_line_received) (GebrJob *job);

	void (*output) (GebrJob     *job,
			GebrTask    *task,
			const gchar *output);

	void (*disconnect) (GebrJob *job);
};

GType gebr_job_get_type() G_GNUC_CONST;

GebrJob *gebr_job_new_with_id(const gchar *rid,
			      const gchar *queue,
			      const gchar *servers);

void gebr_job_set_hostname(GebrJob *job,
			   const gchar *hostname);

const gchar *gebr_job_get_hostname(GebrJob *job);

void gebr_job_show(GebrJob *job);

gboolean gebr_job_is_stopped(GebrJob *job);

gboolean gebr_job_is_queueable(GebrJob *job);

gboolean gebr_job_can_close(GebrJob *job);

gboolean gebr_job_can_kill(GebrJob *job);

GtkTreeIter *gebr_job_get_iter(GebrJob *job);

gchar **gebr_job_get_servers(GebrJob *job, gint *n);

void gebr_job_append_task(GebrJob *job, GebrTask *task);

GebrJob *gebr_job_find(const gchar *rid);

const gchar *gebr_job_get_title(GebrJob *job);

void gebr_job_set_title(GebrJob *job, const gchar *title);

const gchar *gebr_job_get_queue(GebrJob *job);

enum JobStatus gebr_job_get_status(GebrJob *job);

const gchar *gebr_job_get_id(GebrJob *job);

gchar *gebr_job_get_command_line(GebrJob *job);

gchar *gebr_job_get_output(GebrJob *job);

const gchar *gebr_job_get_last_run_date(GebrJob *job);

const gchar *gebr_job_get_start_date(GebrJob *job);

const gchar *gebr_job_get_finish_date(GebrJob *job);

gchar *gebr_job_get_issues(GebrJob *job);

gboolean gebr_job_has_issues(GebrJob *job);

gboolean gebr_job_close(GebrJob *job);

void gebr_job_kill(GebrJob *job);

void gebr_job_set_model(GebrJob *job,
                        GtkTreeModel *model);

void gebr_job_set_io(GebrJob *job,
		     const gchar *input_file,
		     const gchar *output_file,
		     const gchar *log_file);

void gebr_job_get_io(GebrJob *job, gchar **input_file, gchar **output_file,
		gchar **log_file);

void gebr_job_get_resources(GebrJob *job,
                            gchar **nprocs,
                            gchar **niceness);

gchar *gebr_job_get_remaining_servers(GebrJob *job);

const gchar *gebr_job_get_server_group(GebrJob *job);

void gebr_job_set_server_group(GebrJob *job, const gchar *server_group);

gchar *gebr_job_get_running_time(GebrJob *job, const gchar *start_date);

gchar *gebr_job_get_elapsed_time(GebrJob *job);

gint gebr_job_get_exec_speed(GebrJob *job);

void gebr_job_set_exec_speed(GebrJob *job, gint exec_speed);

GList *gebr_job_get_list_of_tasks(GebrJob *job);

GebrTask *gebr_job_get_task_from_server(GebrJob *job,
					const gchar *server);

/**
 * gebr_job_get_partial_status:
 *
 * If a job does not have all of its tasks appended, it have a "partial"
 * status, which is the composition of the statuses of the tasks which where
 * appended.
 *
 * For instance, if a job is composed by 4 tasks but only 3 were appended, then
 * the partial status of this job is the status as if the job were composed by
 * the 3 appended tasks only.
 *
 * If the job is complete, ie all 4 tasks were appended, than
 * gebr_job_get_partial_status() returns the same value as
 * gebr_job_get_status().
 */
enum JobStatus gebr_job_get_partial_status(GebrJob *job);

G_END_DECLS

#endif /* __GEBR_JOB_H__ */
