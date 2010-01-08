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

#ifndef __JOB_H
#define __JOB_H

#include <gtk/gtk.h>
#include <libgebr/geoxml.h>

#include "server.h"

enum JobStatus {
	JOB_STATUS_RUNNING,
	JOB_STATUS_FINISHED,
	JOB_STATUS_CANCELED,
	JOB_STATUS_FAILED,
};

struct job {
	enum JobStatus status;
	GtkTreeIter iter;
	struct server *server;

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
};

struct job *job_add(struct server *server, GString * jid,
		    GString * status, GString * title,
		    GString * start_date, GString * finish_date,
		    GString * hostname, GString * issues, GString * cmd_line, GString * output);

void job_free(struct job *job);

void job_delete(struct job *job);

void job_close(struct job *job);

struct job *job_find(GString * address, GString * jid);

void job_fill_info(struct job *job);

void job_set_active(struct job *job);

gboolean job_is_active(struct job *job);

void job_append_output(struct job *job, GString * output);

void job_update(struct job *job);

void job_update_label(struct job *job);

enum JobStatus job_translate_status(GString * status);

void job_update_status(struct job *job);

#endif				//__JOB_H
