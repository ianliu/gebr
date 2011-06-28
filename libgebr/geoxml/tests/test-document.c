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
#include "parameters.h"
#include "program.h"
#include "program-parameter.h"
#include "flow.h"
#include "line.h"
#include "project.h"
#include "error.h"
#include "gebr-expr.h"
#include "sequence.h"

void test_gebr_geoxml_document_load (void)
{
	GebrGeoXmlDocument *document;
	int value;

	value = gebr_geoxml_document_load(&document, NULL, FALSE, NULL);
	g_assert(value == GEBR_GEOXML_RETV_FILE_NOT_FOUND);

	value = gebr_geoxml_document_load(&document, TEST_DIR"/x", FALSE, NULL);
	g_assert(value == GEBR_GEOXML_RETV_FILE_NOT_FOUND);

	value = gebr_geoxml_document_load(&document, TEST_DIR"/test.mnu", FALSE, NULL);
	g_assert(value == GEBR_GEOXML_RETV_SUCCESS);
	gebr_geoxml_document_free(document);
}

void test_gebr_geoxml_document_get_version (void)
{
	GebrGeoXmlDocument *document = NULL;
	const gchar *version = "";

	// If \p document is NULL, returns NULL.
	version = gebr_geoxml_document_get_version(document);
	g_assert_cmpstr(version, ==, NULL);

	gebr_geoxml_document_load(&document, TEST_DIR"/test.mnu", FALSE, NULL);
	version = gebr_geoxml_document_get_version(document);
	g_assert_cmpstr(version, ==, "0.3.7");
	gebr_geoxml_document_free(document);
}

void test_gebr_geoxml_document_get_type (void)
{
	GebrGeoXmlDocument *document = NULL;
	GebrGeoXmlDocumentType type;

	// If \p document is NULL returns type FLOW
	type = gebr_geoxml_document_get_type(document);
	g_assert(type == GEBR_GEOXML_DOCUMENT_TYPE_FLOW);

	document = GEBR_GEOXML_DOCUMENT (gebr_geoxml_flow_new());
	type = gebr_geoxml_document_get_type(document);
	g_assert(type == GEBR_GEOXML_DOCUMENT_TYPE_FLOW);

	document = GEBR_GEOXML_DOCUMENT (gebr_geoxml_line_new());
	type = gebr_geoxml_document_get_type(document);
	g_assert(type == GEBR_GEOXML_DOCUMENT_TYPE_LINE);

	document = GEBR_GEOXML_DOCUMENT (gebr_geoxml_project_new());
	type = gebr_geoxml_document_get_type(document);
	g_assert(type == GEBR_GEOXML_DOCUMENT_TYPE_PROJECT);
}

void test_gebr_geoxml_document_get_filename (void)
{
	GebrGeoXmlDocument *document = NULL;
	const gchar *filename;

	// If \p document is NULL, returns NULL.
	filename = gebr_geoxml_document_get_filename(document);
	g_assert_cmpstr(filename, ==, NULL);

	document = GEBR_GEOXML_DOCUMENT (gebr_geoxml_flow_new());

	// If \p filename is not set, its value is "" (empty string).
	filename = gebr_geoxml_document_get_filename(document);
	g_assert_cmpstr(filename, ==, "");

	gebr_geoxml_document_set_filename(document, "test-filename");
	filename = gebr_geoxml_document_get_filename(document);
	g_assert_cmpstr(filename, ==, "test-filename");

	// If \p filename is NULL, nothing is done.
	gebr_geoxml_document_set_filename(document, NULL);
	filename = gebr_geoxml_document_get_filename(document);
	g_assert_cmpstr(filename, ==, "test-filename");

	gebr_geoxml_document_set_filename(document, "");
	filename = gebr_geoxml_document_get_filename(document);
	g_assert_cmpstr(filename, ==, "");
}

