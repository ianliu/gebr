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

#include "../libgebr-gettext.h"

#include <gdome.h>
#include <glib/gi18n-lib.h>

#include "line.h"
#include "document.h"
#include "document_p.h"
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
	GdomeElement *root = gebr_geoxml_document_root_element(document);
	gdome_el_unref(__gebr_geoxml_insert_new_element(root, "server-group", NULL), &exception);
	gdome_el_unref(root, &exception);
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
	gint retval;
	if (line == NULL) {
		*line_flow = NULL;
		return GEBR_GEOXML_RETV_NULL_PTR;
	}
	GdomeElement *root = gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(line));
	*line_flow = (GebrGeoXmlSequence *)__gebr_geoxml_get_element_at(root, "flow", index, FALSE);
	gdome_el_unref(root, &exception);

	retval = (*line_flow == NULL) ? GEBR_GEOXML_RETV_INVALID_INDEX : GEBR_GEOXML_RETV_SUCCESS;
	return retval;
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

	GdomeElement *root = gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(line));
	*path = (GebrGeoXmlSequence *)__gebr_geoxml_get_element_at(root, "path", index, FALSE);

	gdome_el_unref(root, &exception);

	return (*path == NULL)
	    ? GEBR_GEOXML_RETV_INVALID_INDEX : GEBR_GEOXML_RETV_SUCCESS;
}

glong gebr_geoxml_line_get_paths_number(GebrGeoXmlLine * line)
{
	if (line == NULL)
		return -1;
	GdomeElement *root = gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(line));
	gulong retval = __gebr_geoxml_get_elements_number(root, "path");
	gdome_el_unref(root, &exception);
	return retval;
}

void gebr_geoxml_line_set_group (GebrGeoXmlLine *line, const gchar *group, gboolean is_fs)
{
	gchar *gen;
	const gchar *prefix;
	GdomeElement *root;
	GdomeElement *group_el;

	prefix = is_fs? "fs:":"g:";
	gen = g_strconcat (prefix, group, NULL);
	root = gebr_geoxml_document_root_element (line);
	group_el = __gebr_geoxml_get_first_element (root, "server-group");
	__gebr_geoxml_set_element_value (group_el, gen,
					 __gebr_geoxml_create_TextNode);
	g_free (gen);
}

const gchar *gebr_geoxml_line_get_group (GebrGeoXmlLine *line, gboolean *is_fs)
{
	gchar *name;
	gchar *group;
	GdomeElement *root;
	GdomeElement *group_el;

	g_return_val_if_fail (line != NULL, NULL);
	g_return_val_if_fail (is_fs != NULL, NULL);

	root = gebr_geoxml_document_root_element (line);
	group_el = __gebr_geoxml_get_first_element (root, "server-group");
	group = __gebr_geoxml_get_element_value (group_el);

	gdome_el_unref(root, &exception);
	gdome_el_unref(group_el, &exception);

	if (g_str_has_prefix (group, "fs:")) {
		*is_fs = TRUE;
		name = g_strdup(group + 3);
		g_free(group);
		return name;
	} else if (g_str_has_prefix (group, "g:")) {
		*is_fs = FALSE;
		name = g_strdup(group + 2);
		g_free(group);
		return name;
	} else {
		*is_fs = FALSE;
		return group;
	}
}

const gchar *gebr_geoxml_line_get_group_label (GebrGeoXmlLine *line)
{
	gboolean is_fs;
	const gchar *group = gebr_geoxml_line_get_group(line, &is_fs);

	if(g_strcmp0(group, "") == 0) {
		return _("All Servers");
	}
	return group;
}
