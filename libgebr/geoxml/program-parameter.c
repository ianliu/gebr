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
#include <gebr-expr.h>

#include "document.h"
#include "error.h"
#include "parameter.h"
#include "parameter_p.h"
#include "parameters.h"
#include "program-parameter.h"
#include "program_p.h"
#include "sequence.h"
#include "types.h"
#include "value_sequence.h"
#include "xml.h"
#include "object.h"

/*
 * internal structures and funcionts
 */

struct gebr_geoxml_program_parameter {
	GdomeElement *element;
};

struct gebr_geoxml_property_value {
	GdomeElement *element;
};

void
__gebr_geoxml_program_parameter_set_all_value(GebrGeoXmlProgramParameter * program_parameter,
					      gboolean default_value, const gchar * value)
{
	GebrGeoXmlSequence *property_value;

	gebr_geoxml_program_parameter_get_value(program_parameter, default_value, &property_value, 0);
	for (; property_value != NULL; gebr_geoxml_sequence_next(&property_value)) {
		gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(property_value), value);
	}
}

void
__gebr_geoxml_program_parameter_remove_value_elements(GebrGeoXmlProgramParameter * program_parameter,
						      gboolean default_value)
{
	GdomeElement *element;

	/* remove all value/default elements */
	element = __gebr_geoxml_get_first_element((GdomeElement *) program_parameter,
						  default_value == FALSE ? "value" : "default");
	while (element) {
		GdomeElement *tmp;
		GdomeNode *parent;

		parent = gdome_el_parentNode(element, &exception);
		tmp = __gebr_geoxml_next_same_element(element);
		gdome_n_unref(gdome_n_removeChild(parent, (GdomeNode *) element, &exception), &exception);
		gdome_n_unref(parent, &exception);
		gdome_el_unref(element, &exception);
		element = tmp;
	}
}

/*
 * library functions.
 */

void gebr_geoxml_program_parameter_set_required(GebrGeoXmlProgramParameter * program_parameter, gboolean required)
{
	if (program_parameter == NULL)
		return;
	if (gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(program_parameter)) == GEBR_GEOXML_PARAMETER_TYPE_FLAG)
		return;

	GdomeElement *first = __gebr_geoxml_get_first_element((GdomeElement *) program_parameter, "property");
	__gebr_geoxml_set_attr_value(first, "required", (required == TRUE ? "yes" : "no"));
	gebr_geoxml_object_unref(first);
}

gboolean gebr_geoxml_program_parameter_get_required(GebrGeoXmlProgramParameter * program_parameter)
{
	gchar *required;
	gboolean is_required;
	GdomeElement *element;

	if (program_parameter == NULL)
		return FALSE;

	if (gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(program_parameter)) == GEBR_GEOXML_PARAMETER_TYPE_FLAG)
		return FALSE;

	if (gebr_geoxml_parameter_get_is_reference((GebrGeoXmlParameter *) program_parameter)) {
		GebrGeoXmlParameter *ref;
		ref = gebr_geoxml_parameter_get_referencee((GebrGeoXmlParameter *) program_parameter);
		is_required = gebr_geoxml_program_parameter_get_required((GebrGeoXmlProgramParameter *) ref);
		gebr_geoxml_object_unref(ref);
		return is_required;
	}

	element = __gebr_geoxml_get_first_element((GdomeElement *)program_parameter, "property");
	required = __gebr_geoxml_get_attr_value(element, "required");
	is_required = (g_strcmp0(required, "yes") == 0);

	g_free(required);
	gdome_el_unref(element, &exception);

	return is_required;
}

void gebr_geoxml_program_parameter_set_keyword(GebrGeoXmlProgramParameter * program_parameter, const gchar * keyword)
{
	if (program_parameter == NULL || keyword == NULL)
		return;
	if (gebr_geoxml_parameter_get_is_reference((GebrGeoXmlParameter *) program_parameter))
		return;
	__gebr_geoxml_set_tag_value((GdomeElement *) program_parameter, "keyword", keyword,
				    __gebr_geoxml_create_TextNode);
}

gchar *gebr_geoxml_program_parameter_get_keyword(GebrGeoXmlProgramParameter * program_parameter)
{
	if (program_parameter == NULL)
		return NULL;

	if (gebr_geoxml_parameter_get_is_reference((GebrGeoXmlParameter *) program_parameter)) {
		GebrGeoXmlParameter *ref = gebr_geoxml_parameter_get_referencee((GebrGeoXmlParameter *) program_parameter);
		gchar *key = gebr_geoxml_program_parameter_get_keyword((GebrGeoXmlProgramParameter *)ref);
		gebr_geoxml_object_unref(ref);
		return key;
	}

	return __gebr_geoxml_get_tag_value((GdomeElement *) program_parameter, "keyword");
}

