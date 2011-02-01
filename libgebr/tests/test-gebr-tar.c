/*   libgebr - GêBR Library
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

#include <glib.h>
#include <string.h>

#include "../gebr-tar.h"

typedef struct {
	gboolean has1;
	gboolean has2;
	gchar *dirname;
} TarTest;

void func (const gchar *path, gpointer data)
{
	gchar *fname;
	TarTest *test = data;

	if (!test->dirname)
		test->dirname = g_path_get_dirname (path);

	fname = g_path_get_basename (path);
	if (g_str_equal (fname, "1"))
		test->has1 = TRUE;
	else if (g_str_equal (fname, "2"))
		test->has2 = TRUE;
	g_free (fname);
}

void test_gebr_tar_uncompress (void)
{
	TarTest test;
	GebrTar *tar = gebr_tar_new_from_file (TAR_TEST_FILE);

	test.has1 = FALSE;
	test.has2 = FALSE;
	test.dirname = NULL;

	g_assert (gebr_tar_uncompress (tar) == TRUE);

	gebr_tar_foreach (tar, func, &test);

	g_assert (test.has1 == TRUE);
	g_assert (test.has2 == TRUE);

	gebr_tar_free (tar);

	g_assert (g_file_test (test.dirname, G_FILE_TEST_IS_DIR) == FALSE);
}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/libgebr/tar/uncompress", test_gebr_tar_uncompress);

	return g_test_run();
}
