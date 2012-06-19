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
#include <glib-object.h>
#include <stdlib.h>

#include "../../date.h"
#include "document.h"
#include "object.h"
#include "error.h"
#include "flow.h"
#include "line.h"
#include "project.h"
#include "program-parameter.h"
#include "sequence.h"
#include "program.h"

typedef struct {
	GebrValidator *validator;
	GebrGeoXmlDocument *flow;
	GebrGeoXmlDocument *line;
	GebrGeoXmlDocument *proj;
} Fixture;

void
fixture_setup(Fixture *fixture, gconstpointer data)
{
	fixture->flow = GEBR_GEOXML_DOCUMENT(gebr_geoxml_flow_new());
	fixture->line = GEBR_GEOXML_DOCUMENT(gebr_geoxml_line_new());
	fixture->proj = GEBR_GEOXML_DOCUMENT(gebr_geoxml_project_new());
	fixture->validator = gebr_validator_new(&fixture->flow,
						&fixture->line,
						&fixture->proj);
}

void
fixture_add_loop(Fixture *fixture)
{
	GebrGeoXmlProgram *prog_loop;
	GebrGeoXmlDocument *loop;
	GebrGeoXmlParameter *iter_param = NULL;

	gebr_geoxml_document_load(&loop, TEST_DIR"/forloop.mnu", FALSE, NULL);
	gebr_geoxml_flow_get_program(GEBR_GEOXML_FLOW(loop), (GebrGeoXmlSequence**) &prog_loop, 0);
	gebr_geoxml_program_set_status(prog_loop, GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED);

	gebr_geoxml_program_control_set_n(prog_loop, "1", "1", "10");
	gebr_geoxml_flow_add_flow(GEBR_GEOXML_FLOW(fixture->flow), GEBR_GEOXML_FLOW(loop));
	iter_param = GEBR_GEOXML_PARAMETER(gebr_geoxml_document_get_dict_parameter(fixture->flow));
	g_assert(gebr_validator_insert(fixture->validator, iter_param, NULL, NULL));

	gebr_geoxml_object_unref(prog_loop);
	gebr_geoxml_document_free(loop);
	gebr_geoxml_object_unref(iter_param);
}

gboolean
fixture_change_iter_value (Fixture *fixture,
                           gchar *ini,
                           gchar *step,
                           gchar *n,
                           GError **error)
{
	GebrGeoXmlProgram *prog_loop;
	GebrGeoXmlProgramParameter *dict_iter;
	gchar *value = NULL;

	prog_loop = gebr_geoxml_flow_get_control_program((GebrGeoXmlFlow*)fixture->flow);

	if(!prog_loop) {
		fixture_add_loop(fixture);
		prog_loop = gebr_geoxml_flow_get_control_program((GebrGeoXmlFlow*)fixture->flow);
	}

	gebr_geoxml_program_control_set_n(prog_loop, step, ini, n);

	gebr_geoxml_flow_update_iter_dict_value((GebrGeoXmlFlow*)fixture->flow);
	dict_iter = GEBR_GEOXML_PROGRAM_PARAMETER(gebr_geoxml_document_get_dict_parameter(GEBR_GEOXML_DOCUMENT(fixture->flow)));

	value = gebr_geoxml_program_parameter_get_first_value(dict_iter, FALSE);
	gboolean retval = gebr_validator_change_value(fixture->validator, GEBR_GEOXML_PARAMETER(dict_iter), value, NULL, error);

	g_free(value);
	gebr_geoxml_object_unref(dict_iter);
	gebr_geoxml_object_unref(prog_loop);
	return retval;
}

void
fixture_teardown(Fixture *fixture, gconstpointer data)
{
	gebr_validator_free(fixture->validator);
	gebr_geoxml_document_free(fixture->flow);
	gebr_geoxml_document_free(fixture->line);
	gebr_geoxml_document_free(fixture->proj);
}


void test_gebr_geoxml_flow_server_get_and_set_group(void)
{
	gchar *type, *name;
	GebrGeoXmlFlow *flow = NULL;

	flow = gebr_geoxml_flow_new();
	gebr_geoxml_flow_server_get_group(flow, &type, &name);
	g_assert_cmpstr(type, ==, "group");
	g_assert_cmpstr(name, ==, "");
	g_free(type);
	g_free(name);

	gebr_geoxml_flow_server_set_group(flow, "abc", "def");
	gebr_geoxml_flow_server_get_group(flow, &type, &name);
	g_assert_cmpstr(type, ==, "abc");
	g_assert_cmpstr(name, ==, "def");
	g_free(type);
	g_free(name);

	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
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

	GebrGeoXmlCategory * cat = gebr_geoxml_flow_append_category(flow, "foo");
	gebr_geoxml_object_unref(cat);
	n = gebr_geoxml_flow_get_categories_number (flow);
	g_assert_cmpint (n, ==, 1);

	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
}

