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
 * File: menu.c
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <glib/gstdio.h>

#include <libgebr/intl.h>
#include <libgebr/utils.h>

#include "menu.h"
#include "../defines.h"
#include "gebr.h"
#include "document.h"

/*
 * Prototypes
 */
static void menu_scan_directory(const gchar * directory, gboolean system_dir, FILE * index_fp);

/*
 * Section: Public
 * Public functions.
 */

/*
 * Function: menu_load
 * Look for a given menu _filename_ and load it if found
 */
GebrGeoXmlFlow *menu_load(const gchar * filename)
{
	GebrGeoXmlFlow *menu;
	GString *path;

	path = menu_get_path(filename);
	if (path == NULL)
		return NULL;

	menu = menu_load_path(path->str);
	g_string_free(path, TRUE);

	return menu;
}

/*
 * Function: menu_load_path
 * Load menu at the given _path_
 */
GebrGeoXmlFlow *menu_load_path(const gchar * path)
{
	return GEBR_GEOXML_FLOW(document_load_path(path));
}

/*
 * Function: menu_get_path
 * Look for a given menu and fill in its path
 */
GString *menu_get_path(const gchar * filename)
{
	GString *path;

	path = g_string_new(NULL);

	/* system directory, for newer menus */
	if (strstr(filename, "/") != NULL) {
		g_string_printf(path, "%s/%s", GEBR_SYS_MENUS_DIR, filename);
		if (g_access((path)->str, F_OK) == 0)
			goto out;
	}
	/* system directory */
	g_string_printf(path, "%s/%s", GEBR_SYS_MENUS_DIR, filename);
	if (g_access((path)->str, F_OK) == 0)
		goto out;
	/* user's menus directory */
	g_string_printf(path, "%s/%s", gebr.config.usermenus->str, filename);
	if (g_access((path)->str, F_OK) == 0)
		goto out;

	/* an error occurred */
	g_string_free(path, TRUE);
	return NULL;

 out:	return path;
}

/*
 * Function: menu_compare_times
 * 
 */
static gboolean menu_compare_times(const gchar * directory, time_t index_time, gboolean recursive)
{
	gchar *filename;
	GString *path;
	gboolean refresh_needed;

	refresh_needed = FALSE;
	path = g_string_new(NULL);
	gebr_directory_foreach_file(filename, directory) {
		struct stat file_stat;

		g_string_printf(path, "%s/%s", directory, filename);
		if (recursive && g_file_test(path->str, G_FILE_TEST_IS_DIR)) {
			if (menu_compare_times(path->str, index_time, TRUE)) {
				refresh_needed = TRUE;
				goto out;
			}
		}
		if (fnmatch("*.mnu", filename, 1))
			continue;

		stat(path->str, &file_stat);
		if (index_time < file_stat.st_mtime) {
			refresh_needed = TRUE;
			goto out;
		}
	}

 out:	g_string_free(path, TRUE);
	return refresh_needed;
}

/*
 * Function: menu_refresh_needed
 * Return TRUE if there is change in menus' directories
 */
gboolean menu_refresh_needed(void)
{
	gboolean needed;
	GString *index_path;

	gchar *subdir;
	GString *menudir_path;
	struct stat _stat;
	time_t index_time;
	time_t menudirs_time;

	needed = FALSE;
	index_path = g_string_new(NULL);
	menudir_path = g_string_new(NULL);

	/* index path string */
	g_string_printf(index_path, "%s/.gebr/gebr/menus.idx", getenv("HOME"));
	if (g_access(index_path->str, F_OK | R_OK) && menu_list_create_index() == FALSE)
		goto out;

	/* Time for index */
	stat(index_path->str, &_stat);
	index_time = _stat.st_mtime;
	/* Times for system menus directories */
	stat(GEBR_SYS_MENUS_DIR, &_stat);
	menudirs_time = _stat.st_mtime;
	gebr_directory_foreach_file(subdir, GEBR_SYS_MENUS_DIR) {
		g_string_printf(menudir_path, "%s/%s", GEBR_SYS_MENUS_DIR, subdir);
		if (!g_file_test(menudir_path->str, G_FILE_TEST_IS_DIR))
			continue;
		stat(menudir_path->str, &_stat);
		if (menudirs_time < _stat.st_mtime)
			menudirs_time = _stat.st_mtime;
	}
	if (menudirs_time > index_time) {
		needed = TRUE;
		goto out;
	}
	/* Times for all menus */
	if (menu_compare_times(GEBR_SYS_MENUS_DIR, index_time, TRUE)) {
		needed = TRUE;
		goto out;
	}
	if (menu_compare_times(gebr.config.usermenus->str, index_time, FALSE)) {
		needed = TRUE;
		goto out;
	}

 out:	g_string_free(index_path, TRUE);
	g_string_free(menudir_path, TRUE);

	return needed;
}

