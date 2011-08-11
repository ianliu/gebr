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

#include "xml.h"
#include "object.h"
#include "document.h"
#include "flow.h"
#include "line.h"
#include "project.h"
#include "program.h"
#include "parameters.h"
#include "parameter_p.h"
#include "parameter_group.h"
#include "program-parameter.h"
#include "sequence.h"

static void
test_gebr_geoxml_leaks_new_flow(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
}

static void
test_gebr_geoxml_leaks_document_load(void)
{
	GebrGeoXmlDocument *doc;
	gebr_geoxml_document_load(&doc, TEST_DIR "/dict_test_flow.flw", TRUE, NULL);
	gebr_geoxml_document_free(doc);
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
test_gebr_geoxml_leaks_get_props(void)
{
	GebrGeoXmlDocument *doc;
	gebr_geoxml_document_load(&doc, TEST_DIR "/test.mnu", TRUE, NULL);

	g_free(gebr_geoxml_document_get_author(doc));
	g_free(gebr_geoxml_document_get_date_created(doc));
	g_free(gebr_geoxml_document_get_date_modified(doc));
	g_free(gebr_geoxml_document_get_description(doc));
	g_free(gebr_geoxml_document_get_email(doc));
	g_free(gebr_geoxml_document_get_help(doc));
	g_free(gebr_geoxml_document_get_title(doc));
	gebr_geoxml_document_get_type(doc);
	gebr_geoxml_document_get_filename(doc);

	gebr_geoxml_document_free(doc);
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
	gchar *key;
	GebrGeoXmlParameter *param;
	GebrGeoXmlParameters *params;
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();

// FIXME: Leaks because of gebr_geoxml_parameters_append_parameter
//	param = gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOC(flow), GEBR_GEOXML_PARAMETER_TYPE_STRING, "x", "y");

	params = gebr_geoxml_document_get_dict_parameters(GEBR_GEOXML_DOC(flow)); // OK
	gebr_geoxml_parameters_get_group(params); // OK
	__gebr_geoxml_parameters_group_check(params); // OK
	param = gebr_geoxml_parameters_append_parameter(params, GEBR_GEOXML_PARAMETER_TYPE_STRING);

	gebr_geoxml_program_parameter_set_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(param), "x");
	key = gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(param));
	g_assert_cmpstr(key, ==, "x");
	g_free(key);

	gebr_geoxml_program_parameter_set_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(param),
						      FALSE, "y");

	gebr_geoxml_object_unref(params);
	gebr_geoxml_object_unref(param);
	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
}

static void
test_gebr_geoxml_leaks_append_parameter(void)
{
	
	GdomeException exception;
	GebrGeoXmlParameter *param, *group;
	GebrGeoXmlParameters *params;
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();

	params = gebr_geoxml_document_get_dict_parameters(GEBR_GEOXML_DOC(flow));

	param = gebr_geoxml_parameters_append_parameter(params, GEBR_GEOXML_PARAMETER_TYPE_STRING);

	group = gebr_geoxml_parameters_append_parameter(params, GEBR_GEOXML_PARAMETER_TYPE_GROUP);

	GebrGeoXmlParameterType type = GEBR_GEOXML_PARAMETER_TYPE_STRING;
	GdomeElement *element;

	element = __gebr_geoxml_insert_new_element((GdomeElement *) params, "parameter", NULL);
	gdome_el_unref(__gebr_geoxml_insert_new_element(element, "label", NULL), &exception); //OK
	gdome_el_unref(__gebr_geoxml_parameter_insert_type((GebrGeoXmlParameter *) element, type), &exception); //OK
	gdome_el_unref(element, &exception);

	gebr_geoxml_object_unref(group);
	gebr_geoxml_object_unref(params);
	gebr_geoxml_object_unref(param);
	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
}

