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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include <glib/gstdio.h>
#include <glib/gi18n.h>

#include <libgebr.h>
#include <glib/gi18n.h>
#include <libgebr/utils.h>
#include <libgebr/date.h>
#include <libgebr/gebr-validator.h>
#include <libgebr/gui/gebr-gui-utils.h>

#include "gebr.h"
#include "project.h"
#include "menu.h"
#include "callbacks.h"
#include "document.h"
#include "flow.h"
#include "ui_project_line.h"
#include "gebr-maestro-server.h"

struct gebr gebr;

static void gebr_log_load(void);

static void gebr_migrate_data_dir(void);

static gboolean gebr_config_load_from_gengetopt(void);

static void gebr_post_config(gboolean has_config);

static gchar *gebr_generate_session_id(void);

static gchar *SESSIONID = NULL;

static void
gebr_force_quit(int sig)
{
	gebr_quit(TRUE);
}

/**
 * gebr_init:
 *
 * This method initializes some widgets of the interface, such as the server
 * list.
 */
void
gebr_init(gboolean has_config)
{
	SESSIONID = gebr_generate_session_id();
	g_debug("Session id: %s", SESSIONID);

	struct sigaction sa;
	sa.sa_handler = gebr_force_quit;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGTERM, &sa, NULL)
			|| sigaction(SIGINT, &sa, NULL)
			|| sigaction(SIGQUIT, &sa, NULL)
			|| sigaction(SIGSEGV, &sa, NULL))
		perror("sigaction");

	gebr.project_line = NULL;
	gebr.project = NULL;
	gebr.line = NULL;
	gebr.flow = NULL;
	gebr.program = NULL;
	gebr.flow_clipboard = NULL;

	gebr.current_report.report_wind = NULL;
	gebr.current_report.report_group = NULL;

	gebr.quit = FALSE;

	/* check/create config dir */
	if (gebr_create_config_dirs() == FALSE) {
		fprintf(stderr, _("Unable to create GêBR configuration files.\n"
				  "You might not have write permission in your own\n"
				  "home directory or there is not enough space left on the device.\n"));
		exit(-1);
	}

	gebr.tmpfiles = NULL;

	gebr.help_edit_windows = g_hash_table_new(NULL, NULL);
	gebr.xmls_by_filename = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	gebr.validator = gebr_validator_new((GebrGeoXmlDocument**)&gebr.flow,
	                                    (GebrGeoXmlDocument**)&gebr.line,
	                                    (GebrGeoXmlDocument**)&gebr.project);

	/* icons */
	gebr.invisible = gtk_invisible_new();
	gebr.pixmaps.stock_cancel =
	    gtk_widget_render_icon(gebr.invisible, GTK_STOCK_CANCEL, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	gebr.pixmaps.stock_apply =
	    gtk_widget_render_icon(gebr.invisible, GTK_STOCK_APPLY, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	gebr.pixmaps.stock_warning =
	    gtk_widget_render_icon(gebr.invisible, GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	gebr.pixmaps.stock_execute =
	    gtk_widget_render_icon(gebr.invisible, GTK_STOCK_EXECUTE, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	gebr.pixmaps.stock_connect =
	    gtk_widget_render_icon(gebr.invisible, GTK_STOCK_CONNECT, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	gebr.pixmaps.stock_disconnect =
	    gtk_widget_render_icon(gebr.invisible, GTK_STOCK_DISCONNECT, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	gebr.pixmaps.stock_go_back =
	    gtk_widget_render_icon(gebr.invisible, GTK_STOCK_GO_BACK, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	gebr.pixmaps.stock_go_forward =
	    gtk_widget_render_icon(gebr.invisible, GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	gebr.pixmaps.stock_info =
	    gtk_widget_render_icon(gebr.invisible, GTK_STOCK_INFO, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	gebr.pixmaps.chronometer = 
	    gtk_widget_render_icon(gebr.invisible, "chronometer", GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);

	gebr_post_config(has_config);

	gtk_adjustment_set_value(gebr.flow_exec_adjustment,
				 gebr_interface_calculate_slider_from_speed(gebr.config.flow_exec_speed));
	gtk_adjustment_value_changed(gebr.flow_exec_adjustment);

	/* check for a menu list change */
	//TODO: DO IT!
//	if (menu_refresh_needed() == TRUE)
		menu_list_populate();
}

static void
free_flow_jobs_list(gpointer key, gpointer value)
{
	GList *list = value;
	g_list_free(list);
}

gboolean gebr_quit(gboolean save_config)
{
	g_free(SESSIONID);

	if (save_config)
		gebr_config_save(FALSE);

	if (gebr.flow_clipboard != NULL) {
		g_list_foreach(gebr.flow_clipboard, (GFunc) g_free, NULL);
		g_list_free(gebr.flow_clipboard);
	}

	g_hash_table_destroy(gebr.help_edit_windows);
	g_hash_table_destroy(gebr.xmls_by_filename);

	gebr_validator_free(gebr.validator);
	gebr.validator = NULL;

	/*
	 * Data frees and cleanups
	 */
	gtk_tree_view_set_model(GTK_TREE_VIEW(gebr.ui_flow_browse->view), NULL);
	flow_free();
	project_line_free();

	gebr_comm_process_kill(gebr.ui_flow_browse->graph_process);
	g_list_free(gebr.ui_flow_browse->select_flows);
	g_hash_table_foreach(gebr.ui_flow_browse->flow_jobs, (GHFunc)free_flow_jobs_list, NULL);
	g_hash_table_destroy(gebr.ui_flow_browse->flow_jobs);

	/* free config stuff */
	g_key_file_free(gebr.config.key_file);
	g_string_free(gebr.config.path, TRUE);
	g_key_file_free(gebr.config.key_file_maestro);
	g_string_free(gebr.config.path_maestro, TRUE);
	g_string_free(gebr.config.username, TRUE);
	g_string_free(gebr.config.email, TRUE);
	g_string_free(gebr.config.editor, TRUE);
	g_string_free(gebr.config.usermenus, TRUE);
	g_string_free(gebr.config.data, TRUE);
	g_string_free(gebr.config.project_line_filename, TRUE);
	g_string_free(gebr.config.flow_treepath_string, TRUE);
	g_string_free(gebr.config.detailed_flow_css, TRUE);
	g_string_free(gebr.config.detailed_line_css, TRUE);

	/* remove temporaries files */
	g_slist_foreach(gebr.tmpfiles, (GFunc) g_unlink, NULL);
	g_slist_foreach(gebr.tmpfiles, (GFunc) g_free, NULL);
	g_slist_free(gebr.tmpfiles);

	GString *path = g_string_new(NULL);
	g_string_printf(path, "%s/.gebr/tmp/*.html", g_get_home_dir());
	gebr_temp_directory_destroy(path);

	gebr_message(GEBR_LOG_END, TRUE, TRUE, _("Finalizing GêBR..."));
	gebr_log_close(gebr.log);

	/*
	 * Interface frees
	 */

	if (gebr.ui_project_line->store)
		g_object_unref (gebr.ui_project_line->store);
	if (gebr.ui_project_line->servers_filter)
		g_object_unref (gebr.ui_project_line->servers_filter);
	if (gebr.ui_project_line->servers_sort)
		g_object_unref (gebr.ui_project_line->servers_sort);

	g_free(gebr.ui_project_line);
	g_free(gebr.ui_flow_browse);
	gebr_job_control_free(gebr.job_control);
	g_free(gebr.ui_server_list);

	g_object_unref(gebr.pixmaps.stock_cancel);
	g_object_unref(gebr.pixmaps.stock_apply);
	g_object_unref(gebr.pixmaps.stock_warning);
	g_object_unref(gebr.pixmaps.stock_execute);
	g_object_unref(gebr.pixmaps.stock_connect);
	g_object_unref(gebr.pixmaps.stock_disconnect);
	g_object_unref(gebr.pixmaps.stock_go_back);
	g_object_unref(gebr.pixmaps.stock_go_forward);
	g_object_unref(gebr.pixmaps.stock_info);
	gtk_widget_destroy(gebr.invisible);

	g_object_unref(gebr.accel_group_array[ACCEL_PROJECT_AND_LINE]);
	g_object_unref(gebr.accel_group_array[ACCEL_FLOW]);
	g_object_unref(gebr.accel_group_array[ACCEL_FLOW_EDITION]);
	g_object_unref(gebr.accel_group_array[ACCEL_JOB_CONTROL]);
	g_object_unref(gebr.accel_group_array[ACCEL_GENERAL]);
	g_object_unref(gebr.accel_group_array[ACCEL_STATUS]);
	g_object_unref(gebr.action_group_general);
	g_object_unref(gebr.action_group_project_line);
	g_object_unref(gebr.action_group_flow);
	g_object_unref(gebr.action_group_flow_edition);
	g_object_unref(gebr.action_group_job_control);
	g_object_unref(gebr.action_group_status);

	gtk_main_quit();

	return FALSE;
}

gboolean
gebr_load_maestro_config(void)
{
	gboolean has_config;

	gebr.config.path_maestro = g_string_new(NULL);
	g_string_printf(gebr.config.path_maestro, "%s/.gebr/gebr/maestros.conf", g_get_home_dir());
	has_config = g_access(gebr.config.path_maestro->str, F_OK | R_OK) == 0 ? TRUE : FALSE;
	gebr.config.key_file_maestro = g_key_file_new();

	if (has_config)
		/* For the sake of backwards compatibility, load deprecated gengetopt file format ... */
		if (!g_key_file_load_from_file(gebr.config.key_file_maestro, gebr.config.path_maestro->str, G_KEY_FILE_NONE, NULL))
			return FALSE;

	gebr.maestro_controller = gebr_maestro_controller_new();
	gebr.config.maestro_address  = gebr_g_key_file_load_string_key(gebr.config.key_file_maestro, "maestro", "address", g_get_host_name());

	return TRUE;
}

/**
 * gebr_config_load:
 *
 * Loads ~/.gebr/gebr.conf file fill the global gebr.config structure.
 *
 * Returns: %TRUE if the configuration exists, %FALSE otherwise.
 */
gboolean
gebr_config_load(void)
{
	gboolean has_config;
	gchar *usermenus = g_strdup_printf("%s/GeBR-Menus", g_get_home_dir());
	gchar *datadir = g_strdup_printf("%s/.gebr/gebr/data", g_get_home_dir());

	gebr.config.path = g_string_new(NULL);
	g_string_printf(gebr.config.path, "%s/.gebr/gebr/gebr.conf", g_get_home_dir());
	has_config = g_access(gebr.config.path->str, F_OK | R_OK) == 0 ? TRUE : FALSE;
	gebr.config.key_file = g_key_file_new();

	if (has_config)
		/* For the sake of backwards compatibility, load deprecated gengetopt file format ... */
		if (!g_key_file_load_from_file(gebr.config.key_file, gebr.config.path->str, G_KEY_FILE_NONE, NULL))
			gebr_config_load_from_gengetopt();

	gebr_load_maestro_config();

	gebr.config.version   = gebr_g_key_file_load_string_key(gebr.config.key_file, "general", "version", "None");
	gebr.config.username  = gebr_g_key_file_load_string_key(gebr.config.key_file, "general", "name", g_get_real_name());
	gebr.config.email     = gebr_g_key_file_load_string_key(gebr.config.key_file, "general", "email", g_get_user_name());
	gebr.config.usermenus = gebr_g_key_file_load_string_key(gebr.config.key_file, "general", "usermenus", usermenus);
	gebr.config.data      = gebr_g_key_file_load_string_key(gebr.config.key_file, "general", "data", datadir);
	gebr.config.editor    = gebr_g_key_file_load_string_key(gebr.config.key_file, "general", "editor", "");
	gebr.config.native_editor = gebr_g_key_file_load_boolean_key(gebr.config.key_file, "general", "native_editor", TRUE);
	gebr.config.width     = gebr_g_key_file_load_int_key(gebr.config.key_file, "general", "width", 700);
	gebr.config.height    = gebr_g_key_file_load_int_key(gebr.config.key_file, "general", "height", 400);
	gebr.config.log_load  = gebr_g_key_file_load_boolean_key(gebr.config.key_file, "general", "log_load", FALSE);
	gebr.config.log_expander_state = gebr_g_key_file_load_boolean_key(gebr.config.key_file, "general", "log_expander_state", FALSE);
	gebr.config.job_log_word_wrap = gebr_g_key_file_load_boolean_key(gebr.config.key_file, "general", "job_log_word_wrap", FALSE);
	gebr.config.job_log_auto_scroll = gebr_g_key_file_load_boolean_key(gebr.config.key_file, "general", "job_log_auto_scroll", FALSE);
	gebr.config.detailed_flow_include_report = gebr_g_key_file_load_boolean_key (gebr.config.key_file, "general", "detailed_flow_include_report", FALSE);
	gebr.config.detailed_flow_include_revisions_report = gebr_g_key_file_load_boolean_key (gebr.config.key_file, "general", "detailed_flow_include_revisions_report", FALSE);
	gebr.config.detailed_flow_parameter_table = gebr_g_key_file_load_int_key (gebr.config.key_file, "general", "detailed_flow_parameter_table", GEBR_PARAM_TABLE_ONLY_CHANGED);
	gebr.config.detailed_line_include_report = gebr_g_key_file_load_boolean_key (gebr.config.key_file, "general", "detailed_line_include_report", TRUE);
	gebr.config.detailed_line_include_flow_report = gebr_g_key_file_load_boolean_key (gebr.config.key_file, "general", "detailed_line_include_flow_report", FALSE);
	gebr.config.detailed_line_include_revisions_report = gebr_g_key_file_load_boolean_key (gebr.config.key_file, "general", "detailed_line_include_revisions_report", FALSE);
	gebr.config.detailed_line_parameter_table = gebr_g_key_file_load_int_key (gebr.config.key_file, "general", "detailed_line_parameter_table", GEBR_PARAM_TABLE_ONLY_CHANGED);
	gebr.config.flow_exec_speed = g_key_file_get_double(gebr.config.key_file, "general", "flow_exec_speed", NULL);
	gebr.config.niceness = gebr_g_key_file_load_int_key(gebr.config.key_file, "general", "niceness", 1);
	gebr.config.detailed_flow_css = gebr_g_key_file_load_string_key(gebr.config.key_file, "general", "detailed_flow_css", "gebr-report.css");
	gebr.config.detailed_line_css = gebr_g_key_file_load_string_key(gebr.config.key_file, "general", "detailed_line_css", "gebr-report.css");

	/*gebr_g_key_file_load_int_key
	 * For the sake of backwards compatibility...
	 */
	if (gebr_g_key_file_has_key(gebr.config.key_file, "general", "notebook")) {
		gebr.config.current_notebook = gebr_g_key_file_load_int_key(gebr.config.key_file, "general", "notebook", 0);
		gebr.config.project_line_filename = g_string_new("");
		gebr.config.flow_treepath_string = gebr_g_key_file_load_string_key(gebr.config.key_file, "general", "flow_string", "");
	} else {
		gebr.config.current_notebook = gebr_g_key_file_load_int_key(gebr.config.key_file, "state", "notebook", 0);
		gebr.config.project_line_filename = gebr_g_key_file_load_string_key(gebr.config.key_file, "state", "project_line_filename", "");
		gebr.config.flow_treepath_string = gebr_g_key_file_load_string_key(gebr.config.key_file, "state", "flow_treepath_string", "");
	}

	g_free(datadir);
	g_free(usermenus);

	return has_config;
}

void
restore_project_line_flow_selection(void)
{
	GtkTreeIter iter;
	gboolean project_line_selected;

	if (gebr_g_key_file_has_key(gebr.config.key_file, "general", "notebook")) {
		GString *project_line_string = gebr_g_key_file_load_string_key(gebr.config.key_file, "general", "project_line_string", "");
		project_line_selected = (project_line_string->len &&
					 gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(gebr.ui_project_line->store),
									     &iter, project_line_string->str));
		g_string_free(project_line_string, FALSE);
	} else {
		project_line_selected = (gebr.config.project_line_filename->len &&
					 gebr_gui_gtk_tree_model_find_by_column(GTK_TREE_MODEL(gebr.ui_project_line->store), &iter,
										PL_FILENAME, gebr.config.project_line_filename->str));
	}

	if (project_line_selected) {
		project_line_select_iter(&iter);
		GtkTreeIter flow_iter;

		if (gebr.config.flow_treepath_string->len &&
		    gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &flow_iter,
							gebr.config.flow_treepath_string->str))
		{
			gtk_notebook_set_current_page(GTK_NOTEBOOK(gebr.notebook), gebr.config.current_notebook);
			flow_browse_select_iter(&flow_iter);
		}
	}
}

/*
 * The config load order must be: log, load file servers, projects/line/flows,
 * menus. New config check must be after all default values loaded.
 */
static void
gebr_post_config(gboolean has_config)
{
	// Generate gebr.key
	gebr_generate_key();

	gebr_log_load();
	g_object_set(gebr.maestro_controller, "window", gebr.window, NULL);
	gebr_message(GEBR_LOG_START, TRUE, TRUE, _("Initiating GêBR..."));

	gebr_path_resolve_home_variable(gebr.config.usermenus);
	if (!g_file_test(gebr.config.usermenus->str, G_FILE_TEST_EXISTS))
		g_mkdir_with_parents(gebr.config.usermenus->str, 0755);

	/* DEPRECATED: old directory structure migration */
	if (g_str_has_suffix(gebr.config.data->str, ".gebr/gebrdata"))
		g_string_printf(gebr.config.data, "%s/.gebr/gebr/data", g_get_home_dir());
	else if (!g_str_has_suffix(gebr.config.data->str, ".gebr/gebr/data"))
		gebr_migrate_data_dir();
	else
		gebr_path_resolve_home_variable(gebr.config.data);

	gtk_window_resize(GTK_WINDOW(gebr.window), gebr.config.width, gebr.config.height);
	gtk_expander_set_expanded(GTK_EXPANDER(gebr.ui_log->widget), gebr.config.log_expander_state);

	menu_list_populate();
	if (!has_config)
		preferences_setup_ui(TRUE, TRUE, TRUE, -1);
	else {
		gebr.restore_selection = FALSE;
		gebr.populate_list = FALSE;

		if (gebr_has_maestro_config() && g_strcmp0(gebr.config.version->str, "None")) {
			gebr.populate_list = TRUE;
			project_list_populate();
			gebr_maestro_controller_connect(gebr.maestro_controller,
			                                gebr.config.maestro_address->str);
			gebr_config_save(FALSE);
		} else {
			preferences_setup_ui(TRUE, TRUE, FALSE, -1);
		}
	}
}

void gebr_config_apply(void)
{
	menu_list_populate();
}

void
gebr_config_maestro_save(void)
{
	gsize length;
	gchar *string;
	FILE *configfp;

	g_key_file_free(gebr.config.key_file_maestro);
	gebr.config.key_file_maestro = g_key_file_new();

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);
	if (maestro && gebr_maestro_server_get_state(maestro) == SERVER_STATE_LOGGED) {
		const gchar *maestro_addr = gebr_maestro_server_get_address(maestro);
		g_key_file_set_string(gebr.config.key_file_maestro, "maestro", "address", maestro_addr);
	} else
		return;

	string = g_key_file_to_data(gebr.config.key_file_maestro, &length, NULL);
	configfp = fopen(gebr.config.path_maestro->str, "w");
	if (configfp == NULL) {
		gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Could not save configuration."));
		goto out;
	}
	fwrite(string, sizeof(gchar), length, configfp);
	fclose(configfp);

out:
	g_free(string);
	return;
}

void gebr_config_save(gboolean verbose)
{
	GtkTreeIter iter;

	gsize length;
	gchar *string;
	FILE *configfp;
	
	gebr_config_maestro_save();

	/* reset key_file, cause we do not sync servers automatically */
	g_key_file_free(gebr.config.key_file);
	gebr.config.key_file = g_key_file_new();

	g_key_file_set_string(gebr.config.key_file, "general", "name", gebr.config.username->str);
	g_key_file_set_string(gebr.config.key_file, "general", "email", gebr.config.email->str);
	g_key_file_set_string(gebr.config.key_file, "general", "editor", gebr.config.editor->str);
	g_key_file_set_boolean(gebr.config.key_file, "general", "native_editor", gebr.config.native_editor);
	g_key_file_set_string(gebr.config.key_file, "general", "version", GEBR_VERSION);

	GString *home_variable;
	home_variable = g_string_new(NULL);
	g_string_assign(home_variable, gebr.config.data->str);
	gebr_path_use_home_variable(home_variable);
	g_key_file_set_string(gebr.config.key_file, "general", "data", home_variable->str);
	g_string_assign(home_variable, gebr.config.usermenus->str);
	gebr_path_use_home_variable(home_variable);
	g_key_file_set_string(gebr.config.key_file, "general", "usermenus", home_variable->str);
	g_string_free(home_variable, TRUE);

	gtk_window_get_size(GTK_WINDOW(gebr.window), &gebr.config.width, &gebr.config.height);
	gebr.config.log_expander_state = gtk_expander_get_expanded(GTK_EXPANDER(gebr.ui_log->widget));
	g_key_file_set_integer(gebr.config.key_file, "general", "width", gebr.config.width);
	g_key_file_set_integer(gebr.config.key_file, "general", "height", gebr.config.height);
	g_key_file_set_boolean(gebr.config.key_file, "general", "log_expander_state", gebr.config.log_expander_state);
	g_key_file_set_boolean(gebr.config.key_file, "general", "log_load", gebr.config.log_load);
	g_key_file_set_boolean(gebr.config.key_file, "general", "job_log_word_wrap", gebr.config.job_log_word_wrap);
	g_key_file_set_boolean(gebr.config.key_file, "general", "job_log_auto_scroll", gebr.config.job_log_auto_scroll);

	g_key_file_set_string (gebr.config.key_file, "general", "detailed_flow_css", gebr.config.detailed_flow_css->str);
	g_key_file_set_boolean (gebr.config.key_file, "general", "detailed_flow_include_report", gebr.config.detailed_flow_include_report);
	g_key_file_set_boolean (gebr.config.key_file, "general", "detailed_flow_include_revisions_report", gebr.config.detailed_flow_include_revisions_report);
	g_key_file_set_integer (gebr.config.key_file, "general", "detailed_flow_parameter_table", gebr.config.detailed_flow_parameter_table);

	g_key_file_set_string (gebr.config.key_file, "general", "detailed_line_css", gebr.config.detailed_line_css->str);
	g_key_file_set_boolean (gebr.config.key_file, "general", "detailed_line_include_report", gebr.config.detailed_line_include_report);
	g_key_file_set_boolean (gebr.config.key_file, "general", "detailed_line_include_flow_report", gebr.config.detailed_line_include_flow_report);
	g_key_file_set_boolean (gebr.config.key_file, "general", "detailed_line_include_revisions_report", gebr.config.detailed_line_include_revisions_report);
	g_key_file_set_integer (gebr.config.key_file, "general", "detailed_line_parameter_table", gebr.config.detailed_line_parameter_table);
	g_key_file_set_double (gebr.config.key_file, "general", "flow_exec_speed", gebr.config.flow_exec_speed);
	g_key_file_set_integer(gebr.config.key_file, "general", "niceness", gebr.config.niceness);

	g_key_file_set_integer(gebr.config.key_file, "state", "notebook", gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook)));

	/* Save selected documents */
	if (project_line_get_selected(&iter, DontWarnUnselection)) {
		gchar *str;
		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->store), &iter, PL_FILENAME, &str, -1);
		g_key_file_set_string(gebr.config.key_file, "state", "project_line_filename", str);
		g_free(str);

		if (flow_browse_get_selected(&iter, FALSE)) {
			GtkTreeIter parent;
			if (gtk_tree_model_iter_parent(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &parent, &iter))
				g_string_assign(gebr.config.flow_treepath_string, gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &parent));
			else
				g_string_assign(gebr.config.flow_treepath_string, gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter));
			g_key_file_set_string(gebr.config.key_file, "state", "flow_treepath_string", gebr.config.flow_treepath_string->str);
		}
	}

	int count = 0;
	/* Save order of projects */
	gebr_gui_gtk_tree_model_foreach_hyg(iter, GTK_TREE_MODEL(gebr.ui_project_line->store), 1) {
		gchar *filename = NULL;
		gchar *title = NULL;
		GString *compose = g_string_new(NULL);

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->store), &iter,
				   PL_FILENAME, &filename,
				   PL_TITLE, &title, -1);
		g_string_printf(compose, "project-%d", count++);
		g_key_file_set_string(gebr.config.key_file, "projects", compose->str, filename);
	}

	string = g_key_file_to_data(gebr.config.key_file, &length, NULL);
	configfp = fopen(gebr.config.path->str, "w");
	if (configfp == NULL) {
		gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Could not save configuration."));
		goto out;
	}
	fwrite(string, sizeof(gchar), length, configfp);
	fclose(configfp);

	if (verbose)
		gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Configuration saved"));

