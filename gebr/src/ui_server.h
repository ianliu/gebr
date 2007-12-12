/*   GÍBR - An environment for seismic processing.
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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

/*
 * Struct: ui_server_list
 *
 * (start code)
 *   struct ui_server_list {
 *                    GtkWidget *		dialog;
 *
 *                    GtkListStore *	store;
 *                    GtkWidget *		view;
 *   };
 * (end code)
*/
struct ui_server_list {
	GtkWidget *		dialog;

	GtkListStore *		store;
	GtkWidget *		view;
};

struct ui_server_list *
server_list_setup_ui(void);

void
server_list_updated_status(struct server * server);

struct ui_server_select {
	GtkWidget *		dialog;

	struct server *		selected;

	/* same as gebr.ui_server_list.store */
	GtkListStore *		store;
	GtkWidget *		view;

	GtkWidget *		ok_button;
};

struct server *
server_select_setup_ui(void);

#endif //__UI_SERVER_H
