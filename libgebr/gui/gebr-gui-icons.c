/*   libgebr - GeBR Library
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

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fnmatch.h>

#include "gtk/gtk.h"

#include "gebr-gui-icons.h"
#include "defines.h"

/*
 * Private functions
 */

/*
 * Library functions
 */

void gebr_gui_setup_icons(void)
{
	static GtkIconFactory *icon_factory = NULL;
	static const gchar *sizes[] = {
		"16x16", "22x22", "48x48", NULL
	};
	static const int gtk_icon_sizes[] = {
		GTK_ICON_SIZE_MENU,
		GTK_ICON_SIZE_SMALL_TOOLBAR,
		GTK_ICON_SIZE_LARGE_TOOLBAR
	};
	GString *size_path;
	GString *path;
	int i;

	if (icon_factory == NULL)
		icon_factory = gtk_icon_factory_new();
	else
		return;

	size_path = g_string_new(NULL);
	path = g_string_new(NULL);

	for (i = 0; sizes[i] != NULL; ++i) {
		DIR *dir;
		struct dirent *size_file;

		g_string_printf(size_path, "%s%s", LIBGEBR_ICONS_DIR "/", sizes[i]);
		if ((dir = opendir(size_path->str)) == NULL)
			continue;

		while ((size_file = readdir(dir)) != NULL) {
			DIR *dir;
			struct dirent *file;

			/* ignore gtk stock items */
			if (strcmp(size_file->d_name, "stock") == 0)
				continue;
			g_string_printf(path, "%s/%s", size_path->str, size_file->d_name);
			if ((dir = opendir(path->str)) == NULL)
				continue;

			while ((file = readdir(dir)) != NULL) {
				GtkIconSet *icon_set;
				GtkIconSource *icon_source;
				GString *filename;
				gchar *stock_id;

				if (fnmatch("*.png", file->d_name, 1))
					continue;

				/* remove ".png" from string */
				filename = g_string_new(NULL);
				if (g_path_is_absolute(path->str))
					g_string_printf(filename, "%s/%s", path->str, file->d_name);
				else {
					gchar *cwd = g_get_current_dir();
					g_string_printf(filename, "%s/%s/%s", cwd, path->str, file->d_name);
					g_free(cwd);
				}
				stock_id = g_strndup(file->d_name, strlen(file->d_name) - 4);

				icon_source = gtk_icon_source_new();
				gtk_icon_source_set_filename(icon_source, filename->str);
				gtk_icon_source_set_size(icon_source, (GtkIconSize) gtk_icon_sizes[i]);
				gtk_icon_source_set_size_wildcarded(icon_source, TRUE);
				gtk_icon_source_set_direction_wildcarded(icon_source, TRUE);
				gtk_icon_source_set_state_wildcarded(icon_source, TRUE);

				icon_set = gtk_icon_factory_lookup(icon_factory, stock_id);
				if (icon_set == NULL)
					icon_set = gtk_icon_set_new();
				gtk_icon_set_add_source(icon_set, icon_source);
				gtk_icon_factory_add(icon_factory, stock_id, icon_set);

				g_string_free(filename, TRUE);
				g_free(stock_id);
				gtk_icon_source_free(icon_source);
			}
		}

		closedir(dir);
	}

	/* finally... add icons */
	gtk_icon_factory_add_default(icon_factory);

	g_string_free(size_path, TRUE);
	g_string_free(path, TRUE);
}

void gebr_gui_setup_theme(void)
{
	gtk_icon_theme_append_search_path(gtk_icon_theme_get_default(), LIBGEBR_ICONS_DIR);
	g_object_set(gtk_settings_get_default(), "gtk-icon-theme-name", "gebr-theme", NULL);
}

const gchar * gebr_gui_get_program_icon(GebrGeoXmlProgram * program)
{
	static gchar string[51];

	GString * temp;
	const gchar * postfix;
	GebrGeoXmlProgramStatus status;

	status = gebr_geoxml_program_get_status(program);
	if (status == GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED ||
	    status == GEBR_GEOXML_PROGRAM_STATUS_UNKNOWN)
		return GTK_STOCK_DIALOG_WARNING;

	switch(status) {
	case GEBR_GEOXML_PROGRAM_STATUS_DISABLED:
		postfix = "disabled";
		break;
	case GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED:
		postfix = "configured";
		break;
	default:
		postfix = "";
		break;
	}
	temp = g_string_new(NULL);
	g_string_printf(temp, "pipe-%c%c-%s",
			gebr_geoxml_program_get_stdin(program)? 'o' : 'c',
			gebr_geoxml_program_get_stdout(program)? 'o' : 'c',
			postfix);
	strcpy(string, temp->str);
	if (temp->len > 50)
		g_error("%s:%d: Icon name length is greater than 50, report to GeBR developers", __FILE__, __LINE__);

	g_string_free(temp, TRUE);
	return string;
}

