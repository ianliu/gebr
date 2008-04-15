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

/*
 * File: ui_server.h
 */

#ifndef __UI_SERVER_H
#define __UI_SERVER_H

#include <gtk/gtk.h>

#include "server.h"

/* Store field */
enum {
	SERVER_STATUS_ICON = 0,
	SERVER_ADDRESS,
	SERVER_POINTER,
	SERVER_N_COLUMN
};

struct ui_server_common {
	GtkWidget *			dialog;

	GtkListStore *			store;
	GtkWidget *			view;

	GtkWidget *			add_local_button;
};

struct ui_server_list {
	struct ui_server_common		common;
};

struct ui_server_list *
server_list_setup_ui(void);

void
server_list_show(struct ui_server_list * ui_server_list);

void
server_list_updated_status(struct server * server);

struct ui_server_select {
	struct ui_server_common		common;

	struct server *			selected;

	GtkWidget *			ok_button;
};

struct server *
server_select_setup_ui(void);

#endif //__UI_SERVER_H
