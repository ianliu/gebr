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

#include <string.h>

#include <gdome.h>

#include "program_parameter.h"
#include "types.h"
#include "xml.h"
#include "error.h"
#include "parameter.h"
#include "parameter_p.h"
#include "program_p.h"
#include "sequence.h"

/*
 * internal structures and funcionts
 */

struct geoxml_program_parameter {
	GdomeElement * element;
};

struct geoxml_property_value {
	GdomeElement * element;
};

void
__geoxml_program_parameter_set_all_value(GeoXmlProgramParameter * program_parameter,
	gboolean default_value, const gchar * value)
{
	GeoXmlSequence *	property_value;

	geoxml_program_parameter_get_value(program_parameter, default_value, &property_value, 0);
	for (; property_value != NULL; geoxml_sequence_next(&property_value)) {
		geoxml_value_sequence_set(GEOXML_VALUE_SEQUENCE(property_value), value);
	}
}

void
__geoxml_program_parameter_remove_value_elements(GeoXmlProgramParameter * program_parameter,
gboolean default_value)
{
	GdomeElement *	element;

	/* remove all value/default elements */
	element = __geoxml_get_first_element((GdomeElement*)program_parameter,
		default_value == FALSE ? "value" : "default");
	while (element != NULL) {
		GdomeElement *	tmp;

		tmp = __geoxml_next_same_element(element);
		gdome_n_removeChild(gdome_el_parentNode(element, &exception), (GdomeNode*)element, &exception);
		element = tmp;
	}
}

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
		if (!strcmp(name->str, "program"))
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
	__geoxml_set_attr_value(
		__geoxml_get_first_element((GdomeElement*)program_parameter, "property"),
		"required", (required == TRUE ? "yes" : "no"));
}

gboolean
geoxml_program_parameter_get_required(GeoXmlProgramParameter * program_parameter)
{
	if (program_parameter == NULL)
		return FALSE;
	if (geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter)) == GEOXML_PARAMETERTYPE_FLAG)
		return FALSE;
	if (geoxml_parameter_get_is_reference((GeoXmlParameter*)program_parameter))
		return geoxml_program_parameter_get_required((GeoXmlProgramParameter*)
			geoxml_parameter_get_referencee((GeoXmlParameter*)program_parameter));
	return (!strcmp(__geoxml_get_attr_value(
			__geoxml_get_first_element((GdomeElement*)program_parameter, "property"),
			"required"), "yes"))
		? TRUE : FALSE;
}

void
geoxml_program_parameter_set_keyword(GeoXmlProgramParameter * program_parameter, const gchar * keyword)
{
	if (program_parameter == NULL || keyword == NULL)
		return;
	if (geoxml_parameter_get_is_reference((GeoXmlParameter*)program_parameter))
		return;
	__geoxml_set_tag_value((GdomeElement*)program_parameter, "keyword", keyword, __geoxml_create_TextNode);
}

const gchar *
geoxml_program_parameter_get_keyword(GeoXmlProgramParameter * program_parameter)
{
	if (program_parameter == NULL)
		return NULL;
	if (geoxml_parameter_get_is_reference((GeoXmlParameter*)program_parameter))
		return geoxml_program_parameter_get_keyword((GeoXmlProgramParameter*)
			geoxml_parameter_get_referencee((GeoXmlParameter*)program_parameter));
	return __geoxml_get_tag_value((GdomeElement*)program_parameter, "keyword");
}

void
geoxml_program_parameter_set_be_list(GeoXmlProgramParameter * program_parameter, gboolean is_list)
{
	if (program_parameter == NULL)
		return;
	if (geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter)) == GEOXML_PARAMETERTYPE_FLAG)
		return;
	if (geoxml_parameter_get_is_reference((GeoXmlParameter*)program_parameter))
		return;

	GdomeElement *		element;
	GdomeDOMString *	string;

	element = __geoxml_get_first_element((GdomeElement*)program_parameter, "property");
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
	if (separator == NULL)
		return;
	if (geoxml_program_parameter_get_is_list(program_parameter) == FALSE)
		return;
	if (geoxml_parameter_get_is_reference((GeoXmlParameter*)program_parameter))
		return;
	__geoxml_set_attr_value(
		__geoxml_get_first_element((GdomeElement*)program_parameter, "property"),
		"separator", separator);
}

