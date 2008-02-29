/*   libgebr - G�BR Library
 *   Copyright (C) 2007-2008 G�BR core team (http://gebr.sourceforge.net)
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

#include <gdome.h>

#include "enum_option.h"
#include "xml.h"
#include "types.h"
#include "sequence.h"

/*
 * internal structures and funcionts
 */

struct geoxml_enum_option {
	GdomeElement * element;
};

/*
 * library functions.
 */

GeoXmlProgramParameter *
geoxml_enum_option_program_parameter(GeoXmlEnumOption * enum_option)
{
	if (enum_option == NULL)
		return NULL;
	return (GeoXmlProgramParameter*)gdome_el_parentNode((GdomeElement*)enum_option, &exception);
}

void
geoxml_enum_option_set_label(GeoXmlEnumOption * enum_option, const gchar * label)
{
	if (enum_option == NULL || label == NULL)
		return;
	__geoxml_set_tag_value((GdomeElement*)enum_option, "label", label, __geoxml_create_TextNode);
}

const gchar *
geoxml_enum_option_get_label(GeoXmlEnumOption * enum_option)
{
	return __geoxml_get_tag_value((GdomeElement*)enum_option, "label");
}

void
geoxml_enum_option_set_value(GeoXmlEnumOption * enum_option, const gchar * value)
{
	if (enum_option == NULL || value == NULL)
		return;
	__geoxml_set_tag_value((GdomeElement*)enum_option, "value", value, __geoxml_create_TextNode);
}

const gchar *
geoxml_enum_option_get_value(GeoXmlEnumOption * enum_option)
{
	return __geoxml_get_tag_value((GdomeElement*)enum_option, "value");
}
