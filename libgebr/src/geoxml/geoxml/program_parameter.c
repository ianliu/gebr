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
	while (element != NULL) {
		GdomeElement *tmp;

		tmp = __gebr_geoxml_next_same_element(element);
		gdome_n_removeChild(gdome_el_parentNode(element, &exception), (GdomeNode *) element, &exception);
		element = tmp;
	}
}

/*
 * library functions.
 */

GebrGeoXmlProgram *gebr_geoxml_program_parameter_program(GebrGeoXmlProgramParameter * program_parameter)
{
	if (program_parameter == NULL)
		return NULL;

	GdomeElement *program_element;

	while (1) {
		GdomeDOMString *name;

		program_element = (GdomeElement *) gdome_n_parentNode((GdomeNode *) program_parameter, &exception);
		name = gdome_el_nodeName(program_element, &exception);
		if (!strcmp(name->str, "program"))
			break;
	}

	return (GebrGeoXmlProgram *) program_element;
}

void gebr_geoxml_program_parameter_set_required(GebrGeoXmlProgramParameter * program_parameter, gboolean required)
{
	if (program_parameter == NULL)
		return;
	if (gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(program_parameter)) == GEBR_GEOXML_PARAMETER_TYPE_FLAG)
		return;
	__gebr_geoxml_set_attr_value(__gebr_geoxml_get_first_element((GdomeElement *) program_parameter, "property"),
				     "required", (required == TRUE ? "yes" : "no"));
}

gboolean gebr_geoxml_program_parameter_get_required(GebrGeoXmlProgramParameter * program_parameter)
{
	if (program_parameter == NULL)
		return FALSE;
	if (gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(program_parameter)) == GEBR_GEOXML_PARAMETER_TYPE_FLAG)
		return FALSE;
	if (gebr_geoxml_parameter_get_is_reference((GebrGeoXmlParameter *) program_parameter))
		return gebr_geoxml_program_parameter_get_required((GebrGeoXmlProgramParameter *)
								  gebr_geoxml_parameter_get_referencee((GebrGeoXmlParameter *) program_parameter));
	return (!strcmp
		(__gebr_geoxml_get_attr_value
		 (__gebr_geoxml_get_first_element((GdomeElement *) program_parameter, "property"), "required"), "yes"))
	    ? TRUE : FALSE;
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

const gchar *gebr_geoxml_program_parameter_get_keyword(GebrGeoXmlProgramParameter * program_parameter)
{
	if (program_parameter == NULL)
		return NULL;
	if (gebr_geoxml_parameter_get_is_reference((GebrGeoXmlParameter *) program_parameter))
		return gebr_geoxml_program_parameter_get_keyword((GebrGeoXmlProgramParameter *)
								 gebr_geoxml_parameter_get_referencee((GebrGeoXmlParameter *) program_parameter));
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
	if (gebr_geoxml_parameter_get_is_reference((GebrGeoXmlParameter *) program_parameter))
		return gebr_geoxml_program_parameter_get_is_list((GebrGeoXmlProgramParameter *)
								 gebr_geoxml_parameter_get_referencee((GebrGeoXmlParameter *) program_parameter));

	GdomeDOMString *string;
	GdomeElement *property_element;
	gboolean is_list;

	property_element = __gebr_geoxml_get_first_element((GdomeElement *) program_parameter, "property");
	string = gdome_str_mkref("separator");
	is_list = gdome_el_hasAttribute(property_element, string, &exception);

	gdome_str_unref(string);

	return is_list;
}

const gchar *gebr_geoxml_program_parameter_get_list_separator(GebrGeoXmlProgramParameter * program_parameter)
{
	if (gebr_geoxml_program_parameter_get_is_list(program_parameter) == FALSE)
		return NULL;
	if (gebr_geoxml_parameter_get_is_reference((GebrGeoXmlParameter *) program_parameter))
		return gebr_geoxml_program_parameter_get_list_separator((GebrGeoXmlProgramParameter *)
									gebr_geoxml_parameter_get_referencee((GebrGeoXmlParameter *) program_parameter));
	return
	    __gebr_geoxml_get_attr_value(__gebr_geoxml_get_first_element
					 ((GdomeElement *) program_parameter, "property"), "separator");
}

