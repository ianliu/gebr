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

#include "../../intl.h"

#include "parameter.h"
#include "parameter_p.h"
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

struct gebr_geoxml_parameter {
	GdomeElement *element;
};

const char *parameter_type_to_str[] = { "",
	"string", "int", "file",
	"flag", "float", "range",
	"enum", "group", "reference"
};

const int parameter_type_to_str_len = 9;

/*
 * private functions
 */

GdomeElement *__gebr_geoxml_parameter_get_type_element(GebrGeoXmlParameter * parameter)
{
	GdomeElement *type_element;
	type_element = __gebr_geoxml_get_first_element((GdomeElement *) parameter, "label");
	return __gebr_geoxml_next_element(type_element);
}

/**
 * __gebr_geoxml_parameter_resolve:
 */
GebrGeoXmlParameter * __gebr_geoxml_parameter_resolve(GebrGeoXmlParameter * parameter)
{
	if (gebr_geoxml_parameter_get_is_reference(parameter))
		return gebr_geoxml_parameter_get_referencee(parameter);
	return parameter;
}

GebrGeoXmlParameterType
__gebr_geoxml_parameter_get_type(GebrGeoXmlParameter * parameter, gboolean resolve_references)
{
	GdomeDOMString *tag_name;
	GdomeElement *type_element;

	if (resolve_references) {
		GebrGeoXmlParameter * referencee;
		referencee = gebr_geoxml_parameter_get_referencee(parameter);
		if (referencee)
			return __gebr_geoxml_parameter_get_type(referencee, FALSE);
	}

	type_element = __gebr_geoxml_parameter_get_type_element(parameter);
	tag_name = gdome_el_tagName(type_element, &exception);
	for (gint i = 1; i <= parameter_type_to_str_len; ++i)
		if (!strcmp(parameter_type_to_str[i], tag_name->str))
			return (GebrGeoXmlParameterType)i;

	return GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN;
}

void __gebr_geoxml_parameter_set_be_reference(GebrGeoXmlParameter * parameter)
{
	GString *default_value;
	GdomeElement *type_element;
	GebrGeoXmlParameterType type;
	GebrGeoXmlProgramParameter *program;

	type = __gebr_geoxml_parameter_get_type(parameter, FALSE);

	g_return_if_fail(type != GEBR_GEOXML_PARAMETER_TYPE_REFERENCE);

	program = GEBR_GEOXML_PROGRAM_PARAMETER(parameter);
	default_value = gebr_geoxml_program_parameter_get_string_value(program, TRUE);
	type_element = __gebr_geoxml_parameter_get_type_element(parameter);

	gdome_el_removeChild((GdomeElement *) parameter, (GdomeNode *) type_element, &exception);
	__gebr_geoxml_parameter_insert_type(parameter, GEBR_GEOXML_PARAMETER_TYPE_REFERENCE);
	gebr_geoxml_program_parameter_set_string_value(program, TRUE, default_value->str);

	g_string_free(default_value, TRUE);
}

GdomeElement *__gebr_geoxml_parameter_insert_type(GebrGeoXmlParameter * parameter, GebrGeoXmlParameterType type)
{
	GdomeElement *type_element;

	type_element = __gebr_geoxml_insert_new_element((GdomeElement *) parameter, parameter_type_to_str[type], NULL);
	if (type == GEBR_GEOXML_PARAMETER_TYPE_GROUP) {
		GdomeElement * template;
		template = __gebr_geoxml_insert_new_element(type_element, "template-instance", NULL);
		__gebr_geoxml_parameters_append_new(template);
		__gebr_geoxml_parameters_append_new(type_element);
		gebr_geoxml_parameter_group_set_is_instanciable((GebrGeoXmlParameterGroup *)parameter, FALSE);
		gebr_geoxml_parameter_group_set_expand((GebrGeoXmlParameterGroup *)parameter, FALSE);
	} else {
		GdomeElement *property_element;
		GdomeElement *value_element;
		GdomeElement *default_value_element;

		property_element = __gebr_geoxml_insert_new_element(type_element, "property", NULL);
		__gebr_geoxml_insert_new_element(property_element, "keyword", NULL);
		value_element = __gebr_geoxml_insert_new_element(property_element, "value", NULL);
		default_value_element = __gebr_geoxml_insert_new_element(property_element, "default", NULL);
		__gebr_geoxml_set_attr_value(property_element, "required", "no");

		switch (type) {
		case GEBR_GEOXML_PARAMETER_TYPE_FILE:
			gebr_geoxml_program_parameter_set_file_be_directory((GebrGeoXmlProgramParameter *) parameter,
									    FALSE);
			break;
		case GEBR_GEOXML_PARAMETER_TYPE_RANGE:
			gebr_geoxml_program_parameter_set_range_properties((GebrGeoXmlProgramParameter *) parameter, "",
									   "", "", "");
			break;
		case GEBR_GEOXML_PARAMETER_TYPE_FLAG:
			__gebr_geoxml_set_element_value(value_element, "off", __gebr_geoxml_create_TextNode);
			__gebr_geoxml_set_element_value(default_value_element, "off", __gebr_geoxml_create_TextNode);
			break;
		default:
			break;
		}
	}

	return type_element;
}

