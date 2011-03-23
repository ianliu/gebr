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

void test_gebr_geoxml_flow_server_get_address(void){
	const gchar *address;
	GebrGeoXmlFlow *flow = NULL;

	// Test if flow is created as NULL
	address = gebr_geoxml_flow_server_get_address(flow);
	g_assert(address == NULL);

	// Test if flow get the right address passed by
	flow = gebr_geoxml_flow_new ();
	gebr_geoxml_flow_server_set_address(flow, "abc/def");
	address = gebr_geoxml_flow_server_get_address(flow);
	g_assert_cmpstr(address, ==, "abc/def");

	// Test a change of address to a empty string (not NULL)
	gebr_geoxml_flow_server_set_address(flow, "");
	address = gebr_geoxml_flow_server_get_address(flow);
	g_assert_cmpstr(address, ==, "");
}

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

void test_gebr_geoxml_flow_get_date_last_run(void){
	const gchar *date;
	GebrGeoXmlFlow *flow = NULL;

	date = gebr_geoxml_flow_get_date_last_run(flow);
	g_assert(date == NULL);

	flow = gebr_geoxml_flow_new ();
	gebr_geoxml_flow_set_date_last_run(flow, "18/03/2011");
	date = gebr_geoxml_flow_get_date_last_run(flow);
	g_assert_cmpstr(date, ==, "18/03/2011");

	gebr_geoxml_flow_set_date_last_run(flow, "");
	date = gebr_geoxml_flow_get_date_last_run(flow);
	g_assert_cmpstr(date, ==, "");

}

void test_gebr_geoxml_flow_server_get_date_last_run(void){
	const gchar *date;
	GebrGeoXmlFlow *flow = NULL;

	flow = gebr_geoxml_flow_new ();
	gebr_geoxml_flow_server_set_date_last_run(flow, "23/03/2011");
	date = gebr_geoxml_flow_server_get_date_last_run(flow);
	g_assert_cmpstr(date, ==, "23/03/2011");

	gebr_geoxml_flow_server_set_date_last_run(flow, "");
	date = gebr_geoxml_flow_server_get_date_last_run(flow);
	g_assert_cmpstr(date, ==, "");

}

/*
 * TODO
 * void test_gebr_geoxml_flow_io_set_input(void){
	const gchar *path;
	GebrGeoXmlFlow *flow = NULL;

	path = gebr_geoxml_flow_io_get_input(flow);
	g_assert(path == NULL);

}*/


int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/libgebr/geoxml/flow/server_get_address", test_gebr_geoxml_flow_server_get_address);
	g_test_add_func("/libgebr/geoxml/flow/get_categories_number", test_gebr_geoxml_flow_get_categories_number);
	g_test_add_func("/libgebr/geoxml/flow/duplicate_categories", test_duplicate_categories);
	g_test_add_func("/libgebr/geoxml/flow/flow_get_date_last_run", test_gebr_geoxml_flow_get_date_last_run);
	g_test_add_func("/libgebr/geoxml/flow/flow_server_get_date_last_run", test_gebr_geoxml_flow_server_get_date_last_run);
	//g_test_add_func("/libgebr/geoxml/flow/io_set_input", test_gebr_geoxml_flow_io_set_input);

	return g_test_run();
}