void
gebr_geoxml_program_parameter_set_first_value(GebrGeoXmlProgramParameter * program_parameter, gboolean default_value,
					      const gchar * value)
{
	if (program_parameter == NULL || value == NULL)
		return;
	__gebr_geoxml_set_tag_value((GdomeElement *) program_parameter,
				    default_value == FALSE ? "value" : "default", value, __gebr_geoxml_create_TextNode);
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

const gchar *gebr_geoxml_program_parameter_get_first_value(GebrGeoXmlProgramParameter * program_parameter,
							   gboolean default_value)
{
	if (program_parameter == NULL)
		return NULL;
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

	GebrGeoXmlPropertyValue *property_value;

	if (default_value == FALSE)
		property_value = (GebrGeoXmlPropertyValue *)
		    __gebr_geoxml_insert_new_element(__gebr_geoxml_get_first_element
						     ((GdomeElement *) program_parameter, "property"), "value",
						     __gebr_geoxml_get_first_element((GdomeElement *) program_parameter,
										     "default"));
	else
		property_value = (GebrGeoXmlPropertyValue *)
		    __gebr_geoxml_insert_new_element(__gebr_geoxml_get_first_element
						     ((GdomeElement *) program_parameter, "property"), "default", NULL);

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
	if (program_parameter == NULL)
		return -1;
	return
	    __gebr_geoxml_get_elements_number(__gebr_geoxml_get_first_element
					      ((GdomeElement *) program_parameter, "property"),
					      default_value == FALSE ? "value" : "default");
}

void
gebr_geoxml_program_parameter_set_string_value(GebrGeoXmlProgramParameter * program_parameter, gboolean default_value,
					       const gchar * value)
{
	if (program_parameter == NULL)
		return;

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

	GString *value;

	value = g_string_new("");
	if (gebr_geoxml_program_parameter_get_is_list(program_parameter) == FALSE)
		g_string_assign(value, gebr_geoxml_program_parameter_get_first_value(program_parameter, default_value));
	else {
		GebrGeoXmlSequence *property_value;
		const gchar *separator;

		separator = gebr_geoxml_program_parameter_get_list_separator(program_parameter);
		gebr_geoxml_program_parameter_get_value(program_parameter, default_value, &property_value, 0);
		g_string_assign(value, gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(property_value)));
		gebr_geoxml_sequence_next(&property_value);
		for (; property_value != NULL; gebr_geoxml_sequence_next(&property_value)) {
			g_string_append(value, separator);
			g_string_append(value,
					gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(property_value)));
		}
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
				     gdome_doc_importNode_protected(document, element),
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
	if (!strlen(value) || !strlen(separator))
		gebr_geoxml_program_parameter_append_value(program_parameter, default_value);
	else {
		gchar **splits;
		int i;

		splits = g_strsplit(value, separator, 0);
		for (i = 0; splits[i] != NULL; ++i)
			gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE
						       (gebr_geoxml_program_parameter_append_value
							(program_parameter, default_value)), splits[i]);
		g_strfreev(splits);
	}
}

void
gebr_geoxml_program_parameter_set_value_from_dict(GebrGeoXmlProgramParameter * program_parameter,
						  GebrGeoXmlProgramParameter * dict_parameter)
{
	if (program_parameter == NULL)
		return;

	GdomeElement *property_element;

	property_element = __gebr_geoxml_get_first_element((GdomeElement *) program_parameter, "property");

	if (dict_parameter == NULL) {
		GdomeDOMString *string;

		string = gdome_str_mkref("dictkeyword");
		gdome_el_removeAttribute(property_element, string, &exception);

		gdome_str_unref(string);
	} else
		__gebr_geoxml_set_attr_value(property_element, "dictkeyword",
					     gebr_geoxml_program_parameter_get_keyword(dict_parameter));
}

void
gebr_geoxml_program_parameter_set_file_be_directory(GebrGeoXmlProgramParameter * program_parameter,
						    gboolean is_directory)
{
	GdomeElement * type_element;

	g_return_if_fail(program_parameter != NULL);
	g_return_if_fail(!gebr_geoxml_parameter_get_is_reference(GEBR_GEOXML_PARAMETER(program_parameter)));
	g_return_if_fail(gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(program_parameter)) == GEBR_GEOXML_PARAMETER_TYPE_FILE);

	type_element = __gebr_geoxml_parameter_get_type_element(GEBR_GEOXML_PARAMETER(program_parameter));
	__gebr_geoxml_set_attr_value(type_element, "directory", (is_directory == TRUE ? "yes" : "no"));
}

