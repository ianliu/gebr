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

#include "libgebr-gettext.h"

#include <string.h>
#include <gdome.h>
#include <glib/gi18n-lib.h>

#include "document.h"
#include "error.h"
#include "object.h"
#include "parameter.h"
#include "parameter_group.h"
#include "parameter_group_p.h"
#include "parameter_p.h"
#include "parameters.h"
#include "parameters_p.h"
#include "program-parameter.h"
#include "program_parameter_p.h"
#include "sequence.h"
#include "types.h"
#include "xml.h"

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
	GdomeElement *next;

	type_element = __gebr_geoxml_get_first_element((GdomeElement *) parameter, "label");
	next = __gebr_geoxml_next_element(type_element);
	gdome_el_unref(type_element, &exception);

	return next;
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
		if (referencee) {
			GebrGeoXmlParameterType type = __gebr_geoxml_parameter_get_type(referencee, FALSE);
			gebr_geoxml_object_unref(referencee);
			return type;
		}
	}

	type_element = __gebr_geoxml_parameter_get_type_element(parameter);
	tag_name = gdome_el_tagName(type_element, &exception);
	for (gint i = 1; i <= parameter_type_to_str_len; ++i) {
		if (!strcmp(parameter_type_to_str[i], tag_name->str)) {
			gdome_str_unref(tag_name);
			gdome_el_unref(type_element, &exception);
			return (GebrGeoXmlParameterType)i;
		}
	}
	gdome_el_unref(type_element, &exception);
	gdome_str_unref(tag_name);
	return GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN;
}

void __gebr_geoxml_parameter_set_be_reference_with_value(GebrGeoXmlParameter * parameter)
{
	GString *value;
	GString *default_value;
	GdomeElement *type_element;
	GebrGeoXmlParameterType type;
	GebrGeoXmlProgramParameter *program;

	type = __gebr_geoxml_parameter_get_type(parameter, FALSE);

	g_return_if_fail(type != GEBR_GEOXML_PARAMETER_TYPE_REFERENCE);

	program = GEBR_GEOXML_PROGRAM_PARAMETER(parameter);
	default_value = gebr_geoxml_program_parameter_get_string_value(program, TRUE);
	value = gebr_geoxml_program_parameter_get_string_value(program, FALSE);
	type_element = __gebr_geoxml_parameter_get_type_element(parameter);

	gdome_n_unref(gdome_el_removeChild((GdomeElement *) parameter, (GdomeNode *) type_element, &exception), &exception);
	gdome_el_unref(__gebr_geoxml_parameter_insert_type(parameter, GEBR_GEOXML_PARAMETER_TYPE_REFERENCE), &exception);
	gebr_geoxml_program_parameter_set_string_value(program, TRUE, default_value->str);
	gebr_geoxml_program_parameter_set_string_value(program, FALSE, value->str);

	gdome_el_unref(type_element, &exception);
	g_string_free(default_value, TRUE);
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

	gdome_n_unref(gdome_el_removeChild((GdomeElement *) parameter, (GdomeNode *) type_element, &exception), &exception);
	gdome_el_unref(__gebr_geoxml_parameter_insert_type(parameter, GEBR_GEOXML_PARAMETER_TYPE_REFERENCE), &exception);
	gdome_el_unref(type_element, &exception);
	gebr_geoxml_program_parameter_set_string_value(program, TRUE, default_value->str);

	g_string_free(default_value, TRUE);
}

