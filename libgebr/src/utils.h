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

#ifndef __GEBR_UTILS_H
#define __GEBR_UTILS_H

#include <dirent.h>
#include <fnmatch.h>

#include <glib.h>

void gebr_g_string_replace(GString * string, const gchar * oldtext, const gchar * newtext);
void gebr_g_string_replace_first_ref(GString * string, const gchar * oldtext, const gchar * newtext);
gboolean gebr_g_string_starts_with(GString * string, const gchar * val);
gboolean gebr_g_string_ends_with(GString * string, const gchar * val);

/**
 * Appends \p extension in \p filename if it is not present.
 * \return TRUE if append was made, FALSE otherwise.
 */
gboolean gebr_append_filename_extension(GString * filename, const gchar * extension);

gboolean gebr_path_use_home_variable(GString * path);
gboolean gebr_path_resolve_home_variable(GString * path);

GString *gebr_temp_directory_create(void);
void gebr_temp_directory_destroy(GString * path);

GString *gebr_make_unique_filename(const gchar * template);
GString *gebr_make_temp_filename(const gchar * template);

int gebr_home_mode(void);

gboolean gebr_create_config_dirs(void);

gchar *gebr_locale_to_utf8(const gchar * string);

gboolean g_key_file_has_key_woe(GKeyFile * key_file, const gchar * group, const gchar * key);
GString *gebr_g_key_file_load_string_key(GKeyFile * key_file, const gchar * group, const gchar * key,
					 const gchar * default_value);
gboolean gebr_g_key_file_load_boolean_key(GKeyFile * key_file, const gchar * group, const gchar * key,
					  gboolean default_value);
int gebr_g_key_file_load_int_key(GKeyFile * key_file, const gchar * group, const gchar * key, int default_value);

#define gebr_foreach_gslist_hyg(element, list, hygid) \
	GSList * __list##hygid = list, * __i##hygid = list; \
	if (__i##hygid != NULL || (g_slist_free(__list##hygid), 0)) \
		for (element = (typeof(element))__i##hygid->data; \
		(__i##hygid != NULL && (element = (typeof(element))__i##hygid->data, 1)) || (g_slist_free(__list##hygid), 0); \
		__i##hygid = g_slist_next(__i##hygid))
#define gebr_foreach_gslist(element, list) \
	gebr_foreach_gslist_hyg(element, list, nohyg)

#define gebr_glist_foreach_hyg(element, list, hygid) \
	GList * __list##hygid = list, * __i##hygid = list; \
	if (g_list_next(__i##hygid) != NULL || (g_list_free(__list##hygid), 0)) \
		for (element = (typeof(element))g_list_first(__i##hygid)->data; \
		(__i##hygid != NULL && (element = (typeof(element))__i##hygid->data, 1)) || (g_list_free(__list##hygid), 0); \
		__i##hygid = g_list_next(__i##hygid))
#define gebr_glist_foreach(element, list) \
	libgebr_gslist_foreach_hyg(element, list, nohyg)

#define gebr_directory_foreach_file_hyg(filename, directory, hygid) \
	DIR *		dir##hygid; \
	struct dirent *	file##hygid; \
	if ((dir##hygid = opendir(directory)) != NULL) \
		while (((file##hygid = readdir(dir##hygid)) != NULL && \
		(filename = file##hygid->d_name, 1)) || \
		(closedir(dir##hygid), 0)) \
			if (strcmp(filename, ".") && strcmp(filename, ".."))
#define gebr_directory_foreach_file(filename, directory) \
	gebr_directory_foreach_file_hyg(filename, directory, nohyg)

const gchar *gebr_validate_int(const gchar * text_value, const gchar * min, const gchar * max);
const gchar *gebr_validate_float(const gchar * text_value, const gchar * min, const gchar * max);

#endif				//__GEBR_UTILS_H
