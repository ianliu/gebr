/*   DeBR - GeBR Designer
 *   Copyright (C) 2007-2008 GeBR core team (http://gebr.sourceforge.net)
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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <glib/gstdio.h>

#include <misc/utils.h>

#include "debr.h"
#include "support.h"
#include "menu.h"
#include "preferences.h"

#define __DEBUG
/* used to load config file in debr_init */
#define load_key(keyfile, gstring, key, default) \
	do { \
		gchar *	value; \
		\
		error = NULL; \
		gstring = g_string_new(""); \
		value = g_key_file_get_string(keyfile, "general", key, &error); \
		if (value != NULL) { \
			g_string_assign(gstring, value); \
			g_free(value); \
		} else \
			g_string_assign(gstring, default); \
	} while(0)

/* global instance for data exchange */
struct debr debr;

void
debr_init(void)
{
	debr_config_load();

	debr.menu = NULL;
	debr.program = NULL;
	debr.parameter = NULL;
	debr.unsaved_count = 0;

	/* list of temporaries files */
	debr.tmpfiles = g_slist_alloc();

	/* load status icons */
	debr.invisible = gtk_invisible_new();
	debr.pixmaps.stock_no = gtk_widget_render_icon(debr.invisible, GTK_STOCK_NO, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);

	if (!strcmp(debr.config.menu_dir->str, ""))
		preferences_dialog_setup_ui();
	else
		menu_load_user_directory();
	if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(debr.ui_menu.list_store), NULL) == 0)
		menu_new();
}

gboolean
debr_quit(void)
{
	if (!menu_cleanup())
		return TRUE;

	gtk_widget_destroy(debr.about.dialog);
	g_object_unref(debr.accel_group);

	/* actions */
	g_object_unref(debr.actions.menu.new);
	g_object_unref(debr.actions.menu.open);
	g_object_unref(debr.actions.menu.save);
	g_object_unref(debr.actions.menu.save_as);
	g_object_unref(debr.actions.menu.revert);
	g_object_unref(debr.actions.menu.delete);
	g_object_unref(debr.actions.menu.close);
	g_object_unref(debr.actions.program.new);
	g_object_unref(debr.actions.program.delete);
	g_object_unref(debr.actions.parameter.new);
	g_object_unref(debr.actions.parameter.delete);

	/* free config stuff */
	g_key_file_free(debr.config.keyfile);
	g_string_free(debr.config.path, TRUE);
	g_string_free(debr.config.name, TRUE);
	g_string_free(debr.config.email, TRUE);
	g_string_free(debr.config.menu_dir, TRUE);
	g_string_free(debr.config.browser, TRUE);
	g_string_free(debr.config.htmleditor, TRUE);

	/* remove temporaries files */
	g_slist_foreach(debr.tmpfiles, (GFunc)g_unlink, NULL);
	g_slist_foreach(debr.tmpfiles, (GFunc)g_free, NULL);
	g_slist_free(debr.tmpfiles);

	g_object_unref(debr.pixmaps.stock_no);
	gtk_widget_destroy(debr.invisible);

	gtk_main_quit();

	return FALSE;
}

void
debr_config_load(void)
{
	GError *	error;

	gebr_create_config_dirs();
	debr.config.path = g_string_new(getenv("HOME"));
	g_string_append(debr.config.path, "/.gebr/debr.conf");

	error = NULL;
	debr.config.keyfile = g_key_file_new();
	g_key_file_load_from_file(debr.config.keyfile, debr.config.path->str, G_KEY_FILE_NONE, &error);

	load_key(debr.config.keyfile, debr.config.name, "name", g_get_real_name());
	load_key(debr.config.keyfile, debr.config.email, "email", g_get_user_name());
	load_key(debr.config.keyfile, debr.config.menu_dir, "menu_dir", "");
	load_key(debr.config.keyfile, debr.config.browser, "browser", "firefox");
	load_key(debr.config.keyfile, debr.config.htmleditor, "htmleditor", "gedit");
}

void
debr_config_save(void)
{
	gsize		length;
	gchar *		string;
	GError *	error;
	FILE *		configfp;

	g_key_file_set_string(debr.config.keyfile, "general", "name", debr.config.name->str);
	g_key_file_set_string(debr.config.keyfile, "general", "email", debr.config.email->str);
	g_key_file_set_string(debr.config.keyfile, "general", "menu_dir", debr.config.menu_dir->str);
	g_key_file_set_string(debr.config.keyfile, "general", "browser", debr.config.browser->str);
	g_key_file_set_string(debr.config.keyfile, "general", "htmleditor", debr.config.htmleditor->str);

	error = NULL;
	string = g_key_file_to_data(debr.config.keyfile, &length, &error);
	configfp = fopen(debr.config.path->str, "w");
	if (configfp == NULL) {
		/* TODO: warn user */
		return;
	}
	fwrite(string, sizeof(gchar), length, configfp);
	fclose(configfp);
}

/*
 * Function: debr_message
 * Log a message. If in_statusbar is TRUE it is writen to status bar.
 *
 */
void
debr_message(enum log_message_type type, const gchar * message, ...)
{
	gchar *		string;
	va_list		argp;

	va_start(argp, message);
	string = g_strdup_vprintf(message, argp);
	va_end(argp);

#ifdef __DEBUG
	if (type == LOG_DEBUG)
		g_print("%s\n", string);
#endif
	gtk_statusbar_push(GTK_STATUSBAR(debr.statusbar), 0, string);

	g_free(string);
}
