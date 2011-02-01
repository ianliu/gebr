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

#ifndef __GEBR_TAR_H__
#define __GEBR_TAR_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct _GebrTar GebrTar;
typedef void (*GebrTarFunc) (const gchar *file, gpointer data);

/**
 * gebr_tar_new:
 * @path: the path for a new tar file
 *
 * Creates a new tar file in @path. Make sure you have write permissions for @path.
 *
 * Returns: if you have write permissions for @path, return a #GebrTar,
 * otherwise returns %NULL.
 */
GebrTar *gebr_tar_create (const gchar *path);

/**
 * gebr_tar_append:
 * @self: a #GebrTar created with gebr_tar_create()
 * @path: a path for a file accessible through @root_dir (see gebr_tar_compact())
 *
 * Inserts @path into the tar represented by @self. After you inserted
 * all files, call gebr_tar_compact() to generate the tar file.
 */
void gebr_tar_append (GebrTar *self, const gchar *path);

/**
 * gebr_tar_compact:
 * @self: a #GebrTar created with gebr_tar_create()
 * @root_dir: the directory in which files are searched for
 *
 * Compacts the files into a gziped tar. The @root_dir specifies the directory in
 * which files are searched for. The @root_dir is equivalent to the -C option of
 * tar command line.
 *
 * Returns: %TRUE if compact was successful, %FALSE otherwise.
 */
gboolean gebr_tar_compact (GebrTar *self, const gchar *root_dir);

/**
 * gebr_tar_new_from_file:
 * @path: a path for a file on the system
 *
 * Creates a #GebrTar from an existing file on the system.
 * You can call gebr_tar_extract() on this object.
 *
 * Returns: If @path exists and it is readable, returns a #GebrTar,
 * otherwise %NULL is returned.
 */
GebrTar *gebr_tar_new_from_file (const gchar *path);

/**
 * gebr_tar_extract:
 * @self: a #GebrTar object created with gebr_tar_new_from_file()
 *
 * If the #GebrTar was created with gebr_tar_new_from_file() constructor, then you
 * can extract it by calling this function on it. The extraction is done in a temporary
 * folder. To access the files you should use the gebr_tar_foreach() method.
 *
 * <note>
 *  <para>
 *   The temporary folder is deleted when gebr_tar_free() is called.
 *  </para>
 * </note>
 *
 * Returns: %TRUE if extraction worked, %FALSE otherwise.
 */
gboolean gebr_tar_extract (GebrTar *self);

/**
 * gebr_tar_foreach:
 * @self: a #GebrTar object, after gebr_tar_extract() was called
 * @func: a function to be called for each file in @self
 * @data: data to pass to @func calls
 *
 * Calls @func for each file inside the tar file represented by @self. You should
 * process the file here, ie move, copy, modify to any place. Notice that the files
 * are extracted into a temporary folder, which will be deleted upon gebr_tar_free()
 * call, so don't rely on them!
 */
void gebr_tar_foreach (GebrTar *self, GebrTarFunc func, gpointer data);

/**
 * gebr_tar_free:
 * @self: a #GebrTar
 *
 * If @self was extracted with gebr_tar_extract(), then this function will remove
 * all extracted files. After that the structure is freed.
 */
void gebr_tar_free (GebrTar *self);

G_END_DECLS

#endif /* __GEBR_TAR_H__ */
