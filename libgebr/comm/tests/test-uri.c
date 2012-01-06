/*
 * test-uri.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011 - GêBR Core team (www.gebrproject.com)
 *
 * GêBR Project is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GêBR Project is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GêBR Project. If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>

#include <gebr-comm-uri.h>
#include <string.h>

void
test_gebr_comm_uri_parse(void)
{
	GebrCommUri *uri = gebr_comm_uri_new();
	gebr_comm_uri_parse(uri, "/foo?bar=3;baz=boo;too=\%20\%3D\%3B\%3F");
	g_assert_cmpstr(gebr_comm_uri_get_param(uri, "bar"), ==, "3");
	g_assert_cmpstr(gebr_comm_uri_get_param(uri, "baz"), ==, "boo");
	g_assert_cmpstr(gebr_comm_uri_get_param(uri, "too"), ==, " =;?");
	g_assert_cmpstr(gebr_comm_uri_get_prefix(uri), ==, "/foo");
	gebr_comm_uri_free(uri);
}

void
test_gebr_comm_uri_to_string()
{
	GebrCommUri *uri = gebr_comm_uri_new();
	gebr_comm_uri_set_prefix(uri, "/foo");
	gebr_comm_uri_add_param(uri, "foo", "bar");
	gebr_comm_uri_add_param(uri, "boo", "baz");
	gebr_comm_uri_add_param(uri, "too", " =;?");

	g_assert_cmpstr(gebr_comm_uri_get_param(uri, "foo"), ==, "bar");
	g_assert_cmpstr(gebr_comm_uri_get_param(uri, "boo"), ==, "baz");
	g_assert_cmpstr(gebr_comm_uri_get_param(uri, "too"), ==, " =;?");
	g_assert_cmpstr(gebr_comm_uri_get_prefix(uri), ==, "/foo");

	gchar *str = gebr_comm_uri_to_string(uri);

	g_assert(g_str_has_prefix(str, "/foo?"));
	g_assert(strstr(str, "foo=bar") != NULL);
	g_assert(strstr(str, "boo=baz") != NULL);
	g_assert(strstr(str, "too=\%20\%3D\%3B\%3F") != NULL);

	g_free(str);
	gebr_comm_uri_free(uri);

	// ----

	uri = gebr_comm_uri_new();
	gebr_comm_uri_set_prefix(uri, "/foo");

	str = gebr_comm_uri_to_string(uri);
	g_assert_cmpstr(str, ==, "/foo");

	g_free(str);
	gebr_comm_uri_free(uri);
}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/libgebr/comm/uri/to_string", test_gebr_comm_uri_to_string);
	g_test_add_func("/libgebr/comm/uri/parse", test_gebr_comm_uri_parse);

	return g_test_run();
}

