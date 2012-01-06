/*
 * gebrm-task.h
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

#ifndef __GEBRM_TASK_H__
#define __GEBRM_TASK_H__

#include "gebrm-daemon.h"
#include <libgebr/geoxml/geoxml.h>
#include <libgebr/comm/gebr-comm-job.h>

G_BEGIN_DECLS

#define GEBRM_TYPE_TASK			(gebrm_task_get_type())
#define GEBRM_TASK(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBRM_TYPE_TASK, GebrmTask))
#define GEBRM_TASK_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GEBRM_TYPE_TASK, GebrmTaskClass))
#define GEBRM_IS_TASK(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBRM_TYPE_TASK))
#define GEBRM_IS_TASK_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBRM_TYPE_TASK))
#define GEBRM_TASK_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBRM_TYPE_TASK, GebrmTaskClass))

typedef struct _GebrmTask GebrmTask;
typedef struct _GebrmTaskPriv GebrmTaskPriv;
typedef struct _GebrmTaskClass GebrmTaskClass;

struct _GebrmTask {
	GObject parent;

	/*< private >*/
	GebrmTaskPriv *priv;
};

struct _GebrmTaskClass {
	GObjectClass parent;

	void (*status_change) (GebrmTask *task,
			       gint old_status,
			       gint new_status,
			       const gchar *parameter);

	void (*output) (GebrmTask *task,
			const gchar *output);
};

GType gebrm_task_get_type(void) G_GNUC_CONST;

/**
 * gebrm_task_build_id:
 */
gchar *gebrm_task_build_id(const gchar *rid,
                           const gchar *frac);

/**
 * gebrm_task_find:
 */
GebrmTask *gebrm_task_find(const gchar *rid,
			   const gchar *frac);

/**
 * gebrm_task_new:
 *
 * Creates a new job for @server.
 */
GebrmTask *gebrm_task_new(GebrmDaemon *daemon,
			  const gchar *rid,
			  const gchar *frac);

gint gebrm_task_get_fraction(GebrmTask *task);

/**
 * gebrm_task_translate_status:
 *
 * Translate a @status protocol string to a status enumeration .
 */
GebrCommJobStatus gebrm_task_translate_status(GString *status);

/**
 */
void
gebrm_task_init_details(GebrmTask *task,
			GString   *issues,
			GString   *cmd_line,
			GString   *moab_jid);

GebrCommJobStatus gebrm_task_get_status(GebrmTask *task);

void gebrm_task_emit_output_signal(GebrmTask *task,
				  const gchar *output);

void gebrm_task_emit_status_changed_signal(GebrmTask *task,
					  GebrCommJobStatus new_status,
					  const gchar *parameter);

const gchar *gebrm_task_get_job_id(GebrmTask *task);

const gchar *gebrm_task_get_cmd_line(GebrmTask *task);

const gchar *gebrm_task_get_start_date(GebrmTask *task);

const gchar *gebrm_task_get_finish_date(GebrmTask *task);

const gchar *gebrm_task_get_issues(GebrmTask *task);

const gchar *gebrm_task_get_output(GebrmTask *task);

void gebrm_task_close(GebrmTask *task, const gchar *rid);

void gebrm_task_kill(GebrmTask *task);

const gchar *gebrm_task_get_queue(GebrmTask *task);

GebrmDaemon *gebrm_task_get_daemon(GebrmTask *task);

GebrCommServer *gebrm_task_get_server(GebrmTask *task);

const gchar *gebrm_task_get_niceness(GebrmTask *task);

const gchar *gebrm_task_get_last_run_date(GebrmTask *task);

G_END_DECLS

#endif /* __GEBRM_TASK_H__ */