gboolean
geoxml_program_parameter_get_is_list(GeoXmlProgramParameter * program_parameter)
{
	if (program_parameter == NULL)
		return FALSE;
	if (geoxml_parameter_get_is_reference((GeoXmlParameter*)program_parameter))
		return geoxml_program_parameter_get_is_list((GeoXmlProgramParameter*)
			geoxml_parameter_get_referencee((GeoXmlParameter*)program_parameter));

	GdomeDOMString *	string;
	GdomeElement *		property_element;
	gboolean		is_list;

	property_element = __geoxml_get_first_element((GdomeElement*)program_parameter, "property");
	string = gdome_str_mkref("separator");
	is_list = gdome_el_hasAttribute(property_element, string, &exception);

	gdome_str_unref(string);

	return is_list;
}

const gchar *
geoxml_program_parameter_get_list_separator(GeoXmlProgramParameter * program_parameter)
{
	if (geoxml_program_parameter_get_is_list(program_parameter) == FALSE)
		return NULL;
	if (geoxml_parameter_get_is_reference((GeoXmlParameter*)program_parameter))
		return geoxml_program_parameter_get_list_separator((GeoXmlProgramParameter*)
			geoxml_parameter_get_referencee((GeoXmlParameter*)program_parameter));
	return __geoxml_get_attr_value(
		__geoxml_get_first_element((GdomeElement*)program_parameter, "property"), "separator");
}

void
geoxml_program_parameter_set_first_value(GeoXmlProgramParameter * program_parameter, gboolean default_value, const gchar * value)
{
	if (program_parameter == NULL || value == NULL)
		return;
	__geoxml_set_tag_value((GdomeElement*)program_parameter,
		default_value == FALSE ? "value" : "default",
		value, __geoxml_create_TextNode);
}

void
geoxml_program_parameter_set_first_boolean_value(GeoXmlProgramParameter * program_parameter, gboolean default_value, gboolean enabled)
{
	if (program_parameter == NULL)
		return;
	__geoxml_set_tag_value((GdomeElement*)program_parameter,
		default_value == FALSE ? "value" : "default",
		(enabled == TRUE ? "on" : "off"),
		__geoxml_create_TextNode);
}

const gchar *
geoxml_program_parameter_get_first_value(GeoXmlProgramParameter * program_parameter, gboolean default_value)
{
	if (program_parameter == NULL)
		return NULL;
	return __geoxml_get_tag_value((GdomeElement*)program_parameter,
		default_value == FALSE ? "value" : "default");
}

gboolean
geoxml_program_parameter_get_first_boolean_value(GeoXmlProgramParameter * program_parameter, gboolean default_value)
{
	if (program_parameter == NULL)
		return FALSE;
	return (!strcmp(__geoxml_get_tag_value((GdomeElement*)program_parameter,
		default_value == FALSE ? "value" : "default"), "on")) ? TRUE : FALSE;
}

GeoXmlPropertyValue *
geoxml_program_parameter_append_value(GeoXmlProgramParameter * program_parameter, gboolean default_value)
{
	if (program_parameter == NULL)
		return NULL;

	GeoXmlPropertyValue *	property_value;

	if (default_value == FALSE)
		property_value = (GeoXmlPropertyValue*)__geoxml_insert_new_element(
			__geoxml_get_first_element((GdomeElement*)program_parameter, "property"), "value",
				__geoxml_get_first_element((GdomeElement*)program_parameter, "default"));
	else
		property_value = (GeoXmlPropertyValue*)__geoxml_insert_new_element(
			__geoxml_get_first_element((GdomeElement*)program_parameter, "property"), "default", NULL);

	return property_value;
}

int
geoxml_program_parameter_get_value(GeoXmlProgramParameter * program_parameter, gboolean default_value, GeoXmlSequence ** property_value, gulong index)
{
	if (program_parameter == NULL) {
		*property_value = NULL;
		return GEOXML_RETV_NULL_PTR;
	}

	*property_value = (GeoXmlSequence*)__geoxml_get_element_at((GdomeElement*)program_parameter,
		default_value == FALSE ? "value" : "default", index, TRUE);

	return (*property_value == NULL)
		? GEOXML_RETV_INVALID_INDEX
		: GEOXML_RETV_SUCCESS;
}

