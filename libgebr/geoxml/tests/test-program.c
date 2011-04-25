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
#include <stdlib.h>
#include <glib.h>
#include <glib/gprintf.h>
#include "error.h"

#include "../object.h"


static void callback (GebrGeoXmlObject *parameter, GString *buffer)
{
	GebrGeoXmlProgramParameter *prog;
	GString *string_value;

	prog = GEBR_GEOXML_PROGRAM_PARAMETER(parameter);
	string_value = gebr_geoxml_program_parameter_get_string_value(prog, TRUE);

	g_string_append_printf(buffer, "%s=%s,", gebr_geoxml_program_parameter_get_keyword(prog), string_value->str);
	g_string_free(string_value, TRUE);
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

	GString *buffer = g_string_new(NULL);

	gebr_geoxml_program_foreach_parameter(program,(GebrGeoXmlCallback)callback, buffer);
	g_assert_cmpstr(buffer->str, ==, "Test keyword 1=11,Test keyword 2=22,Test keyword 3=33,");
	g_string_free(buffer, TRUE);

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

void test_gebr_geoxml_program_get_parameters(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	GebrGeoXmlProgram *program = gebr_geoxml_flow_append_program(flow);
	GebrGeoXmlParameters *parameters_list;

	parameters_list = gebr_geoxml_program_get_parameters(NULL);
	g_assert(parameters_list == NULL);

	parameters_list = gebr_geoxml_program_get_parameters(program);
	g_assert(parameters_list != NULL);
}

void test_gebr_geoxml_program_count_parameters(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	GebrGeoXmlProgram *program = gebr_geoxml_flow_append_program(flow);
	GebrGeoXmlParameters *parameters_list = gebr_geoxml_program_get_parameters(program);
	GebrGeoXmlParameter *parameter;
	int i;
	gsize count;

	count = gebr_geoxml_program_count_parameters(program);
	g_assert_cmpint(count, ==, 0);

	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_INT);
	count = gebr_geoxml_program_count_parameters(program);
	g_assert_cmpint(count, ==, 1);

	// Test inclusion of some parameters
	for(i = 1; i <= 5; i++){
		parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_INT);
		count = gebr_geoxml_program_count_parameters(program);
		g_assert_cmpint(count, ==, i+1);
	}
}

void test_gebr_geoxml_program_get_and_set_stdin(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	GebrGeoXmlProgram *program = gebr_geoxml_flow_append_program(flow);
	gboolean test;

	// Should do nothing, and not crash
	gebr_geoxml_program_set_stdin(NULL,TRUE);

	test = gebr_geoxml_program_get_stdin(NULL);
	g_assert_cmpint(test, ==, FALSE);

	gebr_geoxml_program_set_stdin(program,FALSE);
	test = gebr_geoxml_program_get_stdin(program);
	g_assert_cmpint(test, ==, FALSE);

	gebr_geoxml_program_set_stdin(program,TRUE);
	test = gebr_geoxml_program_get_stdin(program);
	g_assert_cmpint(test, ==, TRUE);
}

void test_gebr_geoxml_program_get_and_set_stdout(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	GebrGeoXmlProgram *program = gebr_geoxml_flow_append_program(flow);
	gboolean test;

	// Should do nothing, and not crash
	gebr_geoxml_program_set_stdout(NULL,TRUE);

	test = gebr_geoxml_program_get_stdout(NULL);
	g_assert_cmpint(test, ==, FALSE);

	gebr_geoxml_program_set_stdout(program,FALSE);
	test = gebr_geoxml_program_get_stdout(program);
	g_assert_cmpint(test, ==, FALSE);

	gebr_geoxml_program_set_stdout(program,TRUE);
	test = gebr_geoxml_program_get_stdout(program);
	g_assert_cmpint(test, ==, TRUE);
}

void test_gebr_geoxml_program_get_and_set_stderr(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	GebrGeoXmlProgram *program = gebr_geoxml_flow_append_program(flow);
	gboolean test;

	// Should do nothing, and not crash
	gebr_geoxml_program_set_stderr(NULL,TRUE);

	test = gebr_geoxml_program_get_stderr(NULL);
	g_assert_cmpint(test, ==, FALSE);

	gebr_geoxml_program_set_stderr(program,FALSE);
	test = gebr_geoxml_program_get_stderr(program);
	g_assert_cmpint(test, ==, FALSE);

	gebr_geoxml_program_set_stderr(program,TRUE);
	test = gebr_geoxml_program_get_stderr(program);
	g_assert_cmpint(test, ==, TRUE);
}

void test_gebr_geoxml_program_get_and_set_status(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	GebrGeoXmlProgram *program = gebr_geoxml_flow_append_program(flow);
	GebrGeoXmlProgramStatus status;

	status = gebr_geoxml_program_get_status(NULL);
	g_assert_cmpint(status, ==, GEBR_GEOXML_PROGRAM_STATUS_UNKNOWN);

	gebr_geoxml_program_set_status(program,GEBR_GEOXML_PROGRAM_STATUS_UNKNOWN);
	status = gebr_geoxml_program_get_status(program);
	g_assert_cmpint(status, ==, GEBR_GEOXML_PROGRAM_STATUS_UNKNOWN);

	gebr_geoxml_program_set_status(program,GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED);
	status = gebr_geoxml_program_get_status(program);
	g_assert_cmpint(status, ==, GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED);

	gebr_geoxml_program_set_status(program,GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED);
	status = gebr_geoxml_program_get_status(program);
	g_assert_cmpint(status, ==, GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED);

	gebr_geoxml_program_set_status(program,GEBR_GEOXML_PROGRAM_STATUS_DISABLED);
	status = gebr_geoxml_program_get_status(program);
	g_assert_cmpint(status, ==, GEBR_GEOXML_PROGRAM_STATUS_DISABLED);
}

