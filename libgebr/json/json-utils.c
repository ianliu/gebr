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

#include "json-utils.h"

GObject * json_gobject_from_data               (GType                    gtype,
                                                const gchar             *data,
                                                gssize                   length,
                                                GError                 **error);
gchar *   json_gobject_to_data                 (GObject                 *gobject,
                                                gsize                   *length);
gboolean json_gobject_to_file(GObject * object, const gchar *filename)
{
	gchar *data;
	gsize length;
	data = json_gobject_to_data(object, &length);
	if (!data)
		return FALSE;

	GError *error = NULL;
	gboolean ret = g_file_set_contents(filename, data, length, &error);
	g_free(data);
	if (!ret)
		return FALSE;

	return ret;
}

GObject *json_gobject_from_file(GType type, const gchar *filename)
{
	gchar *data;
	gsize length;
	GError *error = NULL;
	gboolean ret = g_file_get_contents(filename, &data, &length, &error);
	if (!ret)
		return NULL;

	error = NULL;
	GObject *object = json_gobject_from_data(type, data, length, &error);
	g_free(data);

	return object;
}