/*
 * Function: menu_list_populate
 * Read index and add menus from it to the view
 */
void menu_list_populate(void)
{
	GIOChannel *index_io_channel;
	GError *error;
	GString *index_path;
	GString *line;

	GtkTreeIter category_iter;
	GtkTreeIter *parent_iter;

	/* initialization */
	error = NULL;
	index_path = g_string_new(NULL);
	line = g_string_new(NULL);
	gtk_tree_store_clear(gebr.ui_flow_edition->menu_store);

	g_string_printf(index_path, "%s/.gebr/gebr/menus.idx", getenv("HOME"));
	if (g_access(index_path->str, F_OK | R_OK) && menu_list_create_index() == FALSE)
		goto out;

	index_io_channel = g_io_channel_new_file(index_path->str, "r", &error);
	parent_iter = NULL;
	while (g_io_channel_read_line_string(index_io_channel, line, NULL, &error) == G_IO_STATUS_NORMAL) {
		gchar **parts;
		GString *path;
		GtkTreeIter iter;

		parts = g_strsplit_set(line->str, "|\n", 5);
		path = menu_get_path(parts[3]);
		if (path == NULL)
			goto cont;

		if (!strlen(parts[0]))
			parent_iter = NULL;
		else {
			GString *titlebf;

			titlebf = g_string_new(NULL);
			g_string_printf(titlebf, "<b>%s</b>", parts[0]);

			/* is there a category? */
			if (parent_iter != NULL) {
				gchar *category;

				gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->menu_store), parent_iter,
						   MENU_TITLE_COLUMN, &category, -1);

				/* different category? */
				if (strcmp(category, titlebf->str)) {
					gtk_tree_store_append(gebr.ui_flow_edition->menu_store, &category_iter, NULL);
					gtk_tree_store_set(gebr.ui_flow_edition->menu_store, &category_iter,
							   MENU_TITLE_COLUMN, titlebf->str, -1);
					parent_iter = &category_iter;
				}

				g_free(category);
			} else {
				gtk_tree_store_append(gebr.ui_flow_edition->menu_store, &category_iter, NULL);
				gtk_tree_store_set(gebr.ui_flow_edition->menu_store, &category_iter,
						   MENU_TITLE_COLUMN, titlebf->str, -1);
				parent_iter = &category_iter;
			}

			g_string_free(titlebf, TRUE);
		}

		gtk_tree_store_append(gebr.ui_flow_edition->menu_store, &iter, parent_iter);
		gtk_tree_store_set(gebr.ui_flow_edition->menu_store, &iter,
				   MENU_TITLE_COLUMN, parts[1],
				   MENU_DESC_COLUMN, parts[2], MENU_FILENAME_COLUMN, parts[3], -1);

		g_string_free(path, TRUE);
 cont:		g_strfreev(parts);
	}

	g_io_channel_unref(index_io_channel);
 out:	g_string_free(index_path, TRUE);
	g_string_free(line, TRUE);
}

/*
 * Function: menu_list_create_index
 * Create menus from found using menu_scan_directory
 *
 * Returns TRUE if successful
 */
