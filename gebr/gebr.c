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

#include <libgebr.h>
#include <glib/gi18n.h>
#include <libgebr/utils.h>
#include <libgebr/date.h>
#include <libgebr/gebr-validator.h>
#include <libgebr/gui/gebr-gui-utils.h>

#include "gebr.h"
#include "project.h"
#include "server.h"
#include "job.h"
#include "menu.h"
#include "callbacks.h"
#include "document.h"
#include "flow.h"
#include "ui_project_line.h"

/*
 * Prototypes
 */

struct gebr gebr;

static void gebr_log_load(void);

static void gebr_migrate_data_dir(void);

static gboolean gebr_config_load_from_gengetopt(void);

static void gebr_config_load_servers(void);

static void gebr_config_save_servers(void);

#define SERVERS_PATH ".gebr/gebr/servers.conf"

/*
 * Public functions
 */

void gebr_init(void)
{
	gebr.project_line = NULL;
	gebr.project = NULL;
	gebr.line = NULL;
	gebr.flow = NULL;
	gebr.program = NULL;
	gebr.flow_clipboard = NULL;

	/* check/create config dir */
	if (gebr_create_config_dirs() == FALSE) {
		fprintf(stderr, _("Unable to create GêBR configuration files.\n"
				  "Perhaps you do not have write permission to your own\n"
				  "home directory or there is no space left on device.\n"));
		exit(-1);
	}

	gebr.config.detailed_flow_css = g_string_new (NULL);
	gebr.config.detailed_line_css = g_string_new (NULL);
	gebr.config.path = g_string_new(NULL);
	g_string_printf(gebr.config.path, "%s/.gebr/gebr/gebr.conf", g_get_home_dir());

	/* allocating list of temporary files */
	gebr.tmpfiles = g_slist_alloc();

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

	/* finally the config. file */
	gebr_config_load();

	/* check for a menu list change */
	if (menu_refresh_needed() == TRUE) {
		menu_list_create_index();
		menu_list_populate();
	}
}

gboolean gebr_quit(gboolean save_config)
{
	GtkTreeIter iter;

	if (save_config)
		gebr_config_save(FALSE);

	/* Free servers structs */
	gebr_gui_gtk_tree_model_foreach_hyg(iter, GTK_TREE_MODEL(gebr.ui_server_list->common.store), 1) {
		GebrServer *server;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->common.store), &iter,
				   SERVER_POINTER, &server, -1);
		server_free(server);
	}
	job_control_clear(TRUE);

	if (gebr.flow_clipboard != NULL) {
		g_list_foreach(gebr.flow_clipboard, (GFunc) g_free, NULL);
		g_list_free(gebr.flow_clipboard);
	}

	g_object_unref (gebr.ui_server_list->common.combo_store);
	g_hash_table_destroy(gebr.help_edit_windows);
	g_hash_table_destroy(gebr.xmls_by_filename);

	gebr_validator_free(gebr.validator);
	gebr.validator = NULL;

	/*
	 * Data frees and cleanups
	 */
	flow_free();
	project_line_free();

	/* free config stuff */
	g_key_file_free(gebr.config.key_file);
	g_string_free(gebr.config.path, TRUE);
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

	gebr_message(GEBR_LOG_END, TRUE, TRUE, _("GêBR finalizing..."));
	gebr_log_close(gebr.log);

	/*
	 * Interface frees
	 */

	g_object_unref (gebr.ui_project_line->store);
	g_object_unref (gebr.ui_project_line->servers_filter);
	g_object_unref (gebr.ui_project_line->servers_sort);

	g_free(gebr.ui_project_line);
	g_free(gebr.ui_flow_browse);
	g_free(gebr.ui_flow_edition);
	g_free(gebr.ui_job_control);
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
	g_object_unref(gebr.accel_group_array[ACCEL_SERVER]);
	g_object_unref(gebr.action_group_general);
	g_object_unref(gebr.action_group_project_line);
	g_object_unref(gebr.action_group_flow);
	g_object_unref(gebr.action_group_flow_edition);
	g_object_unref(gebr.action_group_job_control);
	g_object_unref(gebr.action_group_status);
	g_object_unref(gebr.action_group_server);

	gtk_main_quit();

	return FALSE;
}

