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

#ifndef __JOB_QUEUE_H
#define __JOB_QUEUE_H

#include "job.h"

typedef struct _GebrdJobQueue {
	gchar * name;
	gboolean is_busy;
	GList * jobs;
} GebrdJobQueue;

GebrdJobQueue * gebrd_job_queue_new(const gchar * name);

void gebrd_job_queue_append_job(GebrdJobQueue * job_queue, struct job * job);

void gebrd_job_queue_free(GebrdJobQueue * job_queue);

struct job * gebrd_job_queue_pop(GebrdJobQueue * job_queue);

#endif /* __JOB_QUEUE_H */