void gebr_geoxml_program_parameter_set_be_list(GebrGeoXmlProgramParameter * program_parameter, gboolean is_list)
{
	if (program_parameter == NULL)
		return;
	if (gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(program_parameter)) == GEBR_GEOXML_PARAMETER_TYPE_FLAG)
		return;
	if (gebr_geoxml_parameter_get_is_reference((GebrGeoXmlParameter *) program_parameter))
		return;

	GdomeElement *element;
	GdomeDOMString *string;

	element = __gebr_geoxml_get_first_element((GdomeElement *) program_parameter, "property");
	string = gdome_str_mkref("separator");
	if (is_list == TRUE) {
		if (gdome_el_hasAttribute(element, string, &exception) == FALSE)
			__gebr_geoxml_set_attr_value(element, "separator", "");
	} else {
		if (gdome_el_hasAttribute(element, string, &exception) == TRUE)
			gdome_el_removeAttribute(element, string, &exception);
	}

	gdome_str_unref(string);
}

void
gebr_geoxml_program_parameter_set_list_separator(GebrGeoXmlProgramParameter * program_parameter,
						 const gchar * separator)
{
	if (separator == NULL)
		return;
	if (gebr_geoxml_program_parameter_get_is_list(program_parameter) == FALSE)
		return;
	if (gebr_geoxml_parameter_get_is_reference((GebrGeoXmlParameter *) program_parameter))
		return;
	__gebr_geoxml_set_attr_value(__gebr_geoxml_get_first_element((GdomeElement *) program_parameter, "property"),
				     "separator", separator);
}

gboolean gebr_geoxml_program_parameter_get_is_list(GebrGeoXmlProgramParameter * program_parameter)
{
	if (program_parameter == NULL)
		return FALSE;
	if (gebr_geoxml_parameter_get_is_reference((GebrGeoXmlParameter *) program_parameter)) {
		GebrGeoXmlParameter *ref = gebr_geoxml_parameter_get_referencee((GebrGeoXmlParameter *) program_parameter);
		gboolean is_list = gebr_geoxml_program_parameter_get_is_list((GebrGeoXmlProgramParameter *)ref);
		gebr_geoxml_object_unref(ref);
		return is_list;
	}

	GdomeDOMString *string;
	GdomeElement *property_element;
	gboolean is_list;

	property_element = __gebr_geoxml_get_first_element((GdomeElement *) program_parameter, "property");
	string = gdome_str_mkref("separator");
	is_list = gdome_el_hasAttribute(property_element, string, &exception);

	gdome_str_unref(string);
	gdome_el_unref(property_element, &exception);

	return is_list;
}

gchar *gebr_geoxml_program_parameter_get_list_separator(GebrGeoXmlProgramParameter * program_parameter)
{
	if (gebr_geoxml_program_parameter_get_is_list(program_parameter) == FALSE)
		return NULL;
	gchar *list_separator;

	if (gebr_geoxml_parameter_get_is_reference((GebrGeoXmlParameter *) program_parameter)) {
		GebrGeoXmlParameter *ref = gebr_geoxml_parameter_get_referencee((GebrGeoXmlParameter *) program_parameter);
		list_separator = gebr_geoxml_program_parameter_get_list_separator((GebrGeoXmlProgramParameter *)ref);
		gebr_geoxml_object_unref(ref);
		return list_separator;
	}
	GdomeElement *element = __gebr_geoxml_get_first_element((GdomeElement *) program_parameter, "property");
	list_separator = __gebr_geoxml_get_attr_value(element, "separator");
	gdome_el_unref(element, &exception);
	return list_separator;
}

void
gebr_geoxml_program_parameter_set_first_value(GebrGeoXmlProgramParameter * program_parameter, gboolean default_value,
					      const gchar * value)
{
	GebrGeoXmlSequence *property_value = NULL;

	g_return_if_fail(program_parameter != NULL);
	g_return_if_fail(value != NULL);

	gebr_geoxml_program_parameter_get_value(program_parameter, FALSE, &property_value, 0);
	__gebr_geoxml_set_tag_value((GdomeElement *) program_parameter,
				    default_value == FALSE ? "value" : "default", value, __gebr_geoxml_create_TextNode);
	gebr_geoxml_object_unref(property_value);
}

