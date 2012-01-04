/*
 * gebrm-task.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011 - GêBR Core Team (www.gebrproject.com)
 *
 * GêBR Project is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GêBR Project is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GêBR Project. If not, see <http://www.gnu.org/licenses/>.
 */

#include "gebrm-task.h"

#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <glib/gi18n.h>
#include <libgebr/gui/gebr-gui-utils.h>
#include <libgebr/utils.h>
#include <libgebr/date.h>

#include "gebrm-marshal.h"

enum {
	OUTPUT,
	STATUS_CHANGE,
	N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0, };

struct _GebrmTaskPriv {
	GebrmDaemon *daemon;
	GebrCommJobStatus status;
	gchar *rid;
	gint frac;
	GString *start_date;
	GString *finish_date;
	GString *issues;
	GString *cmd_line;
	GString *moab_jid;
	GString *output;
};

G_DEFINE_TYPE(GebrmTask, gebrm_task, G_TYPE_OBJECT);

static GHashTable *tasks_map = NULL;

static GHashTable *
get_tasks_map(void)
{
	if (!tasks_map)
		tasks_map = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	return tasks_map;
}

static void
gebrm_task_free(GebrmTask *task)
{
	g_free(task->priv->rid);
	g_string_free(task->priv->start_date, TRUE);
	g_string_free(task->priv->finish_date, TRUE);
	g_string_free(task->priv->issues, TRUE);
	g_string_free(task->priv->cmd_line, TRUE);
	g_string_free(task->priv->moab_jid, TRUE);
	g_string_free(task->priv->output, TRUE);
}

static void
gebrm_task_finalize(GObject *object)
{
	GebrmTask *task = GEBRM_TASK(object);
	gebrm_task_free(task);
	G_OBJECT_CLASS(gebrm_task_parent_class)->finalize(object);
}

static void gebrm_task_init(GebrmTask *task)
{
	task->priv = G_TYPE_INSTANCE_GET_PRIVATE(task,
	                                         GEBRM_TYPE_TASK,
	                                         GebrmTaskPriv);

	task->priv->status = JOB_STATUS_INITIAL;
	task->priv->output = g_string_new(NULL);
	task->priv->start_date = g_string_new(NULL);
	task->priv->finish_date = g_string_new(NULL);
	task->priv->issues = g_string_new(NULL);
	task->priv->cmd_line = g_string_new(NULL);
	task->priv->moab_jid = g_string_new(NULL);
}

static void gebrm_task_class_init(GebrmTaskClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize = gebrm_task_finalize;

	signals[OUTPUT] =
		g_signal_new("output",
			     G_OBJECT_CLASS_TYPE(gobject_class),
			     G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			     G_STRUCT_OFFSET(GebrmTaskClass, output),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__STRING,
			     G_TYPE_NONE, 1, G_TYPE_STRING);

	signals[STATUS_CHANGE] =
		g_signal_new("status-change",
			     G_OBJECT_CLASS_TYPE(gobject_class),
			     G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			     G_STRUCT_OFFSET(GebrmTaskClass, status_change),
			     NULL, NULL,
			     gebrm_cclosure_marshal_VOID__INT_INT_STRING,
			     G_TYPE_NONE, 3, G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING);

	g_type_class_add_private(klass, sizeof(GebrmTaskPriv));
}

gchar *
gebrm_task_build_id(const gchar *rid,
                    const gchar *frac)
{
	return g_strconcat(rid, ":", frac, NULL);
}

static gchar *
gebrm_task_get_id(GebrmTask *task)
{
	gchar *frac = g_strdup_printf("%d", task->priv->frac);
	gchar *ret = gebrm_task_build_id(task->priv->rid, frac);
	g_free(frac);
	return ret;
}

GebrmTask *
gebrm_task_new(GebrmDaemon  *server,
	       const gchar *rid,
	       const gchar *frac)
{
	GebrmTask *task = g_object_new(GEBRM_TYPE_TASK, NULL);

	task->priv->status = JOB_STATUS_INITIAL; 
	task->priv->daemon = server;
	task->priv->rid = g_strdup(rid);
	task->priv->frac = atoi(frac);

	g_debug("Inserting task %s, rid %s into TASKS hash table (%s)",
		frac, rid, gebrm_task_get_id(task));
	g_hash_table_insert(get_tasks_map(), gebrm_task_get_id(task), task);

	return task;
}

const gchar *
gebrm_task_get_job_id(GebrmTask *task)
{
	return task->priv->rid;
}

