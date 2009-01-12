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

#include <gdome.h>

#include "line.h"
#include "document.h"
#include "document_p.h"
#include "defines.h"
#include "error.h"
#include "xml.h"
#include "types.h"
#include "value_sequence.h"

/*
 * internal structures and funcionts
 */

struct geoxml_line {
	GeoXmlDocument *	document;
};

struct geoxml_line_flow {
	GdomeElement *		element;
};

struct geoxml_line_path {
	GdomeElement *		element;
};

/*
 * library functions.
 */

GeoXmlLine *
geoxml_line_new()
{
	GeoXmlDocument * document = geoxml_document_new("line", GEOXML_LINE_VERSION);
	return GEOXML_LINE(document);
}

GeoXmlLineFlow *
geoxml_line_append_flow(GeoXmlLine * line, const gchar * source)
{
	if (line == NULL)
		return NULL;

	GeoXmlLineFlow *	line_flow;

	line_flow = (GeoXmlLineFlow*)__geoxml_insert_new_element(
		geoxml_document_root_element(GEOXML_DOC(line)), "flow", NULL);
	__geoxml_set_attr_value((GdomeElement*)line_flow, "source", source);

	return line_flow;
}

int
geoxml_line_get_flow(GeoXmlLine * line, GeoXmlSequence ** line_flow, gulong index)
{
	if (line == NULL) {
		*line_flow = NULL;
		return GEOXML_RETV_NULL_PTR;
	}

	*line_flow = (GeoXmlSequence*)__geoxml_get_element_at(
		geoxml_document_root_element(GEOXML_DOC(line)), "flow", index, FALSE);

	return (*line_flow == NULL)
		? GEOXML_RETV_INVALID_INDEX
		: GEOXML_RETV_SUCCESS;
}

glong
geoxml_line_get_flows_number(GeoXmlLine * line)
{
	if (line == NULL)
		return -1;
	return __geoxml_get_elements_number(geoxml_document_root_element(GEOXML_DOC(line)), "flow");
}

void
geoxml_line_set_flow_source(GeoXmlLineFlow * line_flow, const gchar * source)
{
	if (line_flow == NULL)
		return;
	__geoxml_set_attr_value((GdomeElement*)line_flow, "source", source);
}

const gchar *
geoxml_line_get_flow_source(GeoXmlLineFlow * line_flow)
{
	if (line_flow == NULL)
		return NULL;
	return __geoxml_get_attr_value((GdomeElement*)line_flow, "source");
}

GeoXmlLinePath *
geoxml_line_append_path(GeoXmlLine * line, const gchar * path)
{
	if (line == NULL || path == NULL)
		return NULL;

	GeoXmlLinePath *	line_path;

	line_path = (GeoXmlLinePath*)__geoxml_insert_new_element(
		geoxml_document_root_element(GEOXML_DOC(line)), "path",
		__geoxml_get_first_element(geoxml_document_root_element(GEOXML_DOC(line)), "flow"));
	geoxml_value_sequence_set(GEOXML_VALUE_SEQUENCE(line_path), path);

	return line_path;
}

int
geoxml_line_get_path(GeoXmlLine * line, GeoXmlSequence ** path, gulong index)
{
	if (line == NULL) {
		*path = NULL;
		return GEOXML_RETV_NULL_PTR;
	}

	*path = (GeoXmlSequence*)__geoxml_get_element_at(
		geoxml_document_root_element(GEOXML_DOC(line)), "path", index, FALSE);

	return (*path == NULL)
		? GEOXML_RETV_INVALID_INDEX
		: GEOXML_RETV_SUCCESS;
}

glong
geoxml_line_get_paths_number(GeoXmlLine * line)
{
	if (line == NULL)
		return -1;
	return __geoxml_get_elements_number(geoxml_document_root_element(GEOXML_DOC(line)), "path");
}
