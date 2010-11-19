/*   DeBR - GeBR Designer
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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <glib/gstdio.h>

#include <libgebr.h>
#include <glib/gi18n.h>
#include <libgebr/utils.h>
#include <libgebr/gui/gui.h>

#include "debr.h"
#include "help.h"
#include "menu.h"
#include "preferences.h"

struct debr debr;

void debr_init(void)
{
	gboolean configured;

	configured = debr_config_load();
	debr.categories_model = gtk_list_store_new(CATEGORY_N_COLUMN, G_TYPE_STRING, G_TYPE_INT);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(debr.categories_model),
					     CATEGORY_NAME, GTK_SORT_ASCENDING);
	debr.menu = NULL;
	debr.program = NULL;
	debr.parameter = NULL;

	debr.help_edit_windows = g_hash_table_new(NULL, NULL);

	debr.tmpfiles = g_slist_alloc();
	debr.invisible = gtk_invisible_new();
	debr.pixmaps.stock_apply = gtk_widget_render_icon(debr.invisible,
							  GTK_STOCK_APPLY, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	debr.pixmaps.stock_cancel = gtk_widget_render_icon(debr.invisible,
							   GTK_STOCK_CANCEL, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	debr.pixmaps.stock_no = gtk_widget_render_icon(debr.invisible, GTK_STOCK_NO, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
	debr.pixmaps.stock_warning = gtk_widget_render_icon(debr.invisible, GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(debr.ui_menu.model), debr.config.menu_sort_column,
					     debr.config.
					     menu_sort_ascending ? GTK_SORT_ASCENDING : GTK_SORT_DESCENDING);
	if (!configured)
		preferences_dialog_setup_ui();
	else
		menu_load_user_directory();
	if (menu_get_n_menus() == 0)
		menu_new(FALSE);
}

gboolean debr_quit(void)
{
	if (!menu_cleanup_folder(NULL))
		return TRUE;

	debr_config_save();

	gtk_widget_destroy(debr.about.dialog);
	g_object_unref(debr.accel_group);

	g_hash_table_destroy(debr.help_edit_windows);

	/* free config stuff */
	g_hash_table_destroy(debr.config.opened_folders);
	g_key_file_free(debr.config.key_file);
	g_string_free(debr.config.path, TRUE);
	g_string_free(debr.config.name, TRUE);
	g_string_free(debr.config.email, TRUE);
	g_string_free(debr.config.htmleditor, TRUE);

	/* remove temporaries files */
	g_slist_foreach(debr.tmpfiles, (GFunc) g_unlink, NULL);
	g_slist_foreach(debr.tmpfiles, (GFunc) g_free, NULL);
	g_slist_free(debr.tmpfiles);

	g_object_unref(debr.pixmaps.stock_apply);
	g_object_unref(debr.pixmaps.stock_cancel);
	g_object_unref(debr.pixmaps.stock_no);
	g_object_unref(debr.pixmaps.stock_warning);
	gtk_widget_destroy(debr.invisible);

	gtk_main_quit();

	return FALSE;
}

gboolean debr_config_load(void)
{
	GError *error;
	gchar ** list;

	gebr_create_config_dirs();
	debr.config.path = g_string_new(g_get_home_dir());
	g_string_append(debr.config.path, "/.gebr/debr/debr.conf");

	error = NULL;
	debr.config.key_file = g_key_file_new();
	gboolean ret = g_key_file_load_from_file(debr.config.key_file, debr.config.path->str, G_KEY_FILE_NONE, &error);

	debr.config.name = gebr_g_key_file_load_string_key(debr.config.key_file, "general", "name", g_get_real_name());
	debr.config.email = gebr_g_key_file_load_string_key(debr.config.key_file, "general", "email", g_get_user_name());

	debr.config.opened_folders = g_hash_table_new_full((GHashFunc)g_str_hash, (GEqualFunc)g_str_equal,
							   (GDestroyNotify)g_free, NULL);

	list = g_key_file_get_string_list(debr.config.key_file, "general", "menu_dir", NULL, NULL);
	for (guint i = 0; list && list[i]; i++)
		g_hash_table_insert(debr.config.opened_folders, g_strdup(list[i]), NULL);

	debr.config.htmleditor = gebr_g_key_file_load_string_key(debr.config.key_file,
								 "general", "htmleditor", "");
	debr.config.native_editor = gebr_g_key_file_load_boolean_key(debr.config.key_file,
								     "general", "native_editor", TRUE);
	debr.config.menu_sort_ascending = gebr_g_key_file_load_boolean_key(debr.config.key_file,
									   "general", "menu_sort_ascending", TRUE);
	debr.config.menu_sort_column = gebr_g_key_file_load_int_key(debr.config.key_file,
								    "general", "menu_sort_column", MENU_MODIFIED_DATE);

	g_strfreev(list);

	return ret;
}

