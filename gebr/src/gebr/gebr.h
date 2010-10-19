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

/**
 * \file gebr.c General purpose functions
 */

#ifndef __GEBR_H
#define __GEBR_H

#include <gtk/gtk.h>

#include <libgebr/geoxml.h>
#include <libgebr/gui/gebr-gui-about.h>
#include <libgebr/log.h>

#include "cmdline.h"
#include "interface.h"
#include "ui_project_line.h"
#include "ui_flow.h"
#include "ui_flow_browse.h"
#include "ui_flow_edition.h"
#include "ui_job_control.h"
#include "ui_log.h"
#include "ui_preferences.h"
#include "ui_server.h"

G_BEGIN_DECLS

/**
 * The various tabs in GeBR interface.
 */
enum NOTEBOOK_PAGE {
	NOTEBOOK_PAGE_PROJECT_LINE = 0,
	NOTEBOOK_PAGE_FLOW_BROWSE,
	NOTEBOOK_PAGE_FLOW_EDITION,
	NOTEBOOK_PAGE_JOB_CONTROL,
};

/**
 * Global variable of common needed stuff.
 */
extern struct gebr gebr;

/**
 * Main program structure, containing configurations and interface widgets.
 */
struct gebr {
	GtkWidget *window;
	GtkWidget *menu[MENUBAR_N];
	GtkWidget *notebook;
	struct about about;
	GtkAccelGroup *accel_group;
	GtkActionGroup *action_group;

	/** \deprecated This should be avoided. Use Stock icons instead. */
	GtkWidget *invisible;			

	GHashTable * help_edit_windows;
	GHashTable * xmls_by_filename;

	GebrGeoXmlDocument *project_line;
	GebrGeoXmlProject *project;
	GebrGeoXmlLine *line;
	GebrGeoXmlFlow *flow;
	GebrGeoXmlProgram *program;
	GebrGeoXmlFlowServer *flow_server;

	GList *flow_clipboard;
	struct gebr_log *log;
	GSList *tmpfiles;

	/* Persistent GUI */
	GtkWidget *navigation_box_label;
	struct ui_project_line *ui_project_line;
	struct ui_flow_browse *ui_flow_browse;
	struct ui_flow_edition *ui_flow_edition;
	struct ui_job_control *ui_job_control;
	struct ui_log *ui_log;
	struct ui_server_list *ui_server_list;

	struct gebr_config {
		GKeyFile *key_file;
		GString *path;

		GString *username;
		GString *email;
		GString *usermenus;
		GString *data;
		GString *editor;
		int width;
		int height;
		gboolean log_expander_state;
		gboolean log_load;
		gboolean job_log_word_wrap;
		gboolean job_log_auto_scroll;
		gboolean native_editor;
		gint current_notebook;
		GString *project_line_string;
		GString *flow_string;

		// Detailed report options
		// - For flows:
		//  [ ] Include user's report
		//  [ ] Include parameter/value table
		//  (CSS Combo ^)
		GString * detailed_flow_css;
		gboolean detailed_flow_include_report;
		gboolean detailed_flow_include_params;

		// - For lines:
		//  [ ] Include user's report
		//  [ ] Include flow's report
		//    [ ] Include parameter/value table
		//  (CSS Combo ^)
		GString * detailed_line_css;
		gboolean detailed_line_include_report;
		gboolean detailed_line_include_flow_report;
		gboolean detailed_line_include_flow_params;

		gint print_option_flow_use_gebr_css;
		gboolean print_option_flow_include_flows;
		gboolean print_option_flow_detailed_report;
		gboolean print_option_line_use_gebr_css;
		gboolean print_option_line_include_flows;
		gboolean print_option_line_detailed_report;
	} config;

	/* Pixmaps */
	/** \deprecated Should use stock icons instead. */
	struct gebr_pixmaps {
		GdkPixbuf *stock_apply;
		GdkPixbuf *stock_warning;
		GdkPixbuf *stock_cancel;
		GdkPixbuf *stock_execute;
		GdkPixbuf *stock_connect;
		GdkPixbuf *stock_disconnect;
		GdkPixbuf *stock_go_back;
		GdkPixbuf *stock_go_forward;
		GdkPixbuf *stock_info;
		GdkPixbuf *chronometer;
	} pixmaps;				
};

/**
 * Take initial measures. This function is called when \ref gebr.window is shown.
 */
void gebr_init(void);

/**
 * Free memory, remove temporaries files and quit.
 */
gboolean gebr_quit(void);

/**
 * Initialize configuration for GeBR.
 */
int gebr_config_load(void);

/**
 * Populates the various data models, such as menus index and projects & lines.
 */
void gebr_config_apply(void);

/**
 * Save GeBR config to ~/.gebr/.gebr.conf file.
 * \param verbose If TRUE, report 'Configuration saved' in status bar.
 */
void gebr_config_save(gboolean verbose);

/**
 * Logs \p message in various ways depending on \p type, \p in_statusbar and \p in_log_file.
 * \param type The type of the log message.
 * \param in_statusbar If TRUE, shows message in status bar. Depending on \p type, use a different icon in front of the
 * message.
 * \param in_log_file If TRUE, appends \p message in log file (see ~/.gebr/log/ directory).
 * \param message A printf-like formated message.
 */
void gebr_message(enum gebr_log_message_type type, gboolean in_statusbar, gboolean in_log_file,
		  const gchar * message, ...);

/**
 * 
 */
void gebr_path_set_to(GString * path, gboolean relative);

/**
 * gebr_remove_help_edit_window:
 * @document:
 */
void gebr_remove_help_edit_window(gpointer document);

G_END_DECLS

#endif				//__GEBR_H
