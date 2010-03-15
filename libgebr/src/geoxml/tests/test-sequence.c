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

#include <gdome.h>

#include "../geoxml.h"

static GebrGeoXmlFlow * test_menu;
static GString * first;
static GString * after;

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

static void generate_parameter_list_first(GebrGeoXmlProgram * program, gboolean reversed)
{
	generate_parameter_list(first, program, reversed);
}

static void generate_parameter_list_after(GebrGeoXmlProgram * program, gboolean reversed)
{
	generate_parameter_list(after, program, reversed);
}

static void
test_gebr_geoxml_sequence_move_before_with_null(void)
{
	GebrGeoXmlSequence *program;
	GebrGeoXmlParameters *parameters;
	GebrGeoXmlSequence *parameter;
	gint length;

	gebr_geoxml_flow_get_program(test_menu, &program, 0);
	generate_parameter_list_first(GEBR_GEOXML_PROGRAM(program), TRUE);

	parameters = gebr_geoxml_program_get_parameters(GEBR_GEOXML_PROGRAM(program));
	parameter = gebr_geoxml_parameters_get_first_parameter(parameters);
	length = gebr_geoxml_parameters_get_number(parameters);
	for (gint i = 0; i < length; i++, parameter = gebr_geoxml_parameters_get_first_parameter(parameters))
		gebr_geoxml_sequence_move_before(parameter, NULL);

	generate_parameter_list_after(GEBR_GEOXML_PROGRAM(program), TRUE);
	g_assert_cmpstr(first->str, ==, after->str);
}

static void
test_gebr_geoxml_sequence_move_after(void)
{
	GebrGeoXmlSequence *program;
	GebrGeoXmlParameters *parameters;
	GebrGeoXmlSequence *parameter;
	GebrGeoXmlSequence *last_parameter;

	gebr_geoxml_flow_get_program(test_menu, &program, 0);
	generate_parameter_list_first(GEBR_GEOXML_PROGRAM(program), FALSE);

	parameters = gebr_geoxml_program_get_parameters(GEBR_GEOXML_PROGRAM(program));
	parameter = gebr_geoxml_parameters_get_first_parameter(parameters);
	gebr_geoxml_parameters_get_parameter(parameters, &last_parameter,
					     gebr_geoxml_parameters_get_number(parameters)-1);
	for (; parameter != last_parameter; parameter = gebr_geoxml_parameters_get_first_parameter(parameters))
		gebr_geoxml_sequence_move_after(parameter, last_parameter);

	generate_parameter_list_after(GEBR_GEOXML_PROGRAM(program), TRUE);
	g_assert_cmpstr(first->str, ==, after->str);
}

int main(int argc, char *argv[])
{
	GebrGeoXmlDocument * document;
	int ret;

	/* initialization */
	g_test_init(&argc, &argv, NULL);
	gebr_geoxml_document_load(&document, "test.mnu", TRUE, NULL);
	test_menu = GEBR_GEOXML_FLOW(document);
	g_assert(test_menu != NULL);
	first = g_string_new("");
	after = g_string_new("");

	g_test_add_func("/geoxml/sequence/move-before-with-null", test_gebr_geoxml_sequence_move_before_with_null);
	g_test_add_func("/geoxml/sequence/move-after", test_gebr_geoxml_sequence_move_after);
	ret = g_test_run();

	/* frees */
	g_string_free(first, TRUE);
	g_string_free(after, TRUE);

	return ret;
}