void
gebr_geoxml_program_parameter_set_first_boolean_value(GebrGeoXmlProgramParameter * program_parameter,
						      gboolean default_value, gboolean enabled)
{
	if (program_parameter == NULL)
		return;
	__gebr_geoxml_set_tag_value((GdomeElement *) program_parameter,
				    default_value == FALSE ? "value" : "default",
				    (enabled == TRUE ? "on" : "off"), __gebr_geoxml_create_TextNode);
}

gchar *gebr_geoxml_program_parameter_get_first_value(GebrGeoXmlProgramParameter * program_parameter,
							   gboolean default_value)
{
	g_return_val_if_fail(program_parameter != NULL, NULL);

	return __gebr_geoxml_get_tag_value((GdomeElement *) program_parameter,
					   default_value == FALSE ? "value" : "default");
}

gboolean
gebr_geoxml_program_parameter_get_first_boolean_value(GebrGeoXmlProgramParameter * program_parameter,
						      gboolean default_value)
{
	if (program_parameter == NULL)
		return FALSE;
	return (!strcmp(__gebr_geoxml_get_tag_value((GdomeElement *) program_parameter,
						    default_value == FALSE ? "value" : "default"),
			"on")) ? TRUE : FALSE;
}

GebrGeoXmlPropertyValue *gebr_geoxml_program_parameter_append_value(GebrGeoXmlProgramParameter * program_parameter,
								    gboolean default_value)
{
	if (program_parameter == NULL)
		return NULL;

	GdomeElement *property;
	GdomeElement *defaulty;
	GebrGeoXmlPropertyValue *property_value;

	if (default_value == FALSE) {
		property = __gebr_geoxml_get_first_element((GdomeElement *) program_parameter, "property");
		defaulty = __gebr_geoxml_get_first_element((GdomeElement *) program_parameter, "default");
		property_value = (GebrGeoXmlPropertyValue *) __gebr_geoxml_insert_new_element(property, "value", defaulty);
		gdome_el_unref(defaulty, &exception);
	} else {
		property = __gebr_geoxml_get_first_element((GdomeElement *) program_parameter, "property");
		property_value = (GebrGeoXmlPropertyValue *) __gebr_geoxml_insert_new_element(property, "default", NULL);
	}
	gdome_el_unref(property, &exception);
	return property_value;
}

int
gebr_geoxml_program_parameter_get_value(GebrGeoXmlProgramParameter * program_parameter, gboolean default_value,
					GebrGeoXmlSequence ** property_value, gulong index)
{
	if (program_parameter == NULL) {
		*property_value = NULL;
		return GEBR_GEOXML_RETV_NULL_PTR;
	}

	*property_value = (GebrGeoXmlSequence *) __gebr_geoxml_get_element_at((GdomeElement *) program_parameter,
									      default_value ==
									      FALSE ? "value" : "default", index, TRUE);

	return (*property_value == NULL)
	    ? GEBR_GEOXML_RETV_INVALID_INDEX : GEBR_GEOXML_RETV_SUCCESS;
}

glong
gebr_geoxml_program_parameter_get_values_number(GebrGeoXmlProgramParameter * program_parameter, gboolean default_value)
{
	glong number;
	GdomeElement *first;

	if (program_parameter == NULL)
		return -1;

	first = __gebr_geoxml_get_first_element((GdomeElement *) program_parameter, "property");
	number = __gebr_geoxml_get_elements_number(first, default_value? "default":"value");
	gdome_el_unref(first, &exception);

	return number;
}

void
gebr_geoxml_program_parameter_set_string_value(GebrGeoXmlProgramParameter * program_parameter, gboolean default_value,
					       const gchar * value)
{
	if (program_parameter == NULL)
		return;

	g_return_if_fail(gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(program_parameter)) != GEBR_GEOXML_PARAMETER_TYPE_GROUP);

	if (gebr_geoxml_program_parameter_get_is_list(program_parameter) == FALSE)
		gebr_geoxml_program_parameter_set_first_value(program_parameter, default_value, value);
	else
		gebr_geoxml_program_parameter_set_parse_list_value(program_parameter, default_value, value);
}