void test_duplicate_categories(void)
{
	gint n;
	GebrGeoXmlFlow *flow = NULL;
	GebrGeoXmlCategory * cat = NULL;

	flow = gebr_geoxml_flow_new ();

	cat = gebr_geoxml_flow_append_category(flow, "foo");
	gebr_geoxml_object_unref(cat);

	cat = gebr_geoxml_flow_append_category(flow, "foo");
	gebr_geoxml_object_unref(cat);

	n = gebr_geoxml_flow_get_categories_number (flow);
	g_assert_cmpint (n, ==, 1);

	cat = gebr_geoxml_flow_append_category(flow, "bar");
	n = gebr_geoxml_flow_get_categories_number (flow);
	gebr_geoxml_object_unref(cat);
	g_assert_cmpint (n, ==, 2);

	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
}

void test_gebr_geoxml_flow_get_and_set_date_last_run(void){
	const gchar *date;
	GebrGeoXmlFlow *flow = NULL;

	flow = gebr_geoxml_flow_new ();
	gebr_geoxml_flow_set_date_last_run(flow, "18/03/2011");
	date = gebr_geoxml_flow_get_date_last_run(flow);
	g_assert_cmpstr(date, ==, "18/03/2011");

	gebr_geoxml_flow_set_date_last_run(flow, "");
	date = gebr_geoxml_flow_get_date_last_run(flow);
	g_assert_cmpstr(date, ==, "");

	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
}

void test_gebr_geoxml_flow_server_get_and_set_date_last_run(void){
	const gchar *date;
	GebrGeoXmlFlow *flow = NULL;

	//Test if trying to get the date of a NULL flow will fail, as expected
	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		gebr_geoxml_flow_server_get_date_last_run(NULL);
		exit(0);
	}
	g_test_trap_assert_failed();

	// Test if trying to set the server date of a NULL flow will fail, as expected
	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		gebr_geoxml_flow_server_set_date_last_run(flow, "abc/def/ghi");
		exit(0);
	}
	g_test_trap_assert_failed();

	// Test if trying to set a NULL server date to a non-NULL flow will fail, as expected
	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		flow = gebr_geoxml_flow_new();
		gebr_geoxml_flow_server_set_date_last_run(flow, NULL);
		exit(0);
	}
	g_test_trap_assert_failed();

	flow = gebr_geoxml_flow_new ();
	date = gebr_geoxml_flow_server_get_date_last_run(flow);
	g_assert_cmpstr(date, ==, "");

	gebr_geoxml_flow_server_set_date_last_run(flow, "23/03/2011");
	date = gebr_geoxml_flow_server_get_date_last_run(flow);
	g_assert_cmpstr(date, ==, "23/03/2011");

	gebr_geoxml_flow_server_set_date_last_run(flow, "");
	date = gebr_geoxml_flow_server_get_date_last_run(flow);
	g_assert_cmpstr(date, ==, "");
}

void test_gebr_geoxml_flow_io_get_and_set_input(void){
	const gchar *path;
	GebrGeoXmlFlow *flow = NULL;

	// Test if trying to get the input path of a NULL flow will fail, as expected
	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		gebr_geoxml_flow_io_get_input(NULL);
		exit(0);
	}
	g_test_trap_assert_failed();

	// Test if trying to set the input path of a NULL flow will fail, as expected
	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		gebr_geoxml_flow_io_set_input(flow, "abc/def/ghi");
		exit(0);
	}
	g_test_trap_assert_failed();

	// Test if trying to set a NULL input path to a non-NULL flow will fail, as expected
	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		flow = gebr_geoxml_flow_new();
		gebr_geoxml_flow_io_set_input(flow, NULL);
		exit(0);
	}
	g_test_trap_assert_failed();

	// Test if the default path of a new flow is an empty string
	flow = gebr_geoxml_flow_new();
	path = gebr_geoxml_flow_io_get_input(flow);
	g_assert_cmpstr(path, ==, "");

	// Test if the path was correctly passed
	gebr_geoxml_flow_io_set_input(flow, "abc/def/ghi");
	path = gebr_geoxml_flow_io_get_input(flow);
	g_assert_cmpstr(path, ==, "abc/def/ghi");

	// Test if the path was correctly changed
	gebr_geoxml_flow_io_set_input(flow, "qwerty/asdfg");
	path = gebr_geoxml_flow_io_get_input(flow);
	g_assert_cmpstr(path, ==, "qwerty/asdfg");
}

