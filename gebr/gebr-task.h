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
 * Translate a \p status protocol string to a status enumeration 
 */
enum JobStatus job_translate_status(GString * status);

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

enum JobStatus gebr_task_get_status(GebrTask *task);

void gebr_task_emit_output_signal(GebrTask *task,
				  const gchar *output);

void gebr_task_emit_status_changed_signal(GebrTask *task,
					  enum JobStatus new_status,
					  const gchar *parameter);

const gchar *gebr_task_get_cmd_line(GebrTask *task);

const gchar *gebr_task_get_start_date(GebrTask *task);

const gchar *gebr_task_get_finish_date(GebrTask *task);

const gchar *gebr_task_get_issues(GebrTask *task);

void gebr_task_close(GebrTask *task);

G_END_DECLS

#endif /* __GEBR_TASK_H__ */
