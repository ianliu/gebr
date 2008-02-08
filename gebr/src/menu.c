/*   GÍBR - An environment for seismic processing.
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>

#include <glib/gstdio.h>

#include <misc/utils.h>

#include "menu.h"
#include "defines.h"
#include "gebr.h"
#include "support.h"
#include "document.h"

/*
 * Prototypes
 */
static void
menu_scan_directory(const gchar * directory, FILE * index_fp);

/*
 * Section: Public
 * Public functions.
 */

/*
 * Function: menu_load
 * Look for a given menu _filename_ and load it if found
 */
GeoXmlFlow *
menu_load(const gchar * filename)
{
	GeoXmlFlow *	menu;
	GString *	path;

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
GeoXmlFlow *
menu_load_path(const gchar * path)
{
	GeoXmlFlow *	menu;

	menu = GEOXML_FLOW(document_load_path(path));

	return menu;
}

/*
 * Function: menu_get_path
 * Look for a given menu and fill in its path
 */
GString *
menu_get_path(const gchar * filename)
{
	GString *	path;

	/* system directory */
	path = g_string_new(NULL);
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
 * Function: menu_list_populate
 * Read index and add menus from it to the view
 */
void
menu_list_populate(void)
{
	GIOChannel *	index_io_channel;
	GError *	error;
	GString *	index_path;
	GString *	line;

	GtkTreeIter	category_iter;
	GtkTreeIter *	parent_iter;

	/* initialization */
	error = NULL;
	index_path = g_string_new(NULL);
	line = g_string_new(NULL);

	g_string_printf(index_path, "%s/.gebr/menus.idx", getenv("HOME"));
	if (g_access(index_path->str, F_OK | R_OK) && menu_list_create_index() == FALSE)
		goto out;

	/* Remove any previous menus from the list */
	gtk_tree_store_clear(gebr.ui_flow_edition->menu_store);

	index_io_channel = g_io_channel_new_file(index_path->str, "r", &error);
	parent_iter = NULL;
	while (g_io_channel_read_line_string(index_io_channel, line, NULL, &error) == G_IO_STATUS_NORMAL) {
		gchar **	parts;
		GString *	path;
		GtkTreeIter	iter;

		parts = g_strsplit_set(line->str, "|\n", 5);
		path = menu_get_path(parts[3]);
		if (path == NULL)
			goto cont;

		if (!strlen(parts[0]))
			parent_iter = NULL;
		else {
			GString *	titlebf;

			titlebf = g_string_new(NULL);
			g_string_printf(titlebf, "<b>%s</b>", parts[0]);

			/* is there a category? */
			if (parent_iter != NULL) {
				gchar *	category;

				gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->menu_store), parent_iter,
						MENU_TITLE_COLUMN, &category,
						-1);

				/* different category? */
				if (g_ascii_strcasecmp(category, titlebf->str)) {
					gtk_tree_store_append(gebr.ui_flow_edition->menu_store, &category_iter, NULL);
					gtk_tree_store_set(gebr.ui_flow_edition->menu_store, &category_iter,
							MENU_TITLE_COLUMN, titlebf->str,
							-1);
					parent_iter = &category_iter;
				}

				g_free(category);
			} else {
				gtk_tree_store_append(gebr.ui_flow_edition->menu_store, &category_iter, NULL);
				gtk_tree_store_set(gebr.ui_flow_edition->menu_store, &category_iter,
							MENU_TITLE_COLUMN, titlebf->str,
							-1);
				parent_iter = &category_iter;
			}

			g_string_free(titlebf, TRUE);
		}

		gtk_tree_store_append(gebr.ui_flow_edition->menu_store, &iter, parent_iter);
		gtk_tree_store_set(gebr.ui_flow_edition->menu_store, &iter,
				MENU_TITLE_COLUMN, parts[1],
				MENU_DESC_COLUMN, parts[2],
				MENU_FILE_NAME_COLUMN, parts[3],
				-1);

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
gboolean
menu_list_create_index(void)
{
	GString *	path;
	FILE *		index_fp;
	GString *	sort_file;
	GString	*	sort_cmd_line;
	gboolean	ret;

	/* initialization */
	ret = TRUE;
	path = g_string_new(NULL);
	sort_file = g_string_new(NULL);
	sort_cmd_line = g_string_new(NULL);

	g_string_printf(path, "%s/.gebr/menus.idx", getenv("HOME"));
	if ((index_fp = fopen(path->str, "w")) == NULL) {
		gebr_message(ERROR, TRUE, FALSE, _("Unable to write menus' index"));
		ret = FALSE;
		goto out;
	}
	menu_scan_directory(GEBR_SYS_MENUS_DIR, index_fp);
	menu_scan_directory(gebr.config.usermenus->str, index_fp);
	fclose(index_fp);

	/* Sort index */
	sort_file = make_temp_filename("gebrmenusXXXXXX.tmp");
	g_string_printf(sort_cmd_line, "sort %s >%s; mv %s %s",
		path->str, sort_file->str, sort_file->str, path->str);
	system(sort_cmd_line->str);

	/* frees */
out:	g_string_free(path, TRUE);
	g_string_free(sort_cmd_line, TRUE);

	return ret;
}

/*
 * Function: menu_scan_directory
 * Scans _directory_ for menus
 */
GString *
menu_get_help_from_program_ref(GeoXmlProgram * program)
{
	GeoXmlFlow *		menu;
	gchar *			filename;
	gulong			index;
	GString *		help;

	GeoXmlSequence *	menu_program;

	geoxml_program_get_menu(GEOXML_PROGRAM(program), &filename, &index);
	menu = menu_load(filename);
	if (menu == NULL)
		return g_string_new("");

	/* go to menu's program index specified in flow */
	geoxml_flow_get_program(menu, &menu_program, index);
	help = g_string_new(geoxml_program_get_help(GEOXML_PROGRAM(menu_program)));

	geoxml_document_free(GEOXML_DOC(menu));

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
static void
menu_scan_directory(const gchar * directory, FILE * index_fp)
{
	DIR *			dir;
	struct dirent *		file;
	GString *		path;

	/* initialization */
	if ((dir = opendir(directory)) == NULL)
		return;
	path = g_string_new(NULL);

	while ((file = readdir(dir)) != NULL) {
		if (fnmatch("*.mnu", file->d_name, 1))
			continue;

		GeoXmlDocument *	menu;
		GeoXmlSequence *	category;

		g_string_printf(path, "%s/%s", directory, file->d_name);

		menu = document_load_path(path->str);
		if (menu == NULL)
			continue;

		geoxml_flow_get_category(GEOXML_FLOW(menu), &category, 0);
		while (category != NULL) {
			fprintf(index_fp, "%s|%s|%s|%s\n",
				geoxml_category_get_name(GEOXML_CATEGORY(category)),
				geoxml_document_get_title(menu),
				geoxml_document_get_description(menu),
				geoxml_document_get_filename(menu));

			geoxml_sequence_next(&category);
		}

		geoxml_document_free(menu);
	}

	/* frees */
	closedir(dir);
	g_string_free(path, TRUE);
}
