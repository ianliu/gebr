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

#include "document.h"
#include "flow.h"

static void
test_gebr_geoxml_leaks_new_flow(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	GebrGeoXmlProgram *prog = gebr_geoxml_flow_get_control_program(flow);
	gebr_geoxml_document_unref_sequence(GEBR_GEOXML_SEQUENCE(prog));
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
	}

	gebr_geoxml_flow_foreach_parameter(flow, callback, NULL);
	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	gebr_geoxml_document_set_dtd_dir(DTD_DIR);

	g_test_add_func("/libgebr/geoxml/leaks/new_flow", test_gebr_geoxml_leaks_new_flow);
	g_test_add_func("/libgebr/geoxml/leaks/flow_add_flow", test_gebr_geoxml_leaks_flow_add_flow);
	g_test_add_func("/libgebr/geoxml/leaks/flow_foreach_parameter", test_gebr_geoxml_leaks_flow_foreach_parameter);

	return g_test_run();
}