static void
test_gebr_geoxml_leaks_get_program(void)
{
	GebrGeoXmlFlow *flow;
	GebrGeoXmlProgram *prog;

	gebr_geoxml_document_load((GebrGeoXmlDocument **)&flow,
				  TEST_DIR "/test.mnu", TRUE, NULL);

	gebr_geoxml_flow_get_program(flow, (GebrGeoXmlSequence**)&prog, 0);
	gebr_geoxml_object_unref(prog);
	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
}

static void
test_gebr_geoxml_leaks_program_foreach(void)
{
	GebrGeoXmlFlow *flow;
	GebrGeoXmlProgram *prog;

	gebr_geoxml_document_load((GebrGeoXmlDocument **)&flow,
				  TEST_DIR "/test.mnu", TRUE, NULL);

	gboolean callback(GebrGeoXmlObject *object, gpointer user_data) {
		return TRUE;
	}

	gebr_geoxml_flow_get_program(flow, (GebrGeoXmlSequence**)&prog, 0);
	gebr_geoxml_program_foreach_parameter(prog, callback, NULL);
	gebr_geoxml_object_unref(prog);
	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
}

static void
test_gebr_geoxml_leaks_get_parameters(void)
{
	GebrGeoXmlFlow *flow;
	GebrGeoXmlProgram *prog;
	GebrGeoXmlParameters *params;

	gebr_geoxml_document_load((GebrGeoXmlDocument **)&flow,
				  TEST_DIR "/test.mnu", TRUE, NULL);

	gebr_geoxml_flow_get_program(flow, (GebrGeoXmlSequence**)&prog, 0);
	params = gebr_geoxml_program_get_parameters(prog);

	gebr_geoxml_object_unref(prog);
	gebr_geoxml_object_unref(params);
	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
}

static void
test_gebr_geoxml_leaks_parameter_get_type(void)
{
	GebrGeoXmlFlow *flow;
	GebrGeoXmlProgram *prog;
	GebrGeoXmlParameter *param;
	GebrGeoXmlParameters *params;

	gebr_geoxml_document_load((GebrGeoXmlDocument **)&flow,
				  TEST_DIR "/test.mnu", TRUE, NULL);

	gebr_geoxml_flow_get_program(flow, (GebrGeoXmlSequence**)&prog, 0);
	params = gebr_geoxml_program_get_parameters(prog);

	gebr_geoxml_parameters_get_parameter(params, (GebrGeoXmlSequence**)&param, 0);
	g_assert_cmpint(gebr_geoxml_parameter_get_type(param), ==, GEBR_GEOXML_PARAMETER_TYPE_FLAG);
	gebr_geoxml_object_unref(param);

	gebr_geoxml_parameters_get_parameter(params, (GebrGeoXmlSequence**)&param, 1);
	g_assert_cmpint(gebr_geoxml_parameter_get_type(param), ==, GEBR_GEOXML_PARAMETER_TYPE_FLOAT);
	gebr_geoxml_object_unref(param);

	gebr_geoxml_parameters_get_parameter(params, (GebrGeoXmlSequence**)&param, 2);
	g_assert_cmpint(gebr_geoxml_parameter_get_type(param), ==, GEBR_GEOXML_PARAMETER_TYPE_GROUP);
	gebr_geoxml_object_unref(param);

	gebr_geoxml_object_unref(prog);
	gebr_geoxml_object_unref(params);
	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
}

