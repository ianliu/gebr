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

#ifndef __GEBRM_JOB_H__
#define __GEBRM_JOB_H__

#include <libgebr/comm/gebr-comm.h>

#include "gebrm-task.h"

#define GEBRM_TYPE_JOB			(gebrm_job_get_type())
#define GEBRM_JOB(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBRM_TYPE_JOB, GebrmJob))
#define GEBRM_JOB_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GEBRM_TYPE_JOB, GebrmJobClass))
#define GEBRM_IS_JOB(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBRM_TYPE_JOB))
#define GEBRM_IS_JOB_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBRM_TYPE_JOB))
#define GEBRM_JOB_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBRM_TYPE_JOB, GebrmJobClass))

G_BEGIN_DECLS

typedef struct _GebrmJob GebrmJob;
typedef struct _GebrmJobPriv GebrmJobPriv;
typedef struct _GebrmJobClass GebrmJobClass;

struct _GebrmJob {
	GObject parent;
	GebrmJobPriv *priv;
};

struct _GebrmJobClass {
	GObjectClass parent_class;

	void (*status_change) (GebrmJob *job,
			       gint old_status,
			       gint new_status,
			       const gchar *parameter,
			       gpointer user_data);

	void (*issued) (GebrmJob     *job,
			const gchar *issues);

	void (*cmd_line_received) (GebrmJob    *job,
				   GebrmTask   *task,
				   const gchar *cmd);

	void (*output) (GebrmJob     *job,
			GebrmTask    *task,
			const gchar *output);

	void (*disconnect) (GebrmJob *job);
};

typedef struct {
	gchar *id;
	gchar *temp_id;
	gchar *title;
	gchar *hostname;
	gchar *parent_id;
	gchar *servers;
	gchar *nice;
	gchar *input;
	gchar *output;
	gchar *error;
	gchar *submit_date;
	gchar *group;
	gchar *group_type;
	gchar *speed;
} GebrmJobInfo;

void gebrm_job_info_free(GebrmJobInfo *info);

GType gebrm_job_get_type() G_GNUC_CONST;

/**
 * gebrm_job_new:
 *
 * Creates a new job with a unique id.
 */
GebrmJob *gebrm_job_new(void);

void gebrm_job_init_details(GebrmJob *job,
			    GebrmJobInfo *info);

void gebrm_job_set_hostname(GebrmJob *job,
			    const gchar *hostname);

const gchar *gebrm_job_get_hostname(GebrmJob *job);

void gebrm_job_show(GebrmJob *job);

gboolean gebrm_job_is_stopped(GebrmJob *job);

gboolean gebrm_job_is_queueable(GebrmJob *job);

gboolean gebrm_job_can_close(GebrmJob *job);

gboolean gebrm_job_can_kill(GebrmJob *job);

void gebrm_job_append_task(GebrmJob *job, GebrmTask *task);

GebrmJob *gebrm_job_find(const gchar *rid);

const gchar *gebrm_job_get_title(GebrmJob *job);

void gebrm_job_set_title(GebrmJob *job, const gchar *title);

const gchar *gebrm_job_get_queue(GebrmJob *job);

GebrCommJobStatus gebrm_job_get_status(GebrmJob *job);

const gchar *gebrm_job_get_id(GebrmJob *job);

gchar *gebrm_job_get_command_line(GebrmJob *job);

gchar *gebrm_job_get_output(GebrmJob *job);

const gchar *gebrm_job_get_submit_date(GebrmJob *job);

const gchar *gebrm_job_get_last_run_date(GebrmJob *job);

const gchar *gebrm_job_get_start_date(GebrmJob *job);

const gchar *gebrm_job_get_finish_date(GebrmJob *job);

gchar *gebrm_job_get_issues(GebrmJob *job);

gboolean gebrm_job_has_issues(GebrmJob *job);

gboolean gebrm_job_close(GebrmJob *job);

void gebrm_job_kill(GebrmJob *job);

void gebrm_job_set_io(GebrmJob *job,
		      const gchar *input_file,
		      const gchar *output_file,
		      const gchar *log_file);

void gebrm_job_get_io(GebrmJob *job, gchar **input_file, gchar **output_file,
		      gchar **log_file);

const gchar *gebrm_job_get_nice(GebrmJob *job);

const gchar *gebrm_job_get_server_group(GebrmJob *job);

const gchar *gebrm_job_get_server_group_type(GebrmJob *job);

void gebrm_job_set_server_group(GebrmJob *job, const gchar *server_group);

void gebrm_job_set_server_group_type(GebrmJob *job, const gchar *server_group_type);

gchar *gebrm_job_get_running_time(GebrmJob *job, const gchar *start_date);

gchar *gebrm_job_get_elapsed_time(GebrmJob *job);

const gchar *gebrm_job_get_exec_speed(GebrmJob *job);

void gebrm_job_set_exec_speed(GebrmJob *job, gint exec_speed);

GList *gebrm_job_get_list_of_tasks(GebrmJob *job);

GebrmTask *gebrm_job_get_task_from_server(GebrmJob *job,
					  const gchar *server);

/**
 * gebrm_job_get_partial_status:
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
 * gebrm_job_get_partial_status() returns the same value as
 * gebrm_job_get_status().
 */
GebrCommJobStatus gebrm_job_get_partial_status(GebrmJob *job);

/**
 * gebrm_job_append_child:
 *
 * Appends the pair (@runner, @child) into @job's children list. This list will
 * be executed when @job finishes or gets canceled/failed.
 */
void gebrm_job_append_child(GebrmJob *job,
			    GebrCommRunner *runner,
			    GebrmJob *child);

void gebrm_job_set_servers_list(GebrmJob *job,
				const gchar *servers);

void gebrm_job_set_nprocs(GebrmJob *job,
			  const gchar *nprocs);

const gchar *gebrm_job_get_nprocs(GebrmJob *job);

const gchar *gebrm_job_get_servers_list(GebrmJob *job);

void gebrm_job_set_total_tasks(GebrmJob *job, gint total);

const gchar * gebrm_job_get_temp_id(GebrmJob *job);

const gchar *gebrm_job_get_server_group(GebrmJob *job);

const gchar *gebrm_job_get_server_group_type(GebrmJob *job);

G_END_DECLS

#endif /* __GEBRM_JOB_H__ */
