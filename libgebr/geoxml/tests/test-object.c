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
#include <stdlib.h>

#include "document.h"
#include "flow.h"
#include "line.h"
#include "object.h"
#include "parameters.h"
#include "program-parameter.h"
#include "program.h"
#include "project.h"

void test_gebr_geoxml_object_get_type (void)
{
	GebrGeoXmlObject *object;
	GebrGeoXmlObject *flow;
	GebrGeoXmlObject *program;
	GebrGeoXmlObject *parameters;
	GebrGeoXmlObject *parameter;
	GebrGeoXmlObjectType type = GEBR_GEOXML_OBJECT_TYPE_UNKNOWN;

	//Test if trying to get the type of a NULL object will fail, as expected
	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		type = gebr_geoxml_object_get_type(NULL);
		exit(0);
	}
	g_test_trap_assert_failed();

	object = GEBR_GEOXML_OBJECT(gebr_geoxml_project_new());
	type = gebr_geoxml_object_get_type(object);
	g_assert_cmpint(type, ==, GEBR_GEOXML_OBJECT_TYPE_PROJECT);

	gebr_geoxml_document_free(GEBR_GEOXML_DOC(object));

	object = GEBR_GEOXML_OBJECT(gebr_geoxml_line_new());
	type = gebr_geoxml_object_get_type(object);
	g_assert_cmpint(type, ==, GEBR_GEOXML_OBJECT_TYPE_LINE);

	gebr_geoxml_document_free(GEBR_GEOXML_DOC(object));

	flow = GEBR_GEOXML_OBJECT(gebr_geoxml_flow_new());
	type = gebr_geoxml_object_get_type(flow);
	g_assert_cmpint(type, ==, GEBR_GEOXML_OBJECT_TYPE_FLOW);

	program = GEBR_GEOXML_OBJECT(gebr_geoxml_flow_append_program(GEBR_GEOXML_FLOW(flow)));
	type = gebr_geoxml_object_get_type(program);
	g_assert_cmpint(type, ==, GEBR_GEOXML_OBJECT_TYPE_PROGRAM);

	parameters = GEBR_GEOXML_OBJECT(gebr_geoxml_program_get_parameters(GEBR_GEOXML_PROGRAM(program)));
	type = gebr_geoxml_object_get_type(parameters);
	g_assert_cmpint(type, ==, GEBR_GEOXML_OBJECT_TYPE_PARAMETERS);

	parameter = GEBR_GEOXML_OBJECT(gebr_geoxml_parameters_append_parameter(GEBR_GEOXML_PARAMETERS(parameters), 0));
	type = gebr_geoxml_object_get_type(parameter);
	g_assert_cmpint(type, ==, GEBR_GEOXML_OBJECT_TYPE_PARAMETER);

	gebr_geoxml_object_unref(parameter);
	gebr_geoxml_object_unref(parameters);
	gebr_geoxml_object_unref(program);
	gebr_geoxml_document_free(GEBR_GEOXML_DOC(flow));
}

void test_gebr_geoxml_object_get_user_data (void)
{
	GebrGeoXmlObject * object;
	GebrGeoXmlObject * program;
	gpointer data = "foo";
	gpointer user_data;

	//Test if \p object is NULL will fail, as expected
	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		gebr_geoxml_object_set_user_data(NULL, data);
		exit(0);
	}
	g_test_trap_assert_failed();

	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		user_data = gebr_geoxml_object_get_user_data(NULL);
		exit(0);
	}
	g_test_trap_assert_failed();

	object = GEBR_GEOXML_OBJECT(gebr_geoxml_flow_new());
	gebr_geoxml_object_set_user_data(object, data);
	user_data = gebr_geoxml_object_get_user_data(object);
	g_assert_cmpstr((char *)user_data, ==, "foo");

	program = GEBR_GEOXML_OBJECT(gebr_geoxml_flow_append_program(GEBR_GEOXML_FLOW(object)));
	gebr_geoxml_object_set_user_data(program, data);
	user_data = gebr_geoxml_object_get_user_data(program);
	g_assert_cmpstr((char *)user_data, ==, "foo");

	gebr_geoxml_object_unref(program);
	gebr_geoxml_document_free(GEBR_GEOXML_DOC(object));
}

