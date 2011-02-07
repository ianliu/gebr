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
#include <gdome.h>

#include "../geoxml.h"

typedef struct {
	GebrGeoXmlFlow * flow;
	GString * before;
	GString * after;
} Fixture;

static void
fixture_setup(Fixture * fix, gconstpointer data)
{
	gebr_geoxml_document_load((GebrGeoXmlDocument**)(&fix->flow), TEST_DIR"/test.mnu", FALSE, NULL);

	g_assert(fix->flow != NULL);

	fix->before = g_string_new("");
	fix->after = g_string_new("");
}

static void
fixture_teardown(Fixture * fix, gconstpointer data)
{
	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(fix->flow));
	g_string_free(fix->before, TRUE);
	g_string_free(fix->after, TRUE);
}

static void generate_parameter_list(GString * string, GebrGeoXmlProgram * program, gboolean reversed)
{
	GebrGeoXmlParameters *parameters;
	GebrGeoXmlSequence *parameter;

	g_string_assign(string, "");
	parameters = gebr_geoxml_program_get_parameters(program);
	if (reversed) {
		gebr_geoxml_parameters_get_parameter(parameters, &parameter,
						     gebr_geoxml_parameters_get_number(parameters)-1);
		for (; parameter != NULL;  gebr_geoxml_sequence_previous(&parameter)) 
			g_string_append_printf(string, "%s", gebr_geoxml_parameter_get_label(GEBR_GEOXML_PARAMETER(parameter)));
	} else {
		parameter = gebr_geoxml_parameters_get_first_parameter(parameters);
		for (; parameter != NULL;  gebr_geoxml_sequence_next(&parameter)) 
			g_string_append_printf(string, "%s", gebr_geoxml_parameter_get_label(GEBR_GEOXML_PARAMETER(parameter)));
	}
}

static void
test_gebr_geoxml_sequence_move_before_with_null(Fixture * fix, gconstpointer data)
{
	GebrGeoXmlSequence *program;
	GebrGeoXmlParameters *parameters;
	GebrGeoXmlSequence *parameter;
	gint length;

	gebr_geoxml_flow_get_program(fix->flow, &program, 0);
	generate_parameter_list(fix->before, GEBR_GEOXML_PROGRAM(program), TRUE);

	parameters = gebr_geoxml_program_get_parameters(GEBR_GEOXML_PROGRAM(program));
	parameter = gebr_geoxml_parameters_get_first_parameter(parameters);
	length = gebr_geoxml_parameters_get_number(parameters);
	for (gint i = 0; i < length; i++, parameter = gebr_geoxml_parameters_get_first_parameter(parameters))
		gebr_geoxml_sequence_move_before(parameter, NULL);

	generate_parameter_list(fix->after, GEBR_GEOXML_PROGRAM(program), TRUE);
	g_assert_cmpstr(fix->before->str, ==, fix->after->str);
}

static void
test_gebr_geoxml_sequence_move_after(Fixture * fix, gconstpointer data)
{
	GebrGeoXmlSequence *program;
	GebrGeoXmlParameters *parameters;
	GebrGeoXmlSequence *parameter;
	GebrGeoXmlSequence *last_parameter;

	gebr_geoxml_flow_get_program(fix->flow, &program, 0);
	generate_parameter_list(fix->before, GEBR_GEOXML_PROGRAM(program), FALSE);

	parameters = gebr_geoxml_program_get_parameters(GEBR_GEOXML_PROGRAM(program));
	parameter = gebr_geoxml_parameters_get_first_parameter(parameters);
	gebr_geoxml_parameters_get_parameter(parameters, &last_parameter,
					     gebr_geoxml_parameters_get_number(parameters)-1);
	for (; parameter != last_parameter; parameter = gebr_geoxml_parameters_get_first_parameter(parameters))
		gebr_geoxml_sequence_move_after(parameter, last_parameter);

	generate_parameter_list(fix->after, GEBR_GEOXML_PROGRAM(program), TRUE);
	g_assert_cmpstr(fix->before->str, ==, fix->after->str);
}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add("/geoxml/sequence/move-before-with-null", Fixture, NULL, fixture_setup,
		   test_gebr_geoxml_sequence_move_before_with_null, fixture_teardown);
	g_test_add("/geoxml/sequence/move-after", Fixture, NULL, fixture_setup,
		   test_gebr_geoxml_sequence_move_after, fixture_teardown);

	return g_test_run();
}
