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

#include "line.h"
#include "document.h"
#include "document_p.h"
#include "error.h"
#include "xml.h"
#include "types.h"
#include "sequence.h"

/*
 * internal structures and funcionts
 */

struct geoxml_line {
	GeoXmlDocument * document;
};

struct geoxml_line_flow {
	GdomeElement * element;
};

/*
 * library functions.
 */

GeoXmlLine *
geoxml_line_new()
{
	GeoXmlDocument * document = geoxml_document_new("line", "0.1.0");
	return GEOXML_LINE(document);
}

GeoXmlLineFlow *
geoxml_line_new_flow(GeoXmlLine * line, const gchar * source)
{
	if (line == NULL)
		return NULL;

	GeoXmlLineFlow * line_flow;

	line_flow =(GeoXmlLineFlow*)__geoxml_new_element(geoxml_document_root_element(GEOXML_DOC(line)), NULL, "flow");
	__geoxml_set_attr_value((GdomeElement*)line_flow, "source", source);

	return line_flow;
}

int
geoxml_line_get_flow(GeoXmlLine * line, GeoXmlLineFlow ** line_flow, gulong index)
{
	if (line == NULL) {
		*line_flow = NULL;
		return GEOXML_RETV_NULL_PTR;
	}

	*line_flow = (GeoXmlLineFlow*)__geoxml_get_element_at(
		geoxml_document_root_element(GEOXML_DOC(line)), "flow", index);

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

void
geoxml_line_previous_flow(GeoXmlLineFlow ** line_flow)
{
	geoxml_sequence_previous((GeoXmlSequence**)line_flow);
}

void
geoxml_line_next_flow(GeoXmlLineFlow ** line_flow)
{
	geoxml_sequence_next((GeoXmlSequence**)line_flow);
}

GeoXmlLineFlow *
geoxml_line_add_flow(GeoXmlLine * line, const gchar * source)
{
	if (line == NULL)
		return NULL;

	GeoXmlLineFlow* line_flow;

	line_flow = (GeoXmlLineFlow*)__geoxml_new_element(geoxml_document_root_element(GEOXML_DOC(line)), NULL, "flow");
	__geoxml_set_attr_value((GdomeElement*)line_flow, "source", source);

	return line_flow;
}

void
geoxml_line_remove_flow(GeoXmlLine * line, GeoXmlLineFlow * line_flow)
{
	geoxml_sequence_remove((GeoXmlSequence*)line_flow);
}
