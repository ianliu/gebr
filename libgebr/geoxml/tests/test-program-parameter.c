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

#include "flow.h"
#include "parameters.h"
#include "parameter.h"
#include "program.h"
#include "program-parameter.h"

void test_gebr_geoxml_program_parameter_get_and_set_required(void)
{
	GebrGeoXmlFlow *flow;
	GebrGeoXmlProgram *program;
	GebrGeoXmlParameters *parameters_list;
	GebrGeoXmlProgramParameter *parameter;

	flow = gebr_geoxml_flow_new();
	program = gebr_geoxml_flow_append_program(flow);
	parameters_list = gebr_geoxml_program_get_parameters(program);
	parameter = (GebrGeoXmlProgramParameter*) gebr_geoxml_parameters_append_parameter(parameters_list,
	                                                                                  GEBR_GEOXML_PARAMETER_TYPE_STRING);

	gebr_geoxml_program_parameter_set_required(parameter, TRUE);
	g_assert(gebr_geoxml_program_parameter_get_required(parameter) == TRUE);

	gebr_geoxml_program_parameter_set_required(parameter, FALSE);
	g_assert(gebr_geoxml_program_parameter_get_required(parameter) == FALSE);

	gebr_geoxml_program_parameter_set_required(NULL, TRUE);
	g_assert(gebr_geoxml_program_parameter_get_required(NULL) == FALSE);

	parameter = (GebrGeoXmlProgramParameter*) gebr_geoxml_parameters_append_parameter(parameters_list,
	                                                                                  GEBR_GEOXML_PARAMETER_TYPE_FLAG);
	gebr_geoxml_program_parameter_set_required(parameter, TRUE);
	g_assert(gebr_geoxml_program_parameter_get_required(parameter) == FALSE);
}

void test_gebr_geoxml_program_parameter_get_and_set_keyword(void)
{
	GebrGeoXmlFlow *flow;
	GebrGeoXmlProgram *program;
	GebrGeoXmlParameters *parameters_list;
	GebrGeoXmlProgramParameter *parameter;

	flow = gebr_geoxml_flow_new();
	program = gebr_geoxml_flow_append_program(flow);
	parameters_list = gebr_geoxml_program_get_parameters(program);
	parameter = (GebrGeoXmlProgramParameter*) gebr_geoxml_parameters_append_parameter(parameters_list,
	                                                                                  GEBR_GEOXML_PARAMETER_TYPE_STRING);

	gebr_geoxml_program_parameter_set_keyword(parameter, "Keyword guy");
	g_assert_cmpstr(gebr_geoxml_program_parameter_get_keyword(parameter), ==, "Keyword guy");

	gebr_geoxml_program_parameter_set_keyword(parameter, "Keyword guy modified");
	g_assert_cmpstr(gebr_geoxml_program_parameter_get_keyword(parameter), ==, "Keyword guy modified");

	gebr_geoxml_program_parameter_set_keyword(NULL, "Should do nothing");
	g_assert(gebr_geoxml_program_parameter_get_keyword(NULL) == NULL);
}

void test_gebr_geoxml_program_parameter_get_and_set_be_list(void)
{
	GebrGeoXmlFlow *flow;
	GebrGeoXmlProgram *program;
	GebrGeoXmlParameters *parameters_list;
	GebrGeoXmlProgramParameter *parameter;

	flow = gebr_geoxml_flow_new();
	program = gebr_geoxml_flow_append_program(flow);
	parameters_list = gebr_geoxml_program_get_parameters(program);
	parameter = (GebrGeoXmlProgramParameter*) gebr_geoxml_parameters_append_parameter(parameters_list,
	                                                                                  GEBR_GEOXML_PARAMETER_TYPE_STRING);

	gebr_geoxml_program_parameter_set_be_list(parameter, TRUE);
	g_assert(gebr_geoxml_program_parameter_get_is_list(parameter) == TRUE);
	gebr_geoxml_program_parameter_set_be_list(parameter, FALSE);
	g_assert(gebr_geoxml_program_parameter_get_is_list(parameter) == FALSE);

	gebr_geoxml_program_parameter_set_be_list(NULL, TRUE);
	g_assert(gebr_geoxml_program_parameter_get_is_list(NULL) == FALSE);

	parameter = (GebrGeoXmlProgramParameter*) gebr_geoxml_parameters_append_parameter(parameters_list,
	                                                                                  GEBR_GEOXML_PARAMETER_TYPE_FLAG);
	gebr_geoxml_program_parameter_set_be_list(parameter, TRUE);
	g_assert(gebr_geoxml_program_parameter_get_is_list(parameter) == FALSE);
}

void test_gebr_geoxml_program_parameter_get_and_set_list_separator(void)
{
	GebrGeoXmlFlow *flow;
	GebrGeoXmlProgram *program;
	GebrGeoXmlParameters *parameters_list;
	GebrGeoXmlProgramParameter *parameter;

	flow = gebr_geoxml_flow_new();
	program = gebr_geoxml_flow_append_program(flow);
	parameters_list = gebr_geoxml_program_get_parameters(program);
	parameter = (GebrGeoXmlProgramParameter*) gebr_geoxml_parameters_append_parameter(parameters_list,
	                                                                                  GEBR_GEOXML_PARAMETER_TYPE_STRING);
	gebr_geoxml_program_parameter_set_be_list(parameter, TRUE);

	gebr_geoxml_program_parameter_set_list_separator(parameter, ";");
	g_assert_cmpstr(gebr_geoxml_program_parameter_get_list_separator(parameter), ==, ";");

	gebr_geoxml_program_parameter_set_list_separator(NULL, ";");
	g_assert(gebr_geoxml_program_parameter_get_list_separator(NULL) == FALSE);

	gebr_geoxml_program_parameter_set_be_list(parameter, FALSE);
	gebr_geoxml_program_parameter_set_list_separator(parameter, ";");
	g_assert(gebr_geoxml_program_parameter_get_list_separator(NULL) == FALSE);
}

