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

#include "job-queue.h"
#include "queues.h"

GebrdJobQueue * gebrd_job_queue_new(const gchar * name)
{
	GebrdJobQueue * job_queue;
	job_queue = g_new(GebrdJobQueue, 1);
	job_queue->name = g_strdup(name);
	job_queue->is_busy = FALSE;
	job_queue->jobs = NULL;
	return job_queue;
}

void gebrd_job_queue_append_job(GebrdJobQueue * job_queue, struct job * job)
{
	job_queue->jobs = g_list_append(job_queue->jobs, job);
}

void gebrd_job_queue_remove_job(GebrdJobQueue * job_queue, struct job * job)
{
	job_queue->jobs = g_list_remove(job_queue->jobs, job);
	if (job_queue->name[0] == 'j' && !g_list_length(job_queue->jobs))
		gebrd_queues_remove(job_queue->name);	    
}

void gebrd_job_queue_free(GebrdJobQueue * job_queue)
{
	// g_list_foreach(job_queue->jobs, (GFunc)job_free, NULL);
	g_list_free(job_queue->jobs);
	g_free(job_queue->name);
	g_free(job_queue);
}

struct job * gebrd_job_queue_pop(GebrdJobQueue * job_queue)
{
	GList * head;
	struct job * job;
	head = job_queue->jobs;
	job = (struct job*)head->data;
	job_queue->jobs = g_list_delete_link(head, head);
	return job;
}
