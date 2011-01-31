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

#include <glib/gstdio.h>
#include <string.h>

#include "gebr-tar.h"
#include "utils.h"

struct _GebrTar {
	gchar *tar_path;
	gchar *uncompress_dir;
	gchar **files;
};

GebrTar *gebr_tar_new (void)
{
	GebrTar *self;
	self = g_new0 (GebrTar, 1);
	return self;
}

GebrTar *gebr_tar_new_from_file (const gchar *path)
{
	GebrTar *self;
	self = g_new0 (GebrTar, 1);
	self->tar_path = g_strdup (path);
	return self;
}

gboolean gebr_tar_uncompress (GebrTar *self)
{
	GString *tmp;
	gchar *command;
	gchar *output;

	g_return_val_if_fail (self->tar_path != NULL, FALSE);

	tmp = gebr_temp_directory_create ();
	self->uncompress_dir = g_string_free (tmp, FALSE);

	command = g_strdup_printf ("tar zxfv %s -C %s",
				   self->tar_path,
				   self->uncompress_dir);

	if (!g_spawn_command_line_sync (command, &output,
					NULL, NULL, NULL)) {
		g_free (command);
		return FALSE;
	}

	self->files = g_strsplit (output, "\n", 0);

	g_free (command);
	g_free (output);
	return TRUE;
}

void gebr_tar_foreach (GebrTar *self, GebrTarFunc func, gpointer data)
{
	gchar *file;
	gchar *abs;

	if (!self->uncompress_dir)
		return;

	for (int i = 0; self->files[i]; i++) {
		file = self->files[i];

		if (strlen (file) == 0)
			continue;

		abs = g_build_path ("/", self->uncompress_dir, file, NULL);
		func (abs, data);
		g_free (abs);
		file = self->files[i];
	}
}

static void remove_dir (const gchar *directory)
{
	GDir *dir;
	gchar *abs;
	const gchar *name;

	dir = g_dir_open (directory, 0, NULL);
	while ((name = g_dir_read_name (dir))) {
		abs = g_build_path ("/", directory, name, NULL);
		if (g_file_test (abs, G_FILE_TEST_IS_DIR))
			remove_dir (abs);
		else
			g_unlink (abs);
		g_free (abs);
	}
	g_dir_close (dir);
	g_rmdir (directory);
}

void gebr_tar_free (GebrTar *self)
{
	remove_dir (self->uncompress_dir);
	g_free (self->tar_path);
	g_free (self->uncompress_dir);
	g_strfreev (self->files);
	g_free (self);
}
