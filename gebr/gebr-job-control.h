/*   GeBR - An environment for seismic processing.
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

#ifndef __GEBR_JOB_CONTROL_H__
#define __GEBR_JOB_CONTROL_H__

#include <gtk/gtk.h>
#include "gebr-job.h"

G_BEGIN_DECLS

typedef struct _GebrJobControl GebrJobControl;
typedef struct _GebrJobControlPriv GebrJobControlPriv;

struct _GebrJobControl {
	GtkListStore *store;
	GtkWidget *view;

	GtkWidget *label;
	GtkWidget *text_view;
	GtkTextBuffer *text_buffer;
	GtkTextTag *issues_title_tag;

	GtkWidget *cmd_view;
	GtkTextBuffer *cmd_buffer;

	GebrJobControlPriv *priv;
};

/**
 * gebr_job_control_new:
 *
 * Creates the job control page. Free with gebr_job_control_free().
 */
GebrJobControl *gebr_job_control_new(void);

/**
 * gebr_job_control_free:
 *
 * Free the @jc struct.
 */
void gebr_job_control_free(GebrJobControl *jc);

/**
 * gebr_job_control_select_job:
 *
 * Selects @job in Job Control tab. If @job is %NULL, deselects everything; if
 * @job does not exists in the job control list, nothing is done.
 */
void gebr_job_control_select_job(GebrJobControl *jc, GebrJob *job);

/**
 * gebr_job_control_select_job_by_rid:
 *
 * Finds a job with gebr_job_control_job_find() and selects it with
 * gebr_job_control_select_job().
 */
void gebr_job_control_select_job_by_rid(GebrJobControl *jc, const gchar *rid);

/**
 * gebr_job_control_get_widget:
 *
 * Returns: The #GtkWidget containing the Job control interface.
 */
GtkWidget *gebr_job_control_get_widget(GebrJobControl *jc);

/**
 * gebr_job_control_add:
 *
 * Adds @job into the job control list.
 */
void gebr_job_control_add(GebrJobControl *jc, GebrJob *job);

/**
 * gebr_job_control_find:
 *
 * Finds the job with the unique id @rid.
 */
GebrJob *gebr_job_control_find(GebrJobControl *jc, const gchar *rid);

/**
 * gebr_job_control_close_selected:
 *
 * Closes the selected job if applicable. See gebr_job_close().
 */
void gebr_job_control_close_selected(GebrJobControl *jc);

/**
 * gebr_job_control_stop_selected:
 *
 * Stops the selected job if applicable.
 */
void gebr_job_control_stop_selected(GebrJobControl *jc);

G_END_DECLS

#endif /* __GEBR_JOB_CONTROL_H__ */
