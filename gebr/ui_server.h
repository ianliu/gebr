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

/*
 * File: ui_server.h
 */

#ifndef __UI_SERVER_H
#define __UI_SERVER_H

#include <gtk/gtk.h>

#include "server.h"

G_BEGIN_DECLS

/* Store field */
enum {
	SERVER_STATUS_ICON = 0,
	SERVER_AUTOCONNECT,
	SERVER_NAME,
	SERVER_POINTER,
	SERVER_TAGS,
	SERVER_CPU,
	SERVER_MEM,
	SERVER_FS,
	SERVER_N_COLUMN
};

struct ui_server_common {
	GtkWidget *dialog;
	GtkWidget *widget;

	GtkListStore *store;
	GtkWidget *view;

	GtkWidget *add_local_button;

	GtkWidget *combo;
	GtkListStore *combo_store;
	GtkTreeModel *filter;
	GtkTreeModel *sort_store;
};

struct ui_server_list {
	struct ui_server_common common;
};

struct ui_server_list *server_list_setup_ui(void);

void server_list_show(struct ui_server_list *ui_server_list);

void server_list_updated_status(GebrServer *server);

struct ui_server_select {
	struct ui_server_common common;

	GebrServer *selected;

	GtkWidget *ok_button;
};

gchar **ui_server_list_tag (GebrServer *server);

GList *ui_server_servers_with_tag (const gchar *tag);

gboolean ui_server_has_tag (GebrServer *server, const gchar *tag);

void ui_server_set_tags (GebrServer *server, const gchar *str);

gchar **ui_server_get_all_tags (void);

void ui_server_update_tags_combobox (void);

G_END_DECLS

#endif				//__UI_SERVER_H
