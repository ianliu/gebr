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

#include "gebr-maestro-server.h"

G_BEGIN_DECLS

typedef struct _GebrServerAutochoose GebrServerAutochoose;

struct _GebrServerAutochoose {
	GtkListStore *queues;
};

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
	SERVER_IS_AUTO_CHOOSE,
	SERVER_N_COLUMN
};

enum {
	TAG_NAME,
	TAG_ICON,
	TAG_SEP,
	TAG_FS
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
	GebrMaestroServer *maestro;
	struct ui_server_common common;
};

struct ui_server_list *server_list_setup_ui(void);

void server_list_show(struct ui_server_list *ui_server_list);

struct ui_server_select {
	struct ui_server_common common;
	GtkWidget *ok_button;
};

#if 0
/**
 * ui_server_list_tag:
 * @server: Pointer to an server
 *
 * Return tags associated to @server in the form of a gchar list
 */
gchar **
ui_server_list_tag (GebrServer *server);

/**
 * ui_server_servers_with_tag:
 * @tag: Tag to be seached
 *
 * Return a list with all servers with tag @tag
 */
GList *
ui_server_servers_with_tag (const gchar *tag);

/**
 * ui_server_has_tag:
 * @server: Pointer to an server
 * @tag: Tag to be seached
 *
 * Return TRUE if @server is associated to tag @tag, FALSE otherwise. If @server is NULL this
 * fuction returns FALSE.
 */
gboolean
ui_server_has_tag (GebrServer *server,
		   const gchar *tag);

/**
 * ui_server_set_tags:
 * @server: Pointer to an server
 * @str: String containing tags to be set
 *
 * Associate server @sever to the tags contained in a comma-separated-strint @str
 */
void
ui_server_set_tags (GebrServer *server,
		    const gchar *str);

/**
 * ui_server_get_all_tags:
 *
 * Return a list of all tags associated to the server selected
 * at the interface
 */
gchar **
ui_server_get_all_tags (void);

void ui_server_update_tags_combobox (void);

/**
 * ui_server_get_all_fsid:
 *
 * Return a list of all fsid (file system id) of all servers
 */
gchar **
ui_server_get_all_fsid (void);

GtkWidget *ui_server_create_tag_combo_box (void);

gboolean ui_server_ask_for_tags_remove_permission (void);

gchar *gebr_get_groups_of_server(gchar *server_address);
#endif

GtkListStore *gebr_ui_server_list_get_autochoose_store(struct ui_server_list *sl);

void gebr_ui_server_list_connect(struct ui_server_list *sl,
				 const gchar *addr);

G_END_DECLS

#endif				//__UI_SERVER_H
