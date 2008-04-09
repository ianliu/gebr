/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2008 GeBR core team (http://gebr.sourceforge.net)
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

/*
 * File: gebr.c
 * General purpose functions
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include <glib/gstdio.h>

#include <misc/utils.h>
#include <misc/date.h>

#include "gebr.h"
#include "support.h"
#include "cmdline.h"
#include "project.h"
#include "server.h"
#include "job.h"
#include "menu.h"
#include "callbacks.h"
#include "document.h"
#include "flow.h"

struct gebr gebr;

/*
 * Function: gebr_init
 * Take initial measures.
 *
 * This function is the callback of when the gebr.window is shown.
 */
void
gebr_init(int argc, char ** argv)
{
	/* initialization */
	gebr.project_line = NULL;
	gebr.flow = NULL;
	protocol_init();

	/* check/create config dir */
	gebr_create_config_dirs();

	/* allocating list of temporary files */
	gebr.tmpfiles = g_slist_alloc();

	/* icons */
	gebr.invisible = gtk_invisible_new();
	gebr.pixmaps.stock_cancel = gtk_widget_render_icon(gebr.invisible, GTK_STOCK_CANCEL, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	gebr.pixmaps.stock_apply = gtk_widget_render_icon(gebr.invisible, GTK_STOCK_APPLY, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	gebr.pixmaps.stock_warning = gtk_widget_render_icon(gebr.invisible, GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	gebr.pixmaps.stock_execute = gtk_widget_render_icon(gebr.invisible, GTK_STOCK_EXECUTE, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	gebr.pixmaps.stock_connect = gtk_widget_render_icon(gebr.invisible, GTK_STOCK_CONNECT, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	gebr.pixmaps.stock_disconnect = gtk_widget_render_icon(gebr.invisible, GTK_STOCK_DISCONNECT, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	gebr.pixmaps.stock_go_back = gtk_widget_render_icon(gebr.invisible, GTK_STOCK_GO_BACK, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	gebr.pixmaps.stock_go_forward = gtk_widget_render_icon(gebr.invisible, GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	gebr.pixmaps.stock_info = gtk_widget_render_icon(gebr.invisible, GTK_STOCK_INFO, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);

	/* log */
	gebr_log_load();
	gebr_message(LOG_START, TRUE, TRUE, _("GêBR Initiating..."));

	/* finally the config. file */
	gebr_config_load(argc, argv);

	/* check for a menu list change */
	if (menu_refresh_needed() == TRUE) {
		menu_list_create_index();
		menu_list_populate();
	}
}

/*
 * Function: gebr_quit
 * Free memory, remove temporaries files and quit.
 */
gboolean
gebr_quit(void)
{
	GtkTreeIter	iter;
	gboolean	valid;

	gebr_config_save(FALSE);
	/*
	 * Data frees and cleanups
	 */
	flow_free();
	project_line_free();

	g_slist_foreach(gebr.tmpfiles, (GFunc)g_unlink, NULL);
	g_slist_foreach(gebr.tmpfiles, (GFunc)g_free, NULL);

	g_slist_free(gebr.tmpfiles);

	g_string_free(gebr.config.username, TRUE);
	g_string_free(gebr.config.email, TRUE);
	g_string_free(gebr.config.editor, TRUE);
	g_string_free(gebr.config.usermenus, TRUE);
	g_string_free(gebr.config.data, TRUE);
	g_string_free(gebr.config.browser, TRUE);

	gebr_message(LOG_END, TRUE, TRUE, _("GêBR Finalizing..."));
	log_close(gebr.log);

	/* Free servers structs */
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebr.ui_server_list->common.store), &iter);
	while (valid) {
		struct server *	server;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->common.store), &iter,
				SERVER_POINTER, &server,
				-1);
		server_free(server);
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(gebr.ui_server_list->common.store), &iter);
	}
	/* Free jobs structs */
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter);
	while (valid) {
		struct job *	job;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter,
				JC_STRUCT, &job,
				-1);
		job_free(job);
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(gebr.ui_job_control->store), &iter);
	}

	/*
	 * Interface frees
	 */

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

	gtk_main_quit();

	return FALSE;
}

/*
 * Function: gebr_config_load
 * Initialize log on GUI
 */
void
gebr_log_load(void)
{
	GString *	log_filename;
	GList *		messages;
	GList *		i;

	log_filename = g_string_new(NULL);
	g_string_printf(log_filename, "%s/.gebr/log/gebr.log", getenv("HOME"));
	gebr.log = log_open(log_filename->str);

	messages = log_messages_read(gebr.log);
	i = messages;
	while (i != NULL) {
		log_add_message_to_list(gebr.ui_log, (struct log_message *)i->data);
		i = g_list_next(i);
	}

	/* frees */
	g_string_free(log_filename, TRUE);
	log_messages_free(messages);
}

/*
 * Function: gebr_config_load
 * Initialize configuration for G�BR
 */
void
gebr_config_load(int argc, char ** argv)
{
	GString	*	config;

	/* initialization */
	config = g_string_new(NULL);
	gebr.config.username = g_string_new("");
	gebr.config.email = g_string_new("");
	gebr.config.editor = g_string_new("");
	gebr.config.usermenus = g_string_new("");
	gebr.config.data = g_string_new("");
	gebr.config.browser = g_string_new("");

	g_string_printf(config, "%s/.gebr/gebr.conf", getenv("HOME"));
	if (g_access(config->str, F_OK)) {
		preferences_setup_ui(TRUE);
		server_new("127.0.0.1");
		goto out;
	}

	/* Initialize G�BR with options in gebr.conf */
	if (cmdline_parser_configfile(config->str, &gebr.config.ggopt, 1, 1, 0) != 0) {
		fprintf(stderr, "%s: try '--help' option\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/* Filling our structure in */
	g_string_assign(gebr.config.username, gebr.config.ggopt.name_arg);
	g_string_assign(gebr.config.email, gebr.config.ggopt.email_arg);
	g_string_assign(gebr.config.editor, gebr.config.ggopt.editor_arg);
	g_string_assign(gebr.config.usermenus, gebr.config.ggopt.usermenus_arg);
	g_string_assign(gebr.config.data, gebr.config.ggopt.data_arg);
	g_string_assign(gebr.config.browser, gebr.config.ggopt.browser_arg);
	gebr.config.width = gebr.config.ggopt.width_arg;
	gebr.config.height = gebr.config.ggopt.height_arg;
	gebr.config.log_expander_state = (gebr.config.ggopt.logexpand_given ? TRUE : FALSE);

	gtk_window_resize (GTK_WINDOW(gebr.window), gebr.config.width, gebr.config.height);
	gtk_expander_set_expanded(GTK_EXPANDER(gebr.ui_log->widget), gebr.config.log_expander_state);

	if (!gebr.config.ggopt.name_given)
		preferences_setup_ui(TRUE);
	else {
		menu_list_populate();
		project_list_populate();
	}
	if (!gebr.config.ggopt.server_given) {
		server_new("127.0.0.1");
	} else {
		gint	i;

		for (i = 0; i < gebr.config.ggopt.server_given; ++i)
			server_new(gebr.config.ggopt.server_arg[i]);
	}

	/* frees */
out:	g_string_free(config, TRUE);
}

/*
 * Function: gebr_config_apply
 * Apply configurations to G�BR
 */
void
gebr_config_apply(void)
{
	menu_list_create_index();
	menu_list_populate();
	project_list_populate();
}

/*
 * Function: gebr_config_save
 * Save G�BR config to file.
 *
 * Write ~/.gebr/.gebr.conf file.
 */
gboolean
gebr_config_save(gboolean verbose)
{
	FILE *		fp;
	GString *	config;

	GtkTreeIter	iter;
	gboolean	valid;

	/* initialization */
	config = g_string_new(NULL);
	g_string_printf(config, "%s/.gebr/gebr.conf", getenv("HOME"));

	fp = fopen(config->str, "w");
	if (fp == NULL) {
		gebr_message(LOG_ERROR, TRUE, TRUE, _("Unable to write configuration"));
		return FALSE;
	}

	gtk_window_get_size(GTK_WINDOW(gebr.window), &gebr.config.width, &gebr.config.height);
	gebr.config.log_expander_state = gtk_expander_get_expanded(GTK_EXPANDER(gebr.ui_log->widget));

	fprintf(fp, "name = \"%s\"\n", gebr.config.username->str);
	fprintf(fp, "email = \"%s\"\n", gebr.config.email->str);
	fprintf(fp, "usermenus = \"%s\"\n", gebr.config.usermenus->str);
	fprintf(fp, "data = \"%s\"\n",  gebr.config.data->str);
	fprintf(fp, "editor = \"%s\"\n", gebr.config.editor->str);
	fprintf(fp, "browser = \"%s\"\n", gebr.config.browser->str);
	fprintf(fp, "width =\"%d\"\n", gebr.config.width);
	fprintf(fp, "height =\"%d\"\n", gebr.config.height);
	if (gebr.config.log_expander_state)
		fprintf(fp, "logexpand\n");

	/* Save list of servers */
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebr.ui_server_list->common.store), &iter);
	while (valid) {
		struct server *	server;

		gtk_tree_model_get (GTK_TREE_MODEL(gebr.ui_server_list->common.store), &iter,
				SERVER_POINTER, &server,
				-1);
		fprintf(fp, "server = \"%s\"\n", server->comm->address->str);

		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(gebr.ui_server_list->common.store), &iter);
	}

	fclose(fp);
	if (verbose)
		gebr_message(LOG_INFO, FALSE, TRUE, _("Configuration saved"));

	return TRUE;
}

/*
 * Function: gebr_message
 * Log a message. If in_statusbar is TRUE it is writen to status bar.
 *
 */
void
gebr_message(enum log_message_type type, gboolean in_statusbar, gboolean in_log_file, const gchar * message, ...)
{
	gchar *			string;
	va_list			argp;

	va_start(argp, message);
	string = g_strdup_vprintf(message, argp);
	va_end(argp);

#ifdef GEBR_DEBUG
	if (type == LOG_DEBUG)
		g_print("%s\n", string);
	else if (in_statusbar)
		log_set_message(gebr.ui_log, string);
#else
	if (in_statusbar)
		log_set_message(gebr.ui_log, string);
#endif
	if (in_log_file) {
		struct log_message *	log_message;

		log_message = log_message_new(type, iso_date(), string);
		log_add_message(gebr.log, type, string);
		log_add_message_to_list(gebr.ui_log, log_message);

		log_message_free(log_message);
	}

	g_free(string);
}
