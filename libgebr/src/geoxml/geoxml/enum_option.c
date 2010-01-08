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

#include <gdome.h>

#include "enum_option.h"
#include "xml.h"
#include "types.h"
#include "sequence.h"

/*
 * internal structures and funcionts
 */

struct gebr_geoxml_enum_option {
	GdomeElement *element;
};

/*
 * library functions.
 */

void gebr_geoxml_enum_option_set_label(GebrGeoXmlEnumOption * enum_option, const gchar * label)
{
	if (enum_option == NULL || label == NULL)
		return;
	__gebr_geoxml_set_tag_value((GdomeElement *) enum_option, "label", label, __gebr_geoxml_create_TextNode);
}

const gchar *gebr_geoxml_enum_option_get_label(GebrGeoXmlEnumOption * enum_option)
{
	return __gebr_geoxml_get_tag_value((GdomeElement *) enum_option, "label");
}

void gebr_geoxml_enum_option_set_value(GebrGeoXmlEnumOption * enum_option, const gchar * value)
{
	if (enum_option == NULL || value == NULL)
		return;
	__gebr_geoxml_set_tag_value((GdomeElement *) enum_option, "value", value, __gebr_geoxml_create_TextNode);
}

const gchar *gebr_geoxml_enum_option_get_value(GebrGeoXmlEnumOption * enum_option)
{
	return __gebr_geoxml_get_tag_value((GdomeElement *) enum_option, "value");
}
