/*   libgebr - GeBR Library
 *   Copyright (C) 2007 GeBR core team (http://gebr.sourceforge.net)
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

#include "icons.h"

void
gui_setup_stock_theme(void)
{
	static GtkIconFactory *	icon_factory = NULL;
	static const gchar *	sizes [] = {
		"16x16", "22x22", "48x48", NULL
	};
	static const int	gtk_icon_sizes [] = {
		GTK_ICON_SIZE_MENU,
		GTK_ICON_SIZE_SMALL_TOOLBAR,
		GTK_ICON_SIZE_LARGE_TOOLBAR
	};
	GString *		path;
	int			i;

	if (icon_factory == NULL)
		icon_factory = gtk_icon_factory_new();
	else
		return;

	path = g_string_new(NULL);

	puts(ICONS_DIR);
	gtk_icon_theme_append_search_path(gtk_icon_theme_get_default(), ICONS_DIR);
	g_object_set(gtk_settings_get_default(), "gtk-icon-theme-name", "gebr-icons", NULL);

	for (i = 0; sizes[i] != NULL; ++i) {
		DIR *		dir;
		struct dirent *	file;

		g_string_printf(path, "%s%s", ICONS_DIR"/", sizes[i]);
		if ((dir = opendir(path->str)) == NULL)
			continue;

		while ((file = readdir(dir)) != NULL) {
			gchar *		stock_id;
			GtkIconSet *	icon_set;
			GtkIconSource *	icon_source;

			if (fnmatch("*.png", file->d_name, 1))
				continue;

			/* remove ".png" from string */
			stock_id = g_strndup(file->d_name, strlen(file->d_name)-4);
			icon_set = gtk_icon_factory_lookup(icon_factory, stock_id);
			if (icon_set == NULL) {
				icon_set = gtk_icon_set_new();
				gtk_icon_factory_add(icon_factory, stock_id, icon_set);
			}

			icon_source = gtk_icon_source_new();
			gtk_icon_source_set_filename(icon_source, file->d_name);
			gtk_icon_source_set_icon_name(icon_source, stock_id);
			gtk_icon_source_set_size(icon_source, (GtkIconSize)gtk_icon_sizes[i]);
			gtk_icon_source_set_size_wildcarded(icon_source, TRUE);

			gtk_icon_set_add_source(icon_set, icon_source);

			g_free(stock_id);
			gtk_icon_source_free(icon_source);
		}

		closedir(dir);
	}

	/* finally... apply the theme */
	gtk_icon_factory_add_default(icon_factory);

	g_string_free(path, TRUE);
}
