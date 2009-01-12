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

#include "project.h"
#include "document.h"
#include "document_p.h"
#include "defines.h"
#include "error.h"
#include "xml.h"
#include "types.h"

/*
 * internal structures and funcionts
 */

struct geoxml_project {
	GeoXmlDocument *	document;
};

struct geoxml_project_line {
	GdomeElement *		element;
};

/*
 * library functions.
 */

GeoXmlProject *
geoxml_project_new()
{
	GeoXmlDocument * document = geoxml_document_new("project", GEOXML_PROJECT_VERSION);
	return GEOXML_PROJECT(document);
}

GeoXmlProjectLine *
geoxml_project_append_line(GeoXmlProject * project, const gchar * source)
{
	if (project == NULL)
		return NULL;

	GeoXmlProjectLine* project_line;

	project_line = (GeoXmlProjectLine*)__geoxml_insert_new_element(
		geoxml_document_root_element(GEOXML_DOC(project)), "line", NULL);
	__geoxml_set_attr_value((GdomeElement*)project_line, "source", source);

	return project_line;
}

int
geoxml_project_get_line(GeoXmlProject * project, GeoXmlSequence ** project_line, gulong index)
{
	if (project == NULL) {
		*project_line = NULL;
		return GEOXML_RETV_NULL_PTR;
	}

	*project_line = (GeoXmlSequence*)__geoxml_get_element_at(
		geoxml_document_root_element(GEOXML_DOC(project)), "line", index, FALSE);

	return (*project_line == NULL)
		? GEOXML_RETV_INVALID_INDEX
		: GEOXML_RETV_SUCCESS;
}

glong
geoxml_project_get_lines_number(GeoXmlProject * project)
{
	if (project == NULL)
		return -1;
	return __geoxml_get_elements_number(geoxml_document_root_element(GEOXML_DOC(project)), "line");
}

void
geoxml_project_set_line_source(GeoXmlProjectLine * project_line, const gchar * source)
{
	if (project_line == NULL)
		return;
	__geoxml_set_attr_value((GdomeElement*)project_line, "source", source);
}

const gchar *
geoxml_project_get_line_source(GeoXmlProjectLine * project_line)
{
	if (project_line == NULL)
		return NULL;
	return __geoxml_get_attr_value((GdomeElement*)project_line, "source");
}
