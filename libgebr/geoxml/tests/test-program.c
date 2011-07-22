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

#include <glib.h>
#include <glib/gprintf.h>
#include <stdio.h>
#include <stdlib.h>

#include <gebr-iexpr.h>
#include <gebr-dirs-priv.h>

#include "../object.h"
#include "../xml.h"
#include "document.h"
#include "flow.h"
#include "error.h"
#include "parameter.h"
#include "program.h"
#include "program-parameter.h"
#include "parameters.h"

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
	gebr_geoxml_program_parameter_set_string_value(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), TRUE, "11");

	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_STRING);
	gebr_geoxml_program_parameter_set_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), "Test keyword 2");
	gebr_geoxml_program_parameter_set_string_value(GEBR_GEOXML_PROGRAM_PARAMETER(parameter),TRUE,"22");

	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_STRING);
	gebr_geoxml_program_parameter_set_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), "Test keyword 3");
	gebr_geoxml_program_parameter_set_string_value(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), TRUE, "33");

	gchar *keyword = "Parameters: ";

	gebr_geoxml_program_foreach_parameter(program, (GebrGeoXmlCallback)callback, &keyword);
	g_assert_cmpstr(keyword, ==, "Parameters: ,Test keyword 1=11,Test keyword 2=22,Test keyword 3=33");
}

void test_gebr_geoxml_program_is_var_used()
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	GebrGeoXmlProgram *program = gebr_geoxml_flow_append_program(flow);

	GebrGeoXmlParameters *parameters_list = gebr_geoxml_program_get_parameters(program);
	GebrGeoXmlParameter *parameter;

	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_INT);
	gebr_geoxml_program_parameter_set_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), "Test keyword 1");
	gebr_geoxml_program_parameter_set_string_value(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), FALSE, "11");

	g_assert(gebr_geoxml_program_is_var_used(program, "var1") == FALSE);

	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_INT);
	gebr_geoxml_program_parameter_set_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), "Test keyword 2");
	gebr_geoxml_program_parameter_set_string_value(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), FALSE, "12+var2");

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

	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		count = gebr_geoxml_program_count_parameters(NULL);
		exit(0);
	}
	g_test_trap_assert_failed();

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
	gebr_geoxml_program_set_stdin(NULL, TRUE);

	test = gebr_geoxml_program_get_stdin(NULL);
	g_assert_cmpint(test, ==, FALSE);

	gebr_geoxml_program_set_stdin(program, FALSE);
	test = gebr_geoxml_program_get_stdin(program);
	g_assert_cmpint(test, ==, FALSE);

	gebr_geoxml_program_set_stdin(program, TRUE);
	test = gebr_geoxml_program_get_stdin(program);
	g_assert_cmpint(test, ==, TRUE);
}

void test_gebr_geoxml_program_get_and_set_stdout(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	GebrGeoXmlProgram *program = gebr_geoxml_flow_append_program(flow);
	gboolean test;

	// Should do nothing, and not crash
	gebr_geoxml_program_set_stdout(NULL, TRUE);

	test = gebr_geoxml_program_get_stdout(NULL);
	g_assert_cmpint(test, ==, FALSE);

	gebr_geoxml_program_set_stdout(program, FALSE);
	test = gebr_geoxml_program_get_stdout(program);
	g_assert_cmpint(test, ==, FALSE);

	gebr_geoxml_program_set_stdout(program, TRUE);
	test = gebr_geoxml_program_get_stdout(program);
	g_assert_cmpint(test, ==, TRUE);
}

