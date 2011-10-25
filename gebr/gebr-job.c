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

G_DEFINE_TYPE(GebrJob, gebr_job, G_TYPE_OBJECT);

/* Private methods {{{1 */
static void
gebr_job_finalize(GObject *object)
{
	GebrJob *job = GEBR_JOB(object);
	g_free(job->priv->title);
	g_free(job->priv->runid);
	g_free(job->priv->queue);
	g_free(job->priv->servers);
	g_list_free(job->priv->tasks);
	g_string_free(job->priv->output, TRUE);

	G_OBJECT_CLASS(gebr_job_parent_class)->finalize(object);
}

static void
gebr_job_init(GebrJob *job)
{
	job->priv = G_TYPE_INSTANCE_GET_PRIVATE(job,
						GEBR_TYPE_JOB,
						GebrJobPriv);
	job->priv->output = g_string_new(NULL);
	job->priv->status = JOB_STATUS_INITIAL;
}

static void
gebr_job_class_init(GebrJobClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize = gebr_job_finalize;

	g_type_class_add_private(klass, sizeof(GebrJobPriv));
}

/* Public methods {{{1 */
GebrJob *
gebr_job_new_with_id(GtkTreeStore  *store,
		     GtkTextBuffer *buffer,
		     const gchar   *rid,
		     const gchar   *queue,
		     const gchar   *servers)
{
	GebrJob *job = g_object_new(GEBR_TYPE_JOB, NULL);

	job->priv->store = store;
	job->priv->buffer = buffer;
	job->priv->queue = g_strdup(queue);
	job->priv->servers = g_strdup(servers);
	job->priv->runid = g_strdup(rid);

	return job;
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

static gboolean
job_is_active(GebrJob *job)
{
	GtkTreeIter iter;
	GtkTreeModelFilter *filter;

	filter = GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(GTK_TREE_VIEW(gebr.job_control->view)));
	gtk_tree_model_filter_convert_child_iter_to_iter(filter, &iter, &job->priv->iter);
	return gtk_tree_selection_iter_is_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.job_control->view)), &iter);
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
			gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(gebr.job_control->text_view), mark);
		}
	}
}

void
gebr_job_add_issue(GebrJob *job, const gchar *_issues)
{
	g_object_set(gebr.job_control->issues_title_tag, "invisible", FALSE, NULL);

	GString *issues = g_string_new("");
	g_string_assign(issues, _issues);
	if (job_is_active(job) == FALSE) {
		g_string_free(issues, TRUE);
		return;
	}
	g_object_set(gebr.job_control->issues_title_tag, "invisible", FALSE, NULL);
	GtkTextMark * mark = gtk_text_buffer_get_mark(gebr.job_control->text_buffer, "last-issue");
	if (mark != NULL) {
		GtkTextIter iter;
		gtk_text_buffer_get_iter_at_mark(gebr.job_control->text_buffer, &iter, mark);
		gtk_text_buffer_insert_with_tags(gebr.job_control->text_buffer, &iter, issues->str, issues->len,
						 gebr.job_control->issues_title_tag, NULL);

		gtk_text_buffer_delete_mark(gebr.job_control->text_buffer, mark);
		gtk_text_buffer_create_mark(gebr.job_control->text_buffer, "last-issue", &iter, TRUE);
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
		gebr_job_kill(job);
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

GtkTreeIter *
gebr_job_get_iter(GebrJob *job)
{
	return &job->priv->iter;
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
		g_string_append_printf(buf, _("Command line for task %d of %d "), frac, total);
		g_string_append_printf(buf, _(" \(Server: %s)\n"), (gebr_task_get_server(task))->comm->address->str);
		g_string_append_printf(buf, "%s\n", gebr_task_get_cmd_line(task));
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
	if (job->priv->status != JOB_STATUS_FINISHED &&
	    job->priv->status != JOB_STATUS_CANCELED &&
	    job->priv->status != JOB_STATUS_FAILED)
		return;

	for (GList *i = job->priv->tasks; i; i = i->next)
		gebr_task_close(i->data, job->priv->runid);

	gtk_tree_store_remove(job->priv->store, &job->priv->iter);
	g_object_unref(job);
}

void
gebr_job_kill(GebrJob *job)
{
	for (GList *i = job->priv->tasks; i; i = i->next)
		gebr_task_kill(i->data);
}
