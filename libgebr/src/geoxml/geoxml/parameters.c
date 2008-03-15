/*   libgebr - G�BR Library
 *   Copyright (C) 2007-2008 G�BR core team (http://gebr.sourceforge.net)
 *
 *   This parameters is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This parameters is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this parameters.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>

#include <gdome.h>

#include "parameters.h"
#include "xml.h"
#include "types.h"
#include "parameter_group.h"
#include "program_parameter.h"

/*
 * internal stuff
 */

struct geoxml_parameters {
	GdomeElement * element;
};

GeoXmlParameter *
__geoxml_parameters_new_parameter(GeoXmlParameters * parameters, enum GEOXML_PARAMETERTYPE type)
{
	GdomeElement *	parameter_element;

	parameter_element = __geoxml_new_element((GdomeElement*)parameters, parameter_type_to_str[type]);

	if (type != GEOXML_PARAMETERTYPE_GROUP) {
		__geoxml_insert_new_element(parameter_element, "keyword", NULL);
		__geoxml_insert_new_element(parameter_element, "label", NULL);
		__geoxml_insert_new_element(parameter_element, (type != GEOXML_PARAMETERTYPE_FLAG) ? "value" : "state", NULL);

		if (type != GEOXML_PARAMETERTYPE_FLAG)
			geoxml_program_parameter_set_required((GeoXmlProgramParameter*)parameter_element, FALSE);
		else
			geoxml_program_parameter_set_flag_default((GeoXmlProgramParameter*)parameter_element, FALSE);

		switch (type) {
		case GEOXML_PARAMETERTYPE_FILE:
			geoxml_program_parameter_set_file_be_directory((GeoXmlProgramParameter*)parameter_element, FALSE);
			break;
		case GEOXML_PARAMETERTYPE_RANGE:
			geoxml_program_parameter_set_range_properties((GeoXmlProgramParameter*)parameter_element, "", "", "", "");
			break;
		default:
			break;
		}
	} else {
		__geoxml_insert_new_element(parameter_element, "label", NULL);
		/* attributes */
		__geoxml_set_attr_value(parameter_element, "instances", "1");
		geoxml_parameter_group_set_exclusive((GeoXmlParameterGroup*)parameter_element, FALSE);
		geoxml_parameter_group_set_expand((GeoXmlParameterGroup*)parameter_element, TRUE);
	}

	return (GeoXmlParameter*)parameter_element;
}

/*
 * library functions.
 */

GeoXmlParameter *
geoxml_parameters_new_parameter(GeoXmlParameters * parameters, enum GEOXML_PARAMETERTYPE type)
{
	if (parameters == NULL)
		return NULL;
	return __geoxml_parameters_new_parameter(parameters, type);
}

GeoXmlParameter *
geoxml_parameters_append_parameter(GeoXmlParameters * parameters, enum GEOXML_PARAMETERTYPE type)
{
	if (parameters == NULL)
		return NULL;

	GdomeElement *	element;

	element = (GdomeElement*)__geoxml_parameters_new_parameter(parameters, type);
	gdome_el_insertBefore((GdomeElement*)parameters, (GdomeNode*)element, NULL, &exception);

	return (GeoXmlParameter*)element;
}

GeoXmlSequence *
geoxml_parameters_get_first_parameter(GeoXmlParameters * parameters)
{
	if (parameters == NULL)
		return NULL;
	return (GeoXmlSequence*)__geoxml_get_first_element((GdomeElement*)parameters, "*");
}

glong
geoxml_parameters_get_number(GeoXmlParameters * parameters)
{
	if (parameters == NULL)
		return -1;

	gint		i;
	gint		parameters_number = 0;

	for (i = 0; i < parameter_type_to_str_len; ++i)
		parameters_number += __geoxml_get_elements_number((GdomeElement*)parameters, parameter_type_to_str[i]);

	return parameters_number;
}

gboolean
geoxml_parameters_get_is_group(GeoXmlParameters * parameters)
{
	if (parameters == NULL)
		return FALSE;
	return !strcmp(gdome_el_nodeName((GdomeElement*)parameters, &exception)->str, "group")
		? TRUE : FALSE;
}