GdomeElement *
__gebr_geoxml_parameter_insert_type(GebrGeoXmlParameter * parameter,
				    GebrGeoXmlParameterType type)
{
	GdomeElement *type_element;

	type_element = __gebr_geoxml_insert_new_element((GdomeElement *) parameter, parameter_type_to_str[type], NULL);
	if (type == GEBR_GEOXML_PARAMETER_TYPE_GROUP)
	{
		GdomeElement * template;
		template = __gebr_geoxml_insert_new_element(type_element, "template-instance", NULL);
		gebr_geoxml_object_unref(__gebr_geoxml_parameters_append_new(template));
		gebr_geoxml_object_unref(__gebr_geoxml_parameters_append_new(type_element));
		gebr_geoxml_parameter_group_set_is_instanciable((GebrGeoXmlParameterGroup *)parameter, FALSE);
		gebr_geoxml_parameter_group_set_expand((GebrGeoXmlParameterGroup *)parameter, FALSE);
		gebr_geoxml_object_unref(template);
	} 
	else
	{
		GdomeElement *property_element;
		GdomeElement *value_element;
		GdomeElement *default_value_element;

		property_element = __gebr_geoxml_insert_new_element(type_element, "property", NULL);
		gebr_geoxml_object_unref(__gebr_geoxml_insert_new_element(property_element, "keyword", NULL));
		value_element = __gebr_geoxml_insert_new_element(property_element, "value", NULL);
		default_value_element = __gebr_geoxml_insert_new_element(property_element, "default", NULL);
		__gebr_geoxml_set_attr_value(property_element, "required", "no");
		gdome_el_unref(property_element, &exception);

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

		gdome_el_unref(value_element, &exception);
		gdome_el_unref(default_value_element, &exception);

	}

	return type_element;
}

