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

/**
 * \file ui_job_control.c Responsible for UI for job management.
 */

#ifndef __UI_JOB_CONTROL_H
#define __UI_JOB_CONTROL_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

enum JobControlSelectionType {
	JobControlDontWarnUnselection,
	JobControlJobQueueSelection,
	JobControlJobSelection,
	JobControlQueueSelection,
};

/**
 * Store fields 
 */
enum {
	JC_SERVER_ADDRESS, /* for ordering */
	JC_QUEUE_NAME,
	JC_STRUCT, /* non-NULL if it is a job */
	JC_VISIBLE,
	JC_N_COLUMN
};

struct ui_job_control {
	GtkWidget *widget;

	GtkTreeStore *store;
	GtkWidget *view;

	GtkWidget *label;
	GtkWidget *text_view;
	GtkTextBuffer *text_buffer;
	GtkTextTag *issues_title_tag;
};

void gebr_job_control_select_job(struct ui_job_control *jc,
				 const gchar *rid);

/**
 * Assembly the job control page.
 * Return:
 * The structure containing relevant data.
 */
struct ui_job_control *job_control_setup_ui(void);

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

/*
 */
void job_control_selected(void);

void job_control_queue_stop(void);
void job_control_queue_save(void);
void job_control_queue_close(void);

/**
 * gebr_jc_get_queue_group_iter:
 *
 * Fills @iter with the iterator corresponding to the line matching servers
 * group to @group and queue to @queue.
 */
void gebr_jc_get_queue_group_iter(GtkTreeStore *store,
				  const gchar  *queue,
				  const gchar  *group,
				  GtkTreeIter  *iter);

/**
 * Get selected job/queue and put it at \p iter.
 * Return TRUE if there was something selected. Otherwise, return FALSE.
 * \p check_type determine the error message if selection don't match it.
 */
gboolean job_control_get_selected(GtkTreeIter * iter, enum JobControlSelectionType check_type);

G_END_DECLS
#endif				//__UI_JOB_CONTROL_H
