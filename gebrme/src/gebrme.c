/*   GêBR ME - GêBR Menu Editor
 *   Copyright (C) 2007 GêBR core team (http://gebr.sourceforge.net)
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

#include "gebrme.h"
#include "menu.h"
#include "preferences.h"

/* used to load config file in gebrme_init */
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
struct gebrme gebrme;

void
gebrme_init(void)
{
	gebrme_config_load();

	/* list of temporaries files */
	gebrme.tmpfiles = g_slist_alloc();

	/* load status icons */
	{
	   GError * error = NULL;
	   gebrme.unsaved_icon = gdk_pixbuf_new_from_file(GEBRME_PIXMAPS_DIR "gebrme_unsaved.png", &error);
	}

	if (!strcmp(gebrme.config.menu_dir->str, ""))
		create_preferences_window();
	else
		menu_load_user_directory();
	if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(gebrme.menus_liststore), NULL) == 0)
		menu_new();
}

void
gebrme_quit(void)
{
	if (!menu_cleanup())
		return;

	/* free config stuff */
	g_key_file_free(gebrme.config.keyfile);
	g_string_free(gebrme.config.path, TRUE);
	g_string_free(gebrme.config.name, TRUE);
	g_string_free(gebrme.config.email, TRUE);
	g_string_free(gebrme.config.menu_dir, TRUE);
	g_string_free(gebrme.config.browser, TRUE);
	g_string_free(gebrme.config.htmleditor, TRUE);

	/* remove temporaries files */
	g_slist_foreach(gebrme.tmpfiles, (GFunc)unlink, NULL);
	g_slist_foreach(gebrme.tmpfiles, (GFunc)free, NULL);
	g_slist_free(gebrme.tmpfiles);

	gtk_main_quit();
}

void
gebrme_config_load(void)
{
	GError *	error;

	gebrme.config.path = g_string_new(getenv("HOME"));
	g_string_append(gebrme.config.path, "/.gebr/gebrme.conf");

	error = NULL;
	gebrme.config.keyfile = g_key_file_new();
	g_key_file_load_from_file(gebrme.config.keyfile, gebrme.config.path->str, G_KEY_FILE_NONE, &error);

	load_key(gebrme.config.keyfile, gebrme.config.name, "name", "GêBR core team");
	load_key(gebrme.config.keyfile, gebrme.config.email, "email", "gebr@users.sf.net");
	load_key(gebrme.config.keyfile, gebrme.config.menu_dir, "menu_dir", "");
	load_key(gebrme.config.keyfile, gebrme.config.browser, "browser", "firefox");
	load_key(gebrme.config.keyfile, gebrme.config.htmleditor, "htmleditor", "gedit");
}

void
gebrme_config_save(void)
{
	gsize		length;
	gchar *		string;
	GError *	error;
	FILE *		configfp;

	g_key_file_set_string(gebrme.config.keyfile, "general", "name", gebrme.config.name->str);
	g_key_file_set_string(gebrme.config.keyfile, "general", "email", gebrme.config.email->str);
	g_key_file_set_string(gebrme.config.keyfile, "general", "menu_dir", gebrme.config.menu_dir->str);
	g_key_file_set_string(gebrme.config.keyfile, "general", "browser", gebrme.config.browser->str);
	g_key_file_set_string(gebrme.config.keyfile, "general", "htmleditor", gebrme.config.htmleditor->str);

	error = NULL;
	string = g_key_file_to_data(gebrme.config.keyfile, &length, &error);
	configfp = fopen(gebrme.config.path->str, "w");
	if (configfp == NULL) {
		/* TODO: warn user */
		return;
	}
	fwrite(string, sizeof(gchar), length, configfp);
	fclose(configfp);
}
