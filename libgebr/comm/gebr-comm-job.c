/*   libgebr - GeBR Library
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

#include <stdarg.h>

#include <libgebr/utils.h>

#include "gebr-comm-job.h"

static GString *gebr_comm_job_generate_id(void)
{
	gchar *tmp = gebr_id_random_create(16);
	GString *jid = g_string_new(tmp);
	g_free(tmp);

	return jid;
}


/* GOBJECT STUFF */
enum {
	LAST_PROPERTY
};
enum {
	LAST_SIGNAL
};
// static guint object_signals[LAST_SIGNAL];
G_DEFINE_TYPE(GebrCommJob, gebr_comm_job, G_TYPE_OBJECT)
static void gebr_comm_job_init(GebrCommJob * job)
{
	job->hostname = g_string_new(""); //TODO: INITIAL PARAMETER
	job->display = g_string_new(""); //TODO: INITIAL PARAMETER
	job->server_location = GEBR_COMM_SERVER_LOCATION_UNKNOWN; //TODO: INITIAL PARAMETER
	job->run_id = g_string_new(""); //TODO: INITIAL PARAMETER
	job->xml = g_string_new(""); //TODO: INITIAL PARAMETER
	job->queue  = g_string_new(""); //TODO: INITIAL PARAMETER
	job->moab_account = g_string_new(""); //TODO: INITIAL PARAMETER
	job->n_process = g_string_new(""); //TODO: INITIAL PARAMETER
	job->jid = gebr_comm_job_generate_id();
	job->title = g_string_new("");
	job->start_date = g_string_new("");
	job->finish_date = g_string_new("");
	job->issues = g_string_new("");
	job->cmd_line = g_string_new("");
	job->output = g_string_new("");
	job->moab_jid = g_string_new("");
	job->status = JOB_STATUS_INITIAL;
}
static void gebr_comm_job_finalize(GObject * object)
{
	GebrCommJob *job = GEBR_COMM_JOB(object);
	g_string_free(job->hostname, TRUE);
	g_string_free(job->display, TRUE);
	g_string_free(job->run_id, TRUE);
	g_string_free(job->xml, TRUE);
	g_string_free(job->queue, TRUE);
	g_string_free(job->moab_account, TRUE);
	g_string_free(job->n_process, TRUE);
	g_string_free(job->jid, TRUE);
	g_string_free(job->title, TRUE);
	g_string_free(job->start_date, TRUE);
	g_string_free(job->finish_date, TRUE);
	g_string_free(job->issues, TRUE);
	g_string_free(job->cmd_line, TRUE);
	g_string_free(job->output, TRUE);
	g_string_free(job->moab_jid, TRUE);
	G_OBJECT_CLASS(gebr_comm_job_parent_class)->finalize(object);
}
static void gebr_comm_job_class_init(GebrCommJobClass * klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize = gebr_comm_job_finalize;
//	GebrCommJobClass *super_class = GEBR_COMM_JOB_GET_CLASS(klass);
}


GebrCommJob * gebr_comm_job_new(void)
{
	return GEBR_COMM_JOB(g_object_new(GEBR_COMM_JOB_TYPE, NULL));
}


