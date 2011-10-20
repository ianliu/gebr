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

#include "gebr-job.h"

#include <glib/gi18n.h>

#include "gebr.h"

struct _GebrJobPriv {
	GList *tasks;
	gchar *title;
	gchar *runid;
	gchar *queue;
	gchar *servers;
	const gchar *start_date;
	const gchar *finish_date;
	GtkTreeIter iter;
	GString *output;
	enum JobStatus status;
	GtkTreeStore *store;
	GtkTextBuffer *buffer;
};

static void gebr_job_append_task_output(GebrTask *task,
					const gchar *output,
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
				     job->priv->servers,
				     &parent);

	gtk_tree_store_append(job->priv->store, &job->priv->iter, &parent);
	gtk_tree_store_set(job->priv->store, &job->priv->iter,
			   JC_STRUCT, job,
			   JC_VISIBLE, TRUE,
			   -1);
}

static void
gebr_job_free(GebrJob *job)
{
	g_free(job->priv->title);
	g_free(job->priv->runid);
	g_free(job->priv->queue);
	g_free(job->priv->servers);
	g_list_free(job->priv->tasks);
	g_string_free(job->priv->output, TRUE);
	g_free(job->priv);
	g_free(job);
}

/* Public methods {{{1 */
GebrJob *
gebr_job_new(GtkTreeStore  *store,
	     GtkTextBuffer *buffer,
	     const gchar   *queue,
	     const gchar   *servers)
{
	static int runid = 0;
	gchar *new_rid = g_strdup_printf("%d:%s", runid++, gebr_get_session_id());
	GebrJob *job = gebr_job_new_with_id(store, buffer, new_rid, queue, servers);
	g_free(new_rid);
	return job;
}

GebrJob *
gebr_job_new_with_id(GtkTreeStore  *store,
		     GtkTextBuffer *buffer,
		     const gchar   *rid,
		     const gchar   *queue,
		     const gchar   *servers)
{
	GebrJob *job = g_new0(GebrJob, 1);

	job->priv = g_new0(GebrJobPriv, 1);
	job->priv->store = store;
	job->priv->buffer = buffer;
	job->priv->output = g_string_new(NULL);
	job->priv->queue = g_strdup(queue);
	job->priv->servers = g_strdup(servers);
	job->priv->runid = g_strdup(rid);
	job->priv->status = JOB_STATUS_INITIAL;

	g_debug("New job created with rid %s", job->priv->runid);

	return job;
}

void
gebr_job_show(GebrJob *job)
{
	g_return_if_fail(job->priv->queue != NULL || job->priv->servers != NULL);
	insert_into_model(job);
}

const gchar *
gebr_job_get_group(GebrJob *job)
{
	return job->priv->servers;
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

	if (!job->priv->start_date)
		job->priv->start_date = gebr_task_get_start_date(task);

	g_signal_connect(task, "status-change", G_CALLBACK(gebr_job_change_task_status), job);
	g_signal_connect(task, "output", G_CALLBACK(gebr_job_append_task_output), job);
}

enum JobStatus
gebr_job_get_status(GebrJob *job)
{
	return job->priv->status;
}

GebrJob *
gebr_job_find(const gchar *rid)
{
	GebrJob *job = NULL;

	g_return_val_if_fail(rid != NULL, NULL);

	gboolean job_find_foreach_func(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
	{
		GebrJob *i;

		gtk_tree_model_get(model, iter, JC_STRUCT, &i, -1);

		if (!i)
			return FALSE;

		if (g_strcmp0(gebr_job_get_id(i), rid) == 0) {
			job = i;
			return TRUE;
		}
		return FALSE;
	}

	gtk_tree_model_foreach(GTK_TREE_MODEL(gebr.ui_job_control->store), job_find_foreach_func, NULL);

	return job;
}

static gboolean
job_is_active(GebrJob *job)
{
	GtkTreeIter iter;
	GtkTreeModelFilter *filter;

	filter = GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(GTK_TREE_VIEW(gebr.ui_job_control->view)));
	gtk_tree_model_filter_convert_child_iter_to_iter(filter, &iter, &job->priv->iter);
	return gtk_tree_selection_iter_is_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_job_control->view)), &iter);
}

static void
gebr_job_append_task_output(GebrTask *task,
                            const gchar *output,
                            GebrJob *job)
{
	GtkTextIter iter;
	const gchar *text;
	GtkTextMark *mark;

	gint frac, total;

	gebr_task_get_fraction(task, &frac, &total);
	g_debug("==========Output signal from %d of %d: received %s", frac, total, output);

	if (!strlen(output))
		return;

	if (!job->priv->output->len) {
		g_string_printf(job->priv->output, "\n%s\n%s", _("Output:"), output);
		text = job->priv->output->str;
	} else {
		g_string_append(job->priv->output, output);
		text = output;
	}

	if (job_is_active(job)) {
		gtk_text_buffer_get_end_iter(job->priv->buffer, &iter);
		gtk_text_buffer_insert(job->priv->buffer, &iter, text, strlen(text));
		if (gebr.config.job_log_auto_scroll) {
			mark = gtk_text_buffer_get_mark(job->priv->buffer, "end");
			gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(gebr.ui_job_control->text_view), mark);
		}
	}
}