void test_gebr_geoxml_program_parameter_get_and_set_first_value(void)
{
	GebrGeoXmlFlow *flow;
	GebrGeoXmlProgram *program;
	GebrGeoXmlParameters *parameters_list;
	GebrGeoXmlProgramParameter *parameter;

	flow = gebr_geoxml_flow_new();
	program = gebr_geoxml_flow_append_program(flow);
	parameters_list = gebr_geoxml_program_get_parameters(program);
	parameter = (GebrGeoXmlProgramParameter*) gebr_geoxml_parameters_append_parameter(parameters_list,
	                                                                                  GEBR_GEOXML_PARAMETER_TYPE_STRING);

	gebr_geoxml_program_parameter_set_first_value(parameter, FALSE, "Value 1");
	g_assert_cmpstr(gebr_geoxml_program_parameter_get_first_value(parameter, FALSE), ==, "Value 1");

	gebr_geoxml_program_parameter_set_first_value(parameter, FALSE, "Value 2");
	g_assert_cmpstr(gebr_geoxml_program_parameter_get_first_value(parameter, FALSE), ==, "Value 2");

	gebr_geoxml_program_parameter_set_first_value(parameter, TRUE, "Value Default");
	g_assert_cmpstr(gebr_geoxml_program_parameter_get_first_value(parameter, TRUE), ==, "Value Default");

	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		gebr_geoxml_program_parameter_set_first_value(NULL, TRUE, "Value Default");
		exit(0);
	}
	g_test_trap_assert_failed();

	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
			gebr_geoxml_program_parameter_set_first_value(parameter, TRUE, NULL);
			exit(0);
		}
	g_test_trap_assert_failed();

	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		g_assert(gebr_geoxml_program_parameter_get_first_value(NULL, TRUE));
		exit(0);
	}
	g_test_trap_assert_failed();
}

void test_gebr_geoxml_program_parameter_get_and_set_first_boolean_value(void)
{
	GebrGeoXmlFlow *flow;
	GebrGeoXmlProgram *program;
	GebrGeoXmlParameters *parameters_list;
	GebrGeoXmlProgramParameter *parameter;

	flow = gebr_geoxml_flow_new();
	program = gebr_geoxml_flow_append_program(flow);
	parameters_list = gebr_geoxml_program_get_parameters(program);
	parameter = (GebrGeoXmlProgramParameter*) gebr_geoxml_parameters_append_parameter(parameters_list,
	                                                                                  GEBR_GEOXML_PARAMETER_TYPE_FLAG);
	gebr_geoxml_program_parameter_set_first_boolean_value(parameter, TRUE, TRUE);
	g_assert(gebr_geoxml_program_parameter_get_first_boolean_value(parameter, TRUE) == TRUE);

	gebr_geoxml_program_parameter_set_first_boolean_value(parameter, FALSE, TRUE);
	g_assert(gebr_geoxml_program_parameter_get_first_boolean_value(parameter, TRUE) == TRUE);

	gebr_geoxml_program_parameter_set_first_boolean_value(parameter, TRUE, FALSE);
	g_assert(gebr_geoxml_program_parameter_get_first_boolean_value(parameter, TRUE) == FALSE);

	gebr_geoxml_program_parameter_set_first_boolean_value(parameter, FALSE, FALSE);
	g_assert(gebr_geoxml_program_parameter_get_first_boolean_value(parameter, TRUE) == FALSE);

	gebr_geoxml_program_parameter_set_first_boolean_value(NULL, FALSE, TRUE);

	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		g_assert(gebr_geoxml_program_parameter_get_first_boolean_value(parameter, TRUE));
			exit(0);
		}
		g_test_trap_assert_failed();
}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/libgebr/geoxml/program_parameter/get_and_set_required", test_gebr_geoxml_program_parameter_get_and_set_required);
	g_test_add_func("/libgebr/geoxml/program_parameter/get_and_set_keyword", test_gebr_geoxml_program_parameter_get_and_set_keyword);
	g_test_add_func("/libgebr/geoxml/program_parameter/get_and_set_be_list", test_gebr_geoxml_program_parameter_get_and_set_be_list);
	g_test_add_func("/libgebr/geoxml/program_parameter/get_and_set_list_separator", test_gebr_geoxml_program_parameter_get_and_set_list_separator);
	g_test_add_func("/libgebr/geoxml/program_parameter/get_and_set_first_value", test_gebr_geoxml_program_parameter_get_and_set_first_value);
	g_test_add_func("/libgebr/geoxml/program_parameter/get_and_set_first_boolean_value", test_gebr_geoxml_program_parameter_get_and_set_first_boolean_value);

	return g_test_run();
}