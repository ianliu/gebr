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
#include "parameters.h"
#include "parameter_group.h"
#include "parameter.h"
#include "sequence.h"
#include "program.h"
#include "program-parameter.c"

void test_gebr_geoxml_parameters_append_parameter(void){
	GebrGeoXmlParameters *parameters_list = NULL;
	GebrGeoXmlParameter *parameter;
	GebrGeoXmlFlow *flow;
	GebrGeoXmlProgram *program = NULL;

	// Test with a null parameters_list
	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_STRING);
	g_assert(parameter == NULL);

	// Create a parameters list, getting it from a program (which was got from a flow) and use it
	flow = gebr_geoxml_flow_new();
	program = gebr_geoxml_flow_append_program(flow);
	parameters_list = gebr_geoxml_program_get_parameters(program);

	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_STRING);
	g_assert(parameter != NULL);

	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_REFERENCE);
	g_assert(parameter == NULL);

}

void test_gebr_geoxml_parameters_is_var_used()
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	GebrGeoXmlProgram *program = gebr_geoxml_flow_append_program(flow);

	GebrGeoXmlParameters *parameters_list = gebr_geoxml_program_get_parameters(program);
	GebrGeoXmlParameter *parameter;

	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_INT);
	gebr_geoxml_program_parameter_set_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), "Test keyword 1");
	gebr_geoxml_program_parameter_set_string_value(GEBR_GEOXML_PROGRAM_PARAMETER(parameter),FALSE, "a+var1");

	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_INT);
	gebr_geoxml_program_parameter_set_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), "Test keyword 2");
	gebr_geoxml_program_parameter_set_string_value(GEBR_GEOXML_PROGRAM_PARAMETER(parameter),FALSE, "a+var2");

	g_assert(gebr_geoxml_parameters_is_var_used(parameters_list, "var2") == TRUE);
	g_assert(gebr_geoxml_program_parameter_is_var_used(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), "var2") == TRUE);
}

void test_gebr_geoxml_parameters_get_and_set_default_selection(void)
{
	GebrGeoXmlParameters *parameters_list;
	GebrGeoXmlParameter *parameter;
	GebrGeoXmlFlow *flow;
	GebrGeoXmlProgram *program;

	// Check if will return NULL when program is passed as NULL
	g_assert(!gebr_geoxml_parameters_get_default_selection(NULL));

	flow = gebr_geoxml_flow_new();
	program = gebr_geoxml_flow_append_program(flow);
	parameters_list = gebr_geoxml_program_get_parameters(program);
	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_STRING);
	gebr_geoxml_parameter_set_label(parameter, "Default parameter");

	gebr_geoxml_parameters_set_default_selection(parameters_list, parameter);
	parameter = gebr_geoxml_parameters_get_default_selection(parameters_list);

	g_assert_cmpstr(gebr_geoxml_parameter_get_label(parameter), ==, "Default parameter");
}

void test_gebr_geoxml_parameters_get_and_set_selection(void)
{
	GebrGeoXmlParameters *parameters_list;
	GebrGeoXmlParameter *parameter;
	GebrGeoXmlFlow *flow;
	GebrGeoXmlProgram *program;

	// Check if will return NULL when program is passed as NULL
	g_assert(!gebr_geoxml_parameters_get_selection(NULL));

	flow = gebr_geoxml_flow_new();
	program = gebr_geoxml_flow_append_program(flow);
	parameters_list = gebr_geoxml_program_get_parameters(program);
	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_STRING);
	gebr_geoxml_parameter_set_label(parameter, "A parameter");
	// Need to be set as default selection before
	gebr_geoxml_parameters_set_default_selection(parameters_list, parameter);

	gebr_geoxml_parameters_set_selection(parameters_list, parameter);
	parameter = gebr_geoxml_parameters_get_selection(parameters_list);

	g_assert_cmpstr(gebr_geoxml_parameter_get_label(parameter), ==, "A parameter");
}

void test_gebr_geoxml_parameters_get_first_parameter(void)
{
	GebrGeoXmlParameters *parameters_list;
	GebrGeoXmlParameter *parameter;
	GebrGeoXmlFlow *flow;
	GebrGeoXmlProgram *program;

	flow = gebr_geoxml_flow_new();
	program = gebr_geoxml_flow_append_program(flow);
	parameters_list = gebr_geoxml_program_get_parameters(program);

	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_STRING);
	gebr_geoxml_parameter_set_label(parameter, "1st parameter");

	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_STRING);
	gebr_geoxml_parameter_set_label(parameter, "2nd parameter");

	parameter = (GebrGeoXmlParameter*) gebr_geoxml_parameters_get_first_parameter(parameters_list);
	g_assert_cmpstr(gebr_geoxml_parameter_get_label(parameter), ==, "1st parameter");
}