GString *gebr_geoxml_program_parameter_get_string_value(GebrGeoXmlProgramParameter * program_parameter,
							gboolean default_value)
{
	if (program_parameter == NULL)
		return NULL;

	g_return_val_if_fail(gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(program_parameter)) != GEBR_GEOXML_PARAMETER_TYPE_GROUP, NULL);

	GString *value;

	value = g_string_new("");
	if (gebr_geoxml_program_parameter_get_is_list(program_parameter) == FALSE) {
		gchar *first_value = gebr_geoxml_program_parameter_get_first_value(program_parameter, default_value);
		g_string_assign(value, first_value);
		g_free(first_value);
	} else {
		GebrGeoXmlSequence *property_value;
		gchar *separator;

		separator = gebr_geoxml_program_parameter_get_list_separator(program_parameter);
		gebr_geoxml_program_parameter_get_value(program_parameter, default_value, &property_value, 0);
		g_string_assign(value, gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(property_value)));
		gebr_geoxml_sequence_next(&property_value);
		for (; property_value != NULL; gebr_geoxml_sequence_next(&property_value)) {
			g_string_append(value, separator);
			g_string_append(value, gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(property_value)));
		}
		g_free(separator);
	}

	return value;
}

gboolean gebr_geoxml_program_parameter_is_set(GebrGeoXmlProgramParameter * self)
{
	if (!self)
		return FALSE;

	gboolean ret = FALSE;
	GString *value;
	GdomeDOMString *string;
	GdomeElement *property;

	value = gebr_geoxml_program_parameter_get_string_value(self, FALSE);
	string = gdome_str_mkref("dictkeyword");
	property = __gebr_geoxml_get_first_element((GdomeElement*)self, "property");

	if (value->len > 0 || strlen(__gebr_geoxml_get_attr_value(property, "dictkeyword")) > 0)
		ret = TRUE;

	g_string_free(value, TRUE);
	gdome_str_unref(string);
	return ret;
}

GebrGeoXmlProgramParameter *gebr_geoxml_program_parameter_find_dict_parameter(GebrGeoXmlProgramParameter *
									      program_parameter,
									      GebrGeoXmlDocument * dict_document)
{
	GebrGeoXmlProgramParameter *dict_parameter;
	GebrGeoXmlSequence *i;
	const gchar *dict_keyword;

	if (program_parameter == NULL || dict_document == NULL)
		return NULL;

	GdomeDOMString *string;
	GdomeElement *property_element;

	property_element = __gebr_geoxml_get_first_element((GdomeElement *) program_parameter, "property");

	string = gdome_str_mkref("dictkeyword");
	if (!gdome_el_hasAttribute(property_element, string, &exception))
		return NULL;
	gdome_str_unref(string);

	dict_keyword = __gebr_geoxml_get_attr_value(property_element, "dictkeyword");
	dict_parameter = NULL;
	i = GEBR_GEOXML_SEQUENCE(gebr_geoxml_parameters_get_first_parameter
				 (gebr_geoxml_document_get_dict_parameters(dict_document)));
	for (; i != NULL; gebr_geoxml_sequence_next(&i)) {
		if (!strcmp(dict_keyword, gebr_geoxml_program_parameter_get_keyword((GebrGeoXmlProgramParameter *) i))) {
			dict_parameter = (GebrGeoXmlProgramParameter *) i;
			break;
		}
	}

	return dict_parameter;
}

gboolean
gebr_geoxml_program_parameter_copy_value(GebrGeoXmlProgramParameter * program_parameter,
					 GebrGeoXmlProgramParameter * source, gboolean default_value)
{
	if (program_parameter == NULL || source == NULL)
		return FALSE;

	GdomeDocument *document;
	GdomeElement *insert_position;
	GdomeElement *element;

	__gebr_geoxml_program_parameter_remove_value_elements(program_parameter, default_value);

	document = gdome_el_ownerDocument((GdomeElement *) program_parameter, &exception);
	insert_position = default_value ? NULL
	    : __gebr_geoxml_get_first_element((GdomeElement *) program_parameter, "default");
	element = __gebr_geoxml_get_first_element((GdomeElement *) source,
						  default_value == FALSE ? "value" : "default");
	for (; element != NULL; element = __gebr_geoxml_next_same_element(element))
		gdome_n_insertBefore_protected(gdome_el_parentNode(insert_position, &exception),
				     gdome_doc_importNode(document, (GdomeNode*)element, TRUE, &exception),
				     (GdomeNode *) insert_position, &exception);

	return TRUE;
}

