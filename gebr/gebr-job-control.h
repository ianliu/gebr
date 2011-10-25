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

enum JobControlSelectionType {
	JobControlDontWarnUnselection,
	JobControlJobQueueSelection,
	JobControlJobSelection,
	JobControlQueueSelection,
};

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

void gebr_job_control_free(GebrJobControl *jc);

void gebr_job_control_select_job(GebrJobControl *jc, GebrJob *job);

void gebr_job_control_select_job_by_rid(GebrJobControl *jc, const gchar *rid);

GtkWidget *gebr_job_control_get_widget(GebrJobControl *jc);

void gebr_job_control_add(GebrJobControl *jc, GebrJob *job);

GebrJob *gebr_job_control_find(GebrJobControl *jc, const gchar *rid);

/**
 */
gboolean job_control_save(void);
/**
 */
void job_control_cancel(void);
/**
 */
gboolean job_control_close(void);
/**
 */
void job_control_clear(gboolean force);
/**
 */
gboolean job_control_stop(void);

void job_control_queue_stop(void);
void job_control_queue_save(void);
void job_control_queue_close(void);

/**
 * Get selected job/queue and put it at \p iter.
 * Return TRUE if there was something selected. Otherwise, return FALSE.
 * \p check_type determine the error message if selection don't match it.
 */
gboolean job_control_get_selected(GtkTreeIter * iter, enum JobControlSelectionType check_type);

G_END_DECLS

#endif /* __GEBR_JOB_CONTROL_H__ */