glong
geoxml_program_parameter_get_values_number(GeoXmlProgramParameter * program_parameter, gboolean default_value)
{
	if (program_parameter == NULL)
		return -1;
	return __geoxml_get_elements_number(
		__geoxml_get_first_element((GdomeElement*)program_parameter, "property"),
			default_value == FALSE ? "value" : "default");
}

void
geoxml_program_parameter_set_string_value(GeoXmlProgramParameter * program_parameter, gboolean default_value, const gchar * value)
{
	if (program_parameter == NULL)
		return;

	if (geoxml_program_parameter_get_is_list(program_parameter) == FALSE)
		geoxml_program_parameter_set_first_value(program_parameter, default_value, value);
	else
		geoxml_program_parameter_set_parse_list_value(program_parameter, default_value, value);
}

GString *
geoxml_program_parameter_get_string_value(GeoXmlProgramParameter * program_parameter, gboolean default_value)
{
	if (program_parameter == NULL)
		return NULL;

	GString *	value;

	value = g_string_new("");
	if (geoxml_program_parameter_get_is_list(program_parameter) == FALSE)
		g_string_assign(value, geoxml_program_parameter_get_first_value(program_parameter, default_value));
	else {
		GeoXmlSequence *	property_value;
		const gchar *		separator;

		separator = geoxml_program_parameter_get_list_separator(program_parameter);
		geoxml_program_parameter_get_value(program_parameter, default_value, &property_value, 0);
		g_string_assign(value, geoxml_value_sequence_get(GEOXML_VALUE_SEQUENCE(property_value)));
		geoxml_sequence_next(&property_value);
		for (; property_value != NULL; geoxml_sequence_next(&property_value)) {
			g_string_append(value, separator);
			g_string_append(value, geoxml_value_sequence_get(GEOXML_VALUE_SEQUENCE(property_value)));
		}
	}

	return value;
}

GeoXmlProgramParameter *
geoxml_program_parameter_find_dict_parameter(GeoXmlProgramParameter * program_parameter,
GeoXmlDocument * dict_document)
{
	GeoXmlProgramParameter *	dict_parameter;
	GeoXmlSequence *		i;
	const gchar *			dict_keyword;

	if (program_parameter == NULL || dict_document == NULL)
		return NULL;

	GdomeDOMString *	string;
	GdomeElement *		property_element;

	property_element = __geoxml_get_first_element((GdomeElement*)program_parameter, "property");

	string = gdome_str_mkref("dictkeyword");
	if (!gdome_el_hasAttribute(property_element, string, &exception))
		return NULL;
	gdome_str_unref(string);

	dict_keyword = __geoxml_get_attr_value(property_element, "dictkeyword");
	dict_parameter = NULL;
	i = GEOXML_SEQUENCE(geoxml_parameters_get_first_parameter(
	geoxml_document_get_dict_parameters(dict_document)));
	for (; i != NULL; geoxml_sequence_next(&i)) {
		if (!strcmp(dict_keyword, geoxml_program_parameter_get_keyword((GeoXmlProgramParameter*)i))) {
			dict_parameter = (GeoXmlProgramParameter*)i;
			break;
		}
	}

	return dict_parameter;
}

gboolean
geoxml_program_parameter_copy_value(GeoXmlProgramParameter * program_parameter,
GeoXmlProgramParameter * source, gboolean default_value)
{
	if (program_parameter == NULL || source == NULL)
		return FALSE;

	GdomeDocument *	document;
	GdomeElement *	insert_position;
	GdomeElement *	element;

	__geoxml_program_parameter_remove_value_elements(program_parameter, default_value);

	document = gdome_el_ownerDocument((GdomeElement*)program_parameter, &exception);
	insert_position = default_value ? NULL
		: __geoxml_get_first_element((GdomeElement*)program_parameter, "default");
	element = __geoxml_get_first_element((GdomeElement*)source,
		default_value == FALSE ? "value" : "default");
	for (; element != NULL; element = __geoxml_next_same_element(element))
		gdome_n_insertBefore(gdome_el_parentNode(insert_position, &exception),
			gdome_doc_importNode(document, (GdomeNode*)element, TRUE, &exception),
			(GdomeNode*)insert_position, &exception);

	return TRUE;
}

