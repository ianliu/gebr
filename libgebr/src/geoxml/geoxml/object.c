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

#include <string.h>

#include <gdome.h>

#include "object.h"
#include "types.h"
#include "xml.h"

/*
 * internal structures and funcionts
 */

struct geoxml_object {
	GdomeElement * element;
};

/*
 * library functions.
 */

enum GEOXML_OBJECT_TYPE
geoxml_object_get_type(GeoXmlObject * object)
{
	static const gchar *	tag_map [] = { "",
		"project", "line", "flow",
		"program", "parameters", "parameter",
		"option"
	};
	guint			i;

	if (object == NULL)
		return GEOXML_OBJECT_TYPE_UNKNOWN;

	for (i = 1; i <= 7; ++i)
		if (!strcmp(gdome_el_tagName((GdomeElement*)object, &exception)->str, tag_map[i]))
			return (enum GEOXML_OBJECT_TYPE)i;

	return GEOXML_OBJECT_TYPE_UNKNOWN;
}

void
geoxml_object_set_user_data(GeoXmlObject * object, gpointer user_data)
{
	if (object == NULL)
		return;
	((GdomeNode*)object)->user_data = user_data;
}

gpointer
geoxml_object_get_user_data(GeoXmlObject * object)
{
	if (object == NULL)
		return NULL;
	return ((GdomeNode*)object)->user_data;
}

GeoXmlDocument *
geoxml_object_get_owner_document(GeoXmlObject * object)
{
	if (object == NULL)
		return NULL;
	return (GeoXmlDocument*)gdome_el_ownerDocument((GdomeElement*)object, &exception);
}

GeoXmlObject *
geoxml_object_copy(GeoXmlObject * object)
{
	if (object == NULL)
		return NULL;
	return (GeoXmlObject*)gdome_el_cloneNode((GdomeElement*)object, TRUE, &exception);
}
