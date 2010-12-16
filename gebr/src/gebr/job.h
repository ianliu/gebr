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
#include <libgebr/geoxml.h>

#include "server.h"

G_BEGIN_DECLS

/**
 * Find the job structure for the corresponding \p jid and server \p address.
 */
struct job *job_find(GString * address, GString * jid);


struct job {
	GtkTreeIter iter;
	struct server *server;

	enum JobStatus {
		JOB_STATUS_UNKNOWN = 0,
		JOB_STATUS_QUEUED,
		JOB_STATUS_FAILED,
		JOB_STATUS_RUNNING,
		JOB_STATUS_FINISHED,
		JOB_STATUS_CANCELED,
		JOB_STATUS_REQUEUED,
		JOB_STATUS_ISSUED,
	} status;

	GString *jid;
	GString *title;
	/* appears in top label */
	GString *start_date;
	GString *finish_date;
	/* appears in text view */
	GString *hostname;
	GString *issues;
	GString *cmd_line;
	GString *output;
	GString *queue;
	
	/* Moab stuff */
	GString *moab_jid;
};

/**
 * Create a new job (from \p server) and add it to list of jobs
 */
struct job *job_add(struct server *server, GString * jid,
		    GString * status, GString * title,
		    GString * start_date, GString * finish_date,
		    GString * hostname, GString * issues, GString * cmd_line, GString * output, GString * queue, GString * moab_jid);

/**
 * Frees job structure.
 */
void job_free(struct job *job);

/**
 * Frees job structure and delete it from list of jobs.
 * Occurs when cleaned or its server is removed.
 */
void job_delete(struct job *job);

/**
 * Remove job from the list. 
 */
void job_close(struct job *job, gboolean force, gboolean verbose);

/**
 * Select \p job and load it. 
 */
void job_set_active(struct job *job);

/**
 */
gboolean job_is_active(struct job *job);

/**
 */
void job_append_output(struct job *job, GString * output);

/**
 */
void job_update(struct job *job);

/**
 */
void job_update_label(struct job *job);

/**
 * Translate a \p status protocol string to a status enumeration 
 */
enum JobStatus job_translate_status(GString * status);

/**
 * Set UI related with status.
 * If \p job is not active nothing is done.
 */
void job_status_show(struct job *job);

/**
 * Change the status of \p job according to \p status and its \p parameter.
 * Change the GtkTreeIter's icon and handling other status changes. Calls #job_status_show.
 */
void job_status_update(struct job *job, enum JobStatus status, const gchar *parameter);

G_END_DECLS
#endif				//__JOB_H
