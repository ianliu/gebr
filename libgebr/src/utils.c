/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <glib/gstdio.h>

#include "utils.h"

/**
 * Append, if not already present, \p extension into \p filename
 * \p extension must include the dot (e.g. ".mnu")
 */
void
append_filename_extension(GString * filename, const gchar * extension)
{
	if (!g_str_has_suffix(filename->str, extension))
		g_string_append(filename, extension);
}

/**
 * Create a file based on \p template.
 *
 * \p template must have a XXXXXX that will be replaced to random
 * generated and unique string that results on a filename that doens't exists.
 * \p template should be the absolute path of the file.
 */
GString *
make_unique_filename(const gchar * template)
{
	GString *	path;

	/* assembly file path */
	path = g_string_new(NULL);
	g_string_printf(path, "%s", template);

	/* create a temporary file. */
	close(g_mkstemp(path->str));

	return path;
}

/**
 * Create a temporary directory inside GeBR's temporary diretory.
 * Use \ref libgebr_destroy_temp_directory after use to free string memory and
 * delete directory's contents
 */
GString *
libgebr_make_temp_directory(void)
{
	GString *	path;

	/* assembly dir path */
	path = g_string_new(NULL);
	g_string_printf(path, "%s/.gebr/tmp/XXXXXX", getenv("HOME"));

	/* create a temporary file. */
	close(g_mkstemp(path->str));
	unlink(path->str);

	g_mkdir(path->str, home_mode());

	return path;
}

/**
 * Delete dir at \p path and free it.
 */
void
libgebr_destroy_temp_directory(GString * path)
{
	g_string_prepend(path, "rm -fr ");
	system(path->str);
	g_string_free(path, TRUE);
}

/**
 * Create a file based on \p template.
 *
 * \p template must have a XXXXXX that will be replaced to random
 * generated and unique string that results on a filename that doens't exists.
 * \p template is just a filename, not an absolute path.
 * The returned string then is an absolute path of
 * The string returned is a path to the file on the temporary directory.
 */
GString *
make_temp_filename(const gchar * template)
{
	GString *	path;

	/* assembly file path */
	path = g_string_new(NULL);
	g_string_printf(path, "%s/.gebr/tmp/%s", getenv("HOME"), template);

	/* create a temporary file. */
	close(g_mkstemp(path->str));

	return path;
}

/**
 * Returns the home's permission mode. Useful for
 * preserving permissions when creating files
 */
int
home_mode(void)
{
	struct stat	home_stat;
	gchar *		home;

	home = getenv("HOME");
	g_stat(home, &home_stat);

	return home_stat.st_mode;
}

static gboolean
gebr_make_config_dir(const gchar * dirname)
{
	GString *	path;
	gboolean	ret = TRUE;

	path = g_string_new(NULL);
	g_string_printf(path, "%s/.gebr/%s", getenv("HOME"), dirname);
	if (g_file_test(path->str, G_FILE_TEST_IS_DIR | G_FILE_TEST_EXISTS) == FALSE)
		if (g_mkdir(path->str, home_mode()))
			ret = FALSE;
	g_string_free(path, TRUE);

	return ret;
}

/**
 * Create all configurations directories for all GeBR programs.
 * Used by GeBR programs before read/write config..
 */
gboolean
gebr_create_config_dirs(void)
{
	GString *	string;
	gboolean	ret = TRUE;

	string = g_string_new(NULL);

	/* Test for gebr conf dir and subdirs */
	if (!gebr_make_config_dir(""))
		goto err;
	if (!gebr_make_config_dir("run"))
		goto err;
	if (!gebr_make_config_dir("log"))
		goto err;
	if (!gebr_make_config_dir("tmp"))
		goto err;
	if (!gebr_make_config_dir("debr"))
		goto err;
	if (!gebr_make_config_dir("gebr"))
		goto err;
	if (!gebr_make_config_dir("gebr/data"))
		goto err;

	/* DEPRECATED: migration from old structure */
	g_string_printf(string, "%s/.gebr/gebrdata", getenv("HOME"));
	if (g_file_test(string->str, G_FILE_TEST_IS_DIR | G_FILE_TEST_EXISTS) == TRUE) {
		g_string_printf(string, "mv %s/.gebr/gebrdata/* %s/.gebr/gebr/data; rmdir %s/.gebr/gebrdata/",
			getenv("HOME"), getenv("HOME"), getenv("HOME"));
		if (system(string->str))
			goto err;
	}
	g_string_printf(string, "%s/.gebr/menus.idx", getenv("HOME"));
	if (g_file_test(string->str, G_FILE_TEST_EXISTS) == TRUE) {
		g_string_printf(string, "mv %s/.gebr/menus.idx %s/.gebr/gebr",
			getenv("HOME"), getenv("HOME"));
		if (system(string->str))
			goto err;
	}
	g_string_printf(string, "%s/.gebr/gebr.conf", getenv("HOME"));
	if (g_file_test(string->str, G_FILE_TEST_EXISTS) == TRUE) {
		g_string_printf(string, "mv %s/.gebr/gebr.conf %s/.gebr/gebr",
			getenv("HOME"), getenv("HOME"));
		if (system(string->str))
			goto err;
	}
	g_string_printf(string, "%s/.gebr/debr.conf", getenv("HOME"));
	if (g_file_test(string->str, G_FILE_TEST_EXISTS) == TRUE) {
		g_string_printf(string, "mv %s/.gebr/debr.conf %s/.gebr/debr",
			getenv("HOME"), getenv("HOME"));
		if (system(string->str))
			goto err;
	}

	goto out;
err:	ret = FALSE;
out:	g_string_free(string, TRUE);

return ret;
}

/**
 * Simplified version of g_locale_to_utf8
 */
gchar *
g_simple_locale_to_utf8(const gchar * string)
{
	gchar *		output;
	gsize		bytes_read;
	gsize		bytes_written;
	GError *	error;

	error = NULL;
	output = g_locale_to_utf8(string, -1, &bytes_read, &bytes_written, &error);

	return output;
}

gboolean
g_key_file_has_key_woe(GKeyFile * key_file, const gchar * group, const gchar * key)
{
	GError *	error = NULL;

	return g_key_file_has_key(key_file, group, key, &error);
}

GString *
g_key_file_load_string_key(GKeyFile * key_file, const gchar * group, const gchar * key, const gchar * default_value)
{
	GString *	value;
	gchar *		tmp;
	GError *	error;

	error = NULL;
	value = g_string_new(NULL);
	tmp = g_key_file_get_string(key_file, group, key, &error);
	g_string_assign(value, ((error != NULL && error->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND) ||
                                ((tmp != NULL) && (strlen(tmp) == 0)))
		? default_value : tmp);

	g_free(tmp);

	return value;
}

gboolean
g_key_file_load_boolean_key(GKeyFile * key_file, const gchar * group, const gchar * key, gboolean default_value)
{
	gboolean	value;
	gboolean	tmp;
	GError *	error;

	error = NULL;
	tmp = g_key_file_get_boolean(key_file, group, key, &error);
	value = (error != NULL && error->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND)
		? default_value : tmp;

	return value;
}

int
g_key_file_load_int_key(GKeyFile * key_file, const gchar * group, const gchar * key, int default_value)
{
	int		value;
	int		tmp;
	GError *	error;

	error = NULL;
	tmp = g_key_file_get_integer(key_file, group, key, &error);
	value = (error != NULL && error->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND)
		? default_value : tmp;

	return value;
}
