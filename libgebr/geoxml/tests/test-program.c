/*   libgebr - GêBR Library
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

#include <stdio.h>
#include <glib.h>
#include <glib/gprintf.h>
#include "error.h"

#include "../object.h"


static void callback (GebrGeoXmlObject *parameter, const gchar **keyword)
{
	GebrGeoXmlProgramParameter *prog;
	GString *string_value;

	prog = GEBR_GEOXML_PROGRAM_PARAMETER(parameter);
	string_value = gebr_geoxml_program_parameter_get_string_value(prog, TRUE);

//	gebr_geoxml_program_parameter_get_value (prog, FALSE, &value, 0);
	*keyword = g_strdup_printf("%s,%s=%s", *keyword, gebr_geoxml_program_parameter_get_keyword(prog), string_value->str);
}

void test_gebr_geoxml_program_foreach_parameter(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	GebrGeoXmlProgram *program = gebr_geoxml_flow_append_program(flow);

	GebrGeoXmlParameters *parameters_list = gebr_geoxml_program_get_parameters(program);
	GebrGeoXmlParameter *parameter;

	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_STRING);
	gebr_geoxml_program_parameter_set_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), "Test keyword 1");
	gebr_geoxml_program_parameter_set_string_value(GEBR_GEOXML_PROGRAM_PARAMETER(parameter),TRUE,"11");
	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_STRING);
	gebr_geoxml_program_parameter_set_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), "Test keyword 2");
	gebr_geoxml_program_parameter_set_string_value(GEBR_GEOXML_PROGRAM_PARAMETER(parameter),TRUE,"22");
	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_STRING);
	gebr_geoxml_program_parameter_set_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), "Test keyword 3");
	gebr_geoxml_program_parameter_set_string_value(GEBR_GEOXML_PROGRAM_PARAMETER(parameter),TRUE,"33");

	gchar *keyword;

	gebr_geoxml_program_foreach_parameter(program,(GebrGeoXmlCallback)callback, &keyword);
	g_assert_cmpstr(keyword, ==, ",Test keyword 1=11,Test keyword 2=22,Test keyword 3=33");

}

void test_gebr_geoxml_program_is_var_used()
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	GebrGeoXmlProgram *program = gebr_geoxml_flow_append_program(flow);

	GebrGeoXmlParameters *parameters_list = gebr_geoxml_program_get_parameters(program);
	GebrGeoXmlParameter *parameter;

	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_INT);
	gebr_geoxml_program_parameter_set_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), "Test keyword 1");
	gebr_geoxml_program_parameter_set_string_value(GEBR_GEOXML_PROGRAM_PARAMETER(parameter),FALSE,"11");

	g_assert(gebr_geoxml_program_is_var_used(program, "var1") == FALSE);

	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_INT);
	gebr_geoxml_program_parameter_set_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), "Test keyword 2");
	gebr_geoxml_program_parameter_set_string_value(GEBR_GEOXML_PROGRAM_PARAMETER(parameter),FALSE,"12+var2");

	g_assert(gebr_geoxml_program_is_var_used(program, "var2") == TRUE);

}

void test_gebr_geoxml_program_flow(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	GebrGeoXmlProgram *program = gebr_geoxml_flow_append_program(flow);
	GebrGeoXmlFlow *flow2 = NULL;
	const gchar *title;

	// If program is NULL, an error occurs
	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		flow2 = gebr_geoxml_program_flow(NULL);
		exit(0);
	}
	g_test_trap_assert_failed();

	gebr_geoxml_document_set_title((GebrGeoXmlDocument*)flow, "test-title");
	title = gebr_geoxml_document_get_title((GebrGeoXmlDocument*)flow);
	g_assert_cmpstr(title, ==, "test-title");

	flow2 = gebr_geoxml_program_flow(program);
	title = gebr_geoxml_document_get_title((GebrGeoXmlDocument*)flow2);
	g_assert_cmpstr(title, ==, "test-title");

}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/libgebr/geoxml/program/foreach_parameter", test_gebr_geoxml_program_foreach_parameter);
	g_test_add_func("/libgebr/geoxml/program/is_var_used", test_gebr_geoxml_program_is_var_used);
	g_test_add_func("/libgebr/geoxml/program/get_program_flow", test_gebr_geoxml_program_flow);

	return g_test_run();
}