void debr_config_save(void)
{
	gsize length;
	gchar *string;
	GError *error;
	FILE *configfp;
	gchar ** list;
	guint i = 0;

	list = g_new(gchar *, g_hash_table_size(debr.config.opened_folders) + 1);

	void foreach(gchar * key) {
		list[i++] = g_strdup(key);
	}

	g_hash_table_foreach(debr.config.opened_folders, (GHFunc)foreach, NULL);

	list[i] = NULL;

	g_key_file_set_string(debr.config.key_file, "general", "name", debr.config.name->str);
	g_key_file_set_string(debr.config.key_file, "general", "email", debr.config.email->str);
	g_key_file_set_string_list(debr.config.key_file, "general", "menu_dir", (const gchar * const *)list, i);
	g_key_file_set_string(debr.config.key_file, "general", "htmleditor", debr.config.htmleditor->str);
	g_key_file_set_boolean(debr.config.key_file, "general", "native_editor", debr.config.native_editor);
	g_key_file_set_boolean(debr.config.key_file, "general", "menu_sort_ascending", debr.config.menu_sort_ascending);
	g_key_file_set_integer(debr.config.key_file, "general", "menu_sort_column", debr.config.menu_sort_column);

	error = NULL;
	string = g_key_file_to_data(debr.config.key_file, &length, &error);
	configfp = fopen(debr.config.path->str, "w");
	if (configfp == NULL) {
		/* TODO: warn user */
		return;
	}
	fwrite(string, sizeof(gchar), length, configfp);
	fclose(configfp);
	g_strfreev(list);
}

void debr_message(enum gebr_log_message_type type, const gchar * message, ...)
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
		g_print("%s\n", string);
	gtk_statusbar_push(GTK_STATUSBAR(debr.statusbar), 0, string);

	g_free(string);
}

gboolean debr_has_category(const gchar * category, gboolean add)
{
	GtkTreeIter iter;

	gebr_gui_gtk_tree_model_foreach(iter, GTK_TREE_MODEL(debr.categories_model)) {
		gchar *i;
		gint ref_count;

		gtk_tree_model_get(GTK_TREE_MODEL(debr.categories_model), &iter,
				   CATEGORY_NAME, &i, CATEGORY_REF_COUNT, &ref_count, -1);
		if (strcmp(i, category) == 0) {
			if (add)
				gtk_list_store_set(debr.categories_model, &iter, CATEGORY_REF_COUNT, ref_count + 1, -1);
			g_free(i);
			return TRUE;
		}

		g_free(i);
	}
	if (add) {
		gtk_list_store_append(debr.categories_model, &iter);
		gtk_list_store_set(debr.categories_model, &iter, CATEGORY_NAME, category, CATEGORY_REF_COUNT, 1, -1);
		return TRUE;
	}

	return FALSE;
}

void debr_remove_help_edit_window(gpointer object, gboolean remove_only, gboolean destroy_children)
{
	GtkWidget * window;
	GebrGeoXmlObjectType type;

	window = g_hash_table_lookup(debr.help_edit_windows, object);

	/* If this is a GeoXmlFlow, we scan through all programs
	 * and destroy their windows too.
	 */
	type = gebr_geoxml_object_get_type(GEBR_GEOXML_OBJECT(object));
	if (destroy_children && type == GEBR_GEOXML_OBJECT_TYPE_FLOW)
	{
		GebrGeoXmlFlow * flow = GEBR_GEOXML_FLOW(object);
		GebrGeoXmlSequence * program;

		gebr_geoxml_flow_get_program(flow, &program, 0);
		while (program) {
			GtkWidget * prog_window;
			prog_window = g_hash_table_lookup(debr.help_edit_windows, program);
			if (prog_window) {
				gtk_widget_destroy(prog_window);
				g_hash_table_remove(debr.help_edit_windows, program);
			}
			gebr_geoxml_sequence_next(&program);
		}
	}

	if (window != NULL) {
		if (!remove_only)
			gtk_widget_destroy(window);
		g_hash_table_remove(debr.help_edit_windows, object);
	}
}
