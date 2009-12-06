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
__gebr_geoxml_parameter_get_type_element(GebrGeoXmlParameter * parameter, gboolean resolve_references)
{
	GdomeElement *		type_element;

	do {
		type_element = __gebr_geoxml_get_first_element((GdomeElement*)parameter, "label");
		type_element = __gebr_geoxml_next_element(type_element);
	} while (resolve_references && !strcmp(gdome_el_tagName(type_element, &exception)->str, "reference")
	&& ((parameter = gebr_geoxml_parameter_get_referencee(parameter)),1));

	return type_element;
}

enum GEBR_GEOXML_PARAMETER_TYPE
__gebr_geoxml_parameter_get_type(GebrGeoXmlParameter * parameter, gboolean resolve_references)
{
	GdomeDOMString *	tag_name;
	int			i;

	tag_name = gdome_el_tagName(__gebr_geoxml_parameter_get_type_element(parameter, resolve_references), &exception);
	for (i = 1; i <= parameter_type_to_str_len; ++i)
		if (!strcmp(parameter_type_to_str[i], tag_name->str))
			return (enum GEBR_GEOXML_PARAMETER_TYPE)i;

	return GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN;
}

void
__gebr_geoxml_parameter_set_be_reference(GebrGeoXmlParameter * parameter, GebrGeoXmlParameter * referencee)
{
	GdomeElement *			type_element;
	enum GEBR_GEOXML_PARAMETER_TYPE	type;

	type = __gebr_geoxml_parameter_get_type(parameter, FALSE);
	type_element = __gebr_geoxml_parameter_get_type_element(parameter, FALSE);
	if (type == GEBR_GEOXML_PARAMETER_TYPE_GROUP) {
		/* FIXME: handle errors */
		return;
	}
	if (type != GEBR_GEOXML_PARAMETER_TYPE_REFERENCE) {
		gdome_el_removeChild((GdomeElement*)parameter, (GdomeNode*)type_element, &exception);
		type_element = __gebr_geoxml_parameter_insert_type(parameter, GEBR_GEOXML_PARAMETER_TYPE_REFERENCE);
	}

	__gebr_geoxml_element_assign_reference_id(type_element, (GdomeElement*)referencee);
}

GdomeElement *
__gebr_geoxml_parameter_insert_type(GebrGeoXmlParameter * parameter, enum GEBR_GEOXML_PARAMETER_TYPE type)
{
	GdomeElement *	type_element;

	type_element = __gebr_geoxml_insert_new_element((GdomeElement*)parameter, parameter_type_to_str[type], NULL);
	if (type == GEBR_GEOXML_PARAMETER_TYPE_GROUP) {
		__gebr_geoxml_parameters_append_new(type_element);
		gebr_geoxml_parameter_group_set_is_instanciable((GebrGeoXmlParameterGroup*)type_element, TRUE);
		gebr_geoxml_parameter_group_set_expand((GebrGeoXmlParameterGroup*)type_element, TRUE);
	} else {
		GdomeElement *		property_element;
		GdomeElement *		value_element;
		GdomeElement *		default_value_element;

		property_element = __gebr_geoxml_insert_new_element(type_element, "property", NULL);
		__gebr_geoxml_insert_new_element(property_element, "keyword", NULL);
		value_element = __gebr_geoxml_insert_new_element(property_element, "value", NULL);
		default_value_element = __gebr_geoxml_insert_new_element(property_element, "default", NULL);
		__gebr_geoxml_set_attr_value(property_element, "required", "no");

		switch (type) {
		case GEBR_GEOXML_PARAMETER_TYPE_FILE:
			gebr_geoxml_program_parameter_set_file_be_directory(
				(GebrGeoXmlProgramParameter*)parameter, FALSE);
			break;
		case GEBR_GEOXML_PARAMETER_TYPE_RANGE:
			gebr_geoxml_program_parameter_set_range_properties((GebrGeoXmlProgramParameter*)parameter
				, "", "", "", "");
			break;
		case GEBR_GEOXML_PARAMETER_TYPE_FLAG:
			__gebr_geoxml_set_element_value(value_element, "off" ,__gebr_geoxml_create_TextNode);
			__gebr_geoxml_set_element_value(default_value_element, "off" ,__gebr_geoxml_create_TextNode);
			break;
		default:
			break;
		}
	}

	return type_element;
}