void
gebr_geoxml_program_parameter_set_parse_list_value(GebrGeoXmlProgramParameter * program_parameter,
						   gboolean default_value, const gchar * value)
{
	if (program_parameter == NULL || value == NULL)
		return;
	if (gebr_geoxml_program_parameter_get_is_list(program_parameter) == FALSE)
		return;

	const gchar *separator;

	__gebr_geoxml_program_parameter_remove_value_elements(program_parameter, default_value);

	separator = gebr_geoxml_program_parameter_get_list_separator(program_parameter);
	if (!strlen(value) || !strlen(separator)) {
		gebr_geoxml_object_unref(gebr_geoxml_program_parameter_append_value(program_parameter, default_value));
	} else {
		gchar **splits;
		int i;

		splits = g_strsplit(value, separator, 0);
		for (i = 0; splits[i] != NULL; ++i) {
			GebrGeoXmlPropertyValue *value;
			value = gebr_geoxml_program_parameter_append_value(program_parameter, default_value);
			gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(value), splits[i]);
			gebr_geoxml_object_unref(value);
		}
		g_strfreev(splits);
	}
}

void
gebr_geoxml_program_parameter_set_value_from_dict(GebrGeoXmlProgramParameter * program_parameter,
						  GebrGeoXmlProgramParameter * dict_parameter)
{
	GdomeElement *property_element;

	g_return_if_fail(program_parameter != NULL);

	property_element = __gebr_geoxml_get_first_element((GdomeElement *) program_parameter, "property");

	if (dict_parameter == NULL) {
		GdomeDOMString *string;

		string = gdome_str_mkref("dictkeyword");
		gdome_el_removeAttribute(property_element, string, &exception);

		gdome_str_unref(string);
	} else {
		const gchar * keyword;
		keyword = gebr_geoxml_program_parameter_get_keyword(dict_parameter);
		__gebr_geoxml_set_attr_value(property_element, "dictkeyword", keyword);
	}
}

void
gebr_geoxml_program_parameter_set_file_be_directory(GebrGeoXmlProgramParameter * program_parameter,
						    gboolean is_directory)
{
	GdomeElement * type_element;
	GebrGeoXmlParameter * parameter;

	parameter = GEBR_GEOXML_PARAMETER(program_parameter);

	g_return_if_fail(program_parameter != NULL);
	g_return_if_fail(!gebr_geoxml_parameter_get_is_reference(parameter));
	g_return_if_fail(gebr_geoxml_parameter_get_type(parameter) == GEBR_GEOXML_PARAMETER_TYPE_FILE);

	type_element = __gebr_geoxml_parameter_get_type_element(parameter);
	__gebr_geoxml_set_attr_value(type_element, "directory", (is_directory == TRUE ? "yes" : "no"));
	gdome_el_unref(type_element, &exception);
}

gboolean gebr_geoxml_program_parameter_get_file_be_directory(GebrGeoXmlProgramParameter * program_parameter)
{
	const gchar * is_directory;
	GdomeElement * type_element;
	GebrGeoXmlParameter * parameter;

	parameter = __gebr_geoxml_parameter_resolve(GEBR_GEOXML_PARAMETER(program_parameter));

	g_return_val_if_fail(program_parameter != NULL, FALSE);
	g_return_val_if_fail(gebr_geoxml_parameter_get_type(parameter) == GEBR_GEOXML_PARAMETER_TYPE_FILE, FALSE);

	type_element = __gebr_geoxml_parameter_get_type_element(parameter);
	is_directory = __gebr_geoxml_get_attr_value(type_element, "directory");

	return (!strcmp(is_directory, "yes")) ? TRUE : FALSE;
}

void
gebr_geoxml_program_parameter_set_file_filter(GebrGeoXmlProgramParameter * program_parameter,
					      const gchar * name, const gchar * pattern)
{
	GdomeElement * type_element;
	GebrGeoXmlParameter * parameter;

	parameter = GEBR_GEOXML_PARAMETER(program_parameter);

	g_return_if_fail(program_parameter != NULL);
	g_return_if_fail(!gebr_geoxml_parameter_get_is_reference(parameter));
	g_return_if_fail(gebr_geoxml_parameter_get_type(parameter) == GEBR_GEOXML_PARAMETER_TYPE_FILE);

	type_element = __gebr_geoxml_parameter_get_type_element(parameter);

	if (name)
		__gebr_geoxml_set_attr_value(type_element, "filter-name", name);
	if (pattern)
		__gebr_geoxml_set_attr_value(type_element, "filter-pattern", pattern);
}

void
gebr_geoxml_program_parameter_get_file_filter(GebrGeoXmlProgramParameter * program_parameter,
					      const gchar ** name, const gchar ** pattern)
{
	GdomeElement * type_element;
	GebrGeoXmlParameter * parameter;

	parameter = __gebr_geoxml_parameter_resolve(GEBR_GEOXML_PARAMETER(program_parameter));

	g_return_if_fail(program_parameter != NULL);
	g_return_if_fail(gebr_geoxml_parameter_get_type(parameter) == GEBR_GEOXML_PARAMETER_TYPE_FILE);

	type_element = __gebr_geoxml_parameter_get_type_element(parameter);

	if (name)
		*name = __gebr_geoxml_get_attr_value(type_element, "filter-name");
	if (pattern)
		*pattern = __gebr_geoxml_get_attr_value(type_element, "filter-pattern");
}

