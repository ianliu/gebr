/*   libgebr - G�BR Library
 *   Copyright (C) 2007-2008 G�BR core team (http://gebr.sourceforge.net)
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

#include "parameter.h"
#include "xml.h"
#include "types.h"
#include "error.h"
#include "parameters_p.h"
#include "parameter_group.h"
#include "parameter_group_p.h"
#include "program_parameter.h"
#include "program_parameter_p.h"

/*
 * internal stuff
 */

struct geoxml_parameter {
	GdomeElement * element;
};

const char * parameter_type_to_str[] = { "",
	"string", "int", "file",
	"flag", "float", "range",
	"enum", "group", "reference"
};

const int parameter_type_to_str_len = 9;

/*
 * private functions
 */

GdomeElement *
__geoxml_parameter_get_type_element(GeoXmlParameter * parameter)
{
	GdomeElement *	type_element;

	type_element = __geoxml_get_first_element((GdomeElement*)parameter, "label");
	type_element = __geoxml_next_element(type_element);

	return type_element;
}

void
__geoxml_parameter_set_be_reference(GeoXmlParameter * parameter, GeoXmlParameter * reference, gboolean new)
{
	GdomeElement *			reference_element;
	GdomeElement *			type_element;
	enum GEOXML_PARAMETERTYPE	type;

	type = geoxml_parameter_get_type(parameter);
	if (type == GEOXML_PARAMETERTYPE_GROUP) {
		__geoxml_parameter_group_turn_to_reference(GEOXML_PARAMETER_GROUP(parameter));
		return;
	}

	type_element = __geoxml_parameter_get_type_element(parameter);
	reference_element = __geoxml_insert_new_element((GdomeElement*)parameter,
		"reference", type_element);
	gdome_el_removeChild((GdomeElement*)parameter, (GdomeNode*)type_element, &exception);

	__geoxml_element_assign_reference_id(reference_element, (GdomeElement*)reference);
	if (new == TRUE)
		__geoxml_element_assign_new_id((GdomeElement*)parameter);
}

gboolean
__geoxml_parameter_insert_type(GeoXmlParameter * parameter, enum GEOXML_PARAMETERTYPE type)
{
	if (type == GEOXML_PARAMETERTYPE_REFERENCE)
		return FALSE;

	GdomeElement *	type_element;

	type_element = __geoxml_insert_new_element((GdomeElement*)parameter, parameter_type_to_str[type], NULL);
	switch (type) {
	case GEOXML_PARAMETERTYPE_GROUP: {
		GeoXmlParameters *	parameters;

		parameters = (GeoXmlParameters*)__geoxml_insert_new_element(type_element, "parameters", NULL);
		geoxml_parameters_set_exclusive(parameters, NULL);

		break;
	} default: {
		GdomeElement *		property_element;

		property_element = __geoxml_insert_new_element(type_element, "property", NULL);
		__geoxml_insert_new_element(property_element, "keyword", NULL);
		__geoxml_insert_new_element(property_element, "label", NULL);
		__geoxml_insert_new_element(property_element, "value", NULL);

		switch (type) {
		case GEOXML_PARAMETERTYPE_FILE:
			geoxml_program_parameter_set_file_be_directory(
				(GeoXmlProgramParameter*)parameter, FALSE);
			break;
		case GEOXML_PARAMETERTYPE_RANGE:
			geoxml_program_parameter_set_range_properties(
				(GeoXmlProgramParameter*)parameter, "", "", "", "");
			break;
		default:
			geoxml_program_parameter_set_required(
				(GeoXmlProgramParameter*)parameter, FALSE);
			break;
		}
		__geoxml_program_parameter_set_all_value((GeoXmlProgramParameter*)parameter, "", "");

		break;
	}}

	return TRUE;
}

/*
 * library functions.
 */