void test_gebr_geoxml_flow_io_get_and_set_output(void){
	const gchar *path;
	GebrGeoXmlFlow *flow = NULL;

	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		gebr_geoxml_flow_io_get_output(NULL);
		exit(0);
	}
	g_test_trap_assert_failed();

	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		gebr_geoxml_flow_io_set_output(flow, "abc/def/ghi");
		exit(0);
	}
	g_test_trap_assert_failed();

	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		flow = gebr_geoxml_flow_new();
		gebr_geoxml_flow_io_set_output(flow, NULL);
		exit(0);
	}
	g_test_trap_assert_failed();

	flow = gebr_geoxml_flow_new();
	path = gebr_geoxml_flow_io_get_output(flow);
	g_assert_cmpstr(path, ==, "");

	gebr_geoxml_flow_io_set_output(flow, "abc/def/ghi");
	path = gebr_geoxml_flow_io_get_output(flow);
	g_assert_cmpstr(path, ==, "abc/def/ghi");

	gebr_geoxml_flow_io_set_output(flow, "qwerty/asdfg");
	path = gebr_geoxml_flow_io_get_output(flow);
	g_assert_cmpstr(path, ==, "qwerty/asdfg");
}

void test_gebr_geoxml_flow_io_get_and_set_error(void){
	const gchar *path;
	GebrGeoXmlFlow *flow = NULL;

	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		gebr_geoxml_flow_io_get_error(NULL);
		exit(0);
	}
	g_test_trap_assert_failed();

	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		gebr_geoxml_flow_io_set_error(flow, "abc/def/ghi");
		exit(0);
	}
	g_test_trap_assert_failed();

	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		flow = gebr_geoxml_flow_new();
		gebr_geoxml_flow_io_set_error(flow, NULL);
		exit(0);
	}
	g_test_trap_assert_failed();

	flow = gebr_geoxml_flow_new();
	path = gebr_geoxml_flow_io_get_error(flow);
	g_assert_cmpstr(path, ==, "");

	gebr_geoxml_flow_io_set_error(flow, "abc/def/ghi");
	path = gebr_geoxml_flow_io_get_error(flow);
	g_assert_cmpstr(path, ==, "abc/def/ghi");

	gebr_geoxml_flow_io_set_error(flow, "qwerty/asdfg");
	path = gebr_geoxml_flow_io_get_error(flow);
	g_assert_cmpstr(path, ==, "qwerty/asdfg");
}

void test_gebr_geoxml_flow_get_program(void){

	GebrGeoXmlFlow *flow = NULL;
	GebrGeoXmlSequence *program = NULL;
	int returned;

	returned = gebr_geoxml_flow_get_program(flow, &program, 0);
	g_assert_cmpint(returned, ==, GEBR_GEOXML_RETV_NULL_PTR);

	flow = gebr_geoxml_flow_new();
	program = GEBR_GEOXML_SEQUENCE(gebr_geoxml_flow_append_program(flow));
	gebr_geoxml_object_unref(program);
	returned = gebr_geoxml_flow_get_program(flow, &program, 1337);
	gebr_geoxml_object_unref(program);
	g_assert_cmpint(returned, ==, GEBR_GEOXML_RETV_INVALID_INDEX);

	returned = gebr_geoxml_flow_get_program(flow, &program, 0);
	gebr_geoxml_object_unref(program);
	g_assert_cmpint(returned, ==, GEBR_GEOXML_RETV_SUCCESS);

	returned = gebr_geoxml_flow_get_program(flow, &program, 1);
	gebr_geoxml_object_unref(program);
	g_assert_cmpint(returned, ==, GEBR_GEOXML_RETV_INVALID_INDEX);

	program = GEBR_GEOXML_SEQUENCE(gebr_geoxml_flow_append_program(flow));
	gebr_geoxml_object_unref(program);
	returned = gebr_geoxml_flow_get_program(flow, &program, 1);
	gebr_geoxml_object_unref(program);
	g_assert_cmpint(returned, ==, GEBR_GEOXML_RETV_SUCCESS);

	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
}