static void gebr_config_load_servers(void)
{
	gchar *path;
	gchar **groups;
	GString *address;
	GKeyFile *servers;
	gboolean autoconnect;
	gboolean ret;
	GString *tags;
	gboolean has_local_server = FALSE;

	// Migrate servers from old config file to the new servers file
	groups = g_key_file_get_groups(gebr.config.key_file, NULL);
	for (gint i = 0; groups[i] != NULL; ++i) {
		if (!g_str_has_prefix(groups[i], "server-"))
			continue;
		address = gebr_g_key_file_load_string_key(gebr.config.key_file,
							  groups[i], "address", "");
		if (address->len) {
			autoconnect = gebr_g_key_file_load_boolean_key(
					gebr.config.key_file, groups[i], "autoconnect", TRUE);

			tags = gebr_g_key_file_load_string_key(
					gebr.config.key_file, groups[i], "tags", "");

			gebr_server_new (address->str, autoconnect, tags->str);
			g_string_free (tags, TRUE);
		}
		g_string_free(address, TRUE);
		g_key_file_remove_group (gebr.config.key_file, groups[i], NULL);
	}
	g_strfreev(groups);

	path = g_build_path ("/", g_get_home_dir (), SERVERS_PATH, NULL);
	servers = g_key_file_new ();
	ret = g_key_file_load_from_file (servers, path, G_KEY_FILE_NONE, NULL);
	g_free (path);

	if (ret) {
		groups = g_key_file_get_groups(servers, NULL);
		for (int i = 0; groups[i]; i++) {
			address = gebr_g_key_file_load_string_key(servers,
								  groups[i], "address", "");
			if(g_strcmp0(address->str,"127.0.0.1") == 0)
				has_local_server = TRUE;
			autoconnect = gebr_g_key_file_load_boolean_key (
					servers, groups[i], "autoconnect", TRUE);

			tags = gebr_g_key_file_load_string_key(
					servers, groups[i], "tags", "");

			gebr_server_new (groups[i], autoconnect, tags->str);
			g_string_free(address, TRUE);
			g_string_free (tags, TRUE);
		}
		g_key_file_free (servers);
	}
	if(!has_local_server)
		gebr_server_new("127.0.0.1", TRUE, "");

	ui_server_update_tags_combobox ();
}

static void gebr_config_save_servers(void)
{
	gchar *path;
	gchar *contents;
	GtkTreeIter iter;
	GKeyFile *servers;
	GtkTreeModel *model;
	GebrServer *server;
	gboolean autoconnect;
	gboolean ret;
	gchar * tags;

	model = GTK_TREE_MODEL(gebr.ui_server_list->common.store);
	servers = g_key_file_new ();

	gebr_gui_gtk_tree_model_foreach(iter, model) {
		gtk_tree_model_get(model, &iter,
				   SERVER_POINTER, &server,
				   SERVER_AUTOCONNECT, &autoconnect,
				   SERVER_TAGS, &tags,
				   -1);
		g_key_file_set_string(servers, server->comm->address->str,
				      "address", server->comm->address->str);
		g_key_file_set_boolean(servers, server->comm->address->str,
				       "autoconnect", autoconnect);
		g_key_file_set_string(servers, server->comm->address->str,
				      "tags", tags);
	}

	path = g_build_path ("/", g_get_home_dir (), SERVERS_PATH, NULL);
	contents = g_key_file_to_data (servers, NULL, NULL);
	ret = g_file_set_contents (path, contents, -1, NULL);
	g_free (contents);
	g_free (path);
	g_key_file_free (servers);

	if (!ret)
		gebr_message (GEBR_LOG_ERROR, TRUE, TRUE,
			      _("Could not save configuration."));
}