void
gebrm_task_init_details(GebrmTask *task,
			GString   *issues,
			GString   *cmd_line,
			GString   *moab_jid)
{
	g_string_assign(task->priv->issues, issues->str);
	g_string_assign(task->priv->cmd_line, cmd_line->str);
	g_string_assign(task->priv->moab_jid, moab_jid->str);
}

GebrCommJobStatus
gebrm_task_translate_status(GString *status)
{
	GebrCommJobStatus translated_status;

	if (!strcmp(status->str, "unknown"))
		translated_status = JOB_STATUS_INITIAL;
	else if (!strcmp(status->str, "queued"))
		translated_status = JOB_STATUS_QUEUED;
	else if (!strcmp(status->str, "failed"))
		translated_status = JOB_STATUS_FAILED;
	else if (!strcmp(status->str, "running"))
		translated_status = JOB_STATUS_RUNNING;
	else if (!strcmp(status->str, "finished"))
		translated_status = JOB_STATUS_FINISHED;
	else if (!strcmp(status->str, "canceled"))
		translated_status = JOB_STATUS_CANCELED;
	else if (!strcmp(status->str, "requeued"))
		translated_status = JOB_STATUS_REQUEUED;
	else if (!strcmp(status->str, "issued"))
		translated_status = JOB_STATUS_ISSUED;
	else
		translated_status = JOB_STATUS_INITIAL;

	return translated_status;
}

GebrmTask *
gebrm_task_find(const gchar *rid, const gchar *frac)
{
	g_return_val_if_fail(rid != NULL && frac != NULL, NULL);

	gchar *tid = gebrm_task_build_id(rid, frac);
	g_debug("Looking for task %s, rid %s (%s)",
		frac, rid, tid);
	GebrmTask *task = g_hash_table_lookup(get_tasks_map(), tid);
	g_free(tid);
	return task;
}

gint
gebrm_task_get_fraction(GebrmTask *task)
{
	return task->priv->frac;
}

GebrCommJobStatus
gebrm_task_get_status(GebrmTask *task)
{
	return task->priv->status;
}

void
gebrm_task_emit_output_signal(GebrmTask *task,
			     const gchar *output)
{
	g_string_append(task->priv->output, output);
	g_signal_emit(task, signals[OUTPUT], 0, output);
}

void
gebrm_task_emit_status_changed_signal(GebrmTask *task,
				      GebrCommJobStatus new_status,
				      const gchar *parameter)
{
	GebrCommJobStatus old_status;

	old_status = task->priv->status;
	task->priv->status = new_status;

	switch (new_status)
	{
	case JOB_STATUS_RUNNING:
		g_string_assign(task->priv->start_date, parameter);
		break;
	case JOB_STATUS_FINISHED:
	case JOB_STATUS_CANCELED:
		g_string_assign(task->priv->finish_date, parameter);
		break;
	default:
		break;
	}

	g_signal_emit(task, signals[STATUS_CHANGE], 0, old_status, new_status, parameter);
}

const gchar *
gebrm_task_get_cmd_line(GebrmTask *task)
{
	return task->priv->cmd_line->str;
}

const gchar *
gebrm_task_get_start_date(GebrmTask *task)
{
	return task->priv->start_date->len > 0 ? task->priv->start_date->str : NULL;
}

const gchar *
gebrm_task_get_finish_date(GebrmTask *task)
{
	return task->priv->finish_date->str;
}

const gchar *
gebrm_task_get_issues(GebrmTask *task)
{
	return task->priv->issues->str;
}
const gchar *
gebrm_task_get_output(GebrmTask *task)
{
	return task->priv->output->str;
}

void
gebrm_task_close(GebrmTask *task, const gchar *rid)
{
	gchar *tid = gebrm_task_get_id(task);
	GebrCommServer *server = gebrm_daemon_get_server(task->priv->daemon);
	if (gebr_comm_server_is_logged(server))
		gebr_comm_protocol_socket_oldmsg_send(server->socket, FALSE,
						      gebr_comm_protocol_defs.clr_def, 1,
						      rid);

	g_hash_table_remove(get_tasks_map(), tid);
	g_free(tid);
}

void
gebrm_task_kill(GebrmTask *task)
{
	GebrCommServer *server = gebrm_daemon_get_server(task->priv->daemon);
	gebr_comm_protocol_socket_oldmsg_send(server->socket, FALSE,
					      gebr_comm_protocol_defs.kil_def, 1,
					      task->priv->rid);
}

GebrmDaemon *
gebrm_task_get_daemon(GebrmTask *task)
{
	return task->priv->daemon;
}

GebrCommServer *
gebrm_task_get_server(GebrmTask *task)
{
	return gebrm_daemon_get_server(task->priv->daemon);
}