gboolean menu_list_create_index(void)
{
	GString *path;
	FILE *index_fp;
	GString *sort_file;
	GString *sort_cmd_line;
	gboolean ret;

	/* initialization */
	ret = TRUE;
	path = g_string_new(NULL);
	sort_file = g_string_new(NULL);
	sort_cmd_line = g_string_new(NULL);

	g_string_printf(path, "%s/.gebr/gebr/menus.idx", getenv("HOME"));
	if ((index_fp = fopen(path->str, "w")) == NULL) {
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Unable to write menus' index"));
		ret = FALSE;
		goto out;
	}
	menu_scan_directory(GEBR_SYS_MENUS_DIR, TRUE, index_fp);
	menu_scan_directory(gebr.config.usermenus->str, FALSE, index_fp);
	fclose(index_fp);

	/* Sort index */
	sort_file = gebr_make_temp_filename("gebrmenusXXXXXX.tmp");
	g_string_printf(sort_cmd_line, "sort %s >%s; mv %s %s", path->str, sort_file->str, sort_file->str, path->str);
	if (system(sort_cmd_line->str))
		gebr_message(GEBR_LOG_DEBUG, TRUE, TRUE, _("Could not sort menu index"));

	/* frees */
 out:	g_string_free(path, TRUE);
	g_string_free(sort_cmd_line, TRUE);

	return ret;
}

/*
 * Function: menu_scan_directory
 * Scans _directory_ for menus
 */
GString *menu_get_help_from_program_ref(GebrGeoXmlProgram * program)
{
	GebrGeoXmlFlow *menu;
	gchar *filename;
	gulong index;
	GString *help;

	GebrGeoXmlSequence *menu_program;

	gebr_geoxml_program_get_menu(GEBR_GEOXML_PROGRAM(program), &filename, &index);
	menu = menu_load(filename);
	if (menu == NULL)
		return g_string_new("");

	/* go to menu's program index specified in flow */
	gebr_geoxml_flow_get_program(menu, &menu_program, index);
	help = g_string_new(gebr_geoxml_program_get_help(GEBR_GEOXML_PROGRAM(menu_program)));

	gebr_geoxml_document_free(GEBR_GEOXML_DOC(menu));

	return help;
}

/*
 * Section: Private
 * Private functions.
 */

/*
 * Function: menu_scan_directory
 * Scans _directory_ for menus
 */
static void menu_scan_directory(const gchar * directory, gboolean system_dir, FILE * index_fp)
{
	gchar *filename;
	GString *path;

	path = g_string_new(NULL);
	gebr_directory_foreach_file(filename, directory) {
		GebrGeoXmlDocument *menu;
		GebrGeoXmlSequence *category;

		g_string_printf(path, "%s/%s", directory, filename);
		if (system_dir && g_file_test(path->str, G_FILE_TEST_IS_DIR)) {
			menu_scan_directory(path->str, TRUE, index_fp);
			continue;
		}
		if (fnmatch("*.mnu", filename, 1))
			continue;

		menu = document_load_path(path->str);
		if (menu == NULL)
			continue;
		/* verify if filename is correct */
		if (system_dir) {
			gchar *rel_filename;

			rel_filename = path->str + strlen(GEBR_SYS_MENUS_DIR);
			while (*rel_filename == '/')
				rel_filename++;
			if (strcmp(gebr_geoxml_document_get_filename(menu), rel_filename)) {
				gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Invalid menu '%s'"), path->str);
				gebr_geoxml_document_free(menu);
				continue;
			}
		}

		gebr_geoxml_flow_get_category(GEBR_GEOXML_FLOW(menu), &category, 0);
		for (; category != NULL; gebr_geoxml_sequence_next(&category))
			fprintf(index_fp, "%s|%s|%s|%s\n",
				gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(category)),
				gebr_geoxml_document_get_title(menu),
				gebr_geoxml_document_get_description(menu), gebr_geoxml_document_get_filename(menu));

		gebr_geoxml_document_free(menu);
	}

	/* frees */
	g_string_free(path, TRUE);
}
