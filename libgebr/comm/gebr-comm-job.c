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
	PROP_0,
	CLIENT_HOSTNAME, CLIENT_DISPLAY,
	RUN_ID, FLOW_XML, MOAB_ACCOUNT, N_PROCESS, QUEUE_ID,
	JID, TITLE, START_DATE, FINISH_DATE, ISSUES, CMD_LINE, OUTPUT, MOAB_JID,
	LAST_PROPERTY
};
static guint property_member_offset [] = {0,
	G_STRUCT_OFFSET(GebrCommJob, client_hostname), G_STRUCT_OFFSET(GebrCommJob, client_display),
	G_STRUCT_OFFSET(GebrCommJob, run_id), G_STRUCT_OFFSET(GebrCommJob, flow_xml),
	G_STRUCT_OFFSET(GebrCommJob, moab_account), G_STRUCT_OFFSET(GebrCommJob, n_process),
	G_STRUCT_OFFSET(GebrCommJob, queue_id),
       	G_STRUCT_OFFSET(GebrCommJob, jid), G_STRUCT_OFFSET(GebrCommJob, title),
	G_STRUCT_OFFSET(GebrCommJob, start_date), G_STRUCT_OFFSET(GebrCommJob, finish_date),
	G_STRUCT_OFFSET(GebrCommJob, issues), G_STRUCT_OFFSET(GebrCommJob, cmd_line),
	G_STRUCT_OFFSET(GebrCommJob, output), G_STRUCT_OFFSET(GebrCommJob, moab_jid),
};
enum {
	LAST_SIGNAL
};
// static guint object_signals[LAST_SIGNAL];
G_DEFINE_TYPE(GebrCommJob, gebr_comm_job, G_TYPE_OBJECT)
static void gebr_comm_job_init(GebrCommJob * job)
{
	job->client_hostname = g_string_new(""); //TODO: INITIAL PARAMETER
	job->client_display = g_string_new(""); //TODO: INITIAL PARAMETER
	job->server_location = GEBR_COMM_SERVER_LOCATION_UNKNOWN; //TODO: INITIAL PARAMETER
	job->run_id = g_string_new(""); //TODO: INITIAL PARAMETER
	job->flow_xml = g_string_new(""); //TODO: INITIAL PARAMETER
	job->queue_id = g_string_new(""); //TODO: INITIAL PARAMETER
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
	g_string_free(job->client_hostname, TRUE);
	g_string_free(job->client_display, TRUE);
	g_string_free(job->run_id, TRUE);
	g_string_free(job->flow_xml, TRUE);
	g_string_free(job->queue_id, TRUE);
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
static void
gebr_comm_job_set_property(GObject * object, guint property_id, const GValue * value, GParamSpec * pspec)
{
	GebrCommJob *self = (GebrCommJob *) object;
	switch (property_id) {
	case CLIENT_HOSTNAME: case CLIENT_DISPLAY: case RUN_ID: case FLOW_XML: case MOAB_ACCOUNT:
	case N_PROCESS: case QUEUE_ID: case JID: case TITLE: case START_DATE: case FINISH_DATE: case ISSUES:
       	case CMD_LINE: case OUTPUT: case MOAB_JID:
		G_STRUCT_MEMBER(GString *, self, property_member_offset[property_id]) = g_value_get_boxed(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}
static void
gebr_comm_job_get_property(GObject * object, guint property_id, GValue * value, GParamSpec * pspec)
{
	GebrCommJob *self = (GebrCommJob *) object;
	switch (property_id) {
	case CLIENT_HOSTNAME: case CLIENT_DISPLAY: case RUN_ID: case FLOW_XML: case MOAB_ACCOUNT:
	case N_PROCESS: case QUEUE_ID: case JID: case TITLE: case START_DATE: case FINISH_DATE: case ISSUES:
       	case CMD_LINE: case OUTPUT: case MOAB_JID:
		g_value_set_boxed(value, G_STRUCT_MEMBER(GString *, self, property_member_offset[property_id]));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}
static void gebr_comm_job_class_init(GebrCommJobClass * klass)
{ 
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize = gebr_comm_job_finalize;
//	GebrCommJobClass *super_class = GEBR_COMM_JOB_GET_CLASS(klass);
	
	/* properties */
	GParamSpec *pspec;
	gobject_class->set_property = gebr_comm_job_set_property;
	gobject_class->get_property = gebr_comm_job_get_property;
	pspec = g_param_spec_boxed("client-hostname", "", "", G_TYPE_GSTRING, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, CLIENT_HOSTNAME, pspec);
	pspec = g_param_spec_boxed("client-display", "", "", G_TYPE_GSTRING, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, CLIENT_DISPLAY, pspec);
	pspec = g_param_spec_boxed("run-id", "", "", G_TYPE_GSTRING, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, RUN_ID, pspec);
	pspec = g_param_spec_boxed("flow-xml", "", "", G_TYPE_GSTRING, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, FLOW_XML, pspec);
	pspec = g_param_spec_boxed("n_process", "", "", G_TYPE_GSTRING, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, MOAB_ACCOUNT, pspec);
	pspec = g_param_spec_boxed("queue-id", "", "", G_TYPE_GSTRING, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, N_PROCESS, pspec);
	pspec = g_param_spec_boxed("moab-account", "", "", G_TYPE_GSTRING, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, QUEUE_ID, pspec);
	pspec = g_param_spec_boxed("jid", "", "", G_TYPE_GSTRING, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, JID, pspec);
	pspec = g_param_spec_boxed("title", "", "", G_TYPE_GSTRING, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, TITLE, pspec);
	pspec = g_param_spec_boxed("start-date", "", "", G_TYPE_GSTRING, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, START_DATE, pspec);
	pspec = g_param_spec_boxed("finish-date", "", "", G_TYPE_GSTRING, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, FINISH_DATE, pspec);
	pspec = g_param_spec_boxed("issues", "", "", G_TYPE_GSTRING, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, ISSUES, pspec);
	pspec = g_param_spec_boxed("cmd-line", "", "", G_TYPE_GSTRING, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, CMD_LINE, pspec);
	pspec = g_param_spec_boxed("output", "", "", G_TYPE_GSTRING, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, OUTPUT, pspec);
	pspec = g_param_spec_boxed("moab-jid", "", "", G_TYPE_GSTRING, G_PARAM_READWRITE);
	g_object_class_install_property(gobject_class, MOAB_JID, pspec);
}

GebrCommJob * gebr_comm_job_new(void)
{
	return GEBR_COMM_JOB(g_object_new(GEBR_COMM_JOB_TYPE, NULL));
}


