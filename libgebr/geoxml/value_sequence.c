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

#include "value_sequence.h"
#include "xml.h"
#include "types.h"

/*
 * internal stuff
 */

struct gebr_geoxml_value_sequence {
	GdomeElement *element;
};

static gboolean __gebr_geoxml_value_sequence_check(GebrGeoXmlValueSequence * value_sequence)
{
	GdomeDOMString *name;

	name = gdome_el_nodeName((GdomeElement *) value_sequence, &exception);

	gboolean retval = (gboolean) !strcmp(name->str, "value") ||
			  (gboolean) !strcmp(name->str, "default") ||
			  (gboolean) !strcmp(name->str, "path") ||
			  (gboolean) !strcmp(name->str, "category");
	gdome_str_unref(name);
	return retval;
}

/*
 * library functions
 */

void gebr_geoxml_value_sequence_set(GebrGeoXmlValueSequence * value_sequence, const gchar * value)
{
	if (value_sequence == NULL || value == NULL)
		return;
	if (__gebr_geoxml_value_sequence_check(value_sequence) == FALSE)
		return;
	__gebr_geoxml_set_element_value((GdomeElement *) value_sequence, value, __gebr_geoxml_create_TextNode);
}

gchar *gebr_geoxml_value_sequence_get(GebrGeoXmlValueSequence * value_sequence)
{
	if (value_sequence == NULL)
		return NULL;
	if (__gebr_geoxml_value_sequence_check(value_sequence) == FALSE)
		return NULL;
	return __gebr_geoxml_get_element_value((GdomeElement *) value_sequence);
}

const gchar *gebr_geoxml_value_sequence_get_dictkey (GebrGeoXmlValueSequence *self)
{
	g_return_val_if_fail (self != NULL, NULL);

	return __gebr_geoxml_get_attr_value((GdomeElement*) self, "dictkeyword");
}

void gebr_geoxml_value_sequence_set_dictkey (GebrGeoXmlValueSequence *self, const gchar *dictkey)
{
	g_return_if_fail (self != NULL);

	__gebr_geoxml_set_attr_value((GdomeElement*) self, "dictkeyword", dictkey);
}