static void
test_gebr_geoxml_leaks_get_instance(void)
{
	GebrGeoXmlFlow *flow;
	GebrGeoXmlProgram *prog;
	GebrGeoXmlParameter *param;
	GebrGeoXmlParameters *params;
	GebrGeoXmlSequence *seq;

	gebr_geoxml_document_load((GebrGeoXmlDocument **)&flow,
				  TEST_DIR "/test.mnu", TRUE, NULL);

	gebr_geoxml_flow_get_program(flow, (GebrGeoXmlSequence**)&prog, 0);
	params = gebr_geoxml_program_get_parameters(prog);

	// The group is the third parameter
	gebr_geoxml_parameters_get_parameter(params, (GebrGeoXmlSequence**)&param, 2);

	gebr_geoxml_parameter_group_get_instance(GEBR_GEOXML_PARAMETER_GROUP(param), &seq, 0);

	GebrGeoXmlParameter *ref;
	GebrGeoXmlParameter *referencee;
	gebr_geoxml_parameters_get_parameter(GEBR_GEOXML_PARAMETERS(seq),
					     (GebrGeoXmlSequence**)&ref, 0);
	g_assert_cmpint(gebr_geoxml_parameter_get_type(ref), ==, GEBR_GEOXML_PARAMETER_TYPE_STRING);
	referencee = gebr_geoxml_parameter_get_referencee(ref);
	if (referencee)
		gebr_geoxml_object_unref(referencee);
	gebr_geoxml_object_unref(ref);

	gebr_geoxml_object_unref(seq);
	gebr_geoxml_object_unref(prog);
	gebr_geoxml_object_unref(param);
	gebr_geoxml_object_unref(params);
	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
}

static void
test_gebr_geoxml_flow_set_get_date_last_run(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();

	gebr_geoxml_flow_set_date_last_run(flow, "08/08/2011");
	gchar *str = gebr_geoxml_flow_get_date_last_run(flow);
	g_free(str);
	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
}

static void
test_gebr_geoxml_flow_server_set_get_address(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();

	gebr_geoxml_flow_server_set_address(flow, "adress-str");
	gchar *str = gebr_geoxml_flow_server_get_address(flow);
	g_free(str);
	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
}

static void
test_gebr_geoxml_flow_server_set_get_date_last_run(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();

	gebr_geoxml_flow_server_set_date_last_run(flow, "08/08/2011");
	gchar *str = gebr_geoxml_flow_server_get_date_last_run(flow);
	g_free(str);

	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
}

static void
test_gebr_geoxml_flow_io_set_get(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();

	gebr_geoxml_flow_io_set_input(flow, "input");
	gebr_geoxml_flow_io_set_output(flow, "output");
	gebr_geoxml_flow_io_set_error(flow, "error");

	g_free(gebr_geoxml_flow_io_get_input(flow));
	g_free(gebr_geoxml_flow_io_get_output(flow));
	g_free(gebr_geoxml_flow_io_get_error(flow));

	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
}

static void
test_gebr_geoxml_object_get_type(void)
{
	GebrGeoXmlDocument *doc;
	GebrGeoXmlSequence *program;

	gebr_geoxml_document_load(&doc, TEST_DIR "/test.mnu", TRUE, NULL);

	gebr_geoxml_flow_get_program(GEBR_GEOXML_FLOW(doc), &program, 0);
	gebr_geoxml_object_get_type(GEBR_GEOXML_OBJECT(program));

	gebr_geoxml_object_unref(program);
	gebr_geoxml_document_free(doc);
}

static void
test_gebr_geoxml_sequence_remove(void)
{
	GebrGeoXmlSequence *seq;
	GebrGeoXmlDocument *doc;

	gebr_geoxml_document_load(&doc, TEST_DIR "/test.mnu", TRUE, NULL);

	gebr_geoxml_flow_get_program(GEBR_GEOXML_FLOW(doc), &seq, 0);
	gebr_geoxml_sequence_remove(seq);

	gebr_geoxml_document_free(doc);
}