void test_gebr_geoxml_program_get_and_set_stderr(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	GebrGeoXmlProgram *program = gebr_geoxml_flow_append_program(flow);
	gboolean test;

	// Should do nothing, and not crash
	gebr_geoxml_program_set_stderr(NULL, TRUE);

	test = gebr_geoxml_program_get_stderr(NULL);
	g_assert_cmpint(test, ==, FALSE);

	gebr_geoxml_program_set_stderr(program, FALSE);
	test = gebr_geoxml_program_get_stderr(program);
	g_assert_cmpint(test, ==, FALSE);

	gebr_geoxml_program_set_stderr(program, TRUE);
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

	gebr_geoxml_program_set_status(program, GEBR_GEOXML_PROGRAM_STATUS_UNKNOWN);
	status = gebr_geoxml_program_get_status(program);
	g_assert_cmpint(status, ==, GEBR_GEOXML_PROGRAM_STATUS_UNKNOWN);

	gebr_geoxml_program_set_status(program, GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED);
	status = gebr_geoxml_program_get_status(program);
	g_assert_cmpint(status, ==, GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED);

	gebr_geoxml_program_set_status(program, GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED);
	status = gebr_geoxml_program_get_status(program);
	g_assert_cmpint(status, ==, GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED);

	gebr_geoxml_program_set_status(program, GEBR_GEOXML_PROGRAM_STATUS_DISABLED);
	status = gebr_geoxml_program_get_status(program);
	g_assert_cmpint(status, ==, GEBR_GEOXML_PROGRAM_STATUS_DISABLED);
}

void test_gebr_geoxml_program_get_and_set_binary(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	GebrGeoXmlProgram *program = gebr_geoxml_flow_append_program(flow);
	const gchar *program_binary;

	// Should do nothing, and not crash
	gebr_geoxml_program_set_binary(NULL, "101010101010");
	gebr_geoxml_program_set_binary(program, NULL);

	program_binary = gebr_geoxml_program_get_binary(NULL);
	g_assert(program_binary == NULL);

	gebr_geoxml_program_set_binary(program, "10100111001");
	program_binary = gebr_geoxml_program_get_binary(program);
	g_assert_cmpstr(program_binary, ==, "10100111001");

	gebr_geoxml_program_set_binary(program, "00000000000");
	program_binary = gebr_geoxml_program_get_binary(program);
	g_assert_cmpstr(program_binary, ==, "00000000000");
}

void test_gebr_geoxml_program_set_description(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	GebrGeoXmlProgram *program = gebr_geoxml_flow_append_program(flow);
	const gchar *program_description;

	// Should do nothing, and not crash
	gebr_geoxml_program_set_description(NULL, "Description goes here");
	gebr_geoxml_program_set_description(program,NULL);

	program_description = gebr_geoxml_program_get_description(NULL);
	g_assert(program_description == NULL);

	gebr_geoxml_program_set_description(program, "Description goes here");
	program_description = gebr_geoxml_program_get_description(program);
	g_assert_cmpstr(program_description, ==, "Description goes here");

	gebr_geoxml_program_set_description(program, "Change on description goes here");
	program_description = gebr_geoxml_program_get_description(program);
	g_assert_cmpstr(program_description, ==, "Change on description goes here");
}

void test_gebr_geoxml_program_get_and_set_help(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	GebrGeoXmlProgram *program = gebr_geoxml_flow_append_program(flow);
	const gchar *program_help;

	// Should do nothing, and not crash
	gebr_geoxml_program_set_help(NULL, "Help goes here");
	gebr_geoxml_program_set_help(program, NULL);

	program_help = gebr_geoxml_program_get_help(NULL);
	g_assert(program_help == NULL);

	gebr_geoxml_program_set_help(program, "Help goes here");
	program_help = gebr_geoxml_program_get_help(program);
	g_assert_cmpstr(program_help, ==, "Help goes here");

	gebr_geoxml_program_set_help(program, "Change on help goes here");
	program_help= gebr_geoxml_program_get_help(program);
	g_assert_cmpstr(program_help, ==, "Change on help goes here");
}

void test_gebr_geoxml_program_get_and_set_version(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	GebrGeoXmlProgram *program = gebr_geoxml_flow_append_program(flow);
	const gchar *program_version;

	// Should do nothing, and not crash
	gebr_geoxml_program_set_version(NULL, "Version goes here");
	gebr_geoxml_program_set_version(program, NULL);

	program_version = gebr_geoxml_program_get_version(NULL);
	g_assert(program_version == NULL);

	gebr_geoxml_program_set_version(program, "Version goes here");
	program_version = gebr_geoxml_program_get_version(program);
	g_assert_cmpstr(program_version, ==, "Version goes here");

	gebr_geoxml_program_set_version(program, "Change on Version goes here");
	program_version = gebr_geoxml_program_get_version(program);
	g_assert_cmpstr(program_version, ==, "Change on Version goes here");
}