void test_gebr_geoxml_document_get_title (void)
{
	GebrGeoXmlDocument *document = NULL;
	const gchar *title;

	// If \p document is NULL, returns NULL.
	title = gebr_geoxml_document_get_title(document);
	g_assert_cmpstr(title, ==, NULL);

	document = GEBR_GEOXML_DOCUMENT (gebr_geoxml_flow_new());

	// If \p title is not set, its value is "" (empty string).
	title = gebr_geoxml_document_get_title(document);
	g_assert_cmpstr(title, ==, "");

	gebr_geoxml_document_set_title(document, "test-title");
	title = gebr_geoxml_document_get_title(document);
	g_assert_cmpstr(title, ==, "test-title");

	// If \p title is NULL, nothing is done.
	gebr_geoxml_document_set_title(document, NULL);
	title = gebr_geoxml_document_get_title(document);
	g_assert_cmpstr(title, ==, "test-title");

	gebr_geoxml_document_set_title(document, "");
	title = gebr_geoxml_document_get_title(document);
	g_assert_cmpstr(title, ==, "");
}

void test_gebr_geoxml_document_get_author (void)
{
	GebrGeoXmlDocument *document = NULL;
	const gchar *author;

	// If \p document is NULL, returns NULL.
	author = gebr_geoxml_document_get_author(document);
	g_assert_cmpstr(author, ==, NULL);

	document = GEBR_GEOXML_DOCUMENT (gebr_geoxml_flow_new());

	// If \p author is not set, its value is "" (empty string).
	author = gebr_geoxml_document_get_author(document);
	g_assert_cmpstr(author, ==, "");

	gebr_geoxml_document_set_author(document, "test-author");
	author = gebr_geoxml_document_get_author(document);
	g_assert_cmpstr(author, ==, "test-author");

	// If \p author is NULL, nothing is done.
	gebr_geoxml_document_set_author(document, NULL);
	author = gebr_geoxml_document_get_author(document);
	g_assert_cmpstr(author, ==, "test-author");

	gebr_geoxml_document_set_author(document, "");
	author = gebr_geoxml_document_get_author(document);
	g_assert_cmpstr(author, ==, "");
}

void test_gebr_geoxml_document_get_email (void)
{
	GebrGeoXmlDocument *document = NULL;
	const gchar *email;

	// If \p document is NULL, returns NULL.
	email = gebr_geoxml_document_get_email(document);
	g_assert_cmpstr(email, ==, NULL);

	document = GEBR_GEOXML_DOCUMENT (gebr_geoxml_flow_new());

	// If \p email was not set, its value is "" (empty string).
	email = gebr_geoxml_document_get_email(document);
	g_assert_cmpstr(email, ==, "");

	gebr_geoxml_document_set_email(document, "test-email");
	email = gebr_geoxml_document_get_email(document);
	g_assert_cmpstr(email, ==, "test-email");

	// If \p email is NULL, nothing is done.
	gebr_geoxml_document_set_email(document, NULL);
	email = gebr_geoxml_document_get_email(document);
	g_assert_cmpstr(email, ==, "test-email");

	gebr_geoxml_document_set_email(document, "");
	email = gebr_geoxml_document_get_email(document);
	g_assert_cmpstr(email, ==, "");
}

void test_gebr_geoxml_document_get_date_created (void)
{
	GebrGeoXmlDocument *document = NULL;
	const gchar *date_created;

	// If \p document is NULL, returns NULL.
	date_created = gebr_geoxml_document_get_date_created(document);
	g_assert_cmpstr(date_created, ==, NULL);

	document = GEBR_GEOXML_DOCUMENT (gebr_geoxml_flow_new());

	// If \p date was not set, its value is "" (empty string).
	date_created = gebr_geoxml_document_get_date_created(document);
	g_assert_cmpstr(date_created, ==, "");

	gebr_geoxml_document_set_date_created(document, "today");
	date_created = gebr_geoxml_document_get_date_created(document);
	g_assert_cmpstr(date_created, ==, "today");

	// If \p date is NULL, nothing is done.
	gebr_geoxml_document_set_date_created(document, NULL);
	date_created = gebr_geoxml_document_get_date_created(document);
	g_assert_cmpstr(date_created, ==, "today");

	gebr_geoxml_document_set_date_created(document, "");
	date_created = gebr_geoxml_document_get_date_created(document);
	g_assert_cmpstr(date_created, ==, "");
}

