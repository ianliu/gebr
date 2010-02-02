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
#include <glib.h>

#include "job-queue.h"
#include "job.h"

/*
 * Tests for default parameters in a GebrdJobQueue.
 */
static void test_job_queue_new(void)
{
	GebrdJobQueue * job_queue;
	job_queue = gebrd_job_queue_new("name");
	g_assert_cmpstr(job_queue->name, == , "name");
	g_assert(job_queue->is_busy == FALSE);
	g_assert(job_queue->jobs == NULL);
}

/*
 * Tests for append operation in a GebrdJobQueue.
 */
static void test_job_queue_append_job(void)
{
	struct job *job_ptr, job1, job2;
	GebrdJobQueue * job_queue;

	job_queue = gebrd_job_queue_new("test");
	gebrd_job_queue_append_job(job_queue, &job1);
	gebrd_job_queue_append_job(job_queue, &job2);
	g_assert(job_queue->jobs != NULL);

	job_ptr = (struct job *)g_list_nth_data(job_queue->jobs, 0);
	g_assert(job_ptr == &job1);

	job_ptr = (struct job *)g_list_nth_data(job_queue->jobs, 1);
	g_assert(job_ptr == &job2);
}

/*
 * Tests for pop operation in a GebrdJobQueue.
 */
static void test_job_queue_pop(void)
{
	struct job *job_ptr, job1, job2;
	GebrdJobQueue * job_queue;

	job_queue = gebrd_job_queue_new("test");
	gebrd_job_queue_append_job(job_queue, &job1);
	gebrd_job_queue_append_job(job_queue, &job2);

	job_ptr = gebrd_job_queue_pop(job_queue);
	g_assert(job_ptr == &job1);

	job_ptr = gebrd_job_queue_pop(job_queue);
	g_assert(job_ptr == &job2);

	g_assert(job_queue->jobs == NULL);
}

int main(int argc, char * argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/gebrd/job-queue/new", test_job_queue_new);
	g_test_add_func("/gebrd/job-queue/append_job", test_job_queue_append_job);
	g_test_add_func("/gebrd/job-queue/pop", test_job_queue_pop);

	return g_test_run();
}

