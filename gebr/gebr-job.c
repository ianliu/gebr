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

#include "gebr.h"
#include "gebr-job.h"

struct _GebrJobPriv {
	GList *tasks;
	gchar *title;
	gchar *runid;
	gchar *queue;
	gchar *group;
	GtkTreeIter iter;
	GtkTreeStore *store;
};

/* Private methods {{{1 */
static void
insert_into_model(GebrJob *job)
{
	GtkTreeIter parent;

	gebr_jc_get_queue_group_iter(job->priv->store,
				     job->priv->queue,
				     job->priv->group,
				     &parent);

	gtk_tree_store_append(job->priv->store, &job->priv->iter, &parent);
	gtk_tree_store_set(job->priv->store, &job->priv->iter, JC_STRUCT /* GEBR_JC_JOB */, job, -1);
}

/* Public methods {{{1 */
GebrJob *
gebr_job_new(GtkTreeStore *store, const gchar *queue, const gchar *group)
{
	static int runid = 0;

	GebrJob *job = g_new0(GebrJob, 1);

	job->priv = g_new0(GebrJobPriv, 1);
	job->priv->store = store;
	job->priv->queue = g_strdup(queue);
	job->priv->group = g_strdup(group);
	job->priv->runid = g_strdup_printf("%d:%s", runid++, gebr_get_session_id());

	return job;
}

void
gebr_job_show(GebrJob *job)
{
	g_return_if_fail(job->priv->queue != NULL || job->priv->group != NULL);
	insert_into_model(job);
}

const gchar *
gebr_job_get_group(GebrJob *job)
{
	return job->priv->group;
}

const gchar *
gebr_job_get_queue(GebrJob *job)
{
	return job->priv->queue;
}

void
gebr_job_set_title(GebrJob *job, const gchar *title)
{
	if (job->priv->title)
		g_free(job->priv->title);
	job->priv->title = g_strdup(title);
}

const gchar *
gebr_job_get_title(GebrJob *job)
{
	return job->priv->title;
}

void
gebr_job_append_task(GebrJob *job, GebrTask *task)
{
	job->priv->tasks = g_list_prepend(job->priv->tasks, task);
}

enum JobStatus
gebr_job_get_status(GebrJob *job)
{
	/* computa status */
	return 0;
}

void
gebr_job_free(GebrJob *job)
{
	g_free(job->priv->queue);
	g_free(job->priv->group);
	g_list_free(job->priv->tasks);
	g_free(job->priv);
	g_free(job);
}