void test_gebr_geoxml_flow_get_programs_number(void){

	GebrGeoXmlFlow *flow = NULL;
	GebrGeoXmlProgram *program = NULL;
	glong number;

	number = gebr_geoxml_flow_get_programs_number(flow);
	g_assert_cmpint(number, ==, -1);

	flow = gebr_geoxml_flow_new();
	number = gebr_geoxml_flow_get_programs_number(flow);
	g_assert_cmpint(number, ==, 0);

	program = gebr_geoxml_flow_append_program(flow);
	gebr_geoxml_object_unref(program);
	number = gebr_geoxml_flow_get_programs_number(flow);
	g_assert_cmpint(number, ==, 1);

	program = gebr_geoxml_flow_append_program(flow);
	number = gebr_geoxml_flow_get_programs_number(flow);
	gebr_geoxml_object_unref(program);
	g_assert_cmpint(number, ==, 2);

	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
}

void test_gebr_geoxml_flow_get_category(void)
{

	GebrGeoXmlFlow *flow = NULL;
	GebrGeoXmlSequence *category = NULL;
	int returned;

	returned = gebr_geoxml_flow_get_category(flow, &category, 0);
	g_assert_cmpint(returned, ==, GEBR_GEOXML_RETV_NULL_PTR);

	flow = gebr_geoxml_flow_new();
	category = (GebrGeoXmlSequence*) gebr_geoxml_flow_append_category(flow,"qwerty");
	gebr_geoxml_object_unref(category);
	returned = gebr_geoxml_flow_get_category(flow, &category, 1337);
	g_assert_cmpint(returned, ==, GEBR_GEOXML_RETV_INVALID_INDEX);

	returned = gebr_geoxml_flow_get_category(flow, &category, 0);
	gebr_geoxml_object_unref(category);
	g_assert_cmpint(returned, ==, GEBR_GEOXML_RETV_SUCCESS);

	returned = gebr_geoxml_flow_get_category(flow, &category, 1);
	g_assert_cmpint(returned, ==, GEBR_GEOXML_RETV_INVALID_INDEX);

	category = (GebrGeoXmlSequence*) gebr_geoxml_flow_append_category(flow,"zxcvb");
	gebr_geoxml_object_unref(category);
	returned = gebr_geoxml_flow_get_category(flow, &category, 1);
	gebr_geoxml_object_unref(category);
	g_assert_cmpint(returned, ==, GEBR_GEOXML_RETV_SUCCESS);

	// Check if at failure (when flow is NULL), category will be set as NULL
	returned = gebr_geoxml_flow_get_category(NULL, &category, 0);
	g_assert_cmpint(returned, ==, GEBR_GEOXML_RETV_NULL_PTR);
	g_assert(category == NULL);

	// Check if at failure (when an invalid index is passed), category will be set as NULL
	category = (GebrGeoXmlSequence*) gebr_geoxml_flow_append_category(flow,"zxcvb");
	gebr_geoxml_object_unref(category);
	returned = gebr_geoxml_flow_get_category(flow, &category, 24957);
	gebr_geoxml_object_unref(category);
	g_assert_cmpint(returned, ==, GEBR_GEOXML_RETV_INVALID_INDEX);

	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
}

void test_gebr_geoxml_flow_append_revision(void){
	GebrGeoXmlFlow *flow = NULL;
	GebrGeoXmlRevision *revision = NULL;

	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		revision = gebr_geoxml_flow_append_revision(NULL, "1234asdf");
		exit(0);
	}
	g_test_trap_assert_failed();

	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {
		flow = gebr_geoxml_flow_new();
		revision = gebr_geoxml_flow_append_revision(flow, NULL);
		exit(0);
	}
	g_test_trap_assert_failed();
	flow = gebr_geoxml_flow_new();
	revision = gebr_geoxml_flow_append_revision(flow, "poiuyt");
	g_assert(revision != NULL);

}

