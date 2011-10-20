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

G_BEGIN_DECLS

typedef struct _GebrJob GebrJob;
typedef struct _GebrJobPriv GebrJobPriv;

struct _GebrJob {
	GebrJobPriv *priv;
};

GebrJob * gebr_job_new(GtkTreeStore *store,
		       const gchar  *queue,
		       const gchar  *group);

GebrJob *gebr_job_new_with_id(GtkTreeStore *store,
			      const gchar  *rid,
			      const gchar  *queue,
			      const gchar  *group);

void gebr_job_show(GebrJob *job);

void gebr_job_append_task(GebrJob *job, GebrTask *task);

GebrJob *gebr_job_find(const gchar *rid);

const gchar *gebr_job_get_title(GebrJob *job);

void gebr_job_set_title(GebrJob *job, const gchar *title);

const gchar *gebr_job_get_queue(GebrJob *job);

const gchar *gebr_job_get_group(GebrJob *job);

enum JobStatus gebr_job_get_status(GebrJob *job);

void gebr_job_free(GebrJob *job);

const gchar *gebr_job_get_id(GebrJob *job);

G_END_DECLS

#endif /* __GEBR_JOB_H__ */