void test_gebr_geoxml_document_get_date_modified (void)
{
	GebrGeoXmlDocument *document = NULL;
	const gchar *date_modified;

	// If \p document is NULL, returns NULL.
	date_modified = gebr_geoxml_document_get_date_modified(document);
	g_assert_cmpstr(date_modified, ==, NULL);

	document = GEBR_GEOXML_DOCUMENT (gebr_geoxml_flow_new());

	// If \p date was not set, its value is "" (empty string).
	date_modified = gebr_geoxml_document_get_date_modified(document);
	g_assert_cmpstr(date_modified, ==, "");

	gebr_geoxml_document_set_date_modified(document, "today");
	date_modified = gebr_geoxml_document_get_date_modified(document);
	g_assert_cmpstr(date_modified, ==, "today");

	// If \p date is NULL, nothing is done.
	gebr_geoxml_document_set_date_modified(document, NULL);
	date_modified = gebr_geoxml_document_get_date_modified(document);
	g_assert_cmpstr(date_modified, ==, "today");

	gebr_geoxml_document_set_date_modified(document, "");
	date_modified = gebr_geoxml_document_get_date_modified(document);
	g_assert_cmpstr(date_modified, ==, "");
}

void test_gebr_geoxml_document_get_description (void)
{
	GebrGeoXmlDocument *document = NULL;
	const gchar *description;

	// If \p document is NULL, returns NULL.
	description = gebr_geoxml_document_get_description(document);
	g_assert_cmpstr(description, ==, NULL);

	document = GEBR_GEOXML_DOCUMENT (gebr_geoxml_flow_new());

	// If \p description was not set, its value is "" (empty string).
	description = gebr_geoxml_document_get_description(document);
	g_assert_cmpstr(description, ==, "");

	gebr_geoxml_document_set_description(document, "it is a document");
	description = gebr_geoxml_document_get_description(document);
	g_assert_cmpstr(description, ==, "it is a document");

	// If \p description is NULL, nothing is done.
	gebr_geoxml_document_set_description(document, NULL);
	description = gebr_geoxml_document_get_description(document);
	g_assert_cmpstr(description, ==, "it is a document");

	gebr_geoxml_document_set_description(document, "");
	description = gebr_geoxml_document_get_description(document);
	g_assert_cmpstr(description, ==, "");
}

void test_gebr_geoxml_document_get_help (void)
{
	GebrGeoXmlDocument *document = NULL;
	const gchar *help;

	// If \p document is NULL, returns NULL.
	help = gebr_geoxml_document_get_help(document);
	g_assert_cmpstr(help, ==, NULL);

	document = GEBR_GEOXML_DOCUMENT (gebr_geoxml_flow_new());

	// If \p help was not set, its value is "" (empty string).
	help = gebr_geoxml_document_get_help(document);
	g_assert_cmpstr(help, ==, "");

	gebr_geoxml_document_set_help(document, "]]>help accepts HTML");
	help = gebr_geoxml_document_get_help(document);
	g_assert_cmpstr(help, ==, "]]>help accepts HTML");

	// If \p help is NULL, nothing is done.
	gebr_geoxml_document_set_help(document, NULL);
	help = gebr_geoxml_document_get_help(document);
	g_assert_cmpstr(help, ==, "]]>help accepts HTML");

	gebr_geoxml_document_set_help(document, "");
	help = gebr_geoxml_document_get_help(document);
	g_assert_cmpstr(help, ==, "");
}