void test_gebr_geoxml_program_get_and_set_mpi(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	GebrGeoXmlProgram *program = gebr_geoxml_flow_append_program(flow);
	const gchar *program_mpi;

	// Should do nothing, and not crash
	gebr_geoxml_program_set_mpi(NULL," Program MPI goes here");
	gebr_geoxml_program_set_mpi(program, NULL);

	program_mpi = gebr_geoxml_program_get_mpi(NULL);
	g_assert(program_mpi == NULL);

	gebr_geoxml_program_set_mpi(program, "Program MPI goes here");
	program_mpi = gebr_geoxml_program_get_mpi(program);
	g_assert_cmpstr(program_mpi, ==, "Program MPI goes here");

	gebr_geoxml_program_set_mpi(program, "Change on MPI goes here");
	program_mpi = gebr_geoxml_program_get_mpi(program);
	g_assert_cmpstr(program_mpi, ==, "Change on MPI goes here");
}

void test_gebr_geoxml_program_get_and_set_url(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	GebrGeoXmlProgram *program = gebr_geoxml_flow_append_program(flow);
	const gchar *program_url;

	// Should do nothing, and not crash
	gebr_geoxml_program_set_url(NULL, "Program URL goes here");
	gebr_geoxml_program_set_url(program, NULL);

	program_url = gebr_geoxml_program_get_url(NULL);
	g_assert(program_url == NULL);

	gebr_geoxml_program_set_url(program, "Program URL goes here");
	program_url = gebr_geoxml_program_get_url(program);
	g_assert_cmpstr(program_url, ==, "Program URL goes here");

	gebr_geoxml_program_set_url(program,"Change on URL goes here");
	program_url = gebr_geoxml_program_get_url(program);
	g_assert_cmpstr(program_url, ==, "Change on URL goes here");
}

void test_gebr_geoxml_program_get_control(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	GebrGeoXmlProgram *program = gebr_geoxml_flow_append_program(flow);
	GebrGeoXmlProgramControl control;

	control = gebr_geoxml_program_get_control(NULL);
	g_assert_cmpint(control, ==, GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY);

	__gebr_geoxml_set_attr_value((GdomeElement*)program, "control", "for");
	control = gebr_geoxml_program_get_control(program);
	g_assert(control == GEBR_GEOXML_PROGRAM_CONTROL_FOR);

	__gebr_geoxml_set_attr_value((GdomeElement*)program, "control","");
	control = gebr_geoxml_program_get_control(program);
	g_assert(control == GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY);

	__gebr_geoxml_set_attr_value((GdomeElement*)program, "control", "Meaningless text");
	control = gebr_geoxml_program_get_control(program);
	g_assert(control == GEBR_GEOXML_PROGRAM_CONTROL_UNKNOWN);
}

