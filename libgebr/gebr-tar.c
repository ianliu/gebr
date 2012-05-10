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
	gchar *extract_dir;
	GList *files;
};

GebrTar *gebr_tar_create (const gchar *path)
{
	GebrTar *self;
	self = g_new0 (GebrTar, 1);
	self->tar_path = g_strdup (path);
	return self;
}

void gebr_tar_append (GebrTar *self, const gchar *path)
{
	self->files = g_list_append (self->files, g_strdup (path));
}

const gchar * gebr_tar_get_dir (GebrTar *self)
{
	return self->extract_dir;
}

gboolean gebr_tar_compact (GebrTar *self, const gchar *root_dir)
{
	gchar *command;
	gchar *stderr;
	GString *files;
	GError *error = NULL;
	int exit;

	files = g_string_new ("");
	for (GList *i = self->files; i; i = i->next) {
		gchar *file = i->data;
		g_string_append_printf (files, "%s ", g_shell_quote(file));
	}

	gchar *rootdir = g_shell_quote(root_dir);
	gchar *tar_path = g_shell_quote(self->tar_path);
	gchar *subcommand = g_strdup_printf("tar cC %s %s | xz > %s",
	                                    rootdir,
	                                    files->len ? files->str : ".",
	                                    tar_path);

	gchar *quoted_cmd = g_shell_quote(subcommand);
	command = g_strdup_printf ("/bin/bash -c %s", quoted_cmd);

	g_free(rootdir);
	g_free(tar_path);
	g_free(subcommand);
	g_free(quoted_cmd);

	g_string_free (files, TRUE);
	g_spawn_command_line_sync(command, NULL, &stderr, &exit, &error);
	g_free (command);

	if (exit || error) {
		g_warning("Error creating compressed archive: %s", error? error->message : stderr);
		g_free (stderr);
		return FALSE;
	}

	return TRUE;
}

GebrTar *gebr_tar_new_from_file (const gchar *path)
{
	GebrTar *self;
	self = g_new0 (GebrTar, 1);
	self->tar_path = g_strdup (path);
	return self;
}

gboolean gebr_tar_is_gzip (GebrTar *self)
{
	g_return_val_if_fail (self->tar_path != NULL, FALSE);
	int last_char = strlen(self->tar_path);
	return self->tar_path[last_char-1] == 'z';
}

gboolean gebr_tar_extract (GebrTar *self)
{
	GString *tmp;
	gchar *command;
	gchar *output;
	gchar *stderr;
	gchar **files;

	g_return_val_if_fail (self->tar_path != NULL, FALSE);

	if (!g_file_test (self->tar_path, G_FILE_TEST_EXISTS))
		return FALSE;

	tmp = gebr_temp_directory_create ();
	self->extract_dir = g_string_free (tmp, FALSE);

	gchar *tar_path = g_shell_quote(self->tar_path);
	gchar *extract_dir = g_shell_quote(self->extract_dir);

	gchar *subcommand = g_strdup_printf("%s %s | tar xvC %s",
	                                    gebr_tar_is_gzip(self) ? "zcat" : "xz -dc",
	                                    tar_path, extract_dir);

	gchar *quoted_cmd = g_shell_quote(subcommand);
	command = g_strdup_printf ("/bin/bash -c %s", quoted_cmd);

	g_free(tar_path);
	g_free(extract_dir);
	g_free(quoted_cmd);
	g_free(subcommand);

	GError *error = NULL;
	int exit = 0;

	if (!g_spawn_command_line_sync (command, &output,
					&stderr, &exit, &error)) {
		g_free (command);
		return FALSE;
	}

	files = g_strsplit (output, "\n", 0);
	for (int i = 0; files[i]; i++)
		self->files = g_list_prepend (self->files, files[i]);
	self->files = g_list_reverse (self->files);

	g_free (files);
	g_free (command);
	g_free (output);
	g_free (stderr);
	return TRUE;
}

void gebr_tar_foreach (GebrTar *self, GebrTarFunc func, gpointer data)
{
	gchar *file;
	gchar *abs;

	if (!self->extract_dir)
		return;

	for (GList *i = self->files; i; i = i->next) {
		file = i->data;

		if (strlen (file) == 0)
			continue;

		abs = g_build_path ("/", self->extract_dir, file, NULL);
		func (abs, data);
		g_free (abs);
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
	if (self->extract_dir) {
		remove_dir (self->extract_dir);
		g_free (self->extract_dir);
	}
	g_list_foreach (self->files, (GFunc) g_free, NULL);
	g_list_free (self->files);
	g_free (self->tar_path);
	g_free (self);
}
