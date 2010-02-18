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
	GdomeElement *element;
};

/*
 * library functions.
 */

static void gebr_geoxml_parameters_foreach(GebrGeoXmlParameters * parameters, GebrGeoXmlCallback callback, gpointer user_data)
{
	GebrGeoXmlSequence *parameter;

	gebr_geoxml_parameters_get_parameter(parameters, &parameter, 0);
	for (; parameter != NULL; gebr_geoxml_sequence_next(&parameter)) {
		if (gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(parameter)) == GEBR_GEOXML_PARAMETER_TYPE_GROUP) {
			GebrGeoXmlSequence *instance;

			gebr_geoxml_parameter_group_get_instance(GEBR_GEOXML_PARAMETER_GROUP(parameter), &instance, 0);
			for (; instance != NULL; gebr_geoxml_sequence_next(&instance))
				gebr_geoxml_parameters_foreach(GEBR_GEOXML_PARAMETERS(instance), callback, user_data);

			continue;	
		}
		callback(GEBR_GEOXML_OBJECT(parameter), user_data);
	}
}

void gebr_geoxml_program_foreach_parameter(GebrGeoXmlProgram * program, GebrGeoXmlCallback callback, gpointer user_data)
{
	if (program == NULL)
		return;

	gebr_geoxml_parameters_foreach(gebr_geoxml_program_get_parameters(program), callback, user_data);
}

GebrGeoXmlFlow *gebr_geoxml_program_flow(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return (GebrGeoXmlFlow *) gdome_n_parentNode((GdomeNode *) program, &exception);
}

GebrGeoXmlParameters *gebr_geoxml_program_get_parameters(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return (GebrGeoXmlParameters *) __gebr_geoxml_get_first_element((GdomeElement *) program, "parameters");
}

void gebr_geoxml_program_set_stdin(GebrGeoXmlProgram * program, const gboolean enable)
{
	if (program == NULL)
		return;
	__gebr_geoxml_set_attr_value((GdomeElement *) program, "stdin", (enable == TRUE ? "yes" : "no"));
}

void gebr_geoxml_program_set_stdout(GebrGeoXmlProgram * program, const gboolean enable)
{
	if (program == NULL)
		return;
	__gebr_geoxml_set_attr_value((GdomeElement *) program, "stdout", (enable == TRUE ? "yes" : "no"));
}

void gebr_geoxml_program_set_stderr(GebrGeoXmlProgram * program, const gboolean enable)
{
	if (program == NULL)
		return;
	__gebr_geoxml_set_attr_value((GdomeElement *) program, "stderr", (enable == TRUE ? "yes" : "no"));
}

void gebr_geoxml_program_set_status(GebrGeoXmlProgram * program, GebrGeoXmlProgramStatus status)
{
	const gchar * status_str;

	if (program == NULL)
		return;

	switch(status) {
	case GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED:
		status_str = "unconfigured";
		break;
	case GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED:
		status_str = "configured";
		break;
	case GEBR_GEOXML_PROGRAM_STATUS_DISABLED:
		status_str = "disabled";
		break;
	default:
		status_str = "";
		break;
	}
	__gebr_geoxml_set_attr_value((GdomeElement *) program, "status", status_str);
}

void gebr_geoxml_program_set_title(GebrGeoXmlProgram * program, const gchar * title)
{
	if (program == NULL || title == NULL)
		return;
	__gebr_geoxml_set_tag_value((GdomeElement *) program, "title", title, __gebr_geoxml_create_TextNode);
}

void gebr_geoxml_program_set_binary(GebrGeoXmlProgram * program, const gchar * binary)
{
	if (program == NULL || binary == NULL)
		return;
	__gebr_geoxml_set_tag_value((GdomeElement *) program, "binary", binary, __gebr_geoxml_create_TextNode);
}

void gebr_geoxml_program_set_description(GebrGeoXmlProgram * program, const gchar * description)
{
	if (program == NULL || description == NULL)
		return;
	__gebr_geoxml_set_tag_value((GdomeElement *) program, "description", description,
				    __gebr_geoxml_create_TextNode);
}

void gebr_geoxml_program_set_help(GebrGeoXmlProgram * program, const gchar * help)
{
	if (program == NULL || help == NULL)
		return;
	__gebr_geoxml_set_tag_value((GdomeElement *) program, "help", help, __gebr_geoxml_create_CDATASection);
}

void gebr_geoxml_program_set_url(GebrGeoXmlProgram * program, const gchar * url)
{
	if (program == NULL || url == NULL)
		return;
	__gebr_geoxml_set_tag_value((GdomeElement *) program, "url", url, __gebr_geoxml_create_TextNode);
}

gboolean gebr_geoxml_program_get_stdin(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return FALSE;
	return (!strcmp(__gebr_geoxml_get_attr_value((GdomeElement *) program, "stdin"), "yes"))
	    ? TRUE : FALSE;
}

gboolean gebr_geoxml_program_get_stdout(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return FALSE;
	return (!strcmp(__gebr_geoxml_get_attr_value((GdomeElement *) program, "stdout"), "yes"))
	    ? TRUE : FALSE;
}

gboolean gebr_geoxml_program_get_stderr(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return FALSE;
	return (!strcmp(__gebr_geoxml_get_attr_value((GdomeElement *) program, "stderr"), "yes"))
	    ? TRUE : FALSE;
}

GebrGeoXmlProgramStatus gebr_geoxml_program_get_status(GebrGeoXmlProgram * program)
{
	const gchar *status;

	if (program == NULL)
		return GEBR_GEOXML_PROGRAM_STATUS_UNKNOWN;

	status = __gebr_geoxml_get_attr_value((GdomeElement *) program, "status");

	if (!strcmp(status, "unconfigured"))
		return GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED;

	if (!strcmp(status, "configured"))
		return GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED;

	if (!strcmp(status, "disabled"))
		return GEBR_GEOXML_PROGRAM_STATUS_DISABLED;

	return GEBR_GEOXML_PROGRAM_STATUS_UNKNOWN;
}

const gchar *gebr_geoxml_program_get_title(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value((GdomeElement *) program, "title");
}

const gchar *gebr_geoxml_program_get_binary(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value((GdomeElement *) program, "binary");
}

const gchar *gebr_geoxml_program_get_description(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value((GdomeElement *) program, "description");
}

const gchar *gebr_geoxml_program_get_help(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value((GdomeElement *) program, "help");
}

const gchar *gebr_geoxml_program_get_url(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value((GdomeElement *) program, "url");
}
