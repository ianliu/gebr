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

/**
 * @file job-queue.h Job queue API
 * @ingroup gebrd
 */

#ifndef __JOB_QUEUE_H
#define __JOB_QUEUE_H

#include "gebrd-job.h"

G_BEGIN_DECLS

/**
 * Structure representing a queue of jobs.
 */
typedef struct _GebrdJobQueue {
	gchar * name;		/**< The name of this queue. */
	gboolean is_busy;	/**< Whether this queue is busy. */
	GList * jobs;		/**< Actual list of jobs. */
} GebrdJobQueue;

/**
 * Creates a new job queue.
 * @param name The name of this queue.
 * @return A newly allocated job queue. Free with #gebrd_job_queue_free.
 */
GebrdJobQueue * gebrd_job_queue_new(const gchar * name);

/**
 * Appends \p job in \p job_queue.
 */
void gebrd_job_queue_append_job(GebrdJobQueue * job_queue, GebrdJob * job);

/**
 * Removes \p job from \p job_queue.
 */
void gebrd_job_queue_remove_job(GebrdJobQueue * job_queue, GebrdJob * job);

/**
 * Frees \p job_queue structure and members.
 */
void gebrd_job_queue_free(GebrdJobQueue * job_queue);

/**
 * Removes the first job in queue and returns it.
 * @param job_queue Queue to have its first element removed.
 * @return The first job in queue.
 */
GebrdJob * gebrd_job_queue_pop(GebrdJobQueue * job_queue);

G_END_DECLS
#endif /* __JOB_QUEUE_H */
