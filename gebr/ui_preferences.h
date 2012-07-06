/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see
 *   <http://www.gnu.org/licenses/>.
 */

#ifndef __UI_PREFERENCES_H
#define __UI_PREFERENCES_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

struct ui_preferences {
	GtkWidget *dialog;
	GtkBuilder *builder;

	gboolean cancel_assistant;
	gboolean first_run;
	gboolean insert_preferences;
	gboolean tried_to_mount_gvfs;

	gint prev_page;
	GtkWidget *back_button;

	GtkWidget *username;
	GtkWidget *email;
	GtkWidget *usermenus;
	GtkWidget *editor;
	GtkWidget *browser;
	GtkWidget *log_load;
	GtkWidget *built_in_radio_button;
	GtkWidget *user_radio_button;
	GtkWidget *server_entry;
	GtkEntry *maestro_entry;
	gchar *maestro_addr;

	GtkListStore *maestro_chooser;
	GtkCellRenderer *renderer_chooser;
};

typedef enum {
	CANCEL_PAGE,
	INITIAL_PAGE,
	MAESTRO_INFO_PAGE,
	MAESTRO_PAGE,
	SERVERS_INFO_PAGE,
	SERVERS_PAGE,
	GVFS_PAGE,
} GebrUiPreferencesPage;

struct ui_preferences *preferences_setup_ui(gboolean first_run,
                                            gboolean wizard_run,
                                            gboolean insert_preferences,
                                            GebrUiPreferencesPage page);

G_END_DECLS
#endif				//__UI_PREFERENCES_H