void
gebr_geoxml_program_parameter_set_number_min_max(GebrGeoXmlProgramParameter * program_parameter,
						 const gchar * min, const gchar * max)
{
	GdomeElement *type_element;
	GebrGeoXmlParameter * parameter;
	GebrGeoXmlParameterType type;

	parameter = GEBR_GEOXML_PARAMETER(program_parameter);
	type = gebr_geoxml_parameter_get_type(parameter);

	g_return_if_fail(program_parameter != NULL);
	g_return_if_fail(!gebr_geoxml_parameter_get_is_reference(parameter));
	g_return_if_fail(type == GEBR_GEOXML_PARAMETER_TYPE_INT
			 || type == GEBR_GEOXML_PARAMETER_TYPE_FLOAT
			 || type == GEBR_GEOXML_PARAMETER_TYPE_RANGE);

	type_element = __gebr_geoxml_parameter_get_type_element(parameter);

	if (min != NULL)
		__gebr_geoxml_set_attr_value(type_element, "min", min);

	if (max != NULL)
		__gebr_geoxml_set_attr_value(type_element, "max", max);
}

void
gebr_geoxml_program_parameter_get_number_min_max(GebrGeoXmlProgramParameter * program_parameter,
						 const gchar ** min, const gchar ** max)
{
	GdomeElement *type_element;
	GebrGeoXmlParameter * parameter;
	GebrGeoXmlParameterType type;

	g_return_if_fail(program_parameter != NULL);

	parameter = __gebr_geoxml_parameter_resolve(GEBR_GEOXML_PARAMETER(program_parameter));
	type = gebr_geoxml_parameter_get_type(parameter);

	g_return_if_fail(type == GEBR_GEOXML_PARAMETER_TYPE_INT
			 || type == GEBR_GEOXML_PARAMETER_TYPE_FLOAT
			 || type == GEBR_GEOXML_PARAMETER_TYPE_RANGE);

	type_element = __gebr_geoxml_parameter_get_type_element(parameter);

	if (min)
		*min = __gebr_geoxml_get_attr_value(type_element, "min");
	if (max)
		*max = __gebr_geoxml_get_attr_value(type_element, "max");
}

void
gebr_geoxml_program_parameter_set_range_properties(GebrGeoXmlProgramParameter * program_parameter,
						   const gchar * min, const gchar * max,
						   const gchar * inc, const gchar * digits)
{
	GdomeElement *type_element;
	GebrGeoXmlParameter * parameter;
	GebrGeoXmlParameterType type;

	parameter = GEBR_GEOXML_PARAMETER(program_parameter);
	type = gebr_geoxml_parameter_get_type(parameter);

	g_return_if_fail(program_parameter != NULL);
	g_return_if_fail(!gebr_geoxml_parameter_get_is_reference(parameter));
	g_return_if_fail(type == GEBR_GEOXML_PARAMETER_TYPE_RANGE);

	type_element = __gebr_geoxml_parameter_get_type_element(parameter);

	if (min)
		__gebr_geoxml_set_attr_value(type_element, "min", min);
	if (max)
		__gebr_geoxml_set_attr_value(type_element, "max", max);
	if (inc)
		__gebr_geoxml_set_attr_value(type_element, "inc", inc);
	if (digits)
		__gebr_geoxml_set_attr_value(type_element, "digits", digits);

	gdome_el_unref(type_element, &exception);
}

void
gebr_geoxml_program_parameter_get_range_properties(GebrGeoXmlProgramParameter * program_parameter,
						   const gchar ** min, const gchar ** max,
						   const gchar ** inc, const gchar ** digits)
{
	GdomeElement *type_element;
	GebrGeoXmlParameter * parameter;
	GebrGeoXmlParameterType type;

	g_return_if_fail(program_parameter != NULL);

	parameter = __gebr_geoxml_parameter_resolve(GEBR_GEOXML_PARAMETER(program_parameter));
	type = gebr_geoxml_parameter_get_type(parameter);

	g_return_if_fail(type == GEBR_GEOXML_PARAMETER_TYPE_RANGE);

	type_element = __gebr_geoxml_parameter_get_type_element(parameter);

	if (min)
		*min = __gebr_geoxml_get_attr_value(type_element, "min");
	if (max)
		*max = __gebr_geoxml_get_attr_value(type_element, "max");
	if (inc)
		*inc = __gebr_geoxml_get_attr_value(type_element, "inc");
	if (digits)
		*digits = __gebr_geoxml_get_attr_value(type_element, "digits");
}

