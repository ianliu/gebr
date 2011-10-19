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
	GString *output;
	enum JobStatus status;
};

void gebr_job_append_task_output(GebrTask *task,
                                 GString *output,
                                 GebrJob *job);

static void gebr_job_change_task_status(GebrTask *task,
                                        gint old_status,
                                        gint new_status,
                                        const gchar *parameter,
                                        GebrJob *job);

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

	gint frac, total;
	gebr_task_get_fraction(task, &frac, &total);

	g_signal_connect(i->data, "status-changed", gebr_job_change_task_status, job);
	g_signal_connect(task, "output", gebr_job_append_task_output, job);
}

enum JobStatus
gebr_job_get_status(GebrJob *job)
{
	/* computa status */
	return 0;
}

GebrJob *
gebr_job_find(const gchar *rid)
{
	GebrTask *job = NULL;

	g_return_val_if_fail(rid != NULL, NULL);

	gboolean job_find_foreach_func(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
	{
		GebrJob *i;

		gtk_tree_model_get(model, iter, JC_STRUCT, &i, -1);

		if (!i)
			return FALSE;

		if (g_strcmp0(i->parent.run_id->str, rid) == 0) {
			job = i;
			return TRUE;
		}
		return FALSE;
	}

	gtk_tree_model_foreach(GTK_TREE_MODEL(gebr.ui_job_control->store), job_find_foreach_func, NULL);

	return job;
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

static void
gebr_job_append_task_output(GebrTask *task,
                            GString *output,
                            GebrJob *job)
{
	GtkTextIter iter;
	GString *text;
	GtkTextMark *mark;

	if (!output->len)
		return;
	if (!job->priv->output->len) {
		g_string_printf(job->priv->output, "\n%s\n%s", _("Output:"), output->str);
		text = job->priv->output;
	} else {
		g_string_append(job->priv->output, output->str);
		text = output;
	}
	if (job_is_active(job) == TRUE) {
		gtk_text_buffer_get_end_iter(gebr.ui_job_control->text_buffer, &iter);
		gtk_text_buffer_insert(gebr.ui_job_control->text_buffer, &iter, text->str, text->len);
		if (gebr.config.job_log_auto_scroll) {
			mark = gtk_text_buffer_get_mark(gebr.ui_job_control->text_buffer, "end");
			gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(gebr.ui_job_control->text_view), mark);
		}
	}
}

static void
gebr_job_change_task_status(GebrTask *task,
                            gint old_status,
                            gint new_status,
                            const gchar *parameter,
                            GebrJob *job)
{
	gint total;
	gebr_task_get_fraction(task, NULL, &total);

	if (g_list_length(job->priv->tasks) != total)
		//status incomplete do job
		return;

	if (new_status == JOB_STATUS_REQUEUED) {
		job->priv->status = JOB_STATUS_REQUEUED;
		// Remove o job e insere em outro lugar, ou seja
		// altera o pai do job

		return;

	} else if (new_status == JOB_STATUS_RUNNING) {
		if (job->priv->status == JOB_STATUS_RUNNING)
			return;
		else {
			for (GList *i = job->priv->tasks; i; i = i->next) {
				GebrTask *t = i->data;
				if (t->status != JOB_STATUS_RUNNING)
					return;
			}
			job->priv->status = JOB_STATUS_RUNNING;
		}

	} else if (new_status == JOB_STATUS_FINISHED) {
		if (job->priv->status == JOB_STATUS_FINISHED)
			return;
		else {
			for (GList *i = job->priv->tasks; i; i = i->next) {
				GebrTask *t = i->data;
				if (t->status != JOB_STATUS_FINISHED)
					return;
			}
			job->priv->status = JOB_STATUS_FINISHED;
		}

	} else if (new_status == JOB_STATUS_CANCELED || new_status == JOB_STATUS_FAILED) {
		job->priv->status = JOB_STATUS_FAILED;
	}

	else if (new_status == JOB_STATUS_ISSUED) {
		job_add_issue(job, parameter);
		return;
	}

	GtkTreeModel *model = GTK_TREE_MODEL(job->priv->store);
	GtkTreePath *path = gtk_tree_model_get_path(model, &job->priv->iter);
	gtk_tree_model_row_changed(model, path, &job->priv->iter);
}
