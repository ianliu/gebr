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

#include "object.h"
#include "document.h"
#include "flow.h"
#include "line.h"
#include "project.h"

void test_gebr_geoxml_object_get_type (void)
{
	GebrGeoXmlObject *object = NULL;
	GebrGeoXmlObjectType type = GEBR_GEOXML_OBJECT_TYPE_UNKNOWN;
//	GebrGeoXmlDocument *document;
//	GebrGeoXmlProgram *program;

	//Test if trying to get the type of a NULL object will fail, as expected
	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		type = gebr_geoxml_object_get_type(object);
		exit(0);
	}
	g_test_trap_assert_failed();

	object = GEBR_GEOXML_OBJECT (gebr_geoxml_flow_new());
	type = gebr_geoxml_object_get_type(object);
	g_assert(type == GEBR_GEOXML_OBJECT_TYPE_FLOW);

	object = GEBR_GEOXML_OBJECT (gebr_geoxml_project_new());
	type = gebr_geoxml_object_get_type(object);
	g_assert(type == GEBR_GEOXML_OBJECT_TYPE_PROJECT);

	object = GEBR_GEOXML_OBJECT (gebr_geoxml_line_new());
	type = gebr_geoxml_object_get_type(object);
	g_assert(type == GEBR_GEOXML_OBJECT_TYPE_LINE);

//	gebr_geoxml_document_load(&document, TEST_DIR"/test.mnu", FALSE, NULL);
//
//	gebr_geoxml_flow_get_program(document, &program, 0);
//	type = gebr_geoxml_object_get_type(program);
//	g_assert(type == GEBR_GEOXML_OBJECT_TYPE_PROGRAM);
//
//	object = gebr_geoxml_program_get_parameters(program);
//	type = gebr_geoxml_object_get_type(object);
//	g_assert(type == GEBR_GEOXML_OBJECT_TYPE_PARAMETERS);

}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/libgebr/geoxml/object/get_type", test_gebr_geoxml_object_get_type);

	return g_test_run();
}
