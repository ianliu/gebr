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
#include <libgebr/comm/gebr-comm-process.h>
#include <libgebr/comm/gebr-comm-job.h>

#include "gebrd-client.h"

G_BEGIN_DECLS

GType gebrd_job_get_type(void);
#define GEBRD_JOB_TYPE		(gebrd_job_get_type())
#define GEBRD_JOB(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBRD_JOB_TYPE, GebrdJob))
#define GEBRD_JOB_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEBRD_JOB_TYPE, GebrdJobClass))
#define GEBRD_IS_JOB(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBRD_JOB_TYPE))
#define GEBRD_IS_JOB_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBRD_JOB_TYPE))
#define GEBRD_JOB_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBRD_JOB_TYPE, GebrdJobClass))

typedef struct _GebrdJob GebrdJob;
typedef struct _GebrdJobClass GebrdJobClass;

struct _GebrdJob {
	GebrCommJob parent;

	GebrCommProcess *process;
	GebrGeoXmlFlow *flow;
	gboolean critical_error; /* the flow can't be run if TRUE! */
	gboolean user_finished;

	/* client stuff */
	GebrCommProcess *tail_process;
};
struct _GebrdJobClass {
	GebrCommJobClass parent;
};

/**
 */
GebrdJob *job_find(GString * jid);

/**
 */
void job_new(GebrdJob ** _job, struct client * client, GString * queue, GString * account, GString * xml,
	     GString * n_process, GString * run_id);
/**
 */
void job_free(GebrdJob *job);

/**
 */
void job_status_set(GebrdJob *job, enum JobStatus status);
/**
 * Change status and notify clients about it
 */
void job_status_notify(GebrdJob *job, enum JobStatus status, const gchar *parameter, ...);
/**
 * Remember not to send any message to clients here as the job wasn't created
 */
void job_run_flow(GebrdJob *job);

/**
 */
void job_clear(GebrdJob *job);
/**
 */
void job_end(GebrdJob *job);
/**
 */
void job_kill(GebrdJob *job);

/**
 */
void job_notify(GebrdJob *job, struct client *client);
/**
 */
void job_list(struct client *client);
/**
 */
void job_send_clients_job_notify(GebrdJob *job);

G_END_DECLS
#endif				//__JOB_H
