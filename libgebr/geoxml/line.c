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

struct gebr_geoxml_line {
	GebrGeoXmlDocument *document;
};

struct gebr_geoxml_line_flow {
	GdomeElement *element;
};

struct gebr_geoxml_line_path {
	GdomeElement *element;
};

/*
 * library functions.
 */

GebrGeoXmlLine *gebr_geoxml_line_new()
{
	GebrGeoXmlDocument *document = gebr_geoxml_document_new("line", GEBR_GEOXML_LINE_VERSION);
	return GEBR_GEOXML_LINE(document);
}

GebrGeoXmlLineFlow *gebr_geoxml_line_append_flow(GebrGeoXmlLine * line, const gchar * source)
{
	if (line == NULL)
		return NULL;

	GebrGeoXmlLineFlow *line_flow;

	line_flow = (GebrGeoXmlLineFlow *)
	    __gebr_geoxml_insert_new_element(gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(line)), "flow", NULL);
	__gebr_geoxml_set_attr_value((GdomeElement *) line_flow, "source", source);

	return line_flow;
}

int gebr_geoxml_line_get_flow(GebrGeoXmlLine * line, GebrGeoXmlSequence ** line_flow, gulong index)
{
	if (line == NULL) {
		*line_flow = NULL;
		return GEBR_GEOXML_RETV_NULL_PTR;
	}

	*line_flow = (GebrGeoXmlSequence *)
	    __gebr_geoxml_get_element_at(gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(line)), "flow", index,
					 FALSE);

	return (*line_flow == NULL)
	    ? GEBR_GEOXML_RETV_INVALID_INDEX : GEBR_GEOXML_RETV_SUCCESS;
}

glong gebr_geoxml_line_get_flows_number(GebrGeoXmlLine * line)
{
	if (line == NULL)
		return -1;
	return __gebr_geoxml_get_elements_number(gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(line)), "flow");
}

void gebr_geoxml_line_set_flow_source(GebrGeoXmlLineFlow * line_flow, const gchar * source)
{
	if (line_flow == NULL)
		return;
	__gebr_geoxml_set_attr_value((GdomeElement *) line_flow, "source", source);
}

const gchar *gebr_geoxml_line_get_flow_source(GebrGeoXmlLineFlow * line_flow)
{
	if (line_flow == NULL)
		return NULL;
	return __gebr_geoxml_get_attr_value((GdomeElement *) line_flow, "source");
}

GebrGeoXmlLinePath *gebr_geoxml_line_append_path(GebrGeoXmlLine * line, const gchar * path)
{
	if (line == NULL || path == NULL)
		return NULL;

	GebrGeoXmlLinePath *line_path;

	line_path = (GebrGeoXmlLinePath *)
	    __gebr_geoxml_insert_new_element(gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(line)), 
					     "path",
					     __gebr_geoxml_get_first_element(gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(line)), 
									     "flow"));
	gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(line_path), path);

	return line_path;
}

int gebr_geoxml_line_get_path(GebrGeoXmlLine * line, GebrGeoXmlSequence ** path, gulong index)
{
	if (line == NULL) {
		*path = NULL;
		return GEBR_GEOXML_RETV_NULL_PTR;
	}

	*path = (GebrGeoXmlSequence *)
	    __gebr_geoxml_get_element_at(gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(line)), "path", index,
					 FALSE);

	return (*path == NULL)
	    ? GEBR_GEOXML_RETV_INVALID_INDEX : GEBR_GEOXML_RETV_SUCCESS;
}

glong gebr_geoxml_line_get_paths_number(GebrGeoXmlLine * line)
{
	if (line == NULL)
		return -1;
	return __gebr_geoxml_get_elements_number(gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(line)), "path");
}