GebrGeoXmlEnumOption *gebr_geoxml_program_parameter_append_enum_option(GebrGeoXmlProgramParameter * program_parameter,
								       const gchar * label, const gchar * value)
{
	GdomeElement *enum_option;
	GdomeElement *type_element;
	GdomeElement *label_element;
	GdomeElement *value_element;
	GebrGeoXmlParameter * parameter;
	GebrGeoXmlParameterType type;

	g_return_val_if_fail(program_parameter != NULL, NULL);
	g_return_val_if_fail(label != NULL, NULL);
	g_return_val_if_fail(value != NULL, NULL);

	parameter = __gebr_geoxml_parameter_resolve(GEBR_GEOXML_PARAMETER(program_parameter));
	type = gebr_geoxml_parameter_get_type(parameter);

	g_return_val_if_fail(type == GEBR_GEOXML_PARAMETER_TYPE_ENUM, NULL);

	enum_option = __gebr_geoxml_new_element((GdomeElement *) program_parameter, "option");
	label_element = __gebr_geoxml_insert_new_element(enum_option, "label", NULL);
	value_element = __gebr_geoxml_insert_new_element(enum_option, "value", NULL);
	__gebr_geoxml_set_element_value(label_element, label, __gebr_geoxml_create_TextNode);
	__gebr_geoxml_set_element_value(value_element, value, __gebr_geoxml_create_TextNode);

	type_element = __gebr_geoxml_parameter_get_type_element(parameter);
	gdome_el_insertBefore_protected(type_element, (GdomeNode*)enum_option, NULL, &exception);

	return GEBR_GEOXML_ENUM_OPTION(enum_option);
}

int
gebr_geoxml_program_parameter_get_enum_option(GebrGeoXmlProgramParameter * program_parameter,
					      GebrGeoXmlSequence ** enum_option, gulong index)
{
	GdomeElement * type_element;
	GebrGeoXmlParameter * parameter;

	parameter = GEBR_GEOXML_PARAMETER(program_parameter);

	if (program_parameter == NULL) {
		*enum_option = NULL;
		return GEBR_GEOXML_RETV_NULL_PTR;
	}

	if (gebr_geoxml_parameter_get_type(parameter) != GEBR_GEOXML_PARAMETER_TYPE_ENUM)
		return GEBR_GEOXML_RETV_PARAMETER_NOT_ENUM;

	if (gebr_geoxml_parameter_get_is_reference(parameter)) {
		GebrGeoXmlParameter * referencee;
		GebrGeoXmlProgramParameter * program;
		int retval;

		referencee = gebr_geoxml_parameter_get_referencee(parameter);
		program = GEBR_GEOXML_PROGRAM_PARAMETER(referencee);
		retval = gebr_geoxml_program_parameter_get_enum_option(program, enum_option, index);
		gebr_geoxml_object_unref(referencee);
		return retval;
	}

	type_element = __gebr_geoxml_parameter_get_type_element(parameter);
	*enum_option = GEBR_GEOXML_SEQUENCE(__gebr_geoxml_get_element_at(type_element, "option", index, TRUE));
	gdome_el_unref(type_element, &exception);

	return (*enum_option == NULL) ? GEBR_GEOXML_RETV_INVALID_INDEX : GEBR_GEOXML_RETV_SUCCESS;
}

glong gebr_geoxml_program_parameter_get_enum_options_number(GebrGeoXmlProgramParameter * program_parameter)
{
	GdomeElement * type_element;
	GebrGeoXmlParameter * parameter;
	GebrGeoXmlParameterType type;

	parameter = __gebr_geoxml_parameter_resolve(GEBR_GEOXML_PARAMETER(program_parameter));
	type = gebr_geoxml_parameter_get_type(parameter);

	g_return_val_if_fail(program_parameter != NULL, -1);
	g_return_val_if_fail(type == GEBR_GEOXML_PARAMETER_TYPE_ENUM, -1);

	type_element = __gebr_geoxml_parameter_get_type_element(parameter);
	return __gebr_geoxml_get_elements_number(type_element, "options");
}