void test_gebr_geoxml_flow_change_to_revision(void){

	GebrGeoXmlFlow *flow = NULL;
	GebrGeoXmlRevision *revision = NULL;
	gboolean boole;

	boole = gebr_geoxml_flow_change_to_revision(NULL, revision, NULL);
	g_assert(boole == FALSE);

	flow = gebr_geoxml_flow_new();
	boole = gebr_geoxml_flow_change_to_revision(flow, NULL, NULL);
	g_assert(boole == FALSE);

	revision = gebr_geoxml_flow_append_revision(flow, "asdf");
	boole = gebr_geoxml_flow_change_to_revision(flow, revision, NULL);
	gebr_geoxml_object_unref(revision);
	g_assert(boole == TRUE);

	revision = gebr_geoxml_flow_append_revision(flow, "kakarotto");
	boole = gebr_geoxml_flow_change_to_revision(flow, revision, NULL);
	gebr_geoxml_object_unref(revision);
	g_assert(boole == TRUE);

	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));

}
void test_gebr_geoxml_flow_get_and_set_revision_data(void){

	GebrGeoXmlFlow *flow = NULL;
	GebrGeoXmlRevision *revision = NULL;
	gchar *strdate, *strcomment, *strflow, *flow_xml, *now;

	// Test if it will fail when revision is passed as NULL
	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)) {

		gebr_geoxml_flow_set_revision_data(NULL, "theflow", "01/02/8000", "commented", NULL);
		exit(0);
	}
	g_test_trap_assert_failed();

	// Test if the comment passed on append is correct
	flow = gebr_geoxml_flow_new();
	revision = gebr_geoxml_flow_append_revision(flow, "comment");
	gebr_geoxml_flow_get_revision_data(revision, NULL, NULL, &strcomment, NULL);
	g_assert_cmpstr(strcomment, ==, "comment");

	// Convert @flow to a string
	gebr_geoxml_document_to_string(GEBR_GEOXML_DOCUMENT(flow), &flow_xml);

	// Get date
	now = gebr_iso_date();

	gebr_geoxml_flow_set_revision_data(revision, flow_xml, now, "commented", NULL);
	gebr_geoxml_flow_get_revision_data(revision, &strflow, &strdate, &strcomment, NULL);

	g_assert_cmpstr(strflow, ==, flow_xml);
	g_assert_cmpstr(strcomment, ==, "commented");

	// Convert @now date into the correct returned style and compare it
	g_assert_cmpstr(strdate, ==, gebr_localized_date(now));

}

void test_gebr_geoxml_flow_get_revision(void){

	GebrGeoXmlFlow *flow = NULL;
	GebrGeoXmlSequence *revision = NULL;
	int returned;

	// Now this function logs a critical warning in this
	// case. Change this test to conform to that.
	//returned = gebr_geoxml_flow_get_revision(flow, &revision, 0);
	//gebr_geoxml_object_unref(revision);
	//g_assert_cmpint(returned, ==, GEBR_GEOXML_RETV_NULL_PTR);

	flow = gebr_geoxml_flow_new();
	returned = gebr_geoxml_flow_get_revision(flow, &revision, 0);
	gebr_geoxml_object_unref(revision);
	g_assert_cmpint(returned, ==, GEBR_GEOXML_RETV_INVALID_INDEX);

	revision = (GebrGeoXmlSequence*) gebr_geoxml_flow_append_revision(flow,"commented");
	gebr_geoxml_object_unref(revision);
	returned = gebr_geoxml_flow_get_revision(flow, &revision, 0);
	gebr_geoxml_object_unref(revision);
	g_assert_cmpint(returned, ==, GEBR_GEOXML_RETV_SUCCESS);

	revision = (GebrGeoXmlSequence*) gebr_geoxml_flow_append_revision(flow,"brbr");
	gebr_geoxml_object_unref(revision);
	returned = gebr_geoxml_flow_get_revision(flow, &revision, 1);
	gebr_geoxml_object_unref(revision);
	g_assert_cmpint(returned, ==, GEBR_GEOXML_RETV_SUCCESS);

	// Checking if the last procedure didn't messed with first revision
	returned = gebr_geoxml_flow_get_revision(flow, &revision, 0);
	gebr_geoxml_object_unref(revision);
	g_assert_cmpint(returned, ==, GEBR_GEOXML_RETV_SUCCESS);

	// Now this function logs a critical warning in this
	// case. Change this test to conform to that.
	// Check if at failure (when flow is NULL), revision will be set as NULL
	// returned = gebr_geoxml_flow_get_revision(NULL, &revision, 0);
	// g_assert_cmpint(returned, ==, GEBR_GEOXML_RETV_NULL_PTR);
	// g_assert(revision == NULL);

	// Check if at failure (when an invalid index is passed), revision will be set as NULL
	revision = (GebrGeoXmlSequence*) gebr_geoxml_flow_append_revision(flow,"1234pnbmns");
	gebr_geoxml_object_unref(revision);
	returned = gebr_geoxml_flow_get_revision(flow, &revision, 2654);
	g_assert_cmpint(returned, ==, GEBR_GEOXML_RETV_INVALID_INDEX);
	g_assert(revision == NULL);
}

static void test_gebr_geoxml_flow_io_output_append_default(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new ();
	// Default value is FALSE
	g_assert (gebr_geoxml_flow_io_get_output_append (flow) == FALSE);
}