void
geoxml_program_parameter_set_parse_list_value(GeoXmlProgramParameter * program_parameter, gboolean default_value, const gchar * value)
{
	if (program_parameter == NULL || value == NULL)
		return;
	if (geoxml_program_parameter_get_is_list(program_parameter) == FALSE)
		return;

	const gchar *	separator;

	__geoxml_program_parameter_remove_value_elements(program_parameter, default_value);

	separator = geoxml_program_parameter_get_list_separator(program_parameter);
	if (!strlen(value) || !strlen(separator))
		geoxml_program_parameter_append_value(program_parameter, default_value);
	else {
		gchar **	splits;
		int		i;

		splits = g_strsplit(value, separator, 0);
		for (i = 0; splits[i] != NULL; ++i)
			geoxml_value_sequence_set(GEOXML_VALUE_SEQUENCE(
				geoxml_program_parameter_append_value(program_parameter, default_value)),
				splits[i]);
		g_strfreev(splits);
	}
}

void
geoxml_program_parameter_set_value_from_dict(GeoXmlProgramParameter * program_parameter,
GeoXmlProgramParameter * dict_parameter)
{
	if (program_parameter == NULL)
		return;

	GdomeElement *	property_element;

	property_element = __geoxml_get_first_element((GdomeElement*)program_parameter, "property");

	if (dict_parameter == NULL) {
		GdomeDOMString *	string;

		string = gdome_str_mkref("dictkeyword");
		gdome_el_removeAttribute(property_element, string, &exception);
	
		gdome_str_unref(string);
	} else
		__geoxml_set_attr_value(property_element, "dictkeyword",
			geoxml_program_parameter_get_keyword(dict_parameter));
}

void
geoxml_program_parameter_set_file_be_directory(GeoXmlProgramParameter * program_parameter, gboolean is_directory)
{
	if (program_parameter == NULL)
		return;
	if (geoxml_parameter_get_is_reference(GEOXML_PARAMETER(program_parameter)))
		return;
	if (geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter)) != GEOXML_PARAMETERTYPE_FILE)
		return;
	__geoxml_set_attr_value(
		__geoxml_parameter_get_type_element(GEOXML_PARAMETER(program_parameter), FALSE),
		"directory", (is_directory == TRUE ? "yes" : "no"));
}

gboolean
geoxml_program_parameter_get_file_be_directory(GeoXmlProgramParameter * program_parameter)
{
	if (program_parameter == NULL)
		return FALSE;
	if (geoxml_parameter_get_is_reference(GEOXML_PARAMETER(program_parameter)))
		return geoxml_program_parameter_get_file_be_directory((GeoXmlProgramParameter*)
			geoxml_parameter_get_referencee((GeoXmlParameter*)program_parameter));
	if (geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter)) != GEOXML_PARAMETERTYPE_FILE)
		return FALSE;
	return (!strcmp(__geoxml_get_attr_value(
		__geoxml_parameter_get_type_element(GEOXML_PARAMETER(program_parameter), TRUE), "directory"), "yes"))
		? TRUE : FALSE;
}

void
geoxml_program_parameter_set_file_filter(GeoXmlProgramParameter * program_parameter,
	const gchar * name, const gchar * pattern)
{
	if (program_parameter == NULL)
		return;
	if (geoxml_parameter_get_is_reference(GEOXML_PARAMETER(program_parameter)))
		return;
	if (geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter)) != GEOXML_PARAMETERTYPE_FILE)
		return;
	__geoxml_set_attr_value(
		__geoxml_parameter_get_type_element(GEOXML_PARAMETER(program_parameter), FALSE),
		"filter-name", name);
	__geoxml_set_attr_value(
		__geoxml_parameter_get_type_element(GEOXML_PARAMETER(program_parameter), FALSE),
		"filter-pattern", pattern);
}