static void test_gebr_geoxml_document_merge_dict(void)
{
	GebrGeoXmlDocument *line;
	GebrGeoXmlDocument *flow;
	GebrGeoXmlParameters *line_params;
	GebrGeoXmlParameters *flow_params;
	GebrGeoXmlParameter *param;

	line = GEBR_GEOXML_DOCUMENT (gebr_geoxml_line_new ());
	flow = GEBR_GEOXML_DOCUMENT (gebr_geoxml_flow_new ());

	line_params = gebr_geoxml_document_get_dict_parameters (line);
	flow_params = gebr_geoxml_document_get_dict_parameters (flow);

	param = gebr_geoxml_parameters_append_parameter (line_params, GEBR_GEOXML_PARAMETER_TYPE_INT);
	gebr_geoxml_program_parameter_set_keyword (GEBR_GEOXML_PROGRAM_PARAMETER (param), "foo");

	param = gebr_geoxml_parameters_append_parameter (flow_params, GEBR_GEOXML_PARAMETER_TYPE_INT);
	gebr_geoxml_program_parameter_set_keyword (GEBR_GEOXML_PROGRAM_PARAMETER (param), "bar");

	gebr_geoxml_document_merge_dict (flow, line);
	gebr_geoxml_parameters_get_parameter (flow_params, (GebrGeoXmlSequence **)&param, 1);

	g_assert_cmpstr (gebr_geoxml_program_parameter_get_keyword (GEBR_GEOXML_PROGRAM_PARAMETER (param)), ==, "bar");

	gebr_geoxml_document_merge_dict (flow, line);
	g_assert_cmpint (gebr_geoxml_parameters_get_number (flow_params), ==, 2);
}

void test_gebr_geoxml_document_is_dictkey_defined(void)
{
	GebrGeoXmlDocument *line;
	GebrGeoXmlDocument *flow;
	GebrGeoXmlParameters *line_params;
	GebrGeoXmlParameters *flow_params;
	GebrGeoXmlParameter *param;
	gchar *value;

	line = GEBR_GEOXML_DOCUMENT (gebr_geoxml_line_new ());
	flow = GEBR_GEOXML_DOCUMENT (gebr_geoxml_flow_new ());

	line_params = gebr_geoxml_document_get_dict_parameters (line);
	flow_params = gebr_geoxml_document_get_dict_parameters (flow);

	param = gebr_geoxml_parameters_append_parameter (line_params, GEBR_GEOXML_PARAMETER_TYPE_INT);
	gebr_geoxml_program_parameter_set_keyword (GEBR_GEOXML_PROGRAM_PARAMETER (param), "foo");
	gebr_geoxml_program_parameter_set_string_value (GEBR_GEOXML_PROGRAM_PARAMETER (param), FALSE, "1");

	param = gebr_geoxml_parameters_append_parameter (flow_params, GEBR_GEOXML_PARAMETER_TYPE_INT);
	gebr_geoxml_program_parameter_set_keyword (GEBR_GEOXML_PROGRAM_PARAMETER (param), "bar");
	gebr_geoxml_program_parameter_set_string_value (GEBR_GEOXML_PROGRAM_PARAMETER (param), FALSE, "2");

	g_assert (gebr_geoxml_document_is_dictkey_defined ("foo", &value, flow, line, NULL) == TRUE);
	g_assert_cmpstr (value, ==, "1");
	g_free (value), value = NULL;

	g_assert (gebr_geoxml_document_is_dictkey_defined ("bar", &value, flow, line, NULL) == TRUE);
	g_assert_cmpstr (value, ==, "2");
	g_free (value), value = NULL;

	g_assert (gebr_geoxml_document_is_dictkey_defined ("baz", &value, flow, line, NULL) == FALSE);
	g_assert (value == NULL);
}