static void test_gebr_geoxml_flow_io_output_append(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new ();
	gebr_geoxml_flow_io_set_output_append (flow, TRUE);
	g_assert (gebr_geoxml_flow_io_get_output_append (flow) == TRUE);
}

static void test_gebr_geoxml_flow_io_error_append_default(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new ();
	// Default value is TRUE
	g_assert (gebr_geoxml_flow_io_get_error_append (flow) == TRUE);
}

static void test_gebr_geoxml_flow_io_error_append(void)
{
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new ();
	gebr_geoxml_flow_io_set_error_append (flow, TRUE);
	g_assert (gebr_geoxml_flow_io_get_error_append (flow) == TRUE);
}

static void test_gebr_geoxml_flow_is_parallelizable (void)
{
	GebrGeoXmlFlow *loop;
	GebrGeoXmlFlow *flow = gebr_geoxml_flow_new();
	GebrValidator *validator = gebr_validator_new((GebrGeoXmlDocument**)&flow, NULL, NULL);

	gebr_geoxml_document_load((GebrGeoXmlDocument**)&loop, TEST_DIR"/forloop.mnu", TRUE, NULL);

	g_assert (gebr_geoxml_flow_is_parallelizable(flow, validator) == FALSE);

	gebr_geoxml_flow_io_set_output(flow, "/home/eric/Desktop/teste_semLoop.txt");
	g_assert (gebr_geoxml_flow_is_parallelizable(flow, validator) == FALSE);

	gebr_geoxml_flow_add_flow(flow, loop);
	GebrGeoXmlProgram *forloop = gebr_geoxml_flow_get_control_program(flow);
	gebr_geoxml_program_set_status(forloop, GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED);
	gebr_geoxml_object_unref(forloop);

	GebrGeoXmlProgram *program = gebr_geoxml_flow_append_program(flow);
	gebr_geoxml_program_set_status(program, GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED);
	gebr_geoxml_program_set_stdout(program, TRUE);

	gebr_geoxml_flow_insert_iter_dict(flow);
	GebrGeoXmlParameter *iter_param = GEBR_GEOXML_PARAMETER(gebr_geoxml_document_get_dict_parameter(GEBR_GEOXML_DOCUMENT(flow)));
	gebr_validator_insert(validator, iter_param, NULL, NULL);
	g_assert (gebr_geoxml_flow_is_parallelizable(flow, validator) == FALSE);

	gebr_geoxml_flow_io_set_output(flow, "/home/eric/Desktop/teste_[iter].txt");
	g_assert (gebr_geoxml_flow_is_parallelizable(flow, validator) == TRUE);

	gebr_geoxml_flow_io_set_output(flow, "");
	g_assert (gebr_geoxml_flow_is_parallelizable(flow, validator) == TRUE);
}

void test_gebr_geoxml_flow_divide_flows (Fixture *fixture, gconstpointer data)
{
	GList *flows = NULL;
	gint *weigths = g_new(gint, 4);
	gchar *ini, *step;

	fixture_change_iter_value(fixture, "1", "1", "101", NULL);
	gebr_geoxml_flow_io_set_output(GEBR_GEOXML_FLOW(fixture->flow), "");

	// total de 101 passos
	weigths[0] = 40;
	weigths[1] = 30;
	weigths[2] = 20;
	weigths[3] = 11;

	flows = gebr_geoxml_flow_divide_flows(GEBR_GEOXML_FLOW(fixture->flow), fixture->validator, weigths, 4);

	GebrGeoXmlProgram *control1 = gebr_geoxml_flow_get_control_program(flows->data);
	GebrGeoXmlProgram *control2 = gebr_geoxml_flow_get_control_program(flows->next->data);
	GebrGeoXmlProgram *control3 = gebr_geoxml_flow_get_control_program(flows->next->next->data);
	GebrGeoXmlProgram *control4 = gebr_geoxml_flow_get_control_program(flows->next->next->next->data);


	gchar *n1 = gebr_geoxml_program_control_get_n(control1, &step, &ini);
	g_assert_cmpstr(n1, ==, "40");
	g_assert_cmpstr(ini, ==, "1");
	g_assert_cmpstr(step, ==, "1");
	g_free(ini);
	g_free(step);

	gchar *n2 = gebr_geoxml_program_control_get_n(control2, &step, &ini);
	g_assert_cmpstr(n2, ==, "30");
	g_assert_cmpstr(ini, ==, "41");
	g_assert_cmpstr(step, ==, "1");
	g_free(ini);
	g_free(step);

	gchar *n3 = gebr_geoxml_program_control_get_n(control3, &step, &ini);
	g_assert_cmpstr(n3, ==, "20");
	g_assert_cmpstr(ini, ==, "71");
	g_assert_cmpstr(step, ==, "1");
	g_free(ini);
	g_free(step);

	gchar *n4 = gebr_geoxml_program_control_get_n(control4, &step, &ini);
	g_assert_cmpstr(n4, ==, "11");
	g_assert_cmpstr(ini, ==, "91");
	g_assert_cmpstr(step, ==, "1");
	g_free(ini);
	g_free(step);


	gebr_geoxml_object_unref(control1);
	gebr_geoxml_object_unref(control2);
	gebr_geoxml_object_unref(control3);
	gebr_geoxml_object_unref(control4);
	g_free(n1);
	g_free(n2);
	g_free(n3);
	g_free(n4);
	g_free(weigths);
	g_list_free(flows);
}