out:
	g_free(string);
	return;
}

void gebr_message(GebrLogMessageType type, gboolean in_statusbar, gboolean in_log_file,
		  const gchar * message, ...)
{
	gchar *string;
	va_list argp;

#ifndef DEBUG
	if (type == GEBR_LOG_DEBUG)
		return;
#endif
	va_start(argp, message);
	string = g_strdup_vprintf(message, argp);
	va_end(argp);

	if (type == GEBR_LOG_DEBUG)
		g_print("DEBUG: %s\n", string);
	else if (in_statusbar)
		log_set_message(gebr.ui_log, string);
	if (in_log_file) {
		GebrLogMessage *log_message;

		log_message = gebr_log_message_new(type, gebr_iso_date(), string);
		gebr_log_add_message(gebr.log, type, string);
		gebr_log_add_message_to_list(gebr.ui_log, log_message);

		gebr_log_message_free(log_message);
	}

	g_free(string);
}

static void
remove_window(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return;
	GtkWidget * window = g_hash_table_lookup(gebr.help_edit_windows, document);
	if (window)
		gtk_widget_destroy(window);
}

void gebr_remove_help_edit_window(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return;

	switch (gebr_geoxml_document_get_type(document)) {
	case GEBR_GEOXML_DOCUMENT_TYPE_PROJECT: {
		GebrGeoXmlProject *project = GEBR_GEOXML_PROJECT(document);
		GebrGeoXmlSequence * i;
		for (gebr_geoxml_project_get_line(project, &i, 0); i != NULL; gebr_geoxml_sequence_next(&i))
			gebr_remove_help_edit_window(g_hash_table_lookup(gebr.xmls_by_filename, 
									 gebr_geoxml_project_get_line_source(GEBR_GEOXML_PROJECT_LINE(i))));
		remove_window(document);
		break;
	} case GEBR_GEOXML_DOCUMENT_TYPE_LINE: {
		GebrGeoXmlLine *line = GEBR_GEOXML_LINE(document);
		GebrGeoXmlSequence * i;
		for (gebr_geoxml_line_get_flow(line, &i, 0); i != NULL; gebr_geoxml_sequence_next(&i))
			remove_window(g_hash_table_lookup(gebr.xmls_by_filename, 
							  gebr_geoxml_line_get_flow_source(GEBR_GEOXML_LINE_FLOW(i))));
		remove_window(document);
		break;
	} case GEBR_GEOXML_DOCUMENT_TYPE_FLOW:
		remove_window(document);
		break;
	default:
		break;
	}
}