void test_gebr_geoxml_parameters_get_parameter(void)
{
	GebrGeoXmlParameters *parameters_list;
	GebrGeoXmlParameter *parameter;
	GebrGeoXmlFlow *flow;
	GebrGeoXmlProgram *program;
	int index;

	flow = gebr_geoxml_flow_new();
	program = gebr_geoxml_flow_append_program(flow);
	parameters_list = gebr_geoxml_program_get_parameters(program);

	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_STRING);
	gebr_geoxml_parameter_set_label(parameter, "1st parameter");

	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_STRING);
	gebr_geoxml_parameter_set_label(parameter, "2nd parameter");

	index = gebr_geoxml_parameters_get_parameter(parameters_list, (GebrGeoXmlSequence**) &parameter, 0);
	g_assert_cmpint(index, ==, GEBR_GEOXML_RETV_SUCCESS);
	g_assert_cmpstr(gebr_geoxml_parameter_get_label(parameter), ==, "1st parameter");

	index = gebr_geoxml_parameters_get_parameter(parameters_list, (GebrGeoXmlSequence**) &parameter, 1);
	g_assert_cmpint(index, ==, GEBR_GEOXML_RETV_SUCCESS);
	g_assert_cmpstr(gebr_geoxml_parameter_get_label(parameter), ==, "2nd parameter");

	index = gebr_geoxml_parameters_get_parameter(parameters_list, (GebrGeoXmlSequence**) &parameter, 1337);
	g_assert_cmpint(index, ==, GEBR_GEOXML_RETV_INVALID_INDEX);

	index = gebr_geoxml_parameters_get_parameter(NULL, (GebrGeoXmlSequence**) &parameter, 0);
	g_assert_cmpint(index, ==, GEBR_GEOXML_RETV_NULL_PTR);
}

void test_gebr_geoxml_parameters_get_number(void)
{
	GebrGeoXmlParameters *parameters_list;
	GebrGeoXmlParameter *parameter;
	GebrGeoXmlFlow *flow;
	GebrGeoXmlProgram *program;

	flow = gebr_geoxml_flow_new();
	program = gebr_geoxml_flow_append_program(flow);
	parameters_list = gebr_geoxml_program_get_parameters(program);

	g_assert_cmpint(gebr_geoxml_parameters_get_number(NULL), ==, -1);

	g_assert_cmpint(gebr_geoxml_parameters_get_number(parameters_list), ==, 0);

	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_STRING);
	gebr_geoxml_parameter_set_label(parameter, "1st parameter");

	g_assert_cmpint(gebr_geoxml_parameters_get_number(parameters_list), ==, 1);

	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_STRING);
	gebr_geoxml_parameter_set_label(parameter, "2nd parameter");
	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_STRING);
	gebr_geoxml_parameter_set_label(parameter, "3rd parameter");

	g_assert_cmpint(gebr_geoxml_parameters_get_number(parameters_list), ==, 3);
}

void test_gebr_geoxml_parameters_get_is_in_group(void)
{
	GebrGeoXmlParameters *parameters_list, *group_par_list;
	GebrGeoXmlParameter *parameter;
	GebrGeoXmlFlow *flow;
	GebrGeoXmlProgram *program;

	flow = gebr_geoxml_flow_new();
	program = gebr_geoxml_flow_append_program(flow);
	parameters_list = gebr_geoxml_program_get_parameters(program);
	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_GROUP);

	group_par_list = gebr_geoxml_parameter_group_get_template(GEBR_GEOXML_PARAMETER_GROUP(parameter));

	g_assert(gebr_geoxml_parameters_get_is_in_group(group_par_list));
	g_assert(!gebr_geoxml_parameters_get_is_in_group(NULL));
}

void test_gebr_geoxml_parameters_get_group(void)
{
	GebrGeoXmlParameters *parameters_list, *group_par_list;
	GebrGeoXmlParameter *parameter;
	GebrGeoXmlFlow *flow;
	GebrGeoXmlProgram *program;

	flow = gebr_geoxml_flow_new();
	program = gebr_geoxml_flow_append_program(flow);
	parameters_list = gebr_geoxml_program_get_parameters(program);
	parameter = gebr_geoxml_parameters_append_parameter(parameters_list, GEBR_GEOXML_PARAMETER_TYPE_GROUP);

	group_par_list = gebr_geoxml_parameter_group_get_template(GEBR_GEOXML_PARAMETER_GROUP(parameter));

	g_assert(gebr_geoxml_parameters_get_group(group_par_list) == GEBR_GEOXML_PARAMETER_GROUP(parameter));
	g_assert(gebr_geoxml_parameters_get_group(NULL) == NULL);
}

void test_gebr_geoxml_parameters_reset(void)
{

}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/libgebr/geoxml/parameters/append_parameter", test_gebr_geoxml_parameters_append_parameter);
	g_test_add_func("/libgebr/geoxml/parameters/is_var_used", test_gebr_geoxml_parameters_is_var_used);
	g_test_add_func("/libgebr/geoxml/parameters/get_and_set_default_selection", test_gebr_geoxml_parameters_get_and_set_default_selection);
	g_test_add_func("/libgebr/geoxml/parameters/get_and_set_selection", test_gebr_geoxml_parameters_get_and_set_selection);
	g_test_add_func("/libgebr/geoxml/parameters/get_first_parameter", test_gebr_geoxml_parameters_get_first_parameter);
	g_test_add_func("/libgebr/geoxml/parameters/get_parameter", test_gebr_geoxml_parameters_get_parameter);
	g_test_add_func("/libgebr/geoxml/parameters/get_number", test_gebr_geoxml_parameters_get_number);
	g_test_add_func("/libgebr/geoxml/parameters/get_is_in_group", test_gebr_geoxml_parameters_get_is_in_group);
	g_test_add_func("/libgebr/geoxml/parameters/get_group", test_gebr_geoxml_parameters_get_group);

	return g_test_run();
}