void test_gebr_geoxml_flow_create_dot_code(void)
{
        gchar *retorno;
        const gchar *result = "digraph {\n"
        		"Paris->Berlim \n"
        		"Paris->Porto \n"
        		"Texas->Austin \n"
        		"Texas->Londres \n"
        		"ok3->teste \n"
        		"Ohio->Columbus \n"
        		"Ohio->Sorocaba \n"
        		"Ohio->Madrid \n"
        		"Sorocaba->Virginia \n"
        		"Sorocaba->Suzano \n"
        		"Sorocaba->Votorantim \n"
        		"Columbus->Texas \n"
        		"Columbus->Jundiai \n"
        		"Columbus->York \n"
        		"Votorantim->Paris \n"
        		"Virginia->Richmond \n"
        		"Virginia->Campinas \n"
        		"->teste4 \n"
			"}\n";

        GHashTable *hash = g_hash_table_new(g_str_hash, g_str_equal);
        g_hash_table_insert(hash, "Virginia", "Richmond,Campinas");
        g_hash_table_insert(hash, "Texas", "Austin,Londres");
        g_hash_table_insert(hash, "Ohio", "Columbus,Sorocaba,Madrid");
        g_hash_table_insert(hash, "Sorocaba", "Virginia,Suzano,Votorantim");
        g_hash_table_insert(hash, "Columbus", "Texas,Jundiai,York");
        g_hash_table_insert(hash, "Paris", "Berlim,Porto");
        g_hash_table_insert(hash, "Votorantim", "Paris");
        g_hash_table_insert(hash, "","teste,Porto");
        g_hash_table_insert(hash, "","teste2");
        g_hash_table_insert(hash, "","teste3");
        g_hash_table_insert(hash, "ok2","");
        g_hash_table_insert(hash, "ok3","teste");
        g_hash_table_insert(hash, "ok","");
        g_hash_table_insert(hash, "","teste4");

        retorno = gebr_geoxml_flow_create_dot_code(hash);
        g_assert_cmpstr(retorno, ==, result);
        g_hash_table_destroy(hash);
}

void test_gebr_geoxml_flow_revisions_get_root_id(void)
{
//gebr_geoxml_flow_revisions_get_root_id(GHashTable *hash)
	GList *values = NULL;
	gchar *result1, *result2, *result3, *result4;

	void list_free(gpointer data) {
		g_list_free(data);
	}

	GHashTable *hash1 = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, list_free);
	GHashTable *hash2 = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, list_free);
	GHashTable *hash3 = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, list_free);
	GHashTable *hash4 = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, list_free);

	values = NULL;
	values = g_list_prepend(values, "B");
	values = g_list_prepend(values, "C");
	g_hash_table_insert(hash1, "A", values);
	result1 = gebr_geoxml_flow_revisions_get_root_id(hash1);
        g_assert_cmpstr(result1, ==, "A");

	values = NULL;
	values = g_list_prepend(values, "B");
	g_hash_table_insert(hash2, "A", values);
	values = NULL;
	values = g_list_prepend(values, "C");
	g_hash_table_insert(hash2, "B", values);
	values = NULL;
	values = g_list_prepend(values, "D");
	g_hash_table_insert(hash2, "C", values);
	values = NULL;
	values = g_list_prepend(values, "E");
	g_hash_table_insert(hash2, "D", values);
	result2 = gebr_geoxml_flow_revisions_get_root_id(hash2);
        g_assert_cmpstr(result2, ==, "A");

	result3 = gebr_geoxml_flow_revisions_get_root_id(NULL);
        g_assert(result3 == NULL);

	values = g_list_prepend(values, "NULL");
	g_hash_table_insert(hash4, "A", values);
	result4 = gebr_geoxml_flow_revisions_get_root_id(hash4);
        g_assert_cmpstr(result4, ==, "A");


	g_hash_table_destroy(hash1);
	g_hash_table_destroy(hash2);
	g_hash_table_destroy(hash3);
	g_hash_table_destroy(hash4);
}