GSList *__gebr_geoxml_parameter_get_referencee_list(GebrGeoXmlParameter * parameter)
{
	gint index;
	GSList * list = NULL;
	GdomeNode * parent;
	GdomeDOMString * tagname;
	GebrGeoXmlSequence * instance;

	/* This parameter should be inside a template tag, according to flow DTD 0.3.5.
	 * So, the hierarchy should be this:
	 * 	parameter   <-- We want this guy
	 * 	  |-label
	 * 	  `-group
	 * 	      |-template-instance
	 * 	      |   `-parameters
	 * 	      |       `-parameter*   <-- You are here (4 parents below)
	 * 	      `-parameters*
	 */
	parent = gdome_el_parentNode((GdomeElement*)parameter, &exception);
	parent = gdome_el_parentNode((GdomeElement*)parent, &exception);
	parent = gdome_el_parentNode((GdomeElement*)parent, &exception);
	tagname = gdome_el_tagName((GdomeElement*)parent, &exception);

	g_return_val_if_fail("parameter should be inside a template"
			     && strcmp(tagname->str, "group") == 0, NULL);
	parent = gdome_el_parentNode((GdomeElement*)parent, &exception);

	index = gebr_geoxml_sequence_get_index(GEBR_GEOXML_SEQUENCE(parameter));

	/* Iterate in all instances, inserting the index-eth parameter into the list
	 */
	gebr_geoxml_parameter_group_get_instance(GEBR_GEOXML_PARAMETER_GROUP(parent),
						 &instance, 0);
	while (instance) {
		GebrGeoXmlSequence * param;

		gebr_geoxml_parameters_get_parameter(GEBR_GEOXML_PARAMETERS(instance),
						     &param, index);
		list = g_slist_prepend(list, param);
		gebr_geoxml_sequence_next(&instance);
	}
	return g_slist_reverse(list);
}


void __gebr_geoxml_parameter_set_label(GebrGeoXmlParameter * parameter, const gchar * label)
{
	__gebr_geoxml_set_tag_value((GdomeElement *) parameter, "label", label, __gebr_geoxml_create_TextNode);
}

/*
 * library functions.
 */

GebrGeoXmlParameters *gebr_geoxml_parameter_get_parameters(GebrGeoXmlParameter * parameter)
{
	g_return_val_if_fail(parameter != NULL, NULL);
	return (GebrGeoXmlParameters *) gdome_n_parentNode((GdomeNode *) parameter, &exception);
}

gboolean gebr_geoxml_parameter_set_type(GebrGeoXmlParameter * parameter, GebrGeoXmlParameterType type)
{
	GdomeElement * type_element;

	g_return_val_if_fail(parameter != NULL, FALSE);
	g_return_val_if_fail(type != GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN, FALSE);
	g_return_val_if_fail(type != GEBR_GEOXML_PARAMETER_TYPE_REFERENCE, FALSE);

	type_element = __gebr_geoxml_parameter_get_type_element(parameter);
	gdome_n_removeChild((GdomeNode*)parameter, (GdomeNode*)type_element, &exception);
	__gebr_geoxml_parameter_insert_type(parameter, type);

	return TRUE;
}

int gebr_geoxml_parameter_set_be_reference(GebrGeoXmlParameter * parameter, GebrGeoXmlParameter * reference)
{
	if (parameter == NULL || reference == NULL)
		return GEBR_GEOXML_RETV_NULL_PTR;
	if (parameter == reference)
		return GEBR_GEOXML_RETV_REFERENCE_TO_ITSELF;

	__gebr_geoxml_parameter_set_be_reference(parameter);

	return GEBR_GEOXML_RETV_SUCCESS;
}

GebrGeoXmlParameterType gebr_geoxml_parameter_get_type(GebrGeoXmlParameter * parameter)
{
	g_return_val_if_fail(parameter != NULL, GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN);

	return __gebr_geoxml_parameter_get_type(parameter, TRUE);
}

const gchar *gebr_geoxml_parameter_get_type_name(GebrGeoXmlParameter * parameter)
{
	if (parameter == NULL)
		return NULL;

	static const gchar *type_names[] = {
		NULL, N_("string"), N_("integer"), N_("file"), N_("flag"),
		N_("real number"), N_("range"), N_("enum"),
		N_("group"), N_("reference")
	};

	return type_names[__gebr_geoxml_parameter_get_type(parameter, TRUE)];
}

