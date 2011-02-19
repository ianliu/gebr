/*   GeBR - An environment for seismic processing.
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

/**
 * \file job.c
 * Job callbacks
 */

#ifndef __JOB_H
#define __JOB_H

#include <gtk/gtk.h>

#include <libgebr/geoxml/geoxml.h>
#include <libgebr/comm/gebr-comm-job.h>

#include "server.h"

G_BEGIN_DECLS

GType gebr_job_get_type(void);
#define GEBR_JOB_TYPE		(gebr_job_get_type())
#define GEBR_JOB(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_JOB_TYPE, GebrJob))
#define GEBR_JOB_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_JOB_TYPE, GebrJobClass))
#define GEBR_IS_JOB(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_JOB_TYPE))
#define GEBR_IS_JOB_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_JOB_TYPE))
#define GEBR_JOB_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_JOB_TYPE, GebrJobClass))

typedef struct _GebrJob GebrJob;
typedef struct _GebrJobClass GebrJobClass;

struct _GebrJob {
	GebrCommJob parent;
	GtkTreeIter iter;
	struct server *server;

};
struct _GebrJobClass {
	GebrCommJobClass parent;
};

/**
 * Find the job structure for the corresponding \p id and server \p address.
 * If jid is TRUE, find for jid, if FALSE then find using run_id for "incomplete".
 */
GebrJob *job_find(GString * address, GString * id, gboolean jid);

/**
 * Create a new job (from \p server) and add it to list of jobs
 */
GebrJob *job_new_from_flow(struct server *server, GebrGeoXmlFlow * flow, GString *queue);

/**
 * Create a new job (from \p server) and add it to list of jobs
 */
GebrJob *job_new_from_jid(struct server *server, GString * jid, GString * _status, GString * title,
			     GString * start_date, GString * finish_date, GString * hostname, GString * issues,
			     GString * cmd_line, GString * output, GString * queue, GString * moab_jid);

void job_init_details(GebrJob *job, GString * _status, GString * title, GString * start_date, GString * finish_date,
		      GString * hostname, GString * issues, GString * cmd_line, GString * output, GString * queue,
		      GString * moab_jid);
/**
 * Frees job structure.
 */
void job_free(GebrJob *job);

/**
 * Obsolete, same as job_free
 */
void job_delete(GebrJob *job);

/**
 * Return NULL if immediately
 */
const gchar *job_get_queue_name(GebrJob *job);

/**
 * Remove job from the list. 
 */
void job_close(GebrJob *job, gboolean force, gboolean verbose);

/**
 * Select \p job and load it. 
 */
void job_set_active(GebrJob *job);

/**
 */
gboolean job_is_active(GebrJob *job);

/**
 */
void job_append_output(GebrJob *job, GString * output);

/**
 */
void job_update(GebrJob *job);

/**
 */
void job_update_label(GebrJob *job);

/**
 * Translate a \p status protocol string to a status enumeration 
 */
enum JobStatus job_translate_status(GString * status);

/**
 * Set UI related with status.
 * If \p job is not active nothing is done.
 */
void job_status_show(GebrJob *job);

/**
 * Change the status of \p job according to \p status and its \p parameter.
 * Change the GtkTreeIter's icon and handling other status changes. Calls #job_status_show.
 */
void job_status_update(GebrJob *job, enum JobStatus status, const gchar *parameter);

G_END_DECLS
#endif				//__JOB_H
