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

void test_gebr_geoxml_flow_server_get_and_set_address(void){
	const gchar *address;
	GebrGeoXmlFlow *flow = NULL;

	flow = gebr_geoxml_flow_new ();
	address = gebr_geoxml_flow_server_get_address(flow);
	g_assert_cmpstr(address, ==, "");

	gebr_geoxml_flow_server_set_address(flow, "abc/def");
	address = gebr_geoxml_flow_server_get_address(flow);
	g_assert_cmpstr(address, ==, "abc/def");

	gebr_geoxml_flow_server_set_address(flow, "asdf/fdsa");
	address = gebr_geoxml_flow_server_get_address(flow);
	g_assert_cmpstr(address, ==, "asdf/fdsa");

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

		gebr_geoxml_flow_set_revision_data(NULL, "theflow", "01/02/8000", "commented");
		exit(0);
	}
	g_test_trap_assert_failed();

	// Test if the comment passed on append is correct
	flow = gebr_geoxml_flow_new();
	revision = gebr_geoxml_flow_append_revision(flow, "comment");
	gebr_geoxml_flow_get_revision_data(revision, NULL, NULL, &strcomment);
	g_assert_cmpstr(strcomment, ==, "comment");

	// Convert @flow to a string
	gebr_geoxml_document_to_string(GEBR_GEOXML_DOCUMENT(flow), &flow_xml);

	// Get date
	now = gebr_iso_date();

	gebr_geoxml_flow_set_revision_data(revision, flow_xml, now, "commented");
	gebr_geoxml_flow_get_revision_data(revision, &strflow, &strdate, &strcomment);

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

	gebr_geoxml_flow_io_set_output(flow, "/home/eric/Desktop/teste_semLoop.txt" );
	g_assert (gebr_geoxml_flow_is_parallelizable(flow, validator) == FALSE);

	gebr_geoxml_flow_add_flow(flow, loop);
	gebr_geoxml_flow_insert_iter_dict(flow);
	GebrGeoXmlParameter *iter_param = GEBR_GEOXML_PARAMETER(gebr_geoxml_document_get_dict_parameter(GEBR_GEOXML_DOCUMENT(flow)));
	gebr_validator_insert(validator, iter_param, NULL, NULL);
	g_assert (gebr_geoxml_flow_is_parallelizable(flow, validator) == FALSE);

	gebr_geoxml_flow_io_set_output(flow, "/home/eric/Desktop/teste_[iter].txt");
	g_assert (gebr_geoxml_flow_is_parallelizable(flow, validator) == TRUE);

	gebr_geoxml_flow_io_set_output(flow, "");
	g_assert (gebr_geoxml_flow_is_parallelizable(flow, validator) == TRUE);
}

int main(int argc, char *argv[])
{
	g_type_init();
	g_test_init(&argc, &argv, NULL);
	gebr_geoxml_init();

	gebr_geoxml_document_set_dtd_dir(DTD_DIR);

	g_test_add_func("/libgebr/geoxml/flow/server_get_and_set_address", test_gebr_geoxml_flow_server_get_and_set_address);
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
	g_test_add_func("/libgebr/geoxml/flow/change_to_revision", test_gebr_geoxml_flow_change_to_revision);
	g_test_add_func("/libgebr/geoxml/flow/get_and_set_revision_data", test_gebr_geoxml_flow_get_and_set_revision_data);
	g_test_add_func("/libgebr/geoxml/flow/get_revision", test_gebr_geoxml_flow_get_revision);
	g_test_add_func("/libgebr/geoxml/flow/io_output_append", test_gebr_geoxml_flow_io_output_append);
	g_test_add_func("/libgebr/geoxml/flow/io_output_append_default", test_gebr_geoxml_flow_io_output_append_default);
	g_test_add_func("/libgebr/geoxml/flow/io_error_append", test_gebr_geoxml_flow_io_error_append);
	g_test_add_func("/libgebr/geoxml/flow/io_error_append_default", test_gebr_geoxml_flow_io_error_append_default);
	g_test_add_func("/libgebr/geoxml/flow/is_parallelizable", test_gebr_geoxml_flow_is_parallelizable);

	gint ret = g_test_run();
	gebr_geoxml_finalize();
	return ret;
}