void test_gebr_geoxml_document_validate_expr(void)
{
	GError *error = NULL;
	GebrGeoXmlDocument *proj;
	GebrGeoXmlDocument *line;
	GebrGeoXmlDocument *flow;
	GebrGeoXmlParameters *proj_params;
	GebrGeoXmlParameters *line_params;
	GebrGeoXmlParameters *flow_params;
	GebrGeoXmlParameter *param;

	proj = GEBR_GEOXML_DOCUMENT (gebr_geoxml_project_new ());
	line = GEBR_GEOXML_DOCUMENT (gebr_geoxml_line_new ());
	flow = GEBR_GEOXML_DOCUMENT (gebr_geoxml_flow_new ());

	proj_params = gebr_geoxml_document_get_dict_parameters (proj);
	line_params = gebr_geoxml_document_get_dict_parameters (line);
	flow_params = gebr_geoxml_document_get_dict_parameters (flow);

	param = gebr_geoxml_parameters_append_parameter (proj_params, GEBR_GEOXML_PARAMETER_TYPE_INT);
	gebr_geoxml_program_parameter_set_keyword (GEBR_GEOXML_PROGRAM_PARAMETER (param), "foo");
	gebr_geoxml_program_parameter_set_string_value (GEBR_GEOXML_PROGRAM_PARAMETER (param), FALSE, "1");

	param = gebr_geoxml_parameters_append_parameter (line_params, GEBR_GEOXML_PARAMETER_TYPE_INT);
	gebr_geoxml_program_parameter_set_keyword (GEBR_GEOXML_PROGRAM_PARAMETER (param), "bar");
	gebr_geoxml_program_parameter_set_string_value (GEBR_GEOXML_PROGRAM_PARAMETER (param), FALSE, "2");

	param = gebr_geoxml_parameters_append_parameter (flow_params, GEBR_GEOXML_PARAMETER_TYPE_INT);
	gebr_geoxml_program_parameter_set_keyword (GEBR_GEOXML_PROGRAM_PARAMETER (param), "baz");
	gebr_geoxml_program_parameter_set_string_value (GEBR_GEOXML_PROGRAM_PARAMETER (param), FALSE, "3");

	gebr_geoxml_document_validate_expr ("foo+bar+baz", flow, line, proj, &error);
	g_assert_no_error (error);

	gebr_geoxml_document_validate_expr ("foo+bar+", flow, line, proj, &error);
	g_assert_error (error, gebr_expr_error_quark(), GEBR_EXPR_ERROR_SYNTAX);
	g_clear_error (&error);

	gebr_geoxml_document_validate_expr ("foo+bar+bob", flow, line, proj, &error);
	g_assert_error (error, gebr_expr_error_quark(), GEBR_EXPR_ERROR_UNDEFINED_VAR);
	g_clear_error (&error);

}

void test_gebr_geoxml_document_iter(void)
{
	GError *error = NULL;
	GebrGeoXmlDocument *line;
	GebrGeoXmlDocument *flow;
	GebrGeoXmlDocument *proj;
	GebrGeoXmlFlow *forloop;
	GebrGeoXmlProgram *loop;

	gebr_geoxml_document_load((GebrGeoXmlDocument**)&forloop, TEST_DIR"/forloop.mnu",FALSE,NULL);

	line = GEBR_GEOXML_DOCUMENT (gebr_geoxml_line_new ());
	flow = GEBR_GEOXML_DOCUMENT (gebr_geoxml_flow_new ());
	proj = GEBR_GEOXML_DOCUMENT (gebr_geoxml_project_new ());

	gebr_geoxml_flow_get_program(forloop,(GebrGeoXmlSequence **) &loop, 0);
	gebr_geoxml_program_set_status(loop, GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED);

	gebr_geoxml_flow_add_flow(GEBR_GEOXML_FLOW(flow),forloop);

	g_assert (gebr_geoxml_document_is_dictkey_defined ("iter", NULL, flow, line, NULL) == TRUE);

	gebr_geoxml_document_validate_expr ("5*iter", flow, line, proj, &error);
	g_assert_no_error (error);
}

void test_gebr_geoxml_document_no_iter(void)
{
	GebrGeoXmlDocument *line;
	GebrGeoXmlDocument *flow;
	GebrGeoXmlDocument *proj;
	GebrGeoXmlFlow *forloop;
	GebrGeoXmlProgram *loop;

	gebr_geoxml_document_load((GebrGeoXmlDocument**)&forloop, TEST_DIR"/forloop.mnu",FALSE,NULL);

	line = GEBR_GEOXML_DOCUMENT (gebr_geoxml_line_new ());
	flow = GEBR_GEOXML_DOCUMENT (gebr_geoxml_flow_new ());
	proj = GEBR_GEOXML_DOCUMENT (gebr_geoxml_project_new ());

	gebr_geoxml_flow_get_program(forloop,(GebrGeoXmlSequence **) &loop, 0);
	gebr_geoxml_program_set_status(loop, GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED);

	gebr_geoxml_flow_add_flow(GEBR_GEOXML_FLOW(flow),forloop);

	g_assert (gebr_geoxml_document_is_dictkey_defined ("iter", NULL, flow, line, NULL) == FALSE);
}