GSList *
__gebr_geoxml_parameter_get_referencee_list(GdomeElement * context, const gchar * id)
{
	GSList *		idref_list;
	GdomeDOMString *	string, * idref_string;
	GdomeNodeList *		node_list;
	gint			i, l;

	idref_list = NULL;
	idref_string = gdome_str_mkref("idref");
	string = gdome_str_mkref("reference");
	node_list = gdome_el_getElementsByTagName(context, string, &exception);

	l = gdome_nl_length(node_list, &exception);
	for (i = 0; i < l; ++i) {
		GdomeElement *	element;

		element = (GdomeElement*)gdome_nl_item(node_list, i, &exception);
		if (strcmp(gdome_el_getAttribute(element, idref_string, &exception)->str, id) == 0)
			idref_list = g_slist_prepend(idref_list, gdome_el_parentNode(element, &exception));
	}

	gdome_str_unref(string);
	gdome_nl_unref(node_list, &exception);
	gdome_str_unref(idref_string);

	idref_list = g_slist_reverse(idref_list);

	return idref_list;
}

/*
 * library functions.
 */

GebrGeoXmlParameters *
gebr_geoxml_parameter_get_parameters(GebrGeoXmlParameter * parameter)
{
	if (parameter == NULL)
		return NULL;
	return (GebrGeoXmlParameters*)gdome_n_parentNode((GdomeNode*)parameter, &exception);
}

gboolean
gebr_geoxml_parameter_set_type(GebrGeoXmlParameter * parameter, enum GEBR_GEOXML_PARAMETER_TYPE type)
{
	if (parameter == NULL)
		return FALSE;
	if (type == GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN || type == GEBR_GEOXML_PARAMETER_TYPE_REFERENCE)
		return FALSE;

	gdome_n_removeChild((GdomeNode*)parameter,
		(GdomeNode*)__gebr_geoxml_parameter_get_type_element(parameter, FALSE), &exception);
	__gebr_geoxml_parameter_insert_type(parameter, type);

	return TRUE;
}

int
gebr_geoxml_parameter_set_be_reference(GebrGeoXmlParameter * parameter, GebrGeoXmlParameter * reference)
{
	if (parameter == NULL || reference == NULL)
		return GEBR_GEOXML_RETV_NULL_PTR;
	if (parameter == reference)
		return GEBR_GEOXML_RETV_REFERENCE_TO_ITSELF;

	__gebr_geoxml_parameter_set_be_reference(parameter, reference);

	return GEBR_GEOXML_RETV_SUCCESS;
}

enum GEBR_GEOXML_PARAMETER_TYPE
gebr_geoxml_parameter_get_type(GebrGeoXmlParameter * parameter)
{
	if (parameter == NULL)
		return GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN;

	return __gebr_geoxml_parameter_get_type(parameter, TRUE);
}

const gchar *
gebr_geoxml_parameter_get_type_name(GebrGeoXmlParameter * parameter)
{
	if (parameter == NULL)
		return NULL;

	static const gchar * type_names [] = {
		NULL, N_("string"), N_("integer"), N_("file"), N_("flag"),
		N_("real number"), N_("range"), N_("enum"),
		N_("group"), N_("reference")
	};

	return type_names[__gebr_geoxml_parameter_get_type(parameter, TRUE)];
}

gboolean
gebr_geoxml_parameter_get_is_reference(GebrGeoXmlParameter * parameter)
{
	if (parameter == NULL)
		return FALSE;
	return __gebr_geoxml_parameter_get_type(parameter, FALSE) == GEBR_GEOXML_PARAMETER_TYPE_REFERENCE
		? TRUE : FALSE;
}