/*
 * Private functions
 */

/**
 * \internal
 * Initialize log on GUI.
 */
static void gebr_log_load(void)
{
	GString *log_filename;
	GList *i;

	log_filename = g_string_new(NULL);
	g_string_printf(log_filename, "%s/.gebr/log/gebr.log", g_get_home_dir());
	gebr.log = gebr_log_open(log_filename->str);

	if (gebr.config.log_load) {
		GList *messages;

		messages = gebr_log_messages_read(gebr.log);
		for (i = messages; i != NULL; i = g_list_next(i))
			gebr_log_add_message_to_list(gebr.ui_log, (GebrLogMessage *)i->data);

		gebr_log_messages_free(messages);
	}

	/* frees */
	g_string_free(log_filename, TRUE);
}

/**
 * \internal
 * Backwards compatibility function for old configuration file format.
 * \return FALSE if could not parse config with gengetopt.
 */
static gboolean gebr_config_load_from_gengetopt(void)
{
	struct ggopt ggopt;
	gint i;

	if (cmdline_parser_configfile(gebr.config.path->str, &ggopt, 1, 1, 0) != 0)
		return FALSE;

	/* WARNING! don't add new options here, this is for the rather old format */

	/* generates a new version compatible key_file */
	g_key_file_set_string(gebr.config.key_file, "general", "name", ggopt.name_arg);
	g_key_file_set_string(gebr.config.key_file, "general", "email", ggopt.email_arg);
	g_key_file_set_string(gebr.config.key_file, "general", "usermenus", ggopt.usermenus_arg);
	g_key_file_set_string(gebr.config.key_file, "general", "data", ggopt.data_arg);
	g_key_file_set_string(gebr.config.key_file, "general", "editor", ggopt.editor_arg);
	g_key_file_set_integer(gebr.config.key_file, "general", "width", ggopt.width_arg);
	g_key_file_set_integer(gebr.config.key_file, "general", "height", ggopt.height_arg);
	g_key_file_set_boolean(gebr.config.key_file, "general", "log_expander_state", (gboolean) ggopt.logexpand_given);
	for (i = 0; i < ggopt.server_given; ++i) {
		GString *group = g_string_new(NULL);
		g_string_printf(group, "server-%s", ggopt.server_arg[i]);
		g_key_file_set_string(gebr.config.key_file, group->str, "address", ggopt.server_arg[i]);
		g_string_free(group, TRUE);
	}

	return TRUE;
}

