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

#include "object.h"
#include "document.h"
#include "flow.h"

static void
test_gebr_geoxml_leaks_new_flow(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
}

static void
test_gebr_geoxml_leaks_has_control(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	gebr_geoxml_flow_has_control_program(flow);
	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
}

static void
test_gebr_geoxml_leaks_get_control_program(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	GebrGeoXmlProgram *prog = gebr_geoxml_flow_get_control_program(flow);
	gebr_geoxml_object_unref(GEBR_GEOXML_OBJECT(prog));
	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
}

static void
test_gebr_geoxml_leaks_flow_add_flow(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	GebrGeoXmlFlow *flow2 = gebr_geoxml_flow_new();
	gebr_geoxml_flow_add_flow(flow, flow2);
	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow2));
}

static void
test_gebr_geoxml_leaks_flow_foreach_parameter(void)
{
	GebrGeoXmlFlow *flow;

	gebr_geoxml_document_load((GebrGeoXmlDocument **)&flow,
				  TEST_DIR "/test.mnu", TRUE, NULL);

	gboolean callback(GebrGeoXmlObject *object, gpointer user_data) {
		return TRUE;
	}

	gebr_geoxml_flow_foreach_parameter(flow, callback, NULL);
	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
}

static void
test_gebr_geoxml_leaks_set_dict_keyword(void)
{
	GebrGeoXmlParameter *param;
	GebrGeoXmlParameters *params;
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();

// FIXME: Leaks because of gebr_geoxml_parameters_append_parameter
//	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOC(flow), GEBR_GEOXML_PARAMETER_TYPE_STRING, "x", "y");

	params = gebr_geoxml_document_get_dict_parameters(GEBR_GEOXML_DOC(flow)); // OK
	gebr_geoxml_parameters_get_group(params); // OK
	__gebr_geoxml_parameters_group_check(params); // OK

	// FIXME: Leaks
	param = gebr_geoxml_parameters_append_parameter(params, GEBR_GEOXML_PARAMETER_TYPE_STRING);
//	gebr_geoxml_program_parameter_set_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(param), "x");
//	gebr_geoxml_program_parameter_set_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(param),
//						      FALSE, "y");

	gebr_geoxml_object_unref(params);
	gebr_geoxml_object_unref(param);
	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
}


int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	gebr_geoxml_document_set_dtd_dir(DTD_DIR);

	g_test_add_func("/libgebr/geoxml/leaks/flow_new", test_gebr_geoxml_leaks_new_flow);
	g_test_add_func("/libgebr/geoxml/leaks/has_control", test_gebr_geoxml_leaks_has_control);
	g_test_add_func("/libgebr/geoxml/leaks/get_control_program", test_gebr_geoxml_leaks_get_control_program);
	g_test_add_func("/libgebr/geoxml/leaks/flow_add_flow", test_gebr_geoxml_leaks_flow_add_flow);
	g_test_add_func("/libgebr/geoxml/leaks/flow_foreach_parameter", test_gebr_geoxml_leaks_flow_foreach_parameter);

	g_test_add_func("/libgebr/geoxml/leaks/dict/set_keyword", test_gebr_geoxml_leaks_set_dict_keyword);

	return g_test_run();
}
