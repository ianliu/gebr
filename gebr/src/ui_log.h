/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2008 GeBR core team (http://gebr.sourceforge.net)
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

#ifndef __UI_LOG_H
#define __UI_LOG_H

#include <gtk/gtk.h>

#include <misc/log.h>

/* Store fields */
enum {
	LOG_TYPE_ICON = 0,
	LOG_DATE,
	LOG_MESSAGE,
	LOG_N_COLUMN
};

struct ui_log {
	GtkWidget *		widget;

	GtkListStore *		store;
	GtkWidget *		view;
};

struct ui_log *
log_setup_ui(void);

void
log_set_message(struct ui_log * ui_log, const gchar * message);

void
log_add_message_to_list(struct ui_log * ui_log, struct log_message * message);

#endif //__UI_LOG_H
