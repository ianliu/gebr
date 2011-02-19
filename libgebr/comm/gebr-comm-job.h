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

#ifndef __GEBR_COMM_JOB_H
#define __GEBR_COMM_JOB_H

#include <glib.h>
#include <glib-object.h>

#include <libgebr/comm/gebr-comm-server.h>

G_BEGIN_DECLS

GType gebr_comm_job_get_type(void);
#define GEBR_COMM_JOB_TYPE		(gebr_comm_job_get_type())
#define GEBR_COMM_JOB(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_COMM_JOB_TYPE, GebrCommJob))
#define GEBR_COMM_JOB_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_COMM_JOB_TYPE, GebrCommJobClass))
#define GEBR_COMM_IS_JOB(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_COMM_JOB_TYPE))
#define GEBR_COMM_IS_JOB_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_COMM_JOB_TYPE))
#define GEBR_COMM_JOB_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_COMM_JOB_TYPE, GebrCommJobClass))

typedef struct _GebrCommJob GebrCommJob;
typedef struct _GebrCommJobClass GebrCommJobClass;

struct _GebrCommJob {
	GObject parent;

	enum JobStatus {
		JOB_STATUS_INITIAL = 0, /* before the sending of jid to clients */
		JOB_STATUS_QUEUED,
		JOB_STATUS_FAILED,
		JOB_STATUS_RUNNING,
		JOB_STATUS_FINISHED,
		JOB_STATUS_CANCELED,
		JOB_STATUS_REQUEUED, /* false status */
		JOB_STATUS_ISSUED, /* false status */
	} status;

	/* sent by the client */
	GString *client_hostname; // the hostname of the client that ran it
	GString *client_display;
	GebrCommServerLocation server_location;
	GString *run_id;
	GString *flow_xml;
	GString *moab_account;
	GString *n_process;
	GString *queue_id;

	GString *jid;
	GString *title;
	/* appears in top label */
	GString *start_date;
	GString *finish_date;
	/* appears in text view */
	GString *issues;
	GString *cmd_line;
	GString *output;
	
	/* Moab stuff */
	GString *moab_jid;
};

struct _GebrCommJobClass {
	GObjectClass parent;
};

GebrCommJob * gebr_comm_job_new(void);

G_END_DECLS
#endif				//__GEBR_COMM_JOB_H