void
geoxml_program_parameter_get_file_filter(GeoXmlProgramParameter * program_parameter,
	gchar ** name, gchar ** pattern)
{
	if (program_parameter == NULL)
		return;
	if (geoxml_parameter_get_is_reference(GEOXML_PARAMETER(program_parameter)))
		return geoxml_program_parameter_get_file_filter((GeoXmlProgramParameter*)
			geoxml_parameter_get_referencee((GeoXmlParameter*)program_parameter),
			name, pattern);
	if (geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter)) != GEOXML_PARAMETERTYPE_FILE)
		return;
	if (name != NULL)
		*name = (gchar*)__geoxml_get_attr_value(
		__geoxml_parameter_get_type_element(GEOXML_PARAMETER(program_parameter), TRUE),
			"filter-name");
	if (pattern != NULL)
		*pattern = (gchar*)__geoxml_get_attr_value(
		__geoxml_parameter_get_type_element(GEOXML_PARAMETER(program_parameter), TRUE),
			"filter-pattern");
}

void
geoxml_program_parameter_set_number_min_max(GeoXmlProgramParameter * program_parameter,
	const gchar * min, const gchar * max)
{
	enum GEOXML_PARAMETERTYPE	type;

	if (program_parameter == NULL)
		return;
	if (geoxml_parameter_get_is_reference(GEOXML_PARAMETER(program_parameter)))
		return;
	type = geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter));
	if (type != GEOXML_PARAMETERTYPE_INT &&
	type != GEOXML_PARAMETERTYPE_FLOAT &&
	type != GEOXML_PARAMETERTYPE_RANGE)
		return;

	GdomeElement *	type_element;

	type_element = __geoxml_parameter_get_type_element(GEOXML_PARAMETER(program_parameter), FALSE);
	if (min != NULL)
		__geoxml_set_attr_value(type_element, "min", min);
	if (max != NULL)
		__geoxml_set_attr_value(type_element, "max", max);
}

void
geoxml_program_parameter_get_number_min_max(GeoXmlProgramParameter * program_parameter,
	gchar ** min, gchar ** max)
{
	enum GEOXML_PARAMETERTYPE	type;

	if (program_parameter == NULL)
		return;
	if (geoxml_parameter_get_is_reference(GEOXML_PARAMETER(program_parameter)))
		return geoxml_program_parameter_get_number_min_max((GeoXmlProgramParameter*)
			geoxml_parameter_get_referencee((GeoXmlParameter*)program_parameter),
			min, max);
	type = geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter));
	if (type != GEOXML_PARAMETERTYPE_INT &&
	type != GEOXML_PARAMETERTYPE_FLOAT &&
	type != GEOXML_PARAMETERTYPE_RANGE)
		return;

	GdomeElement *	type_element;

	type_element = __geoxml_parameter_get_type_element(GEOXML_PARAMETER(program_parameter), TRUE);
	if (min != NULL)
		*min = (gchar*)__geoxml_get_attr_value(type_element, "min");
	if (max != NULL)
		*max = (gchar*)__geoxml_get_attr_value(type_element, "max");
}

void
geoxml_program_parameter_set_range_properties(GeoXmlProgramParameter * program_parameter,
	const gchar * min, const gchar * max, const gchar * inc, const gchar * digits)
{
	if (program_parameter == NULL)
		return;
	if (geoxml_parameter_get_is_reference(GEOXML_PARAMETER(program_parameter)))
		return;
	if (geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter)) != GEOXML_PARAMETERTYPE_RANGE)
		return;

	GdomeElement *	type_element;

	type_element = __geoxml_parameter_get_type_element(GEOXML_PARAMETER(program_parameter), FALSE);
	if (min != NULL)
		__geoxml_set_attr_value(type_element, "min", min);
	if (max != NULL)
		__geoxml_set_attr_value(type_element, "max", max);
	if (inc != NULL)
		__geoxml_set_attr_value(type_element, "inc", inc);
	if (digits != NULL)
		__geoxml_set_attr_value(type_element, "digits", digits);
}