static void
test_gebr_geoxml_leaks_get_scope_and_required(void)
{
	GebrGeoXmlFlow *flow;
	GebrGeoXmlProgram *prog;
	GebrGeoXmlParameter *param;
	GebrGeoXmlParameters *params;

	gebr_geoxml_document_load((GebrGeoXmlDocument **)&flow,
				  TEST_DIR "/test.mnu", TRUE, NULL);

	gebr_geoxml_flow_get_program(flow, (GebrGeoXmlSequence**)&prog, 0);
	params = gebr_geoxml_program_get_parameters(prog);
	gebr_geoxml_parameters_get_parameter(params, (GebrGeoXmlSequence**)&param, 0);

	g_assert(gebr_geoxml_parameters_get_is_in_group(params) == FALSE);

	gebr_geoxml_parameter_get_scope(param);
	gebr_geoxml_program_parameter_get_required(GEBR_GEOXML_PROGRAM_PARAMETER(param));
	gebr_geoxml_object_unref(param);

	GebrGeoXmlSequence *seq;
	GebrGeoXmlParameter *ref;
	GebrGeoXmlParameters *template;

	gebr_geoxml_parameters_get_parameter(params, (GebrGeoXmlSequence**)&param, 2);
	template = gebr_geoxml_parameter_group_get_template(GEBR_GEOXML_PARAMETER_GROUP(param));

	g_assert(gebr_geoxml_parameters_get_is_in_group(template) == TRUE);

	g_assert(__gebr_geoxml_parameters_group_check(params) == TRUE);
	gebr_geoxml_object_unref(gebr_geoxml_parameters_append_parameter(template, GEBR_GEOXML_PARAMETER_TYPE_INT));

	GdomeException exc;
	GdomeElement *prev = __gebr_geoxml_previous_same_element((GdomeElement*)param);
	g_assert(prev != NULL);
	gdome_el_unref(prev, &exc);

	g_assert_cmpint(2, ==, __gebr_geoxml_get_element_index((GdomeElement*)param));

	gebr_geoxml_parameter_group_is_exclusive(GEBR_GEOXML_PARAMETER_GROUP(param));
	gebr_geoxml_parameter_group_get_instance(GEBR_GEOXML_PARAMETER_GROUP(param), &seq, 0);
	gebr_geoxml_parameters_get_parameter(GEBR_GEOXML_PARAMETERS(seq), (GebrGeoXmlSequence**)&ref, 0);

	gebr_geoxml_program_parameter_get_required(GEBR_GEOXML_PROGRAM_PARAMETER(param));

	gebr_geoxml_object_unref(template);
	gebr_geoxml_object_unref(ref);
	gebr_geoxml_object_unref(seq);
	gebr_geoxml_object_unref(prog);
	gebr_geoxml_object_unref(param);
	gebr_geoxml_object_unref(params);
	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
}

static void
test_gebr_geoxml_leaks_sequence_append_clone(void)
{
	GebrGeoXmlFlow *flow;
	GebrGeoXmlSequence *clone;
	GebrGeoXmlSequence *program;

	gebr_geoxml_document_load((GebrGeoXmlDocument **)&flow,
				  TEST_DIR "/test.mnu", TRUE, NULL);

	gebr_geoxml_flow_get_program(flow, &program, 0);
	clone = gebr_geoxml_sequence_append_clone(program);

	gebr_geoxml_object_unref(clone);
	gebr_geoxml_object_unref(program);
	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
}

static void
test_gebr_geoxml_leaks_parameters_reset_to_default(void)
{
	GebrGeoXmlFlow *flow;
	GebrGeoXmlSequence *program;
	GebrGeoXmlParameters *params;

	gebr_geoxml_document_load((GebrGeoXmlDocument **)&flow,
				  TEST_DIR "/test.mnu", TRUE, NULL);

	gebr_geoxml_flow_get_program(flow, &program, 0);
	params = gebr_geoxml_program_get_parameters(GEBR_GEOXML_PROGRAM(program));
	gebr_geoxml_parameters_reset_to_default(params);

	gebr_geoxml_object_unref(params);
	gebr_geoxml_object_unref(program);
	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
}

static void
test_gebr_geoxml_leaks_flow_revalidate(void)
{
	GebrGeoXmlDocument *flow;
	GebrGeoXmlDocument *line;
	GebrGeoXmlDocument *proj;
	GebrValidator *validator;

	gebr_geoxml_document_load(&flow, TEST_DIR "/test2.flw", TRUE, NULL);
	line = GEBR_GEOXML_DOCUMENT(gebr_geoxml_line_new());
	proj = GEBR_GEOXML_DOCUMENT(gebr_geoxml_project_new());
	validator = gebr_validator_new(&flow, &line, &proj);

	gebr_geoxml_flow_revalidate(GEBR_GEOXML_FLOW(flow), validator);

	gebr_validator_free(validator);
	gebr_geoxml_document_free(flow);
	gebr_geoxml_document_free(line);
	gebr_geoxml_document_free(proj);
}