void test_gebr_geoxml_object_generate_help (void)
{
	GebrGeoXmlObject *object;
	GebrGeoXmlProgram *program;
	gchar *help = NULL;

	//Test if \p object is NULL will fail, as expected
	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		help = gebr_geoxml_object_generate_help(NULL, "foo");
		exit(0);
	}
	g_test_trap_assert_failed();

	object = GEBR_GEOXML_OBJECT(gebr_geoxml_flow_new());
	program = gebr_geoxml_flow_append_program(GEBR_GEOXML_FLOW(object));
	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		gebr_geoxml_program_set_title(program, "Test Program");
		gebr_geoxml_program_set_description(program, "This is a test program.");
		gebr_geoxml_program_set_version(program, "1.0");
		help = gebr_geoxml_object_generate_help(GEBR_GEOXML_OBJECT(program), "Help content.");
		g_print("%s", help);
		exit(0);
	}
	g_test_trap_assert_passed();

	g_test_trap_assert_stdout ("*begin ttl*Test Program*end ttl*");
	g_test_trap_assert_stdout ("*begin tt2*Test Program*end tt2*");
	g_test_trap_assert_stdout ("*begin des*This is a test program.*end des*");
	g_test_trap_assert_stdout ("*begin cnt*Help content.*end cnt*");
	g_test_trap_assert_stdout ("*begin ver*1.0*end ver*");

	g_free(help);
	gebr_geoxml_object_unref(program);
	gebr_geoxml_document_free(GEBR_GEOXML_DOC(object));
}

void test_gebr_geoxml_object_get_help_content(void)
{
	GebrGeoXmlObject *object;
	GebrGeoXmlProgram *program;
	gchar *help_cnt = NULL;
	gchar *help = NULL;

	//Test if \p object is NULL will fail, as expected
	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		help_cnt = gebr_geoxml_object_get_help_content(NULL);
		exit(0);
	}
	g_test_trap_assert_failed();

	//Test if \p object isn't a flow or a program will fail, as expected
	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		help_cnt = gebr_geoxml_object_get_help_content(GEBR_GEOXML_OBJECT(gebr_geoxml_line_new()));
		exit(0);
	}
	g_test_trap_assert_failed();

	object = GEBR_GEOXML_OBJECT(gebr_geoxml_flow_new());
	program = gebr_geoxml_flow_append_program(GEBR_GEOXML_FLOW(object));
	help = gebr_geoxml_object_generate_help(GEBR_GEOXML_OBJECT(program), "Help content.");
	gebr_geoxml_program_set_help(program, help);
	help_cnt = gebr_geoxml_object_get_help_content(GEBR_GEOXML_OBJECT(program));
	g_assert_cmpstr(help_cnt, ==, "Help content.");

	g_free(help_cnt);
	g_free(help);
	gebr_geoxml_object_unref(program);
	gebr_geoxml_document_free(GEBR_GEOXML_DOC(object));
}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	gebr_geoxml_document_set_dtd_dir(DTD_DIR);

	g_test_add_func("/libgebr/geoxml/object/get_type", test_gebr_geoxml_object_get_type);
	g_test_add_func("/libgebr/geoxml/object/get_and_set_user_data", test_gebr_geoxml_object_get_user_data);
	g_test_add_func("/libgebr/geoxml/object/generate_help", test_gebr_geoxml_object_generate_help);
	g_test_add_func("/libgebr/geoxml/object/get_help_content", test_gebr_geoxml_object_get_help_content);

	return g_test_run();
}