GSList *
gebr_geoxml_parameter_get_references_list(GebrGeoXmlParameter * parameter)
{
	if (parameter == NULL)
		return NULL;
	return __gebr_geoxml_parameter_get_referencee_list(
		gdome_doc_documentElement(gdome_el_ownerDocument((GdomeElement*)parameter, &exception), &exception),
		__gebr_geoxml_get_attr_value((GdomeElement*)parameter, "id"));
}

GebrGeoXmlParameter *
gebr_geoxml_parameter_get_referencee(GebrGeoXmlParameter * parameter_reference)
{
	if (!gebr_geoxml_parameter_get_is_reference(parameter_reference))
		return NULL;
	return (GebrGeoXmlParameter*)__gebr_geoxml_get_element_by_id((GdomeElement*)parameter_reference,
		__gebr_geoxml_get_attr_value(__gebr_geoxml_parameter_get_type_element(parameter_reference, FALSE), "idref"));
}

gboolean
gebr_geoxml_parameter_get_is_program_parameter(GebrGeoXmlParameter * parameter)
{
	if (parameter == NULL)
		return FALSE;
	return (gebr_geoxml_parameter_get_type(parameter) != GEBR_GEOXML_PARAMETER_TYPE_GROUP)
		? TRUE : FALSE;
}

void
gebr_geoxml_parameter_set_label(GebrGeoXmlParameter * parameter, const gchar * label)
{
	if (parameter == NULL || label == NULL)
		return;
	__gebr_geoxml_set_tag_value((GdomeElement*)parameter, "label", label, __gebr_geoxml_create_TextNode);
	if (gebr_geoxml_parameter_get_is_in_group(parameter)) {
		GebrGeoXmlParameterGroup *	parameter_group;
		GdomeElement *		reference_element;

		parameter_group = gebr_geoxml_parameter_get_group(parameter);
		__gebr_geoxml_foreach_element(reference_element,
		__gebr_geoxml_get_elements_by_idref((GdomeElement*)parameter_group,
		__gebr_geoxml_get_attr_value((GdomeElement*)parameter, "id"), TRUE))
			gebr_geoxml_parameter_set_label((GebrGeoXmlParameter*)gdome_el_parentNode(reference_element, &exception), label);
	}
}

const gchar *
gebr_geoxml_parameter_get_label(GebrGeoXmlParameter * parameter)
{
	if (parameter == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value((GdomeElement*)parameter, "label");
}

gboolean
gebr_geoxml_parameter_get_is_in_group(GebrGeoXmlParameter * parameter)
{
	if (parameter == NULL)
		return FALSE;
	return gebr_geoxml_parameters_get_is_in_group(gebr_geoxml_parameter_get_parameters(parameter));
}

GebrGeoXmlParameterGroup *
gebr_geoxml_parameter_get_group(GebrGeoXmlParameter * parameter)
{
	if (parameter == NULL)
		return NULL;
	return gebr_geoxml_parameters_get_group(gebr_geoxml_parameter_get_parameters(parameter));
}

void
gebr_geoxml_parameter_reset(GebrGeoXmlParameter * parameter, gboolean recursive)
{
	if (parameter == NULL)
		return;
	if (gebr_geoxml_parameter_get_type(parameter) == GEBR_GEOXML_PARAMETER_TYPE_GROUP) {
		GebrGeoXmlSequence *	instance;

		if (recursive == FALSE)
			return;

		/* call gebr_geoxml_parameter_reset on each child */
		gebr_geoxml_parameter_group_get_instance(GEBR_GEOXML_PARAMETER_GROUP(parameter), &instance, 0);
		for (; instance != NULL; gebr_geoxml_sequence_next(&instance))
			gebr_geoxml_parameters_reset(GEBR_GEOXML_PARAMETERS(instance), recursive);
	} else {
		__gebr_geoxml_program_parameter_set_all_value((GebrGeoXmlProgramParameter*)parameter, FALSE, "");
		__gebr_geoxml_program_parameter_set_all_value((GebrGeoXmlProgramParameter*)parameter, TRUE, "");
	}
}
