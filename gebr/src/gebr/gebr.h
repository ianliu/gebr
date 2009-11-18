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

#ifndef __GEBR_H
#define __GEBR_H

#include <gtk/gtk.h>

#include <libgebr/geoxml.h>
#include <libgebr/gui/about.h>
#include <libgebr/log.h>

#include "cmdline.h"
#include "interface.h"
#include "ui_project_line.h"
#include "ui_flow_browse.h"
#include "ui_flow_edition.h"
#include "ui_job_control.h"
#include "ui_log.h"
#include "ui_preferences.h"
#include "ui_server.h"

enum NOTEBOOK_PAGE {
	NOTEBOOK_PAGE_PROJECT_LINE = 0,
	NOTEBOOK_PAGE_FLOW_BROWSE,
	NOTEBOOK_PAGE_FLOW_EDITION,
	NOTEBOOK_PAGE_JOB_CONTROL,
};

/* global variable of common needed stuff */
extern struct gebr gebr;

struct gebr {
	GtkWidget *			window;
	GtkWidget *			menu[MENUBAR_N];
	GtkWidget *			notebook;
	struct about			about;
	GtkAccelGroup *			accel_group;
	GtkActionGroup *		action_group;

	/* for strange things ;) */
	GtkWidget *			invisible;

	GebrGeoXmlDocument *		project_line;
	GebrGeoXmlProject *			project;
	GebrGeoXmlLine *			line;
	GebrGeoXmlFlow *			flow;
	GebrGeoXmlProgram *			program;

	GList *				flow_clipboard;
	struct gebr_log *			log;
	GSList *			tmpfiles;

	/* Persistant GUI */
	GtkWidget *			navigation_box_label;
	struct ui_project_line *	ui_project_line;
	struct ui_flow_browse *		ui_flow_browse;
	struct ui_flow_edition *	ui_flow_edition;
	struct ui_job_control *		ui_job_control;
	struct ui_log *			ui_log;
	struct ui_server_list *		ui_server_list;

	struct gebr_config {
		GKeyFile *		key_file;
		GString *		path;

		GString *		username;
		GString *		email;
		GString *		usermenus;
		GString *		data;
		GString *		editor;
		GString *		browser;
		int			width;
		int			height;
		gboolean		log_expander_state;
		gboolean		log_load;
		gboolean		job_log_word_wrap;
		gboolean		job_log_auto_scroll;
	} config;

	/* Pixmaps */
	struct gebr_pixmaps {
		GdkPixbuf *		stock_apply;
		GdkPixbuf *		stock_warning;
		GdkPixbuf *		stock_cancel;
		GdkPixbuf *		stock_execute;
		GdkPixbuf *		stock_connect;
		GdkPixbuf *		stock_disconnect;
		GdkPixbuf *		stock_go_back;
		GdkPixbuf *		stock_go_forward;
		GdkPixbuf *		stock_info;
	} pixmaps;
};

void
gebr_init(void);

gboolean
gebr_quit(void);

int
gebr_config_load(gboolean nox);

void
gebr_config_apply(void);

void
gebr_config_save(gboolean verbose);

void
gebr_message(enum gebr_log_message_type type, gboolean in_statusbar, gboolean in_log_file, const gchar * message, ...);

int
gebr_install_private_menus(gchar **menu, gboolean overwrite);

#endif //__GEBR_H
