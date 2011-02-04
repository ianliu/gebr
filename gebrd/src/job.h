/*   GeBR Daemon - Process and control execution of flows
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

#ifndef __JOB_H
#define __JOB_H

#include <libgebr/geoxml/geoxml.h>
#include <libgebr/comm/process.h>

#include "client.h"

G_BEGIN_DECLS

/**
 */
struct job {
	GebrCommProcess *process;
	GebrGeoXmlFlow *flow;
	gboolean critical_error; /* the flow can't be run if TRUE! */
	gboolean user_finished;

	/* client stuff */
	GString *hostname; // the hostname of the client that ran it
	GString *display;
	GebrCommServerLocation server_location;

	GString *run_id;
	GString *jid;
	GString *title;
	GString *start_date;
	GString *finish_date;
	GString *issues;
	GString *cmd_line;
	GString *output;
	GString *queue;

	/* Moab stuff */
	GString *moab_account;
	GString *moab_jid;
	GebrCommProcess *tail_process;

	/* MPI stuff */
	GString *n_process;
	
	/* new status should reflect on status_enum_to_string */
	enum JobStatus {
		JOB_STATUS_UNKNOWN = 0,
		JOB_STATUS_QUEUED,
		JOB_STATUS_FAILED,
		JOB_STATUS_RUNNING,
		JOB_STATUS_FINISHED,
		JOB_STATUS_CANCELED,
		JOB_STATUS_REQUEUED, /* false status */
		JOB_STATUS_ISSUED, /* false status */
	} status;
	GString * status_string;
};

/**
 */
struct job *job_find(GString * jid);
/**
 */
void job_new(struct job ** _job, struct client * client, GString * queue, GString * account, GString * xml,
	     GString * n_process, GString * run_id);
/**
 */
void job_free(struct job *job);

/**
 */
void job_set_status(struct job *job, enum JobStatus status);
/**
 * Change status and notify clients about it
 */
void job_notify_status(struct job *job, enum JobStatus status, const gchar *parameter);
/**
 * Remember not to send any message to clients here as the job wasn't created
 */
void job_run_flow(struct job *job);

/**
 */
void job_clear(struct job *job);
/**
 */
void job_end(struct job *job);
/**
 */
void job_kill(struct job *job);

/**
 */
void job_notify(struct job *job, struct client *client);
/**
 */
void job_list(struct client *client);
/**
 */
void job_send_clients_job_notify(struct job *job);

G_END_DECLS
#endif				//__JOB_H