void
geoxml_program_parameter_get_range_properties(GeoXmlProgramParameter * program_parameter,
	gchar ** min, gchar ** max, gchar ** inc, gchar ** digits)
{
	if (program_parameter == NULL)
		return;
	if (geoxml_parameter_get_is_reference(GEOXML_PARAMETER(program_parameter)))
		return geoxml_program_parameter_get_range_properties((GeoXmlProgramParameter*)
			geoxml_parameter_get_referencee((GeoXmlParameter*)program_parameter),
			min, max, inc, digits);
	if (geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter)) != GEOXML_PARAMETERTYPE_RANGE)
		return;

	GdomeElement *	type_element;

	type_element = __geoxml_parameter_get_type_element(GEOXML_PARAMETER(program_parameter), TRUE);
	if (min != NULL)
		*min = (gchar*)__geoxml_get_attr_value(type_element, "min");
	if (max != NULL)
		*max = (gchar*)__geoxml_get_attr_value(type_element, "max");
	if (inc != NULL)
		*inc = (gchar*)__geoxml_get_attr_value(type_element, "inc");
	if (digits != NULL)
		*digits = (gchar*)__geoxml_get_attr_value(type_element, "digits");
}

GeoXmlEnumOption *
geoxml_program_parameter_append_enum_option(GeoXmlProgramParameter * program_parameter,
	const gchar * label, const gchar * value)
{
	if (program_parameter == NULL || label == NULL || value == NULL)
		return NULL;
	if (geoxml_parameter_get_is_reference(GEOXML_PARAMETER(program_parameter)))
		return geoxml_program_parameter_append_enum_option((GeoXmlProgramParameter*)
			geoxml_parameter_get_referencee((GeoXmlParameter*)program_parameter),
			label, value);
	if (geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter)) != GEOXML_PARAMETERTYPE_ENUM)
		return NULL;

	GeoXmlEnumOption *	enum_option;
	GdomeElement *		label_element;
	GdomeElement *		value_element;

	enum_option = (GeoXmlEnumOption*)__geoxml_new_element((GdomeElement*)program_parameter, "option");
	label_element = __geoxml_insert_new_element((GdomeElement*)enum_option, "label", NULL);
	value_element = __geoxml_insert_new_element((GdomeElement*)enum_option, "value", NULL);
	__geoxml_set_element_value(label_element, label, __geoxml_create_TextNode);
	__geoxml_set_element_value(value_element, value, __geoxml_create_TextNode);

	gdome_el_insertBefore(__geoxml_parameter_get_type_element((GeoXmlParameter*)program_parameter, FALSE),
		(GdomeNode*)enum_option, NULL, &exception);

	return enum_option;
}

int
geoxml_program_parameter_get_enum_option(GeoXmlProgramParameter * program_parameter, GeoXmlSequence ** enum_option, gulong index)
{
	if (program_parameter == NULL) {
		*enum_option = NULL;
		return GEOXML_RETV_NULL_PTR;
	}
	if (geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter)) != GEOXML_PARAMETERTYPE_ENUM)
		return GEOXML_RETV_PARAMETER_NOT_ENUM;
	if (geoxml_parameter_get_is_reference((GeoXmlParameter*)program_parameter))
		return geoxml_program_parameter_get_enum_option((GeoXmlProgramParameter*)
			geoxml_parameter_get_referencee((GeoXmlParameter*)program_parameter),
			enum_option, index);

	*enum_option = (GeoXmlSequence*)__geoxml_get_element_at(
		__geoxml_parameter_get_type_element((GeoXmlParameter*)program_parameter, TRUE),
		"option", index, TRUE);

	return (*enum_option == NULL)
		? GEOXML_RETV_INVALID_INDEX
		: GEOXML_RETV_SUCCESS;
}

glong
geoxml_program_parameter_get_enum_options_number(GeoXmlProgramParameter * program_parameter)
{
	if (program_parameter == NULL)
		return -1;
	if (geoxml_parameter_get_is_reference((GeoXmlParameter*)program_parameter))
		return geoxml_program_parameter_get_enum_options_number((GeoXmlProgramParameter*)
			geoxml_parameter_get_referencee((GeoXmlParameter*)program_parameter));
	if (geoxml_parameter_get_type(GEOXML_PARAMETER(program_parameter)) != GEOXML_PARAMETERTYPE_ENUM)
		return -1;
	return __geoxml_get_elements_number(
		__geoxml_parameter_get_type_element((GeoXmlParameter*)program_parameter, TRUE),
		"options");
}