GSList *__gebr_geoxml_parameter_get_referencee_list(GebrGeoXmlParameter * parameter)
{
	gint index;
	GSList * list = NULL;
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
	GdomeNode *parent1, *parent2, *parent;
	parent1 = gdome_el_parentNode((GdomeElement*)parameter, &exception);
	parent2 = gdome_el_parentNode((GdomeElement*)parent1, &exception);
	parent = gdome_el_parentNode((GdomeElement*)parent2, &exception);
	tagname = gdome_el_tagName((GdomeElement*)parent, &exception);

	gdome_n_unref(parent1, &exception);
	gdome_n_unref(parent2, &exception);

	g_return_val_if_fail("parameter should be inside a template"
			     && strcmp(tagname->str, "group") == 0, NULL);
	gdome_str_unref(tagname);
	parent2 = gdome_el_parentNode((GdomeElement*)parent, &exception);
	gdome_n_unref(parent, &exception);
	parent = parent2;

	index = gebr_geoxml_sequence_get_index(GEBR_GEOXML_SEQUENCE(parameter));

	/* Iterate in all instances, inserting the index-eth parameter into the list
	 */
	gebr_geoxml_parameter_group_get_instance(GEBR_GEOXML_PARAMETER_GROUP(parent),
						 &instance, 0);
	while (instance) {
		GebrGeoXmlSequence * param;

		gebr_geoxml_parameters_get_parameter(GEBR_GEOXML_PARAMETERS(instance),
						     &param, index);
		gebr_geoxml_object_ref(param);
		list = g_slist_prepend(list, param);
		gebr_geoxml_sequence_next(&instance);
	}
	gdome_n_unref(parent, &exception);
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

GebrGeoXmlProgram *
gebr_geoxml_parameter_get_program(GebrGeoXmlParameter * parameter)
{
	if (parameter == NULL)
		return NULL;

	gebr_geoxml_object_ref(parameter);
	GdomeNode *element = (GdomeNode*)parameter;
	while (1) {
		GdomeNode *aux = element;
		element = gdome_n_parentNode(aux, &exception);
		gdome_n_unref(aux, &exception);
		GdomeDOMString *name = gdome_n_nodeName(element, &exception);

		if (!name) {
			gdome_n_unref(element, &exception);
			return NULL;
		}

		if (g_strcmp0(name->str, "program") == 0) {
			gdome_str_unref(name);
			break;
		}

		gdome_str_unref(name);
	}

	return (GebrGeoXmlProgram *) element;
}


gboolean gebr_geoxml_parameter_set_type(GebrGeoXmlParameter * parameter, GebrGeoXmlParameterType type)
{
	g_return_val_if_fail(parameter != NULL, FALSE);
	g_return_val_if_fail(type != GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN, FALSE);
	g_return_val_if_fail(type != GEBR_GEOXML_PARAMETER_TYPE_REFERENCE, FALSE);

	/* backup and remove old type */
	GdomeElement *type_element = __gebr_geoxml_parameter_get_type_element(parameter);
	GdomeElement *first_element = __gebr_geoxml_get_first_element(type_element, "property");
	GdomeNode *property = gdome_el_cloneNode(first_element, TRUE, &exception);
	gdome_n_unref(gdome_n_removeChild((GdomeNode*)parameter, (GdomeNode*)type_element, &exception), &exception);
	gdome_el_unref(type_element, &exception);
	gdome_el_unref(first_element, &exception);

	type_element = __gebr_geoxml_parameter_insert_type(parameter, type);
	first_element = __gebr_geoxml_get_first_element(type_element, "property");
	/* reinsert data */
	gdome_n_unref(gdome_el_removeChild(type_element, (GdomeNode*) first_element, &exception), &exception);
	GdomeElement * before = __gebr_geoxml_get_first_element(type_element, "property");
	gdome_n_unref(gdome_el_insertBefore_protected(type_element, property, (GdomeNode*)before, &exception), &exception);

	gdome_n_unref(property, &exception);
	gdome_el_unref(before, &exception);
	gdome_el_unref(type_element, &exception);
	gdome_el_unref(first_element, &exception);
	return TRUE;
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

	return _(type_names[__gebr_geoxml_parameter_get_type(parameter, TRUE)]);
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
	GdomeNode *parent;
	GdomeNode *grandpa;
	GdomeNode *group;
	GdomeDOMString *node_name;
	GebrGeoXmlSequence *referencee;
	GebrGeoXmlParameters *template;

	if (!gebr_geoxml_parameter_get_is_reference(parameter_reference))
		return NULL;

	parent = gdome_el_parentNode((GdomeElement*)parameter_reference, &exception);
	grandpa = gdome_n_parentNode(parent, &exception);
	node_name = gdome_n_nodeName(grandpa, &exception);

	if (g_strcmp0(node_name->str, "group") != 0) {
		gdome_n_unref(parent, &exception);
		gdome_n_unref(grandpa, &exception);
		gdome_str_unref(node_name);
		g_return_val_if_reached(NULL);
	}

	gdome_str_unref(node_name);

	group = gdome_n_parentNode(grandpa, &exception);
	template = gebr_geoxml_parameter_group_get_template(GEBR_GEOXML_PARAMETER_GROUP(group));

	if (!template) {
		gdome_n_unref(parent, &exception);
		gdome_n_unref(grandpa, &exception);
		gdome_n_unref(group, &exception);
		g_return_val_if_reached(NULL);
	}

	index = gebr_geoxml_sequence_get_index(GEBR_GEOXML_SEQUENCE(parameter_reference));
	gebr_geoxml_parameters_get_parameter(template, &referencee, index);

	gdome_n_unref(parent, &exception);
	gdome_n_unref(grandpa, &exception);
	gdome_n_unref(group, &exception);
	gebr_geoxml_object_unref(template);

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

gchar *gebr_geoxml_parameter_get_label(GebrGeoXmlParameter * parameter)
{
	GebrGeoXmlParameter *template;

	g_return_val_if_fail (parameter != NULL, NULL);

	if (gebr_geoxml_parameter_get_is_reference(parameter))
		template = gebr_geoxml_parameter_get_referencee(parameter);
	else {
		template = parameter;
		gebr_geoxml_object_ref(template);
	}
	gchar *label = __gebr_geoxml_get_tag_value((GdomeElement *) template, "label");
	gebr_geoxml_object_unref(template);
	return label;
}

gboolean gebr_geoxml_parameter_get_is_in_group(GebrGeoXmlParameter * parameter)
{
	if (parameter == NULL)
		return FALSE;
	GebrGeoXmlParameters *params = gebr_geoxml_parameter_get_parameters(parameter);
	gboolean retval = gebr_geoxml_parameters_get_is_in_group(params);
	gebr_geoxml_object_unref(params);
	return retval;
}

GebrGeoXmlParameterGroup *gebr_geoxml_parameter_get_group(GebrGeoXmlParameter * parameter)
{
	if (parameter == NULL)
		return NULL;
	GebrGeoXmlParameters *params = gebr_geoxml_parameter_get_parameters(parameter);
	GebrGeoXmlParameterGroup *group =  gebr_geoxml_parameters_get_group(params);
	gebr_geoxml_object_unref(params);
	return group;
}

gboolean gebr_geoxml_parameter_is_dict_param(GebrGeoXmlParameter *parameter)
{
	GdomeNode *grandpa, *parent;
	GdomeDOMString *name;

	parent = gdome_n_parentNode((GdomeNode*)parameter, &exception);
	grandpa = gdome_n_parentNode(parent, &exception);
	name = gdome_el_nodeName((GdomeElement*)grandpa, &exception);
	gboolean is_dict = g_strcmp0(name->str, "dict") == 0;

	gdome_n_unref(parent, &exception);
	gdome_n_unref(grandpa, &exception);
	gdome_str_unref(name);
	return is_dict;
}
GebrGeoXmlDocumentType gebr_geoxml_parameter_get_scope(GebrGeoXmlParameter *parameter)
{
	GebrGeoXmlDocument *doc;
	GebrGeoXmlDocumentType scope;

	g_return_val_if_fail(parameter != NULL, GEBR_GEOXML_DOCUMENT_TYPE_UNKNOWN);

	doc = gebr_geoxml_object_get_owner_document(GEBR_GEOXML_OBJECT(parameter));

	scope = gebr_geoxml_document_get_type(doc);
	gebr_geoxml_object_unref(doc);
	return scope;
}

gboolean
gebr_geoxml_parameter_type_is_compatible(GebrGeoXmlParameterType base,
					 GebrGeoXmlParameterType type)
{
	switch (base) {
	case GEBR_GEOXML_PARAMETER_TYPE_FILE:
	case GEBR_GEOXML_PARAMETER_TYPE_STRING:
		if (type == GEBR_GEOXML_PARAMETER_TYPE_STRING
		    || type == GEBR_GEOXML_PARAMETER_TYPE_INT
		    || type == GEBR_GEOXML_PARAMETER_TYPE_FLOAT
		    || type == GEBR_GEOXML_PARAMETER_TYPE_RANGE)
			return TRUE;
		else
			return FALSE;
	case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
		if (type == GEBR_GEOXML_PARAMETER_TYPE_FLOAT
		    || type == GEBR_GEOXML_PARAMETER_TYPE_INT)
			return TRUE;
		else
			return FALSE;
	case GEBR_GEOXML_PARAMETER_TYPE_INT:
		if (type == GEBR_GEOXML_PARAMETER_TYPE_INT
		    || type == GEBR_GEOXML_PARAMETER_TYPE_FLOAT)
			return TRUE;
		else
			return FALSE;
	case GEBR_GEOXML_PARAMETER_TYPE_RANGE:
		if (type == GEBR_GEOXML_PARAMETER_TYPE_RANGE
		    || type == GEBR_GEOXML_PARAMETER_TYPE_INT
		    || type == GEBR_GEOXML_PARAMETER_TYPE_FLOAT)
			return TRUE;
		else
			return FALSE;
	case GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN:
	case GEBR_GEOXML_PARAMETER_TYPE_FLAG:
	case GEBR_GEOXML_PARAMETER_TYPE_ENUM:
	case GEBR_GEOXML_PARAMETER_TYPE_GROUP:
	case GEBR_GEOXML_PARAMETER_TYPE_REFERENCE:
		return FALSE;
	}

	return FALSE;
}