void test_gebr_geoxml_document_canonize_dict_parameters(void)
{
	GebrGeoXmlProject * proj;

	GebrGeoXmlSequence * parameters;

	GHashTable * keys_to_canonized = NULL;

	gint i = 0;
	gchar * keywords[] = {
		"A",
		"bA",
		"CDP EM METROS",
		"CDP EM METROS (m)",
		"CDP EM METROS  m ",
		"cd",
		"CdP EM METROs %m%",
		NULL};

	gchar * canonized[] = {
		"a",
		"ba",
		"cdp_em_metros",
		"cdp_em_metros__m_",
		"cdp_em_metros__m__1",
		"cd",
		"cdp_em_metros__m__2",
		NULL};

	proj = gebr_geoxml_project_new();

	for ( i = 0; keywords[i]; i++)
		gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
						      GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
						      keywords[i], "12");


	parameters = gebr_geoxml_parameters_get_first_parameter(
			gebr_geoxml_document_get_dict_parameters(
					GEBR_GEOXML_DOCUMENT(proj)));

	gebr_geoxml_document_canonize_dict_parameters(
			GEBR_GEOXML_DOCUMENT(proj),
			&keys_to_canonized);

	for (i = 0; parameters != NULL; gebr_geoxml_sequence_next(&parameters), i++)
	{
		const gchar * key = gebr_geoxml_program_parameter_get_keyword(
				GEBR_GEOXML_PROGRAM_PARAMETER(parameters));

		g_assert_cmpstr(key, ==, canonized[i]);
	}

	for (i = 0; keywords[i]; i++)
	{
		gchar * value = g_hash_table_lookup(keys_to_canonized, keywords[i]);
		g_assert_cmpstr(value, ==, canonized[i]);
		g_free(value);
	}

	//FIXME: Can't free this hash table! Tests fail!
	//Can't debug with 'libtool --mode=execute gdb test-document'
	//either!!
	//g_hash_table_unref(keys_to_canonized);

}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	gebr_geoxml_document_set_dtd_dir(DTD_DIR);

	g_test_add_func("/libgebr/geoxml/document/load", test_gebr_geoxml_document_load);
	g_test_add_func("/libgebr/geoxml/document/get_version", test_gebr_geoxml_document_get_version);
	g_test_add_func("/libgebr/geoxml/document/get_type", test_gebr_geoxml_document_get_type);
	g_test_add_func("/libgebr/geoxml/document/get_filename", test_gebr_geoxml_document_get_filename);
	g_test_add_func("/libgebr/geoxml/document/get_title", test_gebr_geoxml_document_get_title);
	g_test_add_func("/libgebr/geoxml/document/get_author", test_gebr_geoxml_document_get_author);
	g_test_add_func("/libgebr/geoxml/document/get_email", test_gebr_geoxml_document_get_email);
	g_test_add_func("/libgebr/geoxml/document/get_date_created", test_gebr_geoxml_document_get_date_created);
	g_test_add_func("/libgebr/geoxml/document/get_date_modified", test_gebr_geoxml_document_get_date_modified);
	g_test_add_func("/libgebr/geoxml/document/get_description", test_gebr_geoxml_document_get_description);
	g_test_add_func("/libgebr/geoxml/document/get_help", test_gebr_geoxml_document_get_help);
	g_test_add_func("/libgebr/geoxml/document/merge_dict", test_gebr_geoxml_document_merge_dict);
	g_test_add_func("/libgebr/geoxml/document/is_dictkey_defined", test_gebr_geoxml_document_is_dictkey_defined);
	g_test_add_func("/libgebr/geoxml/document/validate_expr", test_gebr_geoxml_document_validate_expr);
	g_test_add_func("/libgebr/geoxml/document/test_iter", test_gebr_geoxml_document_iter);
	g_test_add_func("/libgebr/geoxml/document/test_no_iter", test_gebr_geoxml_document_no_iter);
	g_test_add_func("/libgebr/geoxml/document/document_canonize_dict_parameters", test_gebr_geoxml_document_canonize_dict_parameters);

	return g_test_run();
}
