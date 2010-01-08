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

#ifndef __UI_JOB_CONTROL_H
#define __UI_JOB_CONTROL_H

#include <gtk/gtk.h>

/* Store fields */
enum {
	JC_ICON = 0,
	JC_TITLE,
	JC_STRUCT,
	JC_N_COLUMN
};

struct ui_job_control {
	GtkWidget *widget;

	GtkListStore *store;
	GtkWidget *view;

	GtkWidget *label;
	GtkWidget *text_view;
	GtkTextBuffer *text_buffer;
};

struct ui_job_control *job_control_setup_ui(void);

void job_control_clear_or_select_first(void);

void job_control_save(void);

void job_control_cancel(void);

void job_control_close(void);

void job_control_clear(void);

void job_control_stop(void);

#endif				//__UI_JOB_CONTROL_H
