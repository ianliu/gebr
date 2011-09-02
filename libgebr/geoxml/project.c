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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <gdome.h>

#include "document.h"
#include "document_p.h"
#include "error.h"
#include "project.h"
#include "sequence.h"
#include "types.h"
#include "xml.h"

/*
 * Internal structures and functions
 */

struct gebr_geoxml_project {
	GebrGeoXmlDocument *document;
};

struct gebr_geoxml_project_line {
	GdomeElement *element;
};


/*
 * library functions.
 */

GebrGeoXmlProject *gebr_geoxml_project_new()
{
	GebrGeoXmlDocument *document = gebr_geoxml_document_new("project", GEBR_GEOXML_PROJECT_VERSION);
	return GEBR_GEOXML_PROJECT(document);
}

/*
 * FIXME: Should this check for duplicate sources? See tests
 */
GebrGeoXmlProjectLine *gebr_geoxml_project_append_line(GebrGeoXmlProject * project, const gchar * source)
{
	if (project == NULL)
		return NULL;

	GebrGeoXmlProjectLine *project_line;

	project_line = (GebrGeoXmlProjectLine *)
	    __gebr_geoxml_insert_new_element(gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(project)), "line", NULL);
	__gebr_geoxml_set_attr_value((GdomeElement *) project_line, "source", source);

	return project_line;
}

gboolean gebr_geoxml_project_remove_line(GebrGeoXmlProject * project, const gchar * source)
{
	GebrGeoXmlSequence * line;
	line = GEBR_GEOXML_SEQUENCE(gebr_geoxml_project_get_line_from_source(project, source));
	if (line) {
		gebr_geoxml_sequence_remove(line);
		return TRUE;
	}
	return FALSE;
}

gboolean gebr_geoxml_project_has_line(GebrGeoXmlProject * project, const gchar * source)
{
	return gebr_geoxml_project_get_line_from_source(project, source) != NULL;
}

int gebr_geoxml_project_get_line(GebrGeoXmlProject * project, GebrGeoXmlSequence ** project_line, gulong index)
{
	GdomeElement *root;

	if (project == NULL) {
		*project_line = NULL;
		return GEBR_GEOXML_RETV_NULL_PTR;
	}

	root = gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(project));
	*project_line = (GebrGeoXmlSequence *) __gebr_geoxml_get_element_at(root, "line", index, FALSE);
	gdome_el_unref(root, &exception);

	return (*project_line == NULL)
	    ? GEBR_GEOXML_RETV_INVALID_INDEX : GEBR_GEOXML_RETV_SUCCESS;
}

glong gebr_geoxml_project_get_lines_number(GebrGeoXmlProject * project)
{
	if (project == NULL)
		return -1;
	return __gebr_geoxml_get_elements_number(gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(project)), "line");
}

void gebr_geoxml_project_set_line_source(GebrGeoXmlProjectLine * project_line, const gchar * source)
{
	if (project_line == NULL)
		return;
	__gebr_geoxml_set_attr_value((GdomeElement *) project_line, "source", source);
}

const gchar *gebr_geoxml_project_get_line_source(GebrGeoXmlProjectLine * project_line)
{
	if (project_line == NULL)
		return NULL;
	return __gebr_geoxml_get_attr_value((GdomeElement *) project_line, "source");
}


GebrGeoXmlProjectLine *gebr_geoxml_project_get_line_from_source(GebrGeoXmlProject * project,
								       const gchar * source)
{
	GebrGeoXmlSequence * line;

	gebr_geoxml_project_get_line(project, &line, 0);
	while (line) {
		const gchar * path;
		path = gebr_geoxml_project_get_line_source(GEBR_GEOXML_PROJECT_LINE(line));
		if (strcmp(source, path) == 0)
			return GEBR_GEOXML_PROJECT_LINE(line);
		gebr_geoxml_sequence_next(&line);
	}
	return NULL;
}