gint gebr_config_load()
{
	gboolean new_config;

	/* the config load order must be: log, load file servers, projects/line/flows, menus.
	 * new config check must be after all default values loaded.
	 */

	/* LOG */
	gebr_log_load();
	gebr_message(GEBR_LOG_START, TRUE, TRUE, _("GêBR initiating..."));

	/* load from file (we still support deprecated gengetopt format) */
	GError *error = NULL;
	gebr.config.key_file = g_key_file_new();
	if (!(new_config = (gboolean) g_access(gebr.config.path->str, F_OK | R_OK)) &&
	    !g_key_file_load_from_file(gebr.config.key_file, gebr.config.path->str, G_KEY_FILE_NONE, &error) &&
	    !gebr_config_load_from_gengetopt())
		gebr_message(GEBR_LOG_ERROR, FALSE, TRUE, _("Error parsing config file; reseting it."));

	/* GENERAL */
	gebr.config.username = gebr_g_key_file_load_string_key(gebr.config.key_file, "general", "name", g_get_real_name());
	gebr.config.email = gebr_g_key_file_load_string_key(gebr.config.key_file, "general", "email", g_get_user_name());

	GString *default_dir = g_string_new(NULL);
	g_string_printf(default_dir, "%s/GeBR-Menus", g_get_home_dir());
	gebr.config.usermenus = gebr_g_key_file_load_string_key(gebr.config.key_file, "general", "usermenus", default_dir->str);
	if (!strcmp(gebr.config.usermenus->str, default_dir->str) && !g_file_test(default_dir->str, G_FILE_TEST_EXISTS))
		g_mkdir_with_parents(default_dir->str, 0755);
	gebr_path_resolve_home_variable(gebr.config.usermenus);

	g_string_printf(default_dir, "%s/.gebr/gebr/data", g_get_home_dir());
	gebr.config.data = gebr_g_key_file_load_string_key(gebr.config.key_file, "general", "data", default_dir->str);
	g_string_free(default_dir, TRUE);

	/* DEPRECATED: old directory structure migration */
	if (g_str_has_suffix(gebr.config.data->str, ".gebr/gebrdata"))
		g_string_printf(gebr.config.data, "%s/.gebr/gebr/data", g_get_home_dir());
	else if (!g_str_has_suffix(gebr.config.data->str, ".gebr/gebr/data"))
		gebr_migrate_data_dir();
	else
		gebr_path_resolve_home_variable(gebr.config.data);

	/* GENERAL */
	gebr.config.editor =
		gebr_g_key_file_load_string_key(gebr.config.key_file, "general", "editor", "");
	gebr.config.native_editor =
		gebr_g_key_file_load_boolean_key(gebr.config.key_file, "general", "native_editor", TRUE);

	gebr.config.width = gebr_g_key_file_load_int_key(gebr.config.key_file, "general", "width", 700);
	gebr.config.height = gebr_g_key_file_load_int_key(gebr.config.key_file, "general", "height", 400);
	gtk_window_resize(GTK_WINDOW(gebr.window), gebr.config.width, gebr.config.height);

	gebr.config.log_load = gebr_g_key_file_load_boolean_key(gebr.config.key_file, "general", "log_load", FALSE);
	gebr.config.log_expander_state = gebr_g_key_file_load_boolean_key(gebr.config.key_file, "general",
									  "log_expander_state", FALSE);
	gtk_expander_set_expanded(GTK_EXPANDER(gebr.ui_log->widget), gebr.config.log_expander_state);


	gebr.config.job_log_word_wrap = gebr_g_key_file_load_boolean_key(gebr.config.key_file, "general",
									 "job_log_word_wrap", FALSE);
	g_object_set(G_OBJECT(gebr.ui_job_control->text_view), "wrap-mode",
		     gebr.config.job_log_word_wrap ? GTK_WRAP_WORD : GTK_WRAP_NONE, NULL);
	gebr.config.job_log_auto_scroll = gebr_g_key_file_load_boolean_key(gebr.config.key_file, "general",
									   "job_log_auto_scroll", FALSE);

	g_string_assign (gebr.config.detailed_flow_css,
			 gebr_g_key_file_load_string_key (gebr.config.key_file, "general", "detailed_flow_css", "")->str);
	gebr.config.detailed_flow_include_report =
		gebr_g_key_file_load_boolean_key (gebr.config.key_file, "general", "detailed_flow_include_report", FALSE);
	gebr.config.detailed_flow_parameter_table =
		gebr_g_key_file_load_int_key (gebr.config.key_file, "general", "detailed_flow_parameter_table", 0);

	g_string_assign (gebr.config.detailed_line_css,
			 gebr_g_key_file_load_string_key (gebr.config.key_file, "general", "detailed_line_css", "")->str);
	gebr.config.detailed_line_include_report =
		gebr_g_key_file_load_boolean_key (gebr.config.key_file, "general", "detailed_line_include_report", FALSE);
	gebr.config.detailed_line_include_flow_report =
		gebr_g_key_file_load_boolean_key (gebr.config.key_file, "general", "detailed_line_include_flow_report", FALSE);
	gebr.config.detailed_line_parameter_table =
		gebr_g_key_file_load_int_key (gebr.config.key_file, "general", "detailed_line_parameter_table", 0);

	gebr_config_load_servers ();

	/* PROJECT/LINE/FLOWS */
	project_list_populate();

	gebr.ui_project_line->servers_filter =
		gtk_tree_model_filter_new (GTK_TREE_MODEL (gebr.ui_server_list->common.store),
					   NULL);

	gebr.ui_project_line->servers_sort =
		gtk_tree_model_sort_new_with_model (gebr.ui_project_line->servers_filter);

	gtk_tree_model_filter_set_visible_func (
			GTK_TREE_MODEL_FILTER (gebr.ui_project_line->servers_filter),
			servers_filter_visible_func, NULL, NULL);

	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (gebr.ui_project_line->servers_sort),
					 0, servers_sort_func, NULL, NULL);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (gebr.ui_project_line->servers_sort), 0,
					      GTK_SORT_ASCENDING);

	gtk_combo_box_set_model (
			GTK_COMBO_BOX (gebr.ui_flow_edition->server_combobox),
			gebr.ui_project_line->servers_sort);

	gboolean project_line_selected;
	if (gebr_g_key_file_has_key(gebr.config.key_file, "general", "notebook")) {
		/* old config */
		gebr.config.current_notebook = gebr_g_key_file_load_int_key(gebr.config.key_file, "general", "notebook", 0);

		gebr.config.project_line_filename = g_string_new("");
		GString *project_line_string = 
			gebr_g_key_file_load_string_key(gebr.config.key_file, "general", "project_line_string", "");
		GtkTreeIter iter;
		if ((project_line_selected = (project_line_string->len &&
					      gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(gebr.ui_project_line->store), &iter, project_line_string->str))))
			project_line_select_iter(&iter);
		g_string_free(project_line_string, TRUE);

		gebr.config.flow_treepath_string =
			gebr_g_key_file_load_string_key(gebr.config.key_file, "general", "flow_string", "");
	} else {
		gebr.config.current_notebook = gebr_g_key_file_load_int_key(gebr.config.key_file, "state", "notebook", 0);
		gebr.config.project_line_filename =
			gebr_g_key_file_load_string_key(gebr.config.key_file, "state", "project_line_filename", "");
		GtkTreeIter iter;
		if ((project_line_selected = (gebr.config.project_line_filename->len &&
					      gebr_gui_gtk_tree_model_find_by_column(GTK_TREE_MODEL(gebr.ui_project_line->store), &iter, PL_FILENAME, gebr.config.project_line_filename->str))))
			project_line_select_iter(&iter);
		gebr.config.flow_treepath_string =
			gebr_g_key_file_load_string_key(gebr.config.key_file, "state", "flow_treepath_string", "");
	}
	if (project_line_selected) {
		GtkTreeIter iter;
		if ((gebr.config.flow_treepath_string->len) && gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter, gebr.config.flow_treepath_string->str)){
			flow_browse_select_iter(&iter);
			/* Restore last selected tab */
			gtk_notebook_set_current_page(GTK_NOTEBOOK(gebr.notebook), gebr.config.current_notebook);
		}
	}

	/* MENUS */
	menu_list_populate();
	/* JOB UI */
	job_control_selected();

	/* NEW CONFIG? */
	if (new_config) {
		//gebr_config_save(FALSE); //default values saved
		preferences_setup_ui(TRUE);
	} else
		gebr_config_save(FALSE); //save config to keep updated

	return 0;
}

