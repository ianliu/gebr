/*   libgeoxml - An interface to describe seismic software in XML
 *   Copyright (C) 2007  Bráulio Barros de Oliveira (brauliobo@gmail.com)
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
#include "program_p.h"
#include "types.h"
#include "sequence.h"

/*
 * internal structures and funcionts
 */

struct geoxml_program_parameter {
	GdomeElement * element;
};

/*
 * library functions.
 */

GeoXmlProgram *
geoxml_program_parameter_program(GeoXmlProgramParameter * parameter)
{
	if (parameter == NULL)
		return NULL;
	return (GeoXmlProgram*)gdome_n_parentNode(
		gdome_n_parentNode((GdomeNode*)parameter, &exception), &exception);
}

void
geoxml_program_parameter_set_type(GeoXmlProgramParameter ** parameter, enum GEOXML_PARAMETERTYPE type)
{
	if (*parameter == NULL)
		return;

	GdomeElement *			program_element;
	GdomeElement *			parameters_element;
	GeoXmlProgramParameter *	old_parameter;

	old_parameter = *parameter;
	parameters_element = (GdomeElement*)gdome_el_parentNode((GdomeElement*)old_parameter, &exception);
	program_element = (GdomeElement*)gdome_el_parentNode((GdomeElement*)parameters_element, &exception);

	*parameter = __geoxml_program_new_parameter((GeoXmlProgram *)program_element, (GdomeElement*)old_parameter, type);
	gdome_el_insertBefore(parameters_element, (GdomeNode*)*parameter, (GdomeNode*)old_parameter, &exception);

	geoxml_program_parameter_set_keyword(*parameter, geoxml_program_parameter_get_keyword(old_parameter));
	geoxml_program_parameter_set_label(*parameter, geoxml_program_parameter_get_label(old_parameter));

	gdome_el_removeChild(parameters_element, (GdomeNode*)old_parameter, &exception);
}

enum GEOXML_PARAMETERTYPE
geoxml_program_parameter_get_type(GeoXmlProgramParameter * parameter)
{
	if (parameter == NULL)
		return GEOXML_PARAMETERTYPE_STRING;

	GdomeDOMString*		tag_name;
	int			i;
	GdomeException		exception;

	tag_name = gdome_el_tagName((GdomeElement*)parameter, &exception);

	for (i = 0; i < parameter_type_to_str_len; ++i)
		if (!g_ascii_strcasecmp(parameter_type_to_str[i], tag_name->str))
			return (enum GEOXML_PARAMETERTYPE)i;

	return GEOXML_PARAMETERTYPE_STRING;
}

void
geoxml_program_parameter_set_required(GeoXmlProgramParameter * parameter, gboolean required)
{
	if (parameter == NULL)
		return;
	if (geoxml_program_parameter_get_type(parameter) == GEOXML_PARAMETERTYPE_FLAG)
		return;
	__geoxml_set_attr_value((GdomeElement*)parameter, "required", (required == TRUE ? "yes" : "no"));
}

void
geoxml_program_parameter_set_keyword(GeoXmlProgramParameter * parameter, const gchar * keyword)
{
	if (parameter == NULL || keyword == NULL)
		return;
	__geoxml_set_tag_value((GdomeElement*)parameter, "keyword", keyword, __geoxml_create_TextNode);
}

void
geoxml_program_parameter_set_label(GeoXmlProgramParameter * parameter, const gchar * label)
{
	if (parameter == NULL || label == NULL)
		return;
	__geoxml_set_tag_value((GdomeElement*)parameter, "label", label, __geoxml_create_TextNode);
}

void
geoxml_program_parameter_set_default(GeoXmlProgramParameter * parameter, const gchar * value)
{
	if (parameter == NULL)
		return;

	gchar * tag_name = (geoxml_program_parameter_get_type(parameter) != GEOXML_PARAMETERTYPE_FLAG)
		? "value" : "state";

	__geoxml_set_attr_value(
		__geoxml_get_first_element((GdomeElement*)parameter, tag_name), "default", value);
}

void
geoxml_program_parameter_set_flag_default(GeoXmlProgramParameter * parameter, gboolean state)
{
	if (parameter == NULL)
		return;
	if (geoxml_program_parameter_get_type(parameter) != GEOXML_PARAMETERTYPE_FLAG)
		return;

	__geoxml_set_attr_value(
		__geoxml_get_first_element((GdomeElement*)parameter, "state"), "default",
		(state == TRUE ? "on" : "off"));
}

void
geoxml_program_parameter_set_value(GeoXmlProgramParameter * parameter, const gchar * value)
{
	if (parameter == NULL || value == NULL)
		return;

	gchar * tag_name = (geoxml_program_parameter_get_type(parameter) != GEOXML_PARAMETERTYPE_FLAG)
		? "value" : "state";

	__geoxml_set_tag_value((GdomeElement*)parameter, tag_name, value, __geoxml_create_TextNode);
}

void
geoxml_program_parameter_set_flag_state(GeoXmlProgramParameter * parameter, gboolean enabled)
{
	if (parameter == NULL)
		return;
	if (geoxml_program_parameter_get_type(parameter) != GEOXML_PARAMETERTYPE_FLAG)
		return;
	__geoxml_set_tag_value((GdomeElement*)parameter, "state", (enabled == TRUE ? "on" : "off"),
		__geoxml_create_TextNode);
}

void
geoxml_program_parameter_set_file_be_directory(GeoXmlProgramParameter * parameter, gboolean is_directory)
{
	if (parameter == NULL)
		return;
	if (geoxml_program_parameter_get_type(parameter) != GEOXML_PARAMETERTYPE_FILE)
		return;
	__geoxml_set_attr_value((GdomeElement*)parameter, "directory", (is_directory == TRUE ? "yes" : "no"));
}