gboolean gebr_geoxml_parameter_get_is_reference(GebrGeoXmlParameter * parameter)
{
	if (parameter == NULL)
		return FALSE;
	return __gebr_geoxml_parameter_get_type(parameter, FALSE) == GEBR_GEOXML_PARAMETER_TYPE_REFERENCE
	    ? TRUE : FALSE;
}

GebrGeoXmlParameter *gebr_geoxml_parameter_get_referencee(GebrGeoXmlParameter * parameter_reference)
{
	gint index;
	GdomeElement * group;
	GdomeDOMString * node_name;
	GebrGeoXmlSequence * referencee;
	GebrGeoXmlParameters * template;

	if (!gebr_geoxml_parameter_get_is_reference(parameter_reference))
		return NULL;

	group = (GdomeElement*)gdome_el_parentNode((GdomeElement*)parameter_reference, &exception);
	group = (GdomeElement*)gdome_el_parentNode(group, &exception);
	
	node_name = gdome_el_nodeName(group, &exception);
	if (strcmp(node_name->str, "group") != 0)
		g_error("DTD is wrong!");

	group = (GdomeElement*)gdome_el_parentNode(group, &exception);
	template = gebr_geoxml_parameter_group_get_template(GEBR_GEOXML_PARAMETER_GROUP(group));

	if (!template)
		g_error("DTD is wrong!");

	index = gebr_geoxml_sequence_get_index(GEBR_GEOXML_SEQUENCE(parameter_reference));
	gebr_geoxml_parameters_get_parameter(template, &referencee, index);

	return GEBR_GEOXML_PARAMETER(referencee);
}

gboolean gebr_geoxml_parameter_get_is_program_parameter(GebrGeoXmlParameter * parameter)
{
	if (parameter == NULL)
		return FALSE;
	return (gebr_geoxml_parameter_get_type(parameter) != GEBR_GEOXML_PARAMETER_TYPE_GROUP)
	    ? TRUE : FALSE;
}

void gebr_geoxml_parameter_set_label(GebrGeoXmlParameter * parameter, const gchar * label)
{
	GebrGeoXmlParameterType type;

	if (parameter == NULL || label == NULL)
		return;

	type = __gebr_geoxml_parameter_get_type(parameter, FALSE);

	g_return_if_fail(type != GEBR_GEOXML_PARAMETER_TYPE_REFERENCE);

	__gebr_geoxml_set_tag_value((GdomeElement *) parameter, "label", label, __gebr_geoxml_create_TextNode);
}

const gchar *gebr_geoxml_parameter_get_label(GebrGeoXmlParameter * parameter)
{
	GebrGeoXmlParameter * template ;

	if (parameter == NULL)
		return NULL;
	if (gebr_geoxml_parameter_get_is_reference(parameter))
		template = gebr_geoxml_parameter_get_referencee(parameter);
	else
		template = parameter;

	return __gebr_geoxml_get_tag_value((GdomeElement *) template, "label");
}

gboolean gebr_geoxml_parameter_get_is_in_group(GebrGeoXmlParameter * parameter)
{
	if (parameter == NULL)
		return FALSE;
	return gebr_geoxml_parameters_get_is_in_group(gebr_geoxml_parameter_get_parameters(parameter));
}

GebrGeoXmlParameterGroup *gebr_geoxml_parameter_get_group(GebrGeoXmlParameter * parameter)
{
	if (parameter == NULL)
		return NULL;
	return gebr_geoxml_parameters_get_group(gebr_geoxml_parameter_get_parameters(parameter));
}

void gebr_geoxml_parameter_reset(GebrGeoXmlParameter * parameter, gboolean recursive)
{
	if (parameter == NULL)
		return;
	if (gebr_geoxml_parameter_get_type(parameter) == GEBR_GEOXML_PARAMETER_TYPE_GROUP) {
		GebrGeoXmlSequence *instance;

		if (recursive == FALSE)
			return;

		/* call gebr_geoxml_parameter_reset on each child */
		gebr_geoxml_parameter_group_get_instance(GEBR_GEOXML_PARAMETER_GROUP(parameter), &instance, 0);
		for (; instance != NULL; gebr_geoxml_sequence_next(&instance))
			gebr_geoxml_parameters_reset(GEBR_GEOXML_PARAMETERS(instance), recursive);
	} else {
		__gebr_geoxml_program_parameter_set_all_value((GebrGeoXmlProgramParameter *) parameter, FALSE, "");
		__gebr_geoxml_program_parameter_set_all_value((GebrGeoXmlProgramParameter *) parameter, TRUE, "");
	}
}
