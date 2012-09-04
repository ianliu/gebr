/*   libgebr - GeBR Library
 *   Copyright (C) 2010 GeBR core team (http://www.gebrproject.com/)
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
#include <stdio.h>
#include <string.h>

#include <gdome.h>
#include <geoxml.h>
#include <glib.h>
#include <libgebr.h>
#include <libgebr/utils.h>

gboolean
gebr_geoxml_scan_menu_dir(const gchar * directory, GKeyFile * menu_key_file, GHashTable * categ_hash)
{
	GDir *dir;
	const gchar *filename;
	GError *dir_error = NULL;
	GebrGeoXmlDocument *document;

	dir = g_dir_open(directory, 0, &dir_error);

	if (dir != NULL) {
		while ((filename  = g_dir_read_name (dir)) != NULL) {
			gchar *path = g_build_filename(directory, filename, NULL);
			if (g_file_test(path, G_FILE_TEST_IS_DIR)) { //Recursion in each folder
				gebr_geoxml_scan_menu_dir(path, menu_key_file, categ_hash);
				g_free(path);
				continue;
			} else if (!g_str_has_suffix(filename, "mnu")) { //If not a menu, do nothing
				g_free(path);
				continue;
			} else if (gebr_geoxml_document_load(&document, path, TRUE, NULL) !=
				   GEBR_GEOXML_RETV_SUCCESS) {
				g_free(path);
				continue;
			}

			gchar *title = gebr_geoxml_document_get_title(document);
			gchar *description = gebr_geoxml_document_get_description(document);
			GString *categ_str = g_string_new(NULL);
			gint i;

			GebrGeoXmlSequence *category;
			gebr_geoxml_flow_get_category(GEBR_GEOXML_FLOW(document), &category, 0);

			for (i = 0; category != NULL; gebr_geoxml_sequence_next(&category), i++) {
				gchar *categ = gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(category));
				categ_str = g_string_append(categ_str, categ);
				categ_str = g_string_append_c(categ_str, ';');

				GString *categ_menus_old = g_hash_table_lookup(categ_hash, categ);
				GString *categ_menus = g_string_new(categ_menus_old ? categ_menus_old->str : NULL);
				g_string_append(categ_menus, path);
				g_string_append_c(categ_menus, ';');
				g_hash_table_replace(categ_hash, categ, categ_menus);
			}

			g_key_file_set_string(menu_key_file, path, "category", categ_str ? categ_str->str : "");
			g_key_file_set_string(menu_key_file, path, "title", title ? title : "");
			g_key_file_set_string(menu_key_file, path, "description", description ? description : "");

			g_string_free(categ_str, TRUE);
			g_free(title);
			g_free(description);
			g_free(path);
			gebr_geoxml_document_free(document);
		}
		g_dir_close(dir);
		return TRUE;
	} else {
		g_debug("Could not open folder '%s'.\n'%s'",
			directory, dir_error ? dir_error->message : "");
		return FALSE;
	}

}

gboolean
parse_command_line_args(gint argc, gchar **argv, gchar **directory,
			gchar **menus_filename, gchar **categ_filename)
{
	if (argc < 4) {
		g_warning("\nRequired arguments:\n"
			 " @1: Directory to search .mnu files\n"
			 " @2: Output filename (menus)\n"
			 " @3: Output filename (categories)\n"
			 );
		return FALSE;
	}

	 *directory = argv[1];
	 *menus_filename = argv[2];
	 *categ_filename = argv[3];

	 if (!g_file_test(*directory, G_FILE_TEST_IS_DIR)) {
		g_warning("\nInvalid folder.\n");
		return FALSE;
	 }

	return TRUE;
}

static void
hash_to_keyfile(gchar *key, GString *value, GKeyFile *categ_key_file)
{
	g_key_file_set_string(categ_key_file, key, "menus", g_strdup(value->str));
}

//Example of call ./gebr-geoxml-menu-indices /usr/share/gebr/menus/Seismic_Unix/ /tmp/menu.idx2 /tmp/categories.idx2
int main(int argc, gchar *argv[])
{
	gchar *directory;
	gchar *menus_filename;
	gchar *categ_filename;

	if (!parse_command_line_args(argc, argv, &directory,
				     &menus_filename, &categ_filename))
		return 0;

	GKeyFile *menu_key_file = g_key_file_new();
	gchar *menu_key_file_str;

	GKeyFile *categ_key_file = g_key_file_new();
	gchar *categ_key_file_str;

	GHashTable *categ_hash = g_hash_table_new_full(g_str_hash,g_str_equal,
						       g_free, (GDestroyNotify) gebr_string_freeall);
	gebr_geoxml_init();
	gebr_geoxml_scan_menu_dir(directory, menu_key_file, categ_hash);

	g_hash_table_foreach(categ_hash, (GHFunc)hash_to_keyfile, categ_key_file);
	menu_key_file_str = g_key_file_to_data(menu_key_file, NULL, NULL);
	categ_key_file_str = g_key_file_to_data(categ_key_file, NULL, NULL);

	g_file_set_contents(menus_filename, menu_key_file_str, -1, NULL);
	g_file_set_contents(categ_filename, categ_key_file_str, -1, NULL);

	g_hash_table_destroy(categ_hash);
	g_free(menu_key_file_str);
	g_key_file_free(menu_key_file);
}
