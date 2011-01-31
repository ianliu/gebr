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
 */
GebrTar *gebr_tar_new (void);

/**
 * gebr_tar_new_from_file:
 */
GebrTar *gebr_tar_new_from_file (const gchar *path);

/**
 * gebr_tar_uncompress:
 */
gboolean gebr_tar_uncompress (GebrTar *self);

/**
 * gebr_tar_foreach:
 */
void gebr_tar_foreach (GebrTar *self, GebrTarFunc func, gpointer data);

/**
 * gebr_tar_free:
 */
void gebr_tar_free (GebrTar *self);

G_END_DECLS

#endif /* __GEBR_TAR_H__ */