gboolean gebr_geoxml_program_parameter_is_var_used (GebrGeoXmlProgramParameter *self,
						    const gchar *var_name)
{
	GebrGeoXmlSequence *seq;
	GebrGeoXmlValueSequence *value;
	const gchar *expr;
	gboolean retval = FALSE;
	GList *vars;

	gebr_geoxml_program_parameter_get_value (self, FALSE, &seq, 0);
	while (seq) {
		value = GEBR_GEOXML_VALUE_SEQUENCE (seq);
		expr = gebr_geoxml_value_sequence_get (value);
		vars = gebr_expr_extract_vars (expr);

		for (GList *i = vars; i; i = i->next) {
			gchar *name = i->data;
			if (g_strcmp0(name, var_name) == 0) {
				retval = TRUE;
				break;
			}
		}

		g_list_foreach (vars, (GFunc) g_free, NULL);
		g_list_free (vars);

		if (retval)
			break;

		gebr_geoxml_sequence_next (&seq);
	}

	return retval;
}

gboolean
gebr_geoxml_program_parameter_has_value(GebrGeoXmlProgramParameter *self)
{
	GString *s = gebr_geoxml_program_parameter_get_string_value(self, FALSE);
	gboolean retval = (s->len != 0);
	g_string_free(s, TRUE);
	return retval;
}

gchar *
gebr_geoxml_program_parameter_get_old_dict_keyword(GebrGeoXmlProgramParameter * program_parameter)
{
	g_return_val_if_fail(program_parameter != NULL, NULL);
	gchar *dict_keyword;

	GdomeDOMString *string;
	GdomeElement *property_element;

	property_element = __gebr_geoxml_get_first_element((GdomeElement *) program_parameter, "property");

	string = gdome_str_mkref("dictkeyword");
	if (!gdome_el_hasAttribute(property_element, string, &exception)) {
		gdome_el_unref(property_element, &exception);
		gdome_str_unref(string);
		return NULL;
	}

	dict_keyword = __gebr_geoxml_get_attr_value(property_element, "dictkeyword");
	gdome_el_unref(property_element, &exception);
	gdome_str_unref(string);

	return dict_keyword;
}

gboolean
gebr_geoxml_program_parameter_update_old_dict_value(GebrGeoXmlObject * param,
						    gpointer keys_to_canonized)
{
	g_return_val_if_fail(keys_to_canonized != NULL, FALSE);
	g_return_val_if_fail(param != NULL, FALSE);

	GebrGeoXmlParameterType type = gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(param));

	gchar *key = gebr_geoxml_program_parameter_get_old_dict_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(param));

	if (key == NULL)
	{
		gchar *value = gebr_geoxml_program_parameter_get_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(param), FALSE);

		switch(type)
		{
		case GEBR_GEOXML_PARAMETER_TYPE_INT:
		case GEBR_GEOXML_PARAMETER_TYPE_FLOAT: {
			if(g_strrstr(value, "e"))
			{
				GString * string = g_string_new(value);
				gebr_g_string_replace(string, "e", "*10^");
				gebr_g_string_replace(string, "E", "*10^");
				gebr_geoxml_program_parameter_set_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(param), FALSE, string->str);
				g_string_free(string, TRUE);
			}

			break;
		}
		case GEBR_GEOXML_PARAMETER_TYPE_STRING:
		case GEBR_GEOXML_PARAMETER_TYPE_FILE: {
			GString * string = g_string_new(value);
			gebr_g_string_replace(string, "[", "[[");
			gebr_g_string_replace(string, "]", "]]");
			gebr_geoxml_program_parameter_set_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(param), FALSE, string->str);
			g_string_free(string, TRUE);

			break;
		}
		default:
			break;
		}
		g_free(value);
		g_free(key);
		return TRUE;
	}

	gchar *spaces = g_strdup(key);
	if (!*g_strstrip(spaces)) {
		g_free(spaces);
		g_free(key);
		return TRUE;
	}
	g_free(spaces);

	const gchar *canonized = g_hash_table_lookup((GHashTable *)keys_to_canonized, key);

	g_return_val_if_fail(canonized != NULL, FALSE);

	switch(type)
	{
	case GEBR_GEOXML_PARAMETER_TYPE_INT:
	case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
		gebr_geoxml_program_parameter_set_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(param), FALSE, canonized);
		break;
	case GEBR_GEOXML_PARAMETER_TYPE_STRING:
	case GEBR_GEOXML_PARAMETER_TYPE_FILE: {
		gchar * brackets = g_strdup_printf("[%s]", canonized);
		gebr_geoxml_program_parameter_set_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(param), FALSE, brackets);
		g_free(brackets);

		break;
	}
	
	default:
		break;
	}

	g_free(key);
	return TRUE;
}
