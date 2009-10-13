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

#ifndef __LIBGEBR_MISC_UTILS_H
#define __LIBGEBR_MISC_UTILS_H

#include <dirent.h>
#include <fnmatch.h>

#include <glib.h>

void
append_filename_extension(GString * filename, const gchar * extension);

GString *
libgebr_make_temp_directory(void);
void
libgebr_destroy_temp_directory(GString * path);

GString *
make_unique_filename(const gchar * template);
GString *
make_temp_filename(const gchar * template);

int
home_mode(void);

gboolean
gebr_create_config_dirs(void);

gchar *
g_simple_locale_to_utf8(const gchar * string);

gboolean
g_key_file_has_key_woe(GKeyFile * key_file, const gchar * group, const gchar * key);
GString *
g_key_file_load_string_key(GKeyFile * key_file, const gchar * group, const gchar * key, const gchar * default_value);
gboolean
g_key_file_load_boolean_key(GKeyFile * key_file, const gchar * group, const gchar * key, gboolean default_value);
int
g_key_file_load_int_key(GKeyFile * key_file, const gchar * group, const gchar * key, int default_value);

#define libgebr_foreach_gslist_hyg(element, list, hygid) \
	GSList * __list##hygid = list, * __i##hygid = list; \
	if (__i##hygid != NULL || (g_slist_free(__list##hygid), 0)) \
		for (element = (GdomeElement*)__i##hygid->data; \
		(__i##hygid != NULL && (element = (GdomeElement*)__i##hygid->data, 1)) || (g_slist_free(__list##hygid), 0); \
		__i##hygid = g_slist_next(__i##hygid))

#define libgebr_foreach_gslist(element, list) \
	libgebr_foreach_gslist_hyg(element, list, nohyg)

#define libgebr_glist_foreach_hyg(element, list, hygid) \
	GList * __list##hygid = list, * __i##hygid = list; \
	if (g_list_next(__i##hygid) != NULL || (g_list_free(__list##hygid), 0)) \
		for (element = (GdomeElement*)g_list_first(__i##hygid)->data; \
		(__i##hygid != NULL && (element = (GdomeElement*)__i##hygid->data, 1)) || (g_list_free(__list##hygid), 0); \
		__i##hygid = g_list_next(__i##hygid))

#define libgebr_glist_foreach(element, list) \
	libgebr_gslist_foreach_hyg(element, list, nohyg)

#define libgebr_directory_foreach_file_hyg(filename, directory, hygid) \
	DIR *		dir##hygid; \
	struct dirent *	file##hygid; \
	if ((dir##hygid = opendir(directory)) != NULL) \
		while (((file##hygid = readdir(dir##hygid)) != NULL && \
		(filename = file##hygid->d_name, 1)) || \
		(closedir(dir##hygid), 0)) \
			if (strcmp(filename, ".") && strcmp(filename, ".."))
#define libgebr_directory_foreach_file(filename, directory) \
	libgebr_directory_foreach_file_hyg(filename, directory, nohyg)

const gchar *
libgebr_validate_int(const gchar * text_value, const gchar * min, const gchar * max);
const gchar *
libgebr_validate_float(const gchar * text_value, const gchar * min, const gchar * max);

#endif //__LIBGEBR_MISC_UTILS_H
