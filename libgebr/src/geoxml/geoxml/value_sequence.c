/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
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

#include "value_sequence.h"
#include "xml.h"
#include "types.h"

/*
 * internal stuff
 */

struct geoxml_value_sequence {
	GdomeElement * element;
};

static gboolean
__geoxml_value_sequence_check(GeoXmlValueSequence * value_sequence)
{
	GdomeDOMString *	name;

	name = gdome_el_nodeName((GdomeElement*)value_sequence, &exception);

	return (gboolean)!strcmp(name->str, "value") ||
		(gboolean)!strcmp(name->str, "default") ||
		(gboolean)!strcmp(name->str, "path") ||
		(gboolean)!strcmp(name->str, "category");
}

/*
 * library functions
 */

void
geoxml_value_sequence_set(GeoXmlValueSequence * value_sequence, const gchar * value)
{
	if (value_sequence == NULL || value == NULL)
		return;
	if (__geoxml_value_sequence_check(value_sequence) == FALSE)
		return;
	__geoxml_set_element_value((GdomeElement*)value_sequence, value, __geoxml_create_TextNode);
}

// void
// geoxml_value_sequence_set_boolean(GeoXmlValueSequence * value_sequence, gboolean state)
// {
// 	if (value_sequence == NULL)
// 		return;
// 	if (__geoxml_value_sequence_check(value_sequence) == FALSE)
// 		return;
// 	__geoxml_set_element_value((GdomeElement*)value_sequence,
// 		state == TRUE ? "on" : "off", __geoxml_create_TextNode);
// }

const gchar *
geoxml_value_sequence_get(GeoXmlValueSequence * value_sequence)
{
	if (value_sequence == NULL)
		return NULL;
	if (__geoxml_value_sequence_check(value_sequence) == FALSE)
		return NULL;
	return __geoxml_get_element_value((GdomeElement*)value_sequence);
}

// gboolean
// geoxml_value_sequence_get_boolean(GeoXmlValueSequence * value_sequence)
// {
// 	if (value_sequence == NULL)
// 		return NULL;
// 	if (__geoxml_value_sequence_check(value_sequence) == FALSE)
// 		return NULL;
// 	return !strcmp(__geoxml_get_element_value((GdomeElement*)value_sequence), "on")
// 		? TRUE : FALSE;
// }
