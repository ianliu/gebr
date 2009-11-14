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

#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include <gdome.h>

#include "program.h"
#include "parameters.h"
#include "xml.h"
#include "error.h"
#include "types.h"
#include "sequence.h"

/*
 * internal structures and funcionts
 */

struct gebr_geoxml_program {
	GdomeElement * element;
};

/*
 * library functions.
 */

GebrGeoXmlFlow *
gebr_geoxml_program_flow(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return (GebrGeoXmlFlow*)gdome_n_parentNode((GdomeNode*)program, &exception);
}

GebrGeoXmlParameters *
gebr_geoxml_program_get_parameters(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return (GebrGeoXmlParameters*)__gebr_geoxml_get_first_element((GdomeElement*)program, "parameters");
}

void
gebr_geoxml_program_set_stdin(GebrGeoXmlProgram * program, const gboolean enable)
{
	if (program == NULL)
		return;
	__gebr_geoxml_set_attr_value((GdomeElement*)program, "stdin", (enable == TRUE ? "yes" : "no"));
}

void
gebr_geoxml_program_set_stdout(GebrGeoXmlProgram * program, const gboolean enable)
{
	if (program == NULL)
		return;
	__gebr_geoxml_set_attr_value((GdomeElement*)program, "stdout", (enable == TRUE ? "yes" : "no"));
}

void
gebr_geoxml_program_set_stderr(GebrGeoXmlProgram * program, const gboolean enable)
{
	if (program == NULL)
		return;
	__gebr_geoxml_set_attr_value((GdomeElement*)program, "stderr", (enable == TRUE ? "yes" : "no"));
}

void
gebr_geoxml_program_set_status(GebrGeoXmlProgram * program, const gchar * status)
{
	if (program == NULL)
		return;
	__gebr_geoxml_set_attr_value((GdomeElement*)program, "status", status);
}

void
gebr_geoxml_program_set_title(GebrGeoXmlProgram * program, const gchar * title)
{
	if (program == NULL || title == NULL)
		return;
	__gebr_geoxml_set_tag_value((GdomeElement*)program, "title", title, __gebr_geoxml_create_TextNode);
}

void
gebr_geoxml_program_set_menu(GebrGeoXmlProgram * program, const gchar * menu, gulong index)
{
	if (program == NULL || menu == NULL)
		return;

	gchar * tmp;

	__gebr_geoxml_set_tag_value((GdomeElement*)program, "menu", menu, __gebr_geoxml_create_TextNode);
	tmp = g_strdup_printf("%ld", index);
	__gebr_geoxml_set_attr_value(
		__gebr_geoxml_get_first_element((GdomeElement*)program, "menu"), "index", tmp);

	g_free(tmp);
}

void
gebr_geoxml_program_set_binary(GebrGeoXmlProgram * program, const gchar * binary)
{
	if (program == NULL || binary == NULL)
		return;
	__gebr_geoxml_set_tag_value((GdomeElement*)program, "binary", binary, __gebr_geoxml_create_TextNode);
}

void
gebr_geoxml_program_set_description(GebrGeoXmlProgram * program, const gchar * description)
{
	if (program == NULL || description == NULL)
		return;
	__gebr_geoxml_set_tag_value((GdomeElement*)program, "description", description, __gebr_geoxml_create_TextNode);
}

void
gebr_geoxml_program_set_help(GebrGeoXmlProgram * program, const gchar * help)
{
	if (program == NULL || help == NULL)
		return;
	__gebr_geoxml_set_tag_value((GdomeElement*)program, "help", help, __gebr_geoxml_create_CDATASection);
}

void
gebr_geoxml_program_set_url(GebrGeoXmlProgram * program, const gchar * url)
{
	if (program == NULL || url == NULL)
		return;
	__gebr_geoxml_set_tag_value((GdomeElement*)program, "url", url, __gebr_geoxml_create_TextNode);
}

gboolean
gebr_geoxml_program_get_stdin(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return FALSE;
	return (!strcmp(__gebr_geoxml_get_attr_value((GdomeElement*)program, "stdin"), "yes"))
		? TRUE : FALSE;
}

gboolean
gebr_geoxml_program_get_stdout(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return FALSE;
	return (!strcmp(__gebr_geoxml_get_attr_value((GdomeElement*)program, "stdout"), "yes"))
		? TRUE : FALSE;
}

gboolean
gebr_geoxml_program_get_stderr(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return FALSE;
	return (!strcmp(__gebr_geoxml_get_attr_value((GdomeElement*)program, "stderr"), "yes"))
		? TRUE : FALSE;
}

const gchar *
gebr_geoxml_program_get_status(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return __gebr_geoxml_get_attr_value((GdomeElement*)program, "status");
}

const gchar *
gebr_geoxml_program_get_title(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value((GdomeElement*)program, "title");
}

void
gebr_geoxml_program_get_menu(GebrGeoXmlProgram * program, gchar ** menu, gulong * index)
{
	if (program == NULL)
		return;
	*menu = (gchar *)__gebr_geoxml_get_tag_value((GdomeElement*)program, "menu");
	*index = (gulong)atol(__gebr_geoxml_get_attr_value(
		__gebr_geoxml_get_first_element((GdomeElement*)program, "menu"), "index"));
}

const gchar *
gebr_geoxml_program_get_binary(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value((GdomeElement*)program, "binary");
}

const gchar *
gebr_geoxml_program_get_description(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value((GdomeElement*)program, "description");
}

const gchar *
gebr_geoxml_program_get_help(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value((GdomeElement*)program, "help");
}

const gchar *
gebr_geoxml_program_get_url(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value((GdomeElement*)program, "url");
}
