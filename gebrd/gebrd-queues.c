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

#include "gebrd.h"
#include "gebrd-queues.h"
#include "gebrd-job-queue.h"
#include "gebrd-job.h"

void gebrd_queues_add_job_to(const gchar * queue, GebrdJob * job)
{
	GebrdJobQueue * job_queue;
	job_queue = g_hash_table_lookup(gebrd.queues, queue);
	if (!job_queue) {
		job_queue = gebrd_job_queue_new(queue);
		g_hash_table_insert(gebrd.queues, g_strdup(queue), job_queue);
	}
	gebrd_job_queue_append_job(job_queue, job);
}

void gebrd_queues_remove_job_from(const gchar * queue, GebrdJob * job)
{
	GebrdJobQueue * job_queue;
	job_queue = g_hash_table_lookup(gebrd.queues, queue);
	if (job_queue != NULL)
		gebrd_job_queue_remove_job(job_queue, job);
}

void gebrd_queues_remove(const gchar * queue)
{
	GebrdJobQueue * job_queue;

	job_queue = g_hash_table_lookup(gebrd.queues, queue);
	g_hash_table_remove(gebrd.queues, queue);
	gebrd_job_queue_free(job_queue);
}

void gebrd_queues_rename(const gchar * queue, const gchar *newname)
{
	GebrdJobQueue * job_queue;
	job_queue = g_hash_table_lookup(gebrd.queues, queue);
	if (job_queue == NULL)
		return;

	/* change queue name reference for all its jobs */
	GList *link = gebrd.jobs;
	while (link) {
		GebrdJob *job = (GebrdJob*)link->data;

		if (strcmp(queue, job->parent.queue_id->str) == 0) {
			g_string_assign(job->parent.queue_id, newname);
			job_status_notify(job, JOB_STATUS_REQUEUED, newname);
		}

		link = g_list_next(link);
	}

	g_hash_table_remove(gebrd.queues, queue);
	g_free(job_queue->name);
	job_queue->name = g_strdup(newname);
	g_hash_table_insert(gebrd.queues, g_strdup(newname), job_queue);
}

gchar * gebrd_queues_get_names()
{
	GString * string;
	string = g_string_new(NULL);

	void gebrd_queues_iter(const gchar * key, const gchar * value, GString * string) {
		g_string_append_printf(string, "%s,", key);
	}

	g_hash_table_foreach(gebrd.queues, (GHFunc)gebrd_queues_iter, string);
	if (string->len > 0)
		string->str[string->len-1] = '\0';
	return g_string_free(string, FALSE);
}

gboolean gebrd_queues_is_queue_busy(const gchar * queue)
{
	GebrdJobQueue * job_queue;
	job_queue = g_hash_table_lookup(gebrd.queues, queue);
	if (!job_queue)
		return FALSE;
	return job_queue->is_busy;
}

void gebrd_queues_set_queue_busy(const gchar * queue, gboolean busy)
{
	GebrdJobQueue * job_queue;
	job_queue = g_hash_table_lookup(gebrd.queues, queue);
	if (!job_queue)
		return;
	job_queue->is_busy = busy;
}

void gebrd_queues_step_queue(const gchar * queue)
{
	GebrdJobQueue * job_queue;
	job_queue = g_hash_table_lookup(gebrd.queues, queue);
	if (!job_queue)
		return;
	GebrdJob * job;
	job = gebrd_job_queue_pop(job_queue);
	job_run_flow(job);
}

gboolean gebrd_queues_has_next(const gchar * queue)
{

	GebrdJobQueue * job_queue;
	job_queue = g_hash_table_lookup(gebrd.queues, queue);
	if (!job_queue)
		return FALSE;
	return g_list_length(job_queue->jobs) != 0;
}