void
gebr_job_add_issue(GebrJob *job, const gchar *_issues)
{
	g_object_set(gebr.ui_job_control->issues_title_tag, "invisible", FALSE, NULL);

	GString *issues = g_string_new("");
	g_string_assign(issues, _issues);
	if (job_is_active(job) == FALSE) {
		g_string_free(issues, TRUE);
		return;
	}
	g_object_set(gebr.ui_job_control->issues_title_tag, "invisible", FALSE, NULL);
	GtkTextMark * mark = gtk_text_buffer_get_mark(gebr.ui_job_control->text_buffer, "last-issue");
	if (mark != NULL) {
		GtkTextIter iter;
		gtk_text_buffer_get_iter_at_mark(gebr.ui_job_control->text_buffer, &iter, mark);
		gtk_text_buffer_insert_with_tags(gebr.ui_job_control->text_buffer, &iter, issues->str, issues->len,
						 gebr.ui_job_control->issues_title_tag, NULL);

		gtk_text_buffer_delete_mark(gebr.ui_job_control->text_buffer, mark);
		gtk_text_buffer_create_mark(gebr.ui_job_control->text_buffer, "last-issue", &iter, TRUE);
	} else
		g_warning("Can't find mark \"issue\"");
	g_string_free(issues, TRUE);
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

	g_debug("Status changed from %d to %d",
		old_status, new_status);

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
				if (gebr_task_get_status(t) != JOB_STATUS_RUNNING)
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
				if (gebr_task_get_status(t) != JOB_STATUS_FINISHED)
					return;
			}
			job->priv->status = JOB_STATUS_FINISHED;
			job->priv->finish_date = gebr_task_get_finish_date(task);
		}

	} else if (new_status == JOB_STATUS_CANCELED || new_status == JOB_STATUS_FAILED) {
		job->priv->status = JOB_STATUS_FAILED;
		job->priv->finish_date = gebr_task_get_finish_date(task);
	}

	else if (new_status == JOB_STATUS_ISSUED) {
		gebr_job_add_issue(job, parameter);
		return;
	}

	GtkTreeModel *model = GTK_TREE_MODEL(job->priv->store);
	GtkTreePath *path = gtk_tree_model_get_path(model, &job->priv->iter);
	gtk_tree_model_row_changed(model, path, &job->priv->iter);
}

const gchar *
gebr_job_get_id(GebrJob *job)
{
	return job->priv->runid;
}

GtkTreeIter
gebr_job_get_iter(GebrJob *job)
{
	return job->priv->iter;
}

const gchar *
gebr_job_get_servers(GebrJob *job)
{
	return job->priv->servers;
}

gchar *
gebr_job_get_command_line(GebrJob *job)
{
	GString *buf = g_string_new(NULL);

	for (GList *i = job->priv->tasks; i; i = i->next) {
		gint frac, total;
		GebrTask *task = i->data;

		gebr_task_get_fraction(task, &frac, &total);
		g_string_append_printf(buf, _("Command line for task %d of %d"), frac, total);
		g_string_append_printf(buf, "\n%s\n", gebr_task_get_cmd_line(task));
	}

	return g_string_free(buf, FALSE);
}

const gchar *
gebr_job_get_output(GebrJob *job)
{
	return job->priv->output->str;
}

const gchar *
gebr_job_get_start_date(GebrJob *job)
{
	return job->priv->start_date;
}

const gchar *
gebr_job_get_finish_date(GebrJob *job)
{
	return job->priv->finish_date;
}

gchar *
gebr_job_get_issues(GebrJob *job)
{
	GString *buf = g_string_new(NULL);
	for (GList *i = job->priv->tasks; i; i = i->next)
		g_string_append_printf(buf, "%s\n", gebr_task_get_issues(i->data));
	return g_string_free(buf, FALSE);
}

void
gebr_job_close(GebrJob *job)
{
	for (GList *i = job->priv->tasks; i; i = i->next)
		gebr_task_close(i->data, job->priv->runid);

	GtkTreeIter parent;

	if (!gtk_tree_model_iter_parent(GTK_TREE_MODEL(job->priv->store),
					&parent, &job->priv->iter))
		g_return_if_reached();

	gtk_tree_store_remove(job->priv->store, &job->priv->iter);

	if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(job->priv->store), &parent) == 0)
		gtk_tree_store_remove(job->priv->store, &parent);

	gebr_job_free(job);
}