void test_gebr_geoxml_program_get_and_set_error_id(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	GebrGeoXmlProgram *program = gebr_geoxml_flow_append_program(flow);
	GebrIExprError error;
	gboolean have_error;

	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		gebr_geoxml_program_set_error_id(NULL, FALSE, GEBR_IEXPR_ERROR_UNDEF_VAR);
		exit(0);
	}
	g_test_trap_assert_failed();

	have_error = gebr_geoxml_program_get_error_id(program, &error);
	g_assert(have_error == FALSE);

	gebr_geoxml_program_set_error_id(program, FALSE, GEBR_IEXPR_ERROR_UNDEF_VAR);
	have_error = gebr_geoxml_program_get_error_id(program, &error);
	g_assert(have_error == TRUE);
	g_assert(error == GEBR_IEXPR_ERROR_UNDEF_VAR);

	gebr_geoxml_program_set_error_id(program, FALSE, GEBR_IEXPR_ERROR_SYNTAX);
	have_error = gebr_geoxml_program_get_error_id(program, &error);
	g_assert(have_error == TRUE);
	g_assert(error == GEBR_IEXPR_ERROR_SYNTAX);

	gebr_geoxml_program_set_error_id(program, FALSE, GEBR_IEXPR_ERROR_BAD_REFERENCE);
	have_error = gebr_geoxml_program_get_error_id(program, &error);
	g_assert(have_error == TRUE);
	g_assert(error == GEBR_IEXPR_ERROR_BAD_REFERENCE);

	gebr_geoxml_program_set_error_id(program, TRUE, 0);
	have_error = gebr_geoxml_program_get_error_id(program, &error);
	g_assert(have_error == FALSE);
}

void test_gebr_geoxml_program_control_get_n(void)
{
	GebrGeoXmlFlow *forloop, *flow;
	GebrGeoXmlProgram *program;
	GebrGeoXmlParameters *parameters_list;
	GebrGeoXmlProgramParameter *parameter;
	const gchar *step, *ini;
	const gchar *iter;

	flow = gebr_geoxml_flow_new ();
	gebr_geoxml_document_load((GebrGeoXmlDocument**)&forloop, TEST_DIR"/forloop.mnu", FALSE, NULL);

	gebr_geoxml_flow_get_program(forloop, (GebrGeoXmlSequence**) &program, 0);

	parameters_list = gebr_geoxml_program_get_parameters(program);

	// Set parameter ini and check if it is a ProgramParameter
	gebr_geoxml_parameters_get_parameter(parameters_list, (GebrGeoXmlSequence**)&parameter, 0);
	g_assert(gebr_geoxml_parameter_get_is_program_parameter((GebrGeoXmlParameter*)parameter));
	gebr_geoxml_program_parameter_set_first_value(parameter, FALSE, "5");

	// Set parameter step and check if it is a ProgramParameter
	gebr_geoxml_parameters_get_parameter(parameters_list, (GebrGeoXmlSequence**)&parameter, 1);
	g_assert(gebr_geoxml_parameter_get_is_program_parameter((GebrGeoXmlParameter*)parameter));
	gebr_geoxml_program_parameter_set_first_value(parameter, FALSE, "8000");

	// Set parameter iter and check if it is a ProgramParameter
	gebr_geoxml_parameters_get_parameter(parameters_list, (GebrGeoXmlSequence**)&parameter, 2);
	g_assert(gebr_geoxml_parameter_get_is_program_parameter((GebrGeoXmlParameter*)parameter));
	gebr_geoxml_program_parameter_set_first_value(parameter, FALSE, "1337");

	iter =	gebr_geoxml_program_control_get_n(program, &step, &ini);

	g_assert_cmpstr(ini, ==, "5");
	g_assert_cmpstr(step, ==, "8000");
	g_assert_cmpstr(iter, ==, "1337");
}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	gebr_dirs_set_dtd_dir(DTD_DIR);

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
	g_test_add_func("/libgebr/geoxml/program/get_and_set_help", test_gebr_geoxml_program_get_and_set_help);
	g_test_add_func("/libgebr/geoxml/program/get_and_set_version", test_gebr_geoxml_program_get_and_set_version);
	g_test_add_func("/libgebr/geoxml/program/get_and_set_mpi", test_gebr_geoxml_program_get_and_set_mpi);
	g_test_add_func("/libgebr/geoxml/program/get_and_set_url", test_gebr_geoxml_program_get_and_set_url);
	g_test_add_func("/libgebr/geoxml/program/get_control", test_gebr_geoxml_program_get_control);
	g_test_add_func("/libgebr/geoxml/program/get_and_set_error_id", test_gebr_geoxml_program_get_and_set_error_id);
	g_test_add_func("/libgebr/geoxml/program/control_get_number_of_iterations", test_gebr_geoxml_program_control_get_n);

	return g_test_run();
}
