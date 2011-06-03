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

#include "flow.h"

void test_gebr_geoxml_flow_get_categories_number(void)
{
	gint n;
	GebrGeoXmlFlow *flow = NULL;

	// If flow is NULL the number of categories should be -1
	n = gebr_geoxml_flow_get_categories_number (flow);
	g_assert_cmpint (n, ==, -1);

	flow = gebr_geoxml_flow_new ();
	n = gebr_geoxml_flow_get_categories_number (flow);
	g_assert_cmpint (n, ==, 0);

	gebr_geoxml_flow_append_category(flow, "foo");
	n = gebr_geoxml_flow_get_categories_number (flow);
	g_assert_cmpint (n, ==, 1);
}

void test_duplicate_categories(void)
{
	gint n;
	GebrGeoXmlFlow *flow = NULL;

	flow = gebr_geoxml_flow_new ();
	gebr_geoxml_flow_append_category(flow, "foo");
	gebr_geoxml_flow_append_category(flow, "foo");

	n = gebr_geoxml_flow_get_categories_number (flow);
	g_assert_cmpint (n, ==, 1);

	gebr_geoxml_flow_append_category(flow, "bar");
	n = gebr_geoxml_flow_get_categories_number (flow);
	g_assert_cmpint (n, ==, 2);
}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	gebr_geoxml_document_set_dtd_dir(DTD_DIR);

	g_test_add_func("/libgebr/geoxml/flow/get_categories_number", test_gebr_geoxml_flow_get_categories_number);
	g_test_add_func("/libgebr/geoxml/flow/duplicate_categories", test_duplicate_categories);

	return g_test_run();
}