void
geoxml_program_parameter_set_range_properties(GeoXmlProgramParameter * parameter,
		const gchar * min, const gchar * max, const gchar * inc)
{
	if (parameter == NULL)
		return;
	if (geoxml_program_parameter_get_type(parameter) != GEOXML_PARAMETERTYPE_RANGE)
		return;
	__geoxml_set_attr_value((GdomeElement*)parameter, "min", min);
	__geoxml_set_attr_value((GdomeElement*)parameter, "max", max);
	__geoxml_set_attr_value((GdomeElement*)parameter, "inc", inc);
}

gboolean
geoxml_program_parameter_get_required(GeoXmlProgramParameter * parameter)
{
	if (parameter == NULL)
		return FALSE;
	if (geoxml_program_parameter_get_type(parameter) == GEOXML_PARAMETERTYPE_FLAG)
		return FALSE;
	return (!g_ascii_strcasecmp(__geoxml_get_attr_value((GdomeElement*)parameter, "required"), "yes"))
		? TRUE : FALSE;
}

const gchar *
geoxml_program_parameter_get_keyword(GeoXmlProgramParameter * parameter)
{
	if (parameter == NULL)
		return NULL;
	return __geoxml_get_tag_value((GdomeElement*)parameter, "keyword");
}

const gchar *
geoxml_program_parameter_get_label(GeoXmlProgramParameter * parameter)
{
	if (parameter == NULL)
		return NULL;
	return __geoxml_get_tag_value((GdomeElement*)parameter, "label");
}

const gchar *
geoxml_program_parameter_get_default(GeoXmlProgramParameter * parameter)
{
	if (parameter == NULL)
		return NULL;

	GdomeElement *		element;
	GdomeDOMString *	string;
	GdomeException		exception;
	gboolean		ret;
	gchar *			tag_name;

	tag_name = (geoxml_program_parameter_get_type(parameter) == GEOXML_PARAMETERTYPE_FLAG) ? "state" : "value";
	element = __geoxml_get_first_element((GdomeElement*)parameter, tag_name);

	string = gdome_str_mkref("default");
	ret = (gboolean)gdome_el_hasAttribute(element, string, &exception);
	gdome_str_unref(string);

	if (ret == FALSE)
		return "";

	return __geoxml_get_attr_value(element, "default");
}

gboolean
geoxml_program_parameter_get_flag_default(GeoXmlProgramParameter * parameter)
{
	if (parameter == NULL)
		return FALSE;
	if (geoxml_program_parameter_get_type(parameter) != GEOXML_PARAMETERTYPE_FLAG)
		return FALSE;

	GdomeElement *		element;
	GdomeDOMString *	string;
	GdomeException		exception;
	gboolean		ret;

	element = __geoxml_get_first_element((GdomeElement*)parameter, "state");

	string = gdome_str_mkref("default");
	ret = (gboolean)gdome_el_hasAttribute(element, string, &exception);
	gdome_str_unref(string);

	if (ret == FALSE)
		return FALSE;

	return (!g_ascii_strcasecmp(__geoxml_get_attr_value(element, "default"), "on"))
		? TRUE : FALSE;
}

const gchar *
geoxml_program_parameter_get_value(GeoXmlProgramParameter * parameter)
{
	if (parameter == NULL)
		return NULL;
	return __geoxml_get_tag_value((GdomeElement*)parameter, "value");
}

gboolean
geoxml_program_parameter_get_flag_status(GeoXmlProgramParameter * parameter)
{
	if (parameter == NULL)
		return FALSE;
	if (geoxml_program_parameter_get_type(parameter) != GEOXML_PARAMETERTYPE_FLAG)
		return FALSE;
	return (!g_ascii_strcasecmp(__geoxml_get_tag_value((GdomeElement*)parameter, "state"), "on"))
		? TRUE : FALSE;
}

gboolean
geoxml_program_parameter_get_file_be_directory(GeoXmlProgramParameter * parameter)
{
	if (parameter == NULL)
		return FALSE;
	if (geoxml_program_parameter_get_type(parameter) != GEOXML_PARAMETERTYPE_FILE)
		return FALSE;
	return (!g_ascii_strcasecmp(__geoxml_get_attr_value((GdomeElement*)parameter, "directory"), "yes"))
		? TRUE : FALSE;
}

void
geoxml_program_parameter_get_range_properties(GeoXmlProgramParameter * parameter,
		gchar ** min, gchar ** max, gchar ** inc)
{
	if (parameter == NULL)
		return;
	if (geoxml_program_parameter_get_type(parameter) != GEOXML_PARAMETERTYPE_RANGE)
		return;
	*min = g_strdup(__geoxml_get_attr_value((GdomeElement*)parameter, "min"));
	*max = g_strdup(__geoxml_get_attr_value((GdomeElement*)parameter, "max"));
	*inc = g_strdup(__geoxml_get_attr_value((GdomeElement*)parameter, "inc"));
}

void
geoxml_program_parameter_previous(GeoXmlProgramParameter ** parameter)
{
	geoxml_sequence_previous((GeoXmlSequence**)parameter);
}

void
geoxml_program_parameter_next(GeoXmlProgramParameter ** parameter)
{
	geoxml_sequence_next((GeoXmlSequence**)parameter);
}

void
geoxml_program_parameter_remove(GeoXmlProgramParameter * parameter)
{
	geoxml_sequence_remove((GeoXmlSequence*)parameter);
}