int main(int argc, char *argv[])
{
	g_type_init();
	g_test_init(&argc, &argv, NULL);
	gebr_geoxml_init();

	gebr_geoxml_document_set_dtd_dir(DTD_DIR);

	g_test_add_func("/libgebr/geoxml/leaks/flow_new", test_gebr_geoxml_leaks_new_flow);
	g_test_add_func("/libgebr/geoxml/leaks/get_props", test_gebr_geoxml_leaks_get_props);
	g_test_add_func("/libgebr/geoxml/leaks/has_control", test_gebr_geoxml_leaks_has_control);
	g_test_add_func("/libgebr/geoxml/leaks/get_control_program", test_gebr_geoxml_leaks_get_control_program);
	g_test_add_func("/libgebr/geoxml/leaks/flow_add_flow", test_gebr_geoxml_leaks_flow_add_flow);
	g_test_add_func("/libgebr/geoxml/leaks/flow_foreach_parameter", test_gebr_geoxml_leaks_flow_foreach_parameter);
	g_test_add_func("/libgebr/geoxml/leaks/dict/set_keyword", test_gebr_geoxml_leaks_set_dict_keyword);
	g_test_add_func("/libgebr/geoxml/leaks/flow_set_get_data_last_run", test_gebr_geoxml_flow_set_get_date_last_run);
	g_test_add_func("/libgebr/geoxml/leaks/flow_server_set_get_address", test_gebr_geoxml_flow_server_set_get_address);
	g_test_add_func("/libgebr/geoxml/leaks/flow_server_set_get_date_last_run", test_gebr_geoxml_flow_server_set_get_date_last_run);
	g_test_add_func("/libgebr/geoxml/leaks/dict/append_parameter", test_gebr_geoxml_leaks_append_parameter);
	g_test_add_func("/libgebr/geoxml/leaks/program_foreach", test_gebr_geoxml_leaks_program_foreach);
	g_test_add_func("/libgebr/geoxml/leaks/get_program", test_gebr_geoxml_leaks_get_program);
	g_test_add_func("/libgebr/geoxml/leaks/flow_io_set_get", test_gebr_geoxml_flow_io_set_get);
	g_test_add_func("/libgebr/geoxml/leaks/get_parameters", test_gebr_geoxml_leaks_get_parameters);
	g_test_add_func("/libgebr/geoxml/leaks/parameter_get_type", test_gebr_geoxml_leaks_parameter_get_type);
	g_test_add_func("/libgebr/geoxml/leaks/get_instance", test_gebr_geoxml_leaks_get_instance);
	g_test_add_func("/libgebr/geoxml/leaks/object_get_type", test_gebr_geoxml_object_get_type);
	g_test_add_func("/libgebr/geoxml/leaks/sequence_remove", test_gebr_geoxml_sequence_remove);
	g_test_add_func("/libgebr/geoxml/leaks/get_scope_and_required", test_gebr_geoxml_leaks_get_scope_and_required);
	g_test_add_func("/libgebr/geoxml/leaks/document_load", test_gebr_geoxml_leaks_document_load);
	g_test_add_func("/libgebr/geoxml/leaks/sequence_append_clone", test_gebr_geoxml_leaks_sequence_append_clone);
	g_test_add_func("/libgebr/geoxml/leaks/parameters_reset_to_default", test_gebr_geoxml_leaks_parameters_reset_to_default);
	g_test_add_func("/libgebr/geoxml/leaks/flow_revalidate", test_gebr_geoxml_leaks_flow_revalidate);

	gint ret = g_test_run();
	gebr_geoxml_finalize();
	return ret;
}
