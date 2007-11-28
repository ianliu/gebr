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

#include <glib.h>
#include <stdlib.h>
#include <gdome.h>

#include "program.h"
#include "xml.h"
#include "error.h"
#include "types.h"
#include "sequence.h"

/*
 * internal structures and funcionts
 */

struct geoxml_program {
	GdomeElement * element;
};

const char * parameter_type_to_str[] = {
	"string", "int", "file",
	"flag", "float", "range"
};

const int parameter_type_to_str_len = 6;

GeoXmlProgramParameter *
__geoxml_program_new_parameter(GeoXmlProgram * program, GdomeElement * before, enum GEOXML_PARAMETERTYPE parameter_type)
{
	GdomeElement *	program_element;
	GdomeElement *	parameter_element;
	gchar *		tag_name;

	program_element   = (GdomeElement*)program;
	parameter_element = __geoxml_new_element(__geoxml_get_first_element((GdomeElement*)program, "parameters"),
		before, parameter_type_to_str[parameter_type]);
	tag_name = (parameter_type != GEOXML_PARAMETERTYPE_FLAG)
		? "value" : "state";

	/* elements/attibutes */
	if (parameter_type != GEOXML_PARAMETERTYPE_FLAG)
		geoxml_program_parameter_set_required((GeoXmlProgramParameter*)parameter_element, FALSE);
	__geoxml_new_element(parameter_element, NULL, "keyword");
	__geoxml_new_element(parameter_element, NULL, "label");
	__geoxml_new_element(parameter_element, NULL, tag_name);
	if (parameter_type == GEOXML_PARAMETERTYPE_FILE)
		geoxml_program_parameter_set_file_be_directory((GeoXmlProgramParameter*)parameter_element, FALSE);
	else if (parameter_type == GEOXML_PARAMETERTYPE_FLAG)
		geoxml_program_parameter_set_flag_default((GeoXmlProgramParameter*)parameter_element, FALSE);
	else if (parameter_type == GEOXML_PARAMETERTYPE_RANGE)
		geoxml_program_parameter_set_range_properties((GeoXmlProgramParameter*)parameter_element, "", "", "");

	return (GeoXmlProgramParameter*)parameter_element;
}

/*
 * library functions.
 */

GeoXmlFlow *
geoxml_program_flow(GeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return (GeoXmlFlow*)gdome_n_parentNode((GdomeNode*)program, &exception);
}

GeoXmlProgramParameter *
geoxml_program_new_parameter(GeoXmlProgram * program, enum GEOXML_PARAMETERTYPE parameter_type)
{
	if (program == NULL)
		return NULL;
	return __geoxml_program_new_parameter(program, NULL, parameter_type);
}

GeoXmlProgramParameter *
geoxml_program_get_first_parameter(GeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return (GeoXmlProgramParameter *)__geoxml_get_first_element(
		__geoxml_get_first_element((GdomeElement*)program, "parameters"), "*");
}

glong
geoxml_program_get_parameters_number(GeoXmlProgram * program)
{
	if (program == NULL)
		return -1;

	GdomeElement*	parameters_element;
	gint		i;
	gint		parameters_number = 0;

	parameters_element = __geoxml_get_first_element((GdomeElement*)program, "parameters");

	for (i = 0; i < parameter_type_to_str_len; ++i)
		parameters_number += __geoxml_get_elements_number(parameters_element, parameter_type_to_str[i]);

	return parameters_number;
}

void
geoxml_program_set_stdin(GeoXmlProgram * program, const gboolean enable)
{
	if (program == NULL)
		return;
	__geoxml_set_attr_value((GdomeElement*)program, "stdin", (enable == TRUE ? "yes" : "no"));
}

void
geoxml_program_set_stdout(GeoXmlProgram * program, const gboolean enable)
{
	if (program == NULL)
		return;
	__geoxml_set_attr_value((GdomeElement*)program, "stdout", (enable == TRUE ? "yes" : "no"));
}

void
geoxml_program_set_stderr(GeoXmlProgram * program, const gboolean enable)
{
	if (program == NULL)
		return;
	__geoxml_set_attr_value((GdomeElement*)program, "stderr", (enable == TRUE ? "yes" : "no"));
}

void
geoxml_program_set_status(GeoXmlProgram * program, const gchar * status)
{
	if (program == NULL)
		return;
	__geoxml_set_attr_value((GdomeElement*)program, "status", status);
}

void
geoxml_program_set_title(GeoXmlProgram * program, const gchar * title)
{
	if (program == NULL || title == NULL)
		return;
	__geoxml_set_tag_value((GdomeElement*)program, "title", title, __geoxml_create_TextNode);
}

void
geoxml_program_set_menu(GeoXmlProgram * program, const gchar * menu, gulong index)
{
	if (program == NULL || menu == NULL)
		return;

	gchar * tmp;

	__geoxml_set_tag_value((GdomeElement*)program, "menu", menu, __geoxml_create_TextNode);
	tmp = g_strdup_printf("%ld", index);
	__geoxml_set_attr_value(
		__geoxml_get_first_element((GdomeElement*)program, "menu"), "index", tmp);

	g_free(tmp);
}

void
geoxml_program_set_binary(GeoXmlProgram * program, const gchar * binary)
{
	if (program == NULL || binary == NULL)
		return;
	__geoxml_set_tag_value((GdomeElement*)program, "binary", binary, __geoxml_create_TextNode);
}

void
geoxml_program_set_description(GeoXmlProgram * program, const gchar * description)
{
	if (program == NULL || description == NULL)
		return;
	__geoxml_set_tag_value((GdomeElement*)program, "description", description, __geoxml_create_TextNode);
}

void
geoxml_program_set_help(GeoXmlProgram * program, const gchar * help)
{
	if (program == NULL || help == NULL)
		return;
	__geoxml_set_tag_value((GdomeElement*)program, "help", help, __geoxml_create_CDATASection);
}

gboolean
geoxml_program_get_stdin(GeoXmlProgram * program)
{
	if (program == NULL)
		return FALSE;
	return (!g_ascii_strcasecmp(__geoxml_get_attr_value((GdomeElement*)program, "stdin"), "yes"))
		? TRUE : FALSE;
}

gboolean
geoxml_program_get_stdout(GeoXmlProgram * program)
{
	if (program == NULL)
		return FALSE;
	return (!g_ascii_strcasecmp(__geoxml_get_attr_value((GdomeElement*)program, "stdout"), "yes"))
		? TRUE : FALSE;
}

gboolean
geoxml_program_get_stderr(GeoXmlProgram * program)
{
	if (program == NULL)
		return FALSE;
	return (!g_ascii_strcasecmp(__geoxml_get_attr_value((GdomeElement*)program, "stderr"), "yes"))
		? TRUE : FALSE;
}

const gchar *
geoxml_program_get_status(GeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return __geoxml_get_attr_value((GdomeElement*)program, "status");
}

const gchar *
geoxml_program_get_title(GeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return __geoxml_get_tag_value((GdomeElement*)program, "title");
}

void
geoxml_program_get_menu(GeoXmlProgram * program, gchar ** menu, gulong * index)
{
	if (program == NULL)
		return;
	*menu = (gchar *)__geoxml_get_tag_value((GdomeElement*)program, "menu");
	*index = (gulong)atol(__geoxml_get_attr_value(
		__geoxml_get_first_element((GdomeElement*)program, "menu"), "index"));
}

const gchar *
geoxml_program_get_binary(GeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return __geoxml_get_tag_value((GdomeElement*)program, "binary");
}

const gchar *
geoxml_program_get_description(GeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return __geoxml_get_tag_value((GdomeElement*)program, "description");
}

const gchar *
geoxml_program_get_help(GeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return __geoxml_get_tag_value((GdomeElement*)program, "help");
}

void
geoxml_program_previous(GeoXmlProgram ** program)
{
	geoxml_sequence_previous((GeoXmlSequence**)program);
}

void
geoxml_program_next(GeoXmlProgram ** program)
{
	geoxml_sequence_next((GeoXmlSequence**)program);
}

void
geoxml_program_remove(GeoXmlProgram * program)
{
	geoxml_sequence_remove((GeoXmlSequence*)program);
}

void
geoxml_program_remove_parameter(GeoXmlProgram * program, GeoXmlProgramParameter * parameter)
{
	geoxml_sequence_remove((GeoXmlSequence*)parameter);
}
