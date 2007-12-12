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

#include <glib.h>
#include <stdlib.h>
#include <gdome.h>

#include "program.h"
#include "parameters.h"
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

GeoXmlParameters *
geoxml_program_get_parameters(GeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return (GeoXmlParameters*)__geoxml_get_first_element((GdomeElement*)program, "parameters");
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

GeoXmlProgramParameter *
geoxml_program_new_parameter(GeoXmlProgram * program, enum GEOXML_PARAMETERTYPE type)
{
	return (GeoXmlProgramParameter*)geoxml_parameters_append_parameter((GeoXmlParameters *)
		__geoxml_get_first_element((GdomeElement*)program, "parameters"), type);
}

GeoXmlProgramParameter *
geoxml_program_get_first_parameter(GeoXmlProgram * program)
{
	return (GeoXmlProgramParameter*)geoxml_parameters_get_first_parameter((GeoXmlParameters *)
		__geoxml_get_first_element((GdomeElement*)program, "parameters"));
}

glong
geoxml_program_get_parameters_number(GeoXmlProgram * program)
{
	return geoxml_parameters_get_number((GeoXmlParameters *)
		__geoxml_get_first_element((GdomeElement*)program, "parameters"));
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