gboolean gebr_geoxml_program_parameter_get_file_be_directory(GebrGeoXmlProgramParameter * program_parameter)
{
	const gchar * is_directory;
	GdomeElement * type_element;
	GebrGeoXmlParameter * param;

	parameter = GEBR_GEOXML_PARAMETER(program_parameter);

	g_return_val_if_fail(program_parameter != NULL, FALSE);
	g_return_val_if_fail(gebr_geoxml_parameter_get_type(parameter) == GEBR_GEOXML_PARAMETER_TYPE_FILE, FALSE);

	if (gebr_geoxml_parameter_get_is_reference(parameter)) {
		GebrGeoXmlParameter * referencee;
		GebrGeoXmlProgramParameter * program;

		referencee = gebr_geoxml_parameter_get_referencee(parameter);
		program = GEBR_GEOXML_PROGRAM_PARAMETER(referencee);

		return gebr_geoxml_program_parameter_get_file_be_directory(program);
	}

	type_element = __gebr_geoxml_parameter_get_type_element(GEBR_GEOXML_PARAMETER(program_parameter), TRUE);
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

	parameter = GEBR_GEOXML_PARAMETER(program_parameter);

	g_return_if_fail(program_parameter != NULL);
	g_return_if_fail(gebr_geoxml_parameter_get_type(parameter) == GEBR_GEOXML_PARAMETER_TYPE_FILE);

	if (gebr_geoxml_parameter_get_is_reference(parameter)) {
		GebrGeoXmlParameter * referencee;
		GebrGeoXmlProgramParameter * program;

		referencee = gebr_geoxml_parameter_get_referencee(parameter);
		program = GEBR_GEOXML_PROGRAM_PARAMETER(referencee);

		return gebr_geoxml_program_parameter_get_file_filter(program, name, pattern);
	}

	type_element = __gebr_geoxml_parameter_get_type_element(parameter);

	if (name != NULL)
		*name = __gebr_geoxml_get_attr_value(type_element, "filter-name");

	if (pattern != NULL)
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

	type_element = __gebr_geoxml_parameter_get_type_element(GEBR_GEOXML_PARAMETER(program_parameter));

	if (min != NULL)
		__gebr_geoxml_set_attr_value(type_element, "min", min);

	if (max != NULL)
		__gebr_geoxml_set_attr_value(type_element, "max", max);
}

void
gebr_geoxml_program_parameter_get_number_min_max(GebrGeoXmlProgramParameter * program_parameter,
						 gchar ** min, gchar ** max)
{
	GebrGeoXmlParameterType type;

	if (program_parameter == NULL)
		return;
	if (gebr_geoxml_parameter_get_is_reference(GEBR_GEOXML_PARAMETER(program_parameter)))
		return gebr_geoxml_program_parameter_get_number_min_max((GebrGeoXmlProgramParameter *)
									gebr_geoxml_parameter_get_referencee((GebrGeoXmlParameter *) program_parameter), min, max);
	type = gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(program_parameter));
	if (type != GEBR_GEOXML_PARAMETER_TYPE_INT &&
	    type != GEBR_GEOXML_PARAMETER_TYPE_FLOAT && type != GEBR_GEOXML_PARAMETER_TYPE_RANGE)
		return;

	GdomeElement *type_element;

	type_element = __gebr_geoxml_parameter_get_type_element(GEBR_GEOXML_PARAMETER(program_parameter), TRUE);
	if (min != NULL)
		*min = (gchar *) __gebr_geoxml_get_attr_value(type_element, "min");
	if (max != NULL)
		*max = (gchar *) __gebr_geoxml_get_attr_value(type_element, "max");
}

void
gebr_geoxml_program_parameter_set_range_properties(GebrGeoXmlProgramParameter * program_parameter,
						   const gchar * min, const gchar * max, const gchar * inc,
						   const gchar * digits)
{
	if (program_parameter == NULL)
		return;
	if (gebr_geoxml_parameter_get_is_reference(GEBR_GEOXML_PARAMETER(program_parameter)))
		return;
	if (gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(program_parameter)) !=
	    GEBR_GEOXML_PARAMETER_TYPE_RANGE)
		return;

	GdomeElement *type_element;

	type_element = __gebr_geoxml_parameter_get_type_element(GEBR_GEOXML_PARAMETER(program_parameter), FALSE);
	if (min != NULL)
		__gebr_geoxml_set_attr_value(type_element, "min", min);
	if (max != NULL)
		__gebr_geoxml_set_attr_value(type_element, "max", max);
	if (inc != NULL)
		__gebr_geoxml_set_attr_value(type_element, "inc", inc);
	if (digits != NULL)
		__gebr_geoxml_set_attr_value(type_element, "digits", digits);
}

