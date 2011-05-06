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
#include <stdio.h>
#include <stdlib.h>
#include <gdome.h>

#include "parameters.h"
#include "parameter.h"
#include "parameter_p.h"
#include "error.h"

void test_gebr_geoxml_parameter_get_parameters(void)
{
	GebrGeoXmlParameters *parameters_list;
	GebrGeoXmlParameter *parameter;
	GebrGeoXmlFlow *flow;
	GebrGeoXmlProgram *program;

	flow = gebr_geoxml_flow_new();
	program = gebr_geoxml_flow_append_program(flow);
	parameters_list = gebr_geoxml_program_get_parameters(program);

	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_STRING);

	g_assert(gebr_geoxml_parameter_get_parameters(parameter) == parameters_list);

	// Will fail if parameter is NULL
	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		g_assert(gebr_geoxml_parameter_get_parameters(NULL) == NULL);
		exit(0);
	}
	g_test_trap_assert_failed();
}

void test_gebr_geoxml_parameter_get_program(void)
{
	GebrGeoXmlParameters *parameters_list;
	GebrGeoXmlParameter *parameter;
	GebrGeoXmlFlow *flow;
	GebrGeoXmlProgram *program;

	flow = gebr_geoxml_flow_new();
	program = gebr_geoxml_flow_append_program(flow);
	parameters_list = gebr_geoxml_program_get_parameters(program);
	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_STRING);

	g_assert(gebr_geoxml_parameter_get_program(parameter) == program);

	// Will fail if parameter is NULL
	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		g_assert(gebr_geoxml_parameter_get_parameters(NULL) == NULL);
		exit(0);
	}
	g_test_trap_assert_failed();
}

void test_gebr_geoxml_parameter_get_and_set_type(void)
{
	GebrGeoXmlParameters *parameters_list;
	GebrGeoXmlParameter *parameter;
	GebrGeoXmlFlow *flow;
	GebrGeoXmlProgram *program;

	flow = gebr_geoxml_flow_new();
	program = gebr_geoxml_flow_append_program(flow);
	parameters_list = gebr_geoxml_program_get_parameters(program);
	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_STRING);
	gebr_geoxml_parameter_set_label(parameter,"Parameter guy");

	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		gebr_geoxml_parameter_set_type(NULL, GEBR_GEOXML_PARAMETER_TYPE_STRING);
		exit(0);
	}
	g_test_trap_assert_failed();

	gebr_geoxml_parameter_set_type(parameter,GEBR_GEOXML_PARAMETER_TYPE_INT);
	g_assert_cmpstr(gebr_geoxml_parameter_get_label(parameter), ==, "Parameter guy");

	g_assert_cmpint(gebr_geoxml_parameter_get_type(parameter), ==, GEBR_GEOXML_PARAMETER_TYPE_INT);

	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		gebr_geoxml_parameter_get_type(NULL);
		exit(0);
	}
	g_test_trap_assert_failed();
}

void test_gebr_geoxml_parameter_set_be_reference(void)
{
	/* TODO:
	 * Make tests for __gebr_geoxml_parameter_set_be_reference_with_value and __gebr_geoxml_parameter_set_be_reference
	 * Refactor __gebr_geoxml_parameter_set_be_reference_with_value using __gebr_geoxml_parameter_set_be_reference to clean code
	 */
}

/*void test__gebr_geoxml_parameter_set_be_reference(void)
{
	GebrGeoXmlParameters *parameters_list;
	GebrGeoXmlParameter *parameter;
	GebrGeoXmlFlow *test_menu;
	GebrGeoXmlProgram *program;

	gebr_geoxml_document_load((GebrGeoXmlDocument**)&test_menu, TEST_DIR"/test.mnu", FALSE, NULL);

	gebr_geoxml_flow_get_program(test_menu, (GebrGeoXmlSequence**)&program, 0);
	parameters_list = gebr_geoxml_program_get_parameters(program);

	g_assert_cmpint(gebr_geoxml_parameters_get_parameter(parameters_list,(GebrGeoXmlSequence**)&parameter,2), ==, GEBR_GEOXML_RETV_SUCCESS);


	g_assert(gebr_geoxml_program_parameter_get_is_list(parameter) == FALSE);

	g_assert_cmpstr(gebr_geoxml_program_parameter_get_first_value(parameter, TRUE), ==, "123");
	g_assert_cmpstr(gebr_geoxml_program_parameter_get_first_value(parameter, FALSE), ==, "456");

	__gebr_geoxml_parameter_set_be_reference(parameter);
	g_assert_cmpint(gebr_geoxml_parameter_get_type(parameter), ==, GEBR_GEOXML_PARAMETER_TYPE_REFERENCE);
}*/

void test_gebr_geoxml_parameter_get_type_name(void)
{
	GebrGeoXmlParameters *parameters_list;
	GebrGeoXmlParameter *parameter;
	GebrGeoXmlFlow *flow;
	GebrGeoXmlProgram *program;

	flow = gebr_geoxml_flow_new();
	program = gebr_geoxml_flow_append_program(flow);
	parameters_list = gebr_geoxml_program_get_parameters(program);
	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_STRING);

	g_assert_cmpstr(gebr_geoxml_parameter_get_type_name(parameter), ==, "string");

	gebr_geoxml_parameter_set_type(parameter,GEBR_GEOXML_PARAMETER_TYPE_INT);

	g_assert_cmpstr(gebr_geoxml_parameter_get_type_name(parameter), ==, "integer");

	g_assert(gebr_geoxml_parameter_get_type_name(NULL) == NULL);
}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/libgebr/geoxml/parameter/get_parameters", test_gebr_geoxml_parameter_get_parameters);
	g_test_add_func("/libgebr/geoxml/parameter/get_program", test_gebr_geoxml_parameter_get_program);
	g_test_add_func("/libgebr/geoxml/parameter/get_and_set_type", test_gebr_geoxml_parameter_get_and_set_type);
//	g_test_add_func("/libgebr/geoxml/parameter/set_be_reference", test__gebr_geoxml_parameter_set_be_reference);
	g_test_add_func("/libgebr/geoxml/parameter/get_type_name", test_gebr_geoxml_parameter_get_type_name);

	return g_test_run();
}