GeoXmlParameters *
geoxml_parameter_get_parameters(GeoXmlParameter * parameter)
{
	if (parameter == NULL)
		return NULL;
	return (GeoXmlParameters*)gdome_n_parentNode((GdomeNode*)parameter, &exception);
}

gboolean
geoxml_parameter_set_type(GeoXmlParameter * parameter, enum GEOXML_PARAMETERTYPE type)
{
	if (parameter == NULL)
		return FALSE;
	if (type == GEOXML_PARAMETERTYPE_UNKNOWN || type == GEOXML_PARAMETERTYPE_REFERENCE)
		return FALSE;

	GdomeElement *		old_type_element;

	old_type_element = __geoxml_parameter_get_type_element(parameter);
	if (__geoxml_parameter_insert_type(parameter, type) == FALSE)
		return FALSE;
	gdome_n_removeChild((GdomeNode*)parameter, (GdomeNode*)old_type_element, &exception);

	return TRUE;
}

int
geoxml_parameter_set_be_reference(GeoXmlParameter * parameter, GeoXmlParameter * reference)
{
	if (parameter == NULL || reference == NULL)
		return GEOXML_RETV_NULL_PTR;
	if (parameter == reference)
		return GEOXML_RETV_REFERENCE_TO_ITSELF;

	__geoxml_parameter_set_be_reference(parameter, reference, FALSE);

	return GEOXML_RETV_SUCCESS;
}

enum GEOXML_PARAMETERTYPE
geoxml_parameter_get_type(GeoXmlParameter * parameter)
{
	if (parameter == NULL)
		return GEOXML_PARAMETERTYPE_UNKNOWN;

	GdomeDOMString *	tag_name;
	int			i;

	tag_name = gdome_el_tagName(__geoxml_parameter_get_type_element(parameter), &exception);

	for (i = 1; i <= parameter_type_to_str_len; ++i)
		if (!strcmp(parameter_type_to_str[i], tag_name->str))
			return (enum GEOXML_PARAMETERTYPE)i;

	return GEOXML_PARAMETERTYPE_UNKNOWN;
}

gboolean
geoxml_parameter_get_is_program_parameter(GeoXmlParameter * parameter)
{
	if (parameter == NULL)
		return FALSE;
	return (geoxml_parameter_get_type(parameter) != GEOXML_PARAMETERTYPE_GROUP)
		? TRUE : FALSE;
}

void
geoxml_parameter_set_label(GeoXmlParameter * parameter, const gchar * label)
{
	if (parameter == NULL || label == NULL)
		return;
	__geoxml_set_tag_value((GdomeElement*)parameter, "label", label, __geoxml_create_TextNode);
}

const gchar *
geoxml_parameter_get_label(GeoXmlParameter * parameter)
{
	if (parameter == NULL)
		return NULL;
	return __geoxml_get_tag_value((GdomeElement*)parameter, "label");
}

gboolean
geoxml_parameter_get_is_in_group(GeoXmlParameter * parameter)
{
	if (parameter == NULL)
		return FALSE;
	return geoxml_parameters_get_is_in_group(
		(GeoXmlParameters*)gdome_n_parentNode((GdomeNode*)parameter, &exception));
}

void
geoxml_parameter_reset(GeoXmlParameter * parameter, gboolean recursive)
{
	if (parameter == NULL)
		return;
	if (geoxml_parameter_get_type(parameter) == GEOXML_PARAMETERTYPE_GROUP) {
		GeoXmlSequence *	instance;

		if (recursive == FALSE)
			return;

		/* call geoxml_parameter_reset on each child */
		geoxml_parameter_group_get_instance(GEOXML_PARAMETER_GROUP(parameter), &instance, 0);
		for (; instance != NULL; geoxml_sequence_next(&instance))
			geoxml_parameters_reset(GEOXML_PARAMETERS(instance), recursive);
	} else
		__geoxml_program_parameter_set_all_value(GEOXML_PROGRAM_PARAMETER(parameter), "", "");
}