void
gebr_geoxml_program_parameter_get_range_properties(GebrGeoXmlProgramParameter * program_parameter,
						   gchar ** min, gchar ** max, gchar ** inc, gchar ** digits)
{
	if (program_parameter == NULL)
		return;
	if (gebr_geoxml_parameter_get_is_reference(GEBR_GEOXML_PARAMETER(program_parameter)))
		return gebr_geoxml_program_parameter_get_range_properties((GebrGeoXmlProgramParameter *)
									  gebr_geoxml_parameter_get_referencee((GebrGeoXmlParameter *) program_parameter), min, max, inc, digits);
	if (gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(program_parameter)) !=
	    GEBR_GEOXML_PARAMETER_TYPE_RANGE)
		return;

	GdomeElement *type_element;

	type_element = __gebr_geoxml_parameter_get_type_element(GEBR_GEOXML_PARAMETER(program_parameter), TRUE);
	if (min != NULL)
		*min = (gchar *) __gebr_geoxml_get_attr_value(type_element, "min");
	if (max != NULL)
		*max = (gchar *) __gebr_geoxml_get_attr_value(type_element, "max");
	if (inc != NULL)
		*inc = (gchar *) __gebr_geoxml_get_attr_value(type_element, "inc");
	if (digits != NULL)
		*digits = (gchar *) __gebr_geoxml_get_attr_value(type_element, "digits");
}

GebrGeoXmlEnumOption *gebr_geoxml_program_parameter_append_enum_option(GebrGeoXmlProgramParameter * program_parameter,
								       const gchar * label, const gchar * value)
{
	if (program_parameter == NULL || label == NULL || value == NULL)
		return NULL;
	if (gebr_geoxml_parameter_get_is_reference(GEBR_GEOXML_PARAMETER(program_parameter)))
		return gebr_geoxml_program_parameter_append_enum_option((GebrGeoXmlProgramParameter *)
									gebr_geoxml_parameter_get_referencee((GebrGeoXmlParameter *) program_parameter), label, value);
	if (gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(program_parameter)) != GEBR_GEOXML_PARAMETER_TYPE_ENUM)
		return NULL;

	GebrGeoXmlEnumOption *enum_option;
	GdomeElement *label_element;
	GdomeElement *value_element;

	enum_option = (GebrGeoXmlEnumOption *) __gebr_geoxml_new_element((GdomeElement *) program_parameter, "option");
	label_element = __gebr_geoxml_insert_new_element((GdomeElement *) enum_option, "label", NULL);
	value_element = __gebr_geoxml_insert_new_element((GdomeElement *) enum_option, "value", NULL);
	__gebr_geoxml_set_element_value(label_element, label, __gebr_geoxml_create_TextNode);
	__gebr_geoxml_set_element_value(value_element, value, __gebr_geoxml_create_TextNode);

	gdome_el_insertBefore_protected(__gebr_geoxml_parameter_get_type_element
			      ((GebrGeoXmlParameter *) program_parameter, FALSE), (GdomeNode *) enum_option, NULL,
			      &exception);

	return enum_option;
}

int
gebr_geoxml_program_parameter_get_enum_option(GebrGeoXmlProgramParameter * program_parameter,
					      GebrGeoXmlSequence ** enum_option, gulong index)
{
	if (program_parameter == NULL) {
		*enum_option = NULL;
		return GEBR_GEOXML_RETV_NULL_PTR;
	}
	if (gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(program_parameter)) != GEBR_GEOXML_PARAMETER_TYPE_ENUM)
		return GEBR_GEOXML_RETV_PARAMETER_NOT_ENUM;
	if (gebr_geoxml_parameter_get_is_reference((GebrGeoXmlParameter *) program_parameter))
		return gebr_geoxml_program_parameter_get_enum_option((GebrGeoXmlProgramParameter *)
								     gebr_geoxml_parameter_get_referencee((GebrGeoXmlParameter *) program_parameter), enum_option, index);

	*enum_option = (GebrGeoXmlSequence *)
	    __gebr_geoxml_get_element_at(__gebr_geoxml_parameter_get_type_element
					 ((GebrGeoXmlParameter *) program_parameter, TRUE), "option", index, TRUE);

	return (*enum_option == NULL)
	    ? GEBR_GEOXML_RETV_INVALID_INDEX : GEBR_GEOXML_RETV_SUCCESS;
}

glong gebr_geoxml_program_parameter_get_enum_options_number(GebrGeoXmlProgramParameter * program_parameter)
{
	if (program_parameter == NULL)
		return -1;
	if (gebr_geoxml_parameter_get_is_reference((GebrGeoXmlParameter *) program_parameter))
		return gebr_geoxml_program_parameter_get_enum_options_number((GebrGeoXmlProgramParameter *)
									     gebr_geoxml_parameter_get_referencee((GebrGeoXmlParameter *) program_parameter));
	if (gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(program_parameter)) != GEBR_GEOXML_PARAMETER_TYPE_ENUM)
		return -1;
	return
	    __gebr_geoxml_get_elements_number(__gebr_geoxml_parameter_get_type_element
					      ((GebrGeoXmlParameter *) program_parameter, TRUE), "options");
}
