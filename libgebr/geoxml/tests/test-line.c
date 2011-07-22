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
#include <gebr-dirs-priv.h>

#include "line.h"
#include "value_sequence.h"
#include "document.h"

void test_gebr_geoxml_line_get_flows_number(void)
{
	glong n;
	GebrGeoXmlLine *line = NULL;

	n = gebr_geoxml_line_get_flows_number (line);
	g_assert_cmpint (n, ==, -1);

	line = gebr_geoxml_line_new();
	n = gebr_geoxml_line_get_flows_number (line);
	g_assert_cmpint (n, ==, 0);

	gebr_geoxml_line_append_flow(line, "flow1");
	n = gebr_geoxml_line_get_flows_number (line);
	g_assert_cmpint (n, ==, 1);
}

void test_gebr_geoxml_line_get_flow(void)
{
	GebrGeoXmlLine *line = NULL;
	GebrGeoXmlLineFlow *line_flow;
	GebrGeoXmlSequence *sequence = NULL;
	int x;
	const gchar *source;

	line = gebr_geoxml_line_new();

	x = gebr_geoxml_line_get_flow(line, &sequence, 0);
	g_assert_cmpint(x,==,-5);

	line_flow = gebr_geoxml_line_append_flow(line, "flow1");
	x = gebr_geoxml_line_get_flow(line, &sequence, 0);
	g_assert_cmpint(x,==,0);
	//compare if line_flow created is equal a sequence passed by function get_flow
	g_assert(line_flow == GEBR_GEOXML_LINE_FLOW(sequence));
	//now, verified if the name of line_flow returned is the same of the line_flow created
	source = gebr_geoxml_line_get_flow_source(GEBR_GEOXML_LINE_FLOW(sequence));
	g_assert_cmpstr("flow1",==,source);

	line_flow = gebr_geoxml_line_append_flow(line, "flow2");
	x = gebr_geoxml_line_get_flow(line, &sequence, 1);
	g_assert_cmpint(x,==,0);
	//compare if line_flow created is equal a sequence passed by function get_flow
	g_assert(line_flow == GEBR_GEOXML_LINE_FLOW(sequence));
	//now, verified if the name of line_flow returned is the same of the line_flow created
	source = gebr_geoxml_line_get_flow_source(GEBR_GEOXML_LINE_FLOW(sequence));
	g_assert_cmpstr("flow2",==,source);

}

void test_gebr_geoxml_line_set_flow_source(void)
{
	GebrGeoXmlLineFlow *line_flow = NULL;
	GebrGeoXmlLine *line = NULL;
	const gchar *source;

	line = gebr_geoxml_line_new();
	line_flow = gebr_geoxml_line_append_flow(line, "flow1");
	source = gebr_geoxml_line_get_flow_source(line_flow);
	g_assert_cmpstr("flow1",==,source);

	gebr_geoxml_line_set_flow_source(line_flow, "foo");
	source = gebr_geoxml_line_get_flow_source(line_flow);
	g_assert_cmpstr("foo",==,source);
}

void test_gebr_geoxml_line_get_paths_number(void)
{
	GebrGeoXmlLine *line = NULL;
	//const gchar *path;
	glong n;

	n = gebr_geoxml_line_get_paths_number(line);
	g_assert_cmpint(n,==,-1);

	line = gebr_geoxml_line_new();
	n = gebr_geoxml_line_get_paths_number(line);
	g_assert_cmpint(n,==,0);

	gebr_geoxml_line_append_path(line, "path1");
	n = gebr_geoxml_line_get_paths_number(line);
	g_assert_cmpint(n,==,1);
}

void test_gebr_geoxml_line_get_path(void)
{
	GebrGeoXmlLine *line = NULL;
	GebrGeoXmlLinePath *line_path;
	GebrGeoXmlSequence *sequence = NULL;
	int x;
	const gchar *path;

	line = gebr_geoxml_line_new();

	x = gebr_geoxml_line_get_path(line, &sequence, 0);
	g_assert_cmpint(x,==,-5);

	line_path = gebr_geoxml_line_append_path(line, "path1");
	x = gebr_geoxml_line_get_path(line, &sequence, 0);
	g_assert_cmpint(x,==,0);
	path = gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(line_path));
	g_assert_cmpstr("path1",==,path);

	line_path = gebr_geoxml_line_append_path(line, "path2");
	x = gebr_geoxml_line_get_path(line, &sequence, 1);
	g_assert_cmpint(x,==,0);
	path = gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(line_path));
	g_assert_cmpstr("path2",==,path);
}

//void test_duplicate_paths(void)
//{
//	glong n;
//	GebrGeoXmlLine *line = NULL;
//
//	line = gebr_geoxml_line_new();
//	gebr_geoxml_line_append_path(line, "path1");
//	gebr_geoxml_line_append_path(line, "path1");
//
//	n = gebr_geoxml_line_get_paths_number(line);
//	g_assert_cmpint (n, ==, 1);
//
//	gebr_geoxml_line_append_path(line, "path-new");
//	n = gebr_geoxml_line_get_paths_number(line);
//	g_assert_cmpint (n, ==, 2);
//}

void test_gebr_geoxml_line_get_group(void)
{
	GebrGeoXmlLine *line = NULL;
	const gchar *group = "group", *receive, *test;
	gboolean is_fs;

	line = gebr_geoxml_line_new();
	gebr_geoxml_line_set_group(line, group, is_fs);
	receive = gebr_geoxml_line_get_group(line, &is_fs);
	g_assert_cmpstr(receive,==,group);
	test = gebr_geoxml_line_get_group_label(line);
	g_assert_cmpstr("group",==,test);
}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	gebr_dirs_set_dtd_dir(DTD_DIR);

	g_test_add_func("/libgebr/geoxml/line/get_flow_number",test_gebr_geoxml_line_get_flows_number);
	g_test_add_func("/libgebr/geoxml/line/get_flow",test_gebr_geoxml_line_get_flow);
	g_test_add_func("/libgebr/geoxml/line/set_flow_source",test_gebr_geoxml_line_set_flow_source);
	g_test_add_func("/libgebr/geoxml/line/get_paths_number",test_gebr_geoxml_line_get_paths_number);
	g_test_add_func("/libgebr/geoxml/line/get_paths",test_gebr_geoxml_line_get_path);
//	g_test_add_func("/libgebr/geoxml/line/duplicate_paths",test_duplicate_paths);
	g_test_add_func("/libgebr/geoxml/line/get_group",test_gebr_geoxml_line_get_group);

	return g_test_run();
}
