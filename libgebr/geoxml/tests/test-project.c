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

#include "project.h"
#include "../geoxml.h"
#include "../geoxml/xml.h"

void test_gebr_geoxml_project_remove_line(void)
{
	GebrGeoXmlProject *project = NULL;
	int check;

	project = gebr_geoxml_project_new();
	check = gebr_geoxml_project_has_line(project,"line1");
	g_assert_cmpint(check,==,0);

	gebr_geoxml_project_append_line(project,"line1");
	check = gebr_geoxml_project_has_line(project,"line1");
	g_assert_cmpint(check,==,1);
	check = gebr_geoxml_project_has_line(project,"line2");
	g_assert_cmpint(check,==,0);

	gebr_geoxml_project_remove_line(project,"line1");
	check = gebr_geoxml_project_has_line(project,"line1");
	g_assert_cmpint(check,==,0);

	check = gebr_geoxml_project_remove_line(project,"line1");
	g_assert_cmpint(check,==,0);
}

void test_gebr_geoxml_project_get_lines_number(void)
{
	GebrGeoXmlProject *project = NULL;
	glong n;

	n = gebr_geoxml_project_get_lines_number(project);
	g_assert_cmpint(n,==,-1);

	project = gebr_geoxml_project_new();
	n = gebr_geoxml_project_get_lines_number(project);
	g_assert_cmpint(n,==,0);

	gebr_geoxml_project_append_line(project,"line1");
	n = gebr_geoxml_project_get_lines_number(project);
	g_assert_cmpint(n,==,1);
}

void test_gebr_geoxml_project_get_line(void)
{
	GebrGeoXmlProject *project = NULL;
	GebrGeoXmlSequence *sequence = NULL;
	GebrGeoXmlProjectLine *project_line;
	const gchar *source;
	int check;

	project = gebr_geoxml_project_new();

	check = gebr_geoxml_project_get_line(project, &sequence, 0);
	g_assert_cmpint(check,==,-5);

	project_line = gebr_geoxml_project_append_line(project,"line1");
	check = gebr_geoxml_project_get_line(project, &sequence, 0);
	g_assert_cmpint(check,==,0);
	//compare if project_line created is equal a sequence passed by function get_line
	g_assert(project_line == GEBR_GEOXML_PROJECT_LINE(sequence));
	//now, verified if the name of project_line returned is the same of the get_line created
	source = gebr_geoxml_project_get_line_source(GEBR_GEOXML_PROJECT_LINE(sequence));
	g_assert_cmpstr(source,==,"line1");
}

void test_gebr_geoxml_project_set_line_source(void)
{
	GebrGeoXmlProject *project = NULL;
	GebrGeoXmlProjectLine *project_line;
	const gchar *source;

	project = gebr_geoxml_project_new();

	project_line = gebr_geoxml_project_append_line(project,"line1");
	source = gebr_geoxml_project_get_line_source(project_line);
	g_assert_cmpstr(source,==,"line1");

	gebr_geoxml_project_set_line_source(project_line,"line-diff");
	source = gebr_geoxml_project_get_line_source(project_line);
	g_assert_cmpstr(source,==,"line-diff");
}

void test_gebr_geoxml_project_get_line_from_source(void)
{
	GebrGeoXmlProject *project = NULL;
	GebrGeoXmlProjectLine *project_line, *receive;
	const gchar *source;

	project = gebr_geoxml_project_new();
	project_line = gebr_geoxml_project_append_line(project,"line1");

	receive = gebr_geoxml_project_get_line_from_source(project,"line2");
	g_assert(receive == NULL);

	receive = gebr_geoxml_project_get_line_from_source(project,"line1");
	g_assert(receive == project_line);
	source = gebr_geoxml_project_get_line_source(receive);
	g_assert_cmpstr(source,==,"line1");
}

void test_project_append_line()
{
	const gchar * value;
	GebrGeoXmlProject * project = gebr_geoxml_project_new();
	GebrGeoXmlProjectLine * line;

	line = gebr_geoxml_project_append_line(project, "test1");
	g_assert(line != NULL);

	value = __gebr_geoxml_get_attr_value((GdomeElement*)line, "source");
	g_assert_cmpstr(value, ==, "test1");

	line = gebr_geoxml_project_append_line(project, "test2");
	g_assert(line != NULL);

	value = __gebr_geoxml_get_attr_value((GdomeElement*)line, "source");
	g_assert_cmpstr(value, ==, "test2");

	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(project));
}

void test_project_get_lines_number()
{
	GebrGeoXmlProject * project = gebr_geoxml_project_new();

	glong null_project_length;
	null_project_length = gebr_geoxml_project_get_lines_number(NULL);
	g_assert_cmpint(null_project_length, ==, -1);

	glong empty_project_length;
	empty_project_length = gebr_geoxml_project_get_lines_number(project);
	g_assert_cmpint(empty_project_length, ==, 0);

	gebr_geoxml_project_append_line(project, "test1");
	gebr_geoxml_project_append_line(project, "test2");
	gebr_geoxml_project_append_line(project, "test3");

	glong three_lines_project_length;
	three_lines_project_length = gebr_geoxml_project_get_lines_number(project);
	g_assert_cmpint(three_lines_project_length, ==, 3);

	gebr_geoxml_project_append_line(project, "test4");
	gebr_geoxml_project_append_line(project, "test4");
	gebr_geoxml_project_append_line(project, "test4");

	/* FIXME: this test should work?
	glong duplicate_lines_project_length;
	duplicate_lines_project_length = gebr_geoxml_project_get_lines_number(project);
	g_assert_cmpint(duplicate_lines_project_length, ==, 4);
	*/

	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(project));
}

void test_project_has_line()
{
	gboolean value;
	GebrGeoXmlProject * project = gebr_geoxml_project_new();

	gebr_geoxml_project_append_line(project, "test1");
	value = gebr_geoxml_project_has_line(project, "test1");
	g_assert(value == TRUE);

	value = gebr_geoxml_project_has_line(project, "test2");
	g_assert(value == FALSE);

	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(project));
}


int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	gebr_geoxml_document_set_dtd_dir(DTD_DIR);

	g_test_add_func("/libgebr/geoxml/project/remove_line",test_gebr_geoxml_project_remove_line);
	g_test_add_func("/libgebr/geoxml/project/get_number_line",test_gebr_geoxml_project_get_lines_number);
	g_test_add_func("/libgebr/geoxml/project/set_line_source",test_gebr_geoxml_project_set_line_source);
	g_test_add_func("/libgebr/geoxml/project/get_line_from_source",test_gebr_geoxml_project_get_line_from_source);
	g_test_add_func("/libgebr/geoxml/project/append-line", test_project_append_line);
	g_test_add_func("/libgebr/geoxml/project/get-lines-number", test_project_get_lines_number);
	g_test_add_func("/libgebr/geoxml/project/has-line", test_project_has_line);

	return g_test_run();
}