void test_gebr_geoxml_program_get_and_set_binary(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	GebrGeoXmlProgram *program = gebr_geoxml_flow_append_program(flow);
	const gchar *program_binary;

	// Should do nothing, and not crash
	gebr_geoxml_program_set_binary(NULL,"101010101010");
	gebr_geoxml_program_set_binary(program,NULL);

	program_binary = gebr_geoxml_program_get_binary(NULL);
	g_assert(program_binary == NULL);

	gebr_geoxml_program_set_binary(program,"10100111001");
	program_binary = gebr_geoxml_program_get_binary(program);
	g_assert_cmpstr(program_binary, ==, "10100111001");

	gebr_geoxml_program_set_binary(program,"00000000000");
	program_binary = gebr_geoxml_program_get_binary(program);
	g_assert_cmpstr(program_binary, ==, "00000000000");
}

void test_gebr_geoxml_program_set_description(void)
{

	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	GebrGeoXmlProgram *program = gebr_geoxml_flow_append_program(flow);
	const gchar *program_description;

	// Should do nothing, and not crash
	gebr_geoxml_program_set_description(NULL,"Description goes here");
	gebr_geoxml_program_set_description(program,NULL);

	program_description = gebr_geoxml_program_get_description(NULL);
	g_assert(program_description == NULL);

	gebr_geoxml_program_set_description(program,"Description goes here");
	program_description = gebr_geoxml_program_get_description(program);
	g_assert_cmpstr(program_description, ==, "Description goes here");

	gebr_geoxml_program_set_description(program,"Change on description goes here");
	program_description = gebr_geoxml_program_get_description(program);
	g_assert_cmpstr(program_description, ==, "Change on description goes here");
}

void test_gebr_geoxml_program_set_help(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	GebrGeoXmlProgram *program = gebr_geoxml_flow_append_program(flow);
	const gchar *program_help;

	// Should do nothing, and not crash
	gebr_geoxml_program_set_help(NULL,"Help goes here");
	gebr_geoxml_program_set_help(program,NULL);

	program_help = gebr_geoxml_program_get_help(NULL);
	g_assert(program_help == NULL);

	gebr_geoxml_program_set_help(program,"Help goes here");
	program_help = gebr_geoxml_program_get_help(program);
	g_assert_cmpstr(program_help, ==, "Help goes here");

	gebr_geoxml_program_set_help(program,"Change on help goes here");
	program_help= gebr_geoxml_program_get_help(program);
	g_assert_cmpstr(program_help, ==, "Change on help goes here");
}

void test_gebr_geoxml_program_error_id(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	GebrGeoXmlProgram *program = gebr_geoxml_flow_append_program(flow);
	GebrGeoXmlProgramError errorid;

	g_assert(gebr_geoxml_program_get_error_id(program, &errorid) == FALSE);

	gebr_geoxml_program_set_error_id(program, FALSE, GEBR_GEOXML_PROGRAM_ERROR_REQ_UNFILL);
	g_assert(gebr_geoxml_program_get_error_id(program, &errorid) == TRUE);
	g_assert_cmpint(errorid, ==, GEBR_GEOXML_PROGRAM_ERROR_REQ_UNFILL);

	gebr_geoxml_program_set_error_id(program, TRUE, 0);
	g_assert(gebr_geoxml_program_get_error_id(program, &errorid) == FALSE);
}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/libgebr/geoxml/program/foreach_parameter", test_gebr_geoxml_program_foreach_parameter);
	g_test_add_func("/libgebr/geoxml/program/is_var_used", test_gebr_geoxml_program_is_var_used);
	g_test_add_func("/libgebr/geoxml/program/get_program_flow", test_gebr_geoxml_program_flow);
	g_test_add_func("/libgebr/geoxml/program/get_parameters", test_gebr_geoxml_program_get_parameters);
	g_test_add_func("/libgebr/geoxml/program/count_parameters", test_gebr_geoxml_program_count_parameters);
	g_test_add_func("/libgebr/geoxml/program/get_and_set_stdin", test_gebr_geoxml_program_get_and_set_stdin);
	g_test_add_func("/libgebr/geoxml/program/get_and_set_stdout", test_gebr_geoxml_program_get_and_set_stdout);
	g_test_add_func("/libgebr/geoxml/program/get_and_set_stderr", test_gebr_geoxml_program_get_and_set_stderr);
	g_test_add_func("/libgebr/geoxml/program/get_and_set_status", test_gebr_geoxml_program_get_and_set_status);
	g_test_add_func("/libgebr/geoxml/program/get_and_set_binary", test_gebr_geoxml_program_get_and_set_binary);
	g_test_add_func("/libgebr/geoxml/program/get_and_set_description", test_gebr_geoxml_program_set_description);
	g_test_add_func("/libgebr/geoxml/program/get_and_set_help", test_gebr_geoxml_program_set_help);
	g_test_add_func("/libgebr/geoxml/program/get_and_set_help", test_gebr_geoxml_program_set_help);
	g_test_add_func("/libgebr/geoxml/program/error_id", test_gebr_geoxml_program_error_id);

	return g_test_run();
}
