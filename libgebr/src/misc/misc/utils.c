/*   libgebr - GÍBR Library
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

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <glib/gstdio.h>

#include "utils.h"

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
	g_string_printf(path, "%s/%s", g_get_tmp_dir(), template);

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

/**
 * Create all configurations directories for all GÍBR programs.
 * Used by GÍBR programs before read/write config..
 */
gboolean
gebr_create_config_dirs(void)
{
	GString *	gebr;
	GString *	aux;
	gboolean	ret;

	ret = TRUE;
	gebr = g_string_new(NULL);
	aux = g_string_new(NULL);

	/* Test for gebr conf dir */
	g_string_printf(gebr, "%s/.gebr", getenv("HOME"));
	if (g_file_test(gebr->str, G_FILE_TEST_IS_DIR | G_FILE_TEST_EXISTS) == FALSE) {
		if (g_mkdir(gebr->str, home_mode())) {
			ret = FALSE;
			goto out;
		}
	}

	/* Test for .gebr/run conf dir */
	g_string_printf(aux, "%s/run", gebr->str);
	if (g_file_test(aux->str, G_FILE_TEST_IS_DIR | G_FILE_TEST_EXISTS) == FALSE) {
		if (g_mkdir(aux->str, home_mode())) {
			ret = FALSE;
			goto out;
		}
	}

	/* Test for .gebr/log conf dir */
	g_string_printf(aux, "%s/log", gebr->str);
	if (g_file_test(aux->str, G_FILE_TEST_IS_DIR | G_FILE_TEST_EXISTS) == FALSE) {
		if (g_mkdir(aux->str, home_mode())) {
			ret = FALSE;
			goto out;
		}
	}

out:	g_string_free(gebr, TRUE);
	g_string_free(aux, TRUE);
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