//static void test_gebr_geoxml_flow_calulate_weights(void)
//{
//	gdouble *weights;
//	gint n_servers = 4;
//
//	weights = gebr_geoxml_flow_calulate_weights(n_servers);
//
//	g_assert_cmpfloat(ABS(weights[0] - 0.4), <, 1E-8);
//	g_assert_cmpfloat(ABS(weights[1] - 0.3), <, 1E-8);
//	g_assert_cmpfloat(ABS(weights[2] - 0.2), <, 1E-8);
//	g_assert_cmpfloat(ABS(weights[3] - 0.1), <, 1E-8);
//}

int main(int argc, char *argv[])
{
	g_type_init();
	g_test_init(&argc, &argv, NULL);
	gebr_geoxml_init();

	g_test_add("/libgebr/geoxml/flow/divide_flow", Fixture, NULL,
		           fixture_setup,
		           test_gebr_geoxml_flow_divide_flows,
		           fixture_teardown);

	g_test_add_func("/libgebr/geoxml/flow/server_get_and_set_address", test_gebr_geoxml_flow_server_get_and_set_group);
	g_test_add_func("/libgebr/geoxml/flow/get_categories_number", test_gebr_geoxml_flow_get_categories_number);
	g_test_add_func("/libgebr/geoxml/flow/duplicate_categories", test_duplicate_categories);
	g_test_add_func("/libgebr/geoxml/flow/flow_get_and_set_date_last_run", test_gebr_geoxml_flow_get_and_set_date_last_run);
	g_test_add_func("/libgebr/geoxml/flow/flow_server_get_and_set_date_last_run", test_gebr_geoxml_flow_server_get_and_set_date_last_run);
	g_test_add_func("/libgebr/geoxml/flow/io_get_and_set_input", test_gebr_geoxml_flow_io_get_and_set_input);
	g_test_add_func("/libgebr/geoxml/flow/io_get_and_set_output", test_gebr_geoxml_flow_io_get_and_set_output);
	g_test_add_func("/libgebr/geoxml/flow/io_get_and_set_error", test_gebr_geoxml_flow_io_get_and_set_error);
	g_test_add_func("/libgebr/geoxml/flow/get_program", test_gebr_geoxml_flow_get_program);
	g_test_add_func("/libgebr/geoxml/flow/get_programs_number", test_gebr_geoxml_flow_get_programs_number);
	g_test_add_func("/libgebr/geoxml/flow/get_category", test_gebr_geoxml_flow_get_category);
	g_test_add_func("/libgebr/geoxml/flow/append_revision", test_gebr_geoxml_flow_append_revision);
//	g_test_add_func("/libgebr/geoxml/flow/change_to_revision", test_gebr_geoxml_flow_change_to_revision);
	g_test_add_func("/libgebr/geoxml/flow/get_and_set_revision_data", test_gebr_geoxml_flow_get_and_set_revision_data);
	g_test_add_func("/libgebr/geoxml/flow/get_revision", test_gebr_geoxml_flow_get_revision);
	g_test_add_func("/libgebr/geoxml/flow/io_output_append", test_gebr_geoxml_flow_io_output_append);
	g_test_add_func("/libgebr/geoxml/flow/io_output_append_default", test_gebr_geoxml_flow_io_output_append_default);
	g_test_add_func("/libgebr/geoxml/flow/io_error_append", test_gebr_geoxml_flow_io_error_append);
	g_test_add_func("/libgebr/geoxml/flow/io_error_append_default", test_gebr_geoxml_flow_io_error_append_default);
	g_test_add_func("/libgebr/geoxml/flow/is_parallelizable", test_gebr_geoxml_flow_is_parallelizable);
//	g_test_add_func("/libgebr/geoxml/flow/calculate_weights", test_gebr_geoxml_flow_calulate_weights);
	g_test_add_func("/libgebr/geoxml/flow/gebr_geoxml_flow_create_dot_code", test_gebr_geoxml_flow_create_dot_code);
	g_test_add_func("/libgebr/geoxml/flow/gebr_geoxml_flow_revisions_get_root_id", test_gebr_geoxml_flow_revisions_get_root_id);
	gint ret = g_test_run();

	gebr_geoxml_finalize();
	return ret;
}
