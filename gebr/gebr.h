/*
 * gebr.h
 * This file is part of GêBR Project
 *
 * Copyright (C) 2007-2011 - GêBR Core Team (www.gebrproject.com)
 *
 * GêBR Project is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GêBR Project is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GêBR Project. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GEBR_H
#define __GEBR_H

#include <gtk/gtk.h>

#include <libgebr/log.h>
#include <libgebr/gebr-validator.h>
#include <libgebr/geoxml/geoxml.h>
#include <libgebr/gui/gebr-gui-about.h>

#include "cmdline.h"
#include "interface.h"
#include "ui_flow.h"
#include "ui_flow_browse.h"
#include "gebr-flow-edition.h"
#include "ui_help.h"
#include "gebr-job-control.h"
#include "ui_log.h"
#include "ui_preferences.h"
#include "ui_project_line.h"

#include "gebr-maestro-controller.h"

G_BEGIN_DECLS

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
	gint last_notebook;
	GtkAccelGroup *accel_group_array[ACCEL_N];
	GtkActionGroup *action_group_general;
	GtkActionGroup *action_group_project_line;
	GtkActionGroup *action_group_flow;
	GtkActionGroup *action_group_flow_edition;
	GtkActionGroup *action_group_job_control;
	GtkActionGroup *action_group_status;

	gboolean restore_selection;
	gboolean populate_list;
	gboolean quit;

	struct gebr_report {
		GtkWidget *report_wind;
		GtkActionGroup *report_group;
	} current_report;

	/** \deprecated This should be avoided. Use Stock icons instead. */
	GtkWidget *invisible;			

	GHashTable * help_edit_windows;
	GHashTable * xmls_by_filename;

	GtkAdjustment *flow_exec_adjustment;

	GebrValidator *validator;

	GebrGeoXmlDocument *project_line;
	GebrGeoXmlProject *project;
	GebrGeoXmlLine *line;
	GebrGeoXmlFlow *flow;
	GebrGeoXmlProgram *program;

	GList *flow_clipboard;
	GebrLog *log;
	GSList *tmpfiles;

	GebrMaestroController *maestro_controller;

	/* Persistent GUI */
	GtkWidget *navigation_box_label;
	struct ui_project_line *ui_project_line;
	GebrUiFlowBrowse *ui_flow_browse;
	GebrFlowEdition *ui_flow_edition;
	GebrJobControl *job_control;
	struct ui_log *ui_log;
	struct ui_server_list *ui_server_list;

	struct gebr_config {
		GKeyFile *key_file;
		GString *path;

		GKeyFile *key_file_maestro;
		GString *path_maestro;

		GString *version;

		GString *maestro_address;
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

		gdouble flow_exec_speed;
		gint niceness;

		// Selections state
		gint current_notebook;
		GString *project_line_filename;
		GString *flow_treepath_string;

		// Configurations for flow report generation
		GString *detailed_flow_css;
		gboolean detailed_flow_include_report;
		gboolean detailed_flow_include_revisions_report;
		GebrHelpParamTable detailed_flow_parameter_table;

		// Configurations for line report generation
		GString *detailed_line_css;
		gboolean detailed_line_include_report;
		gboolean detailed_line_include_flow_report;
		gboolean detailed_line_include_revisions_report;
		GebrHelpParamTable detailed_line_parameter_table;
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

void gebr_init(gboolean has_config);

/**
 * Free memory, remove temporaries files and quit.
 */
gboolean gebr_quit(gboolean save_config);

gboolean gebr_config_load(void);

/**
 * Populates the various data models, such as menus index and projects & lines.
 */
void gebr_config_apply(void);

/**
 * Save GeBR's Maestro config to ~/.gebr/.maestros.conf file.
 */
void gebr_config_maestro_save(void);

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
void gebr_message(GebrLogMessageType type, gboolean in_statusbar, gboolean in_log_file,
		  const gchar * message, ...);

/**
 * gebr_remove_help_edit_window:
 * @document:
 */
void gebr_remove_help_edit_window(GebrGeoXmlDocument * document);

const gchar *gebr_get_session_id(void);

gboolean gebr_has_maestro_config(void);

void restore_project_line_flow_selection(void);

G_END_DECLS

#endif				//__GEBR_H
