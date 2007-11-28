/*   libgeoxml - An interface to describe seismic software in XML
 *   Copyright (C) 2007  Bráulio Barros de Oliveira (brauliobo@gmail.com)
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

#include "category.h"
#include "xml.h"
#include "types.h"
#include "sequence.h"

/*
 * internal structures and funcionts
 */

struct geoxml_category {
	GdomeElement * element;
};

/*
 * library functions.
 */

void
geoxml_category_set_name(GeoXmlCategory * category, const gchar * name)
{
	if (category == NULL)
		return;
	__geoxml_set_element_value((GdomeElement*)category, name, __geoxml_create_TextNode);
}

const gchar *
geoxml_category_get_name(GeoXmlCategory * category)
{
	if (category == NULL)
		return NULL;
	return __geoxml_get_element_value((GdomeElement*)category);
}

GeoXmlFlow *
geoxml_category_flow(GeoXmlCategory * category)
{
	if (category == NULL)
		return NULL;
	return (GeoXmlFlow*)gdome_n_parentNode((GdomeNode*)category, &exception);
}

void
geoxml_category_previous(GeoXmlCategory ** category)
{
	geoxml_sequence_previous((GeoXmlSequence**)category);
}

void
geoxml_category_next(GeoXmlCategory ** category)
{
	geoxml_sequence_next((GeoXmlSequence**)category);
}

void
geoxml_category_remove(GeoXmlCategory * category)
{
	geoxml_sequence_remove((GeoXmlSequence*)category);
}