/**
 * \internal
 * Initialize configuration for GeBR.
 */
static void gebr_migrate_data_dir(void)
{
	GtkWidget *dialog;
	GString *new_data_dir;
	GString *command_line;
	gchar *filename;
	gboolean empty;

	new_data_dir = g_string_new(NULL);
	g_string_printf(new_data_dir, "%s/.gebr/gebr/data", g_get_home_dir());

	command_line = g_string_new("");
	gebr_directory_foreach_file(filename, gebr.config.data->str)
		if (!fnmatch("*.prj", filename, 1) || !fnmatch("*.lne", filename, 1) || !fnmatch("*.flw", filename, 1))
			g_string_append_printf(command_line, "%s/%s ", gebr.config.data->str, filename);
	empty = command_line->len == 0 ? TRUE : FALSE;
	g_string_prepend(command_line, "cp ");
	g_string_append(command_line, new_data_dir->str);

	if (empty || !gebr_system(command_line->str)) {
		dialog = gtk_message_dialog_new(GTK_WINDOW(gebr.window),
						(GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
						GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
						_("GêBR now stores data files on its own directory. You may now delete GêBR's files at %s."),
						gebr.config.data->str);
		g_string_assign(gebr.config.data, new_data_dir->str);
	} else {
		dialog = gtk_message_dialog_new(GTK_WINDOW(gebr.window),
						(GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
						GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
						_("Cannot copy data files to GêBR's directory. Please check your write permissions in your home directory."));
	}
	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));

	g_string_free(command_line, TRUE);
	g_string_free(new_data_dir, TRUE);
	gtk_widget_destroy(dialog);
}

// FIXME How can we improve the session id?
static gchar *
gebr_generate_session_id(void)
{
	GTimeVal tv;
	g_get_current_time(&tv);
	return g_strdup_printf("%ld", tv.tv_usec);
}

const gchar *
gebr_get_session_id(void)
{
	return SESSIONID;
}

gboolean
gebr_has_maestro_config(void)
{
	gboolean has_config;
	GString *path = g_string_new(NULL);

	g_string_printf(path, "%s/.gebr/gebr/maestros.conf", g_get_home_dir());

	has_config = g_access(path->str, F_OK | R_OK) == 0 ? TRUE : FALSE;

	g_string_free(path, TRUE);

	return has_config;
}