void gebr_config_apply(void)
{
	menu_list_create_index();
	menu_list_populate();
}

void gebr_config_save(gboolean verbose)
{
	GtkTreeIter iter;

	gsize length;
	gchar *string;
	FILE *configfp;

	/* reset key_file, cause we do not sync servers automatically */
	g_key_file_free(gebr.config.key_file);
	gebr.config.key_file = g_key_file_new();

	g_key_file_set_string(gebr.config.key_file, "general", "name", gebr.config.username->str);
	g_key_file_set_string(gebr.config.key_file, "general", "email", gebr.config.email->str);
	g_key_file_set_string(gebr.config.key_file, "general", "editor", gebr.config.editor->str);
	g_key_file_set_boolean(gebr.config.key_file, "general", "native_editor", gebr.config.native_editor);

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
	g_key_file_set_integer (gebr.config.key_file, "general", "detailed_flow_parameter_table", gebr.config.detailed_flow_parameter_table);

	g_key_file_set_string (gebr.config.key_file, "general", "detailed_line_css", gebr.config.detailed_line_css->str);
	g_key_file_set_boolean (gebr.config.key_file, "general", "detailed_line_include_report", gebr.config.detailed_line_include_report);
	g_key_file_set_boolean (gebr.config.key_file, "general", "detailed_line_include_flow_report", gebr.config.detailed_line_include_flow_report);
	g_key_file_set_integer (gebr.config.key_file, "general", "detailed_line_parameter_table", gebr.config.detailed_line_parameter_table);

	g_key_file_set_integer(gebr.config.key_file, "state", "notebook", gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook)));

	gebr_config_save_servers ();

	/* Save selected documents */
	if (project_line_get_selected(&iter, DontWarnUnselection)) {
		gchar *str;
		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->store), &iter, PL_FILENAME, &str, -1);
		g_string_assign(gebr.config.project_line_filename, str);
		g_free(str);
		g_key_file_set_string(gebr.config.key_file, "state", "project_line_filename", gebr.config.project_line_filename->str);

		if (flow_browse_get_selected(&iter, FALSE)) {
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

void gebr_message(enum gebr_log_message_type type, gboolean in_statusbar, gboolean in_log_file,
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
		struct gebr_log_message *log_message;

		log_message = gebr_log_message_new(type, gebr_iso_date(), string);
		gebr_log_add_message(gebr.log, type, string);
		gebr_log_add_message_to_list(gebr.ui_log, log_message);

		gebr_log_message_free(log_message);
	}

	g_free(string);
}

void gebr_remove_help_edit_window(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return;

	void remove_window(GebrGeoXmlDocument * document)
	{
		if (document == NULL)
			return;
		GtkWidget * window = g_hash_table_lookup(gebr.help_edit_windows, document);
		if (window)
			gtk_widget_destroy(window);
	}

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
			gebr_log_add_message_to_list(gebr.ui_log, (struct gebr_log_message *)i->data);

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
						_("Cannot copy data files to GêBR's directory. Please check your write permissions to your home directory."));
	}
	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));

	g_string_free(command_line, TRUE);
	g_string_free(new_data_dir, TRUE);
	gtk_widget_destroy(dialog);
}
