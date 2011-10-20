/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2011 GeBR core team (http://www.gebrproject.com/)
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

#ifndef __GEBR_TASK_H__
#define __GEBR_TASK_H__

#include <gtk/gtk.h>

#include <libgebr/geoxml/geoxml.h>
#include <libgebr/comm/gebr-comm-job.h>

#include "server.h"

G_BEGIN_DECLS

#define GEBR_TYPE_TASK			(gebr_task_get_type())
#define GEBR_TASK(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_TASK_TYPE, GebrTask))
#define GEBR_TASK_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_TASK_TYPE, GebrTaskClass))
#define GEBR_IS_TASK(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_TASK_TYPE))
#define GEBR_IS_TASK_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_TASK_TYPE))
#define GEBR_TASK_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_TASK_TYPE, GebrTaskClass))

typedef struct _GebrTask GebrTask;
typedef struct _GebrTaskPriv GebrTaskPriv;
typedef struct _GebrTaskClass GebrTaskClass;

struct _GebrTask {
	GObject parent;

	/*< private >*/
	GebrTaskPriv *priv;
};

struct _GebrTaskClass {
	GObjectClass parent;

	void (*status_change) (GebrTask *task,
			       gint old_status,
			       gint new_status,
			       const gchar *parameter,
			       gpointer user_data);

	void (*output) (GebrTask *task,
			const gchar *output,
			gpointer user_data);
};

GType gebr_task_get_type(void) G_GNUC_CONST;

/**
 * gebr_task_find:
 */
GebrTask *gebr_task_find(const gchar *rid,
			 const gchar *frac);

/**
 * gebr_task_new:
 *
 * Creates a new job for @server.
 */
GebrTask *gebr_task_new(GebrServer  *server,
			const gchar *rid,
			const gchar *frac);

void gebr_task_get_fraction(GebrTask *task,
			    gint *frac,
			    gint *total);

/**
 * Create a new job (from \p server) and add it to list of jobs
 */
GebrTask *gebr_task_new_from_jid(GebrServer *server, GString * jid, GString * _status, GString * title,
				 GString * start_date, GString * finish_date, GString * hostname, GString * issues,
				 GString * cmd_line, GString * output, GString * queue, GString * moab_jid);

void job_init_details(GebrTask *job, GString * _status, GString * title, GString * start_date, GString * finish_date,
		      GString * hostname, GString * issues, GString * cmd_line, GString * output, GString * queue,
		      GString * moab_jid);
/**
 * Frees job structure.
 */
void job_free(GebrTask *job);

/**
 * Obsolete, same as job_free
 */
void job_delete(GebrTask *job);

/**
 * Return NULL if immediately
 */
const gchar *job_get_queue_name(GebrTask *job);

/**
 * Remove job from the list. 
 */
void job_close(GebrTask *job, gboolean force, gboolean verbose);

/**
 * Select \p job and load it. 
 */
void job_set_active(GebrTask *job);

/**
 * gebr_task_append_output:
 *
 * Appends @output
 */
void gebr_task_append_output(GebrTask *job, const gchar *output);

/**
 */
void job_update(GebrTask *job);

/**
 */
void job_update_label(GebrTask *job);

/**
 * Translate a \p status protocol string to a status enumeration 
 */
enum JobStatus job_translate_status(GString * status);

/**
 * Set UI related with status.
 * If \p job is not active nothing is done.
 */
void job_status_show(GebrTask *job);

/*
 * Updates the job text buffer
 */
void job_load_details(GebrTask *job);

/**
 */
void job_add_issue(GebrTask *job, const gchar *issues);

/**
 */
void gebr_task_init_details(GebrTask *task,
		       GString  *status,
		       GString  *start_date,
		       GString  *finish_date,
		       GString  *issues,
		       GString  *cmd_line,
		       GString  *queue,
		       GString  *moab_jid);

/**
 */
gboolean job_is_running(GebrTask *job);

/**
 */
gboolean job_has_finished(GebrTask *job);

/**
 * Change the status of \p job according to \p status and its \p parameter.
 * Change the GtkTreeIter's icon and handling other status changes. Calls #job_status_show.
 */
void job_status_update(GebrTask *job, enum JobStatus status, const gchar *parameter);

/**
 * gebr_job_bind:
 *
 * Associates the pair (@server, @jid) with @rid.
 */
void gebr_job_hash_bind(GebrServer *server, const gchar *jid, const gchar *rid);

/**
 * gebr_job_hash_get:
 *
 * Returns: the run id associated with the pair (@server, @jid), or %NULL if it
 * does not exist.
 */
const gchar *gebr_job_hash_get(GebrServer *server, const gchar *jid);

enum JobStatus gebr_task_get_status(GebrTask *task);

void gebr_task_emit_output_signal(GebrTask *task,
				  const gchar *output);

void gebr_task_emit_status_changed_signal(GebrTask *task,
					  enum JobStatus new_status,
					  const gchar *parameter);

G_END_DECLS

#endif /* __GEBR_TASK_H__ */