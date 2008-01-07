/*   libgebr - GÍBR Library
 *   Copyright (C) 2007  Br·ulio Barros de Oliveira (brauliobo@gmail.com)
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
#include <stdio.h>

#include "program_parameter.h"
#include "types.h"
#include "xml.h"
#include "error.h"
#include "parameter.h"
#include "program_p.h"
#include "types.h"
#include "sequence.h"

/*
 * internal structures and funcionts
 */

struct geoxml_program_parameter {
	GdomeElement * element;
};

struct geoxml_enum_option {
	GdomeElement * element;
};

/*
 * library functions.
 */

GeoXmlProgram *
geoxml_program_parameter_program(GeoXmlProgramParameter * program_parameter)
{
	if (program_parameter == NULL)
		return NULL;

	GdomeElement *		program_element;

	while (1) {
		GdomeDOMString *	name;

		program_element = (GdomeElement*)gdome_n_parentNode((GdomeNode*)program_parameter, &exception);
		name = gdome_el_nodeName(program_element, &exception);
		if (!g_ascii_strcasecmp(name->str, "program"))
			break;
	}

	return (GeoXmlProgram *)program_element;
}

void
geoxml_program_parameter_set_required(GeoXmlProgramParameter * program_parameter, gboolean required)
{
	if (program_parameter == NULL)
		return;
	if (geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter)) == GEOXML_PARAMETERTYPE_FLAG)
		return;
	__geoxml_set_attr_value((GdomeElement*)program_parameter, "required", (required == TRUE ? "yes" : "no"));
}

void
geoxml_program_parameter_set_keyword(GeoXmlProgramParameter * program_parameter, const gchar * keyword)
{
	if (program_parameter == NULL || keyword == NULL)
		return;
	__geoxml_set_tag_value((GdomeElement*)program_parameter, "keyword", keyword, __geoxml_create_TextNode);
}

void
geoxml_program_parameter_set_be_list(GeoXmlProgramParameter * program_parameter, gboolean is_list)
{
	if (program_parameter == NULL)
		return;

	GdomeElement *		element;
	GdomeDOMString *	string;

	element = (GdomeElement*)program_parameter;
	string = gdome_str_mkref("separator");
	if (is_list == TRUE) {
		if (gdome_el_hasAttribute(element, string, &exception) == FALSE)
			__geoxml_set_attr_value(element, "separator", "");
	} else {
		if (gdome_el_hasAttribute(element, string, &exception) == TRUE)
			gdome_el_removeAttribute(element, string, &exception);
	}

	gdome_str_unref(string);
}

void
geoxml_program_parameter_set_list_separator(GeoXmlProgramParameter * program_parameter, const gchar * separator)
{
	if (geoxml_program_parameter_get_is_list(program_parameter) == FALSE)
		return;
	if (separator == NULL)
		return;
	return __geoxml_set_attr_value((GdomeElement*)program_parameter, "separator", separator);
}

void
geoxml_program_parameter_set_default(GeoXmlProgramParameter * program_parameter, const gchar * value)
{
	if (program_parameter == NULL)
		return;

	gchar * tag_name = (geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter)) != GEOXML_PARAMETERTYPE_FLAG)
		? "value" : "state";

	__geoxml_set_attr_value(
		__geoxml_get_first_element((GdomeElement*)program_parameter, tag_name), "default", value);
}

void
geoxml_program_parameter_set_flag_default(GeoXmlProgramParameter * program_parameter, gboolean state)
{
	if (program_parameter == NULL)
		return;
	if (geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter)) != GEOXML_PARAMETERTYPE_FLAG)
		return;

	__geoxml_set_attr_value(
		__geoxml_get_first_element((GdomeElement*)program_parameter, "state"), "default",
		(state == TRUE ? "on" : "off"));
}

void
geoxml_program_parameter_set_value(GeoXmlProgramParameter * program_parameter, const gchar * value)
{
	if (program_parameter == NULL || value == NULL)
		return;

	gchar * tag_name = (geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter)) != GEOXML_PARAMETERTYPE_FLAG)
		? "value" : "state";

	__geoxml_set_tag_value((GdomeElement*)program_parameter, tag_name, value, __geoxml_create_TextNode);
}

void
geoxml_program_parameter_set_flag_state(GeoXmlProgramParameter * program_parameter, gboolean enabled)
{
	if (program_parameter == NULL)
		return;
	if (geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter)) != GEOXML_PARAMETERTYPE_FLAG)
		return;
	__geoxml_set_tag_value((GdomeElement*)program_parameter, "state", (enabled == TRUE ? "on" : "off"),
		__geoxml_create_TextNode);
}

void
geoxml_program_parameter_set_file_be_directory(GeoXmlProgramParameter * program_parameter, gboolean is_directory)
{
	if (program_parameter == NULL)
		return;
	if (geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter)) != GEOXML_PARAMETERTYPE_FILE)
		return;
	__geoxml_set_attr_value((GdomeElement*)program_parameter, "directory", (is_directory == TRUE ? "yes" : "no"));
}

void
geoxml_program_parameter_set_range_properties(GeoXmlProgramParameter * program_parameter,
		const gchar * min, const gchar * max, const gchar * inc)
{
	if (program_parameter == NULL)
		return;
	if (geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter)) != GEOXML_PARAMETERTYPE_RANGE)
		return;
	__geoxml_set_attr_value((GdomeElement*)program_parameter, "min", min);
	__geoxml_set_attr_value((GdomeElement*)program_parameter, "max", max);
	__geoxml_set_attr_value((GdomeElement*)program_parameter, "inc", inc);
}

GeoXmlEnumOption *
geoxml_program_parameter_new_enum_option(GeoXmlProgramParameter * program_parameter, const gchar * value)
{
	if (program_parameter == NULL || value == NULL)
		return NULL;
	if (geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter)) != GEOXML_PARAMETERTYPE_ENUM)
		return NULL;

	GeoXmlEnumOption *	enum_option;

	enum_option = (GeoXmlEnumOption*)__geoxml_new_element((GdomeElement*)program_parameter, "option");
	geoxml_value_sequence_set(GEOXML_VALUE_SEQUENCE(enum_option), value);

	return enum_option;
}

GeoXmlEnumOption *
geoxml_program_parameter_append_enum_option(GeoXmlProgramParameter * program_parameter, const gchar * value)
{
	if (program_parameter == NULL || value == NULL)
		return NULL;
	if (geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter)) != GEOXML_PARAMETERTYPE_ENUM)
		return NULL;

	GeoXmlEnumOption *	enum_option;

	enum_option = (GeoXmlEnumOption*)__geoxml_insert_new_element((GdomeElement*)program_parameter, "option", NULL);
	geoxml_value_sequence_set(GEOXML_VALUE_SEQUENCE(enum_option), value);

	return enum_option;
}

int
geoxml_program_parameter_get_enum_option(GeoXmlProgramParameter * program_parameter, GeoXmlValueSequence ** enum_option, gulong index)
{
	if (program_parameter == NULL) {
		*enum_option = NULL;
		return GEOXML_RETV_NULL_PTR;
	}
	if (geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter)) != GEOXML_PARAMETERTYPE_ENUM)
		return GEOXML_RETV_PARAMETER_NOT_ENUM;

	*enum_option = (GeoXmlValueSequence*)__geoxml_get_element_at((GdomeElement*)program_parameter, "option", index);

	return (*enum_option == NULL)
		? GEOXML_RETV_INVALID_INDEX
		: GEOXML_RETV_SUCCESS;
}

glong
geoxml_program_parameter_get_enum_options_number(GeoXmlProgramParameter * program_parameter)
{
	if (program_parameter == NULL)
		return -1;
	if (geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter)) != GEOXML_PARAMETERTYPE_ENUM)
		return -1;
	return __geoxml_get_elements_number((GdomeElement*)program_parameter, "options");
}

gboolean
geoxml_program_parameter_get_required(GeoXmlProgramParameter * program_parameter)
{
	if (program_parameter == NULL)
		return FALSE;
	if (geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter)) == GEOXML_PARAMETERTYPE_FLAG)
		return FALSE;
	return (!g_ascii_strcasecmp(__geoxml_get_attr_value((GdomeElement*)program_parameter, "required"), "yes"))
		? TRUE : FALSE;
}

const gchar *
geoxml_program_parameter_get_keyword(GeoXmlProgramParameter * program_parameter)
{
	if (program_parameter == NULL)
		return NULL;
	return __geoxml_get_tag_value((GdomeElement*)program_parameter, "keyword");
}

gboolean
geoxml_program_parameter_get_is_list(GeoXmlProgramParameter * program_parameter)
{
	if (program_parameter == NULL)
		return FALSE;

	GdomeDOMString *	string;
	gboolean		is_list;

	string = gdome_str_mkref("separator");
	is_list = gdome_el_hasAttribute((GdomeElement*)program_parameter, string, &exception);
	gdome_str_unref(string);

	return is_list;
}

const gchar *
geoxml_program_parameter_get_list_separator(GeoXmlProgramParameter * program_parameter)
{
	if (geoxml_program_parameter_get_is_list(program_parameter) == FALSE)
		return NULL;

	return __geoxml_get_attr_value((GdomeElement*)program_parameter, "separator");
}

const gchar *
geoxml_program_parameter_get_default(GeoXmlProgramParameter * program_parameter)
{
	if (program_parameter == NULL)
		return NULL;

	GdomeElement *		element;
// 	GdomeDOMString *	string;
// 	gboolean		ret;
	gchar *			tag_name;

	tag_name = (geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter)) == GEOXML_PARAMETERTYPE_FLAG) ? "state" : "value";
	element = __geoxml_get_first_element((GdomeElement*)program_parameter, tag_name);

	/* TODO: add support for removing or adding a default value */
// 	string = gdome_str_mkref("default");
// 	ret = (gboolean)gdome_el_hasAttribute(element, string, &exception);
// 	gdome_str_unref(string);
//
// 	if (ret == FALSE)
// 		return "";

	return __geoxml_get_attr_value(element, "default");
}

gboolean
geoxml_program_parameter_get_flag_default(GeoXmlProgramParameter * program_parameter)
{
	if (program_parameter == NULL)
		return FALSE;
	if (geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter)) != GEOXML_PARAMETERTYPE_FLAG)
		return FALSE;

	GdomeElement *		element;
	GdomeDOMString *	string;
	gboolean		ret;

	element = __geoxml_get_first_element((GdomeElement*)program_parameter, "state");

	string = gdome_str_mkref("default");
	ret = (gboolean)gdome_el_hasAttribute(element, string, &exception);
	gdome_str_unref(string);

	if (ret == FALSE)
		return FALSE;

	return (!g_ascii_strcasecmp(__geoxml_get_attr_value(element, "default"), "on"))
		? TRUE : FALSE;
}

const gchar *
geoxml_program_parameter_get_value(GeoXmlProgramParameter * program_parameter)
{
	if (program_parameter == NULL)
		return NULL;
	if (geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter)) == GEOXML_PARAMETERTYPE_FLAG)
		return NULL;
	return __geoxml_get_tag_value((GdomeElement*)program_parameter, "value");
}

gboolean
geoxml_program_parameter_get_flag_status(GeoXmlProgramParameter * program_parameter)
{
	if (program_parameter == NULL)
		return FALSE;
	if (geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter)) != GEOXML_PARAMETERTYPE_FLAG)
		return FALSE;
	return (!g_ascii_strcasecmp(__geoxml_get_tag_value((GdomeElement*)program_parameter, "state"), "on"))
		? TRUE : FALSE;
}

gboolean
geoxml_program_parameter_get_file_be_directory(GeoXmlProgramParameter * program_parameter)
{
	if (program_parameter == NULL)
		return FALSE;
	if (geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter)) != GEOXML_PARAMETERTYPE_FILE)
		return FALSE;
	return (!g_ascii_strcasecmp(__geoxml_get_attr_value((GdomeElement*)program_parameter, "directory"), "yes"))
		? TRUE : FALSE;
}

void
geoxml_program_parameter_get_range_properties(GeoXmlProgramParameter * program_parameter,
		gchar ** min, gchar ** max, gchar ** inc)
{
	if (program_parameter == NULL)
		return;
	if (geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter)) != GEOXML_PARAMETERTYPE_RANGE)
		return;
	*min = (gchar*)__geoxml_get_attr_value((GdomeElement*)program_parameter, "min");
	*max = (gchar*)__geoxml_get_attr_value((GdomeElement*)program_parameter, "max");
	*inc = (gchar*)__geoxml_get_attr_value((GdomeElement*)program_parameter, "inc");
}

void
geoxml_program_parameter_set_type(GeoXmlProgramParameter ** program_parameter, enum GEOXML_PARAMETERTYPE type)
{
	GeoXmlParameter *	parameter;

	parameter = GEOXML_PARAMETER(*program_parameter);
	geoxml_parameter_set_type(&parameter, type);

	*program_parameter = GEOXML_PROGRAM_PARAMETER(parameter);
}

void
geoxml_program_parameter_set_label(GeoXmlProgramParameter * program_parameter, const gchar * label)
{
	geoxml_parameter_set_label(GEOXML_PARAMETER(program_parameter), label);
}

enum GEOXML_PARAMETERTYPE
geoxml_program_parameter_get_type(GeoXmlProgramParameter * program_parameter)
{
	return geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter));
}

const gchar *
geoxml_program_parameter_get_label(GeoXmlProgramParameter * program_parameter)
{
	return geoxml_parameter_get_label(GEOXML_PARAMETER(program_parameter));
}

void
geoxml_program_parameter_previous(GeoXmlProgramParameter ** program_parameter)
{
	geoxml_sequence_previous((GeoXmlSequence**)program_parameter);
}

void
geoxml_program_parameter_next(GeoXmlProgramParameter ** program_parameter)
{
	geoxml_sequence_next((GeoXmlSequence**)program_parameter);
}

void
geoxml_program_parameter_remove(GeoXmlProgramParameter * program_parameter)
{
	geoxml_sequence_remove((GeoXmlSequence*)program_parameter);
}
