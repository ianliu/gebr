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
#include <gdome.h>

void test_gebr_geoxml_document_load (void)
{
	GebrGeoXmlDocument *document;
	int value;

	value = gebr_geoxml_document_load(&document, NULL, FALSE, NULL);
	g_assert(value == GEBR_GEOXML_RETV_FILE_NOT_FOUND);

	value = gebr_geoxml_document_load(&document, TEST_DIR"/x", FALSE, NULL);
	g_assert(value == GEBR_GEOXML_RETV_FILE_NOT_FOUND);

	value = gebr_geoxml_document_load(&document, TEST_DIR "/test.mnu", FALSE, NULL);
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
	gchar *help;

	// If \p document is NULL, returns NULL.
	help = gebr_geoxml_document_get_help(document);
	g_assert_cmpstr(help, ==, NULL);

	document = GEBR_GEOXML_DOCUMENT (gebr_geoxml_flow_new());

	// If \p help was not set, its value is "" (empty string).
	help = gebr_geoxml_document_get_help(document);
	g_assert_cmpstr(help, ==, "");
	g_free (help);

	gebr_geoxml_document_set_help(document, "]]>help accepts HTML");
	help = gebr_geoxml_document_get_help(document);
	g_assert_cmpstr(help, ==, "]]>help accepts HTML");
	g_free (help);

	// If \p help is NULL, nothing is done.
	gebr_geoxml_document_set_help(document, NULL);
	help = gebr_geoxml_document_get_help(document);
	g_assert_cmpstr(help, ==, "]]>help accepts HTML");
	g_free (help);

	gebr_geoxml_document_set_help(document, "");
	help = gebr_geoxml_document_get_help(document);
	g_assert_cmpstr(help, ==, "");
	g_free (help);
}

void test_gebr_geoxml_document_canonize_dict_parameters(void)
{
	GebrGeoXmlProject * proj;
	GebrGeoXmlProject * proj2;

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
		"cdp_em_metros__m",
		"cdp_em_metros__m_1",
		"cd",
		"cdp_em_metros__m_2",
		NULL};

	proj = gebr_geoxml_project_new();
	proj2 = gebr_geoxml_project_new();

	// Setting parameters for the first project.
	for ( i = 0; keywords[i]; i++)
		gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj),
						      GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
						      keywords[i], "12");

	// Setting parameters for the second project.
	int indices[] = {0,1,2,3,4,-1};
	for ( i = 0; indices[i] >= 0; i++)
	{
		gebr_geoxml_document_set_dict_keyword(GEBR_GEOXML_DOCUMENT(proj2),
						      GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
						      keywords[indices[i]], "12");

	}


	// Canonize the first project dict
	gebr_geoxml_document_canonize_dict_parameters(
			GEBR_GEOXML_DOCUMENT(proj),
			&keys_to_canonized);

	// Check canonized vars for the first dict
	parameters = gebr_geoxml_parameters_get_first_parameter(
			gebr_geoxml_document_get_dict_parameters(
					GEBR_GEOXML_DOCUMENT(proj)));

	for (i = 0; parameters != NULL; gebr_geoxml_sequence_next(&parameters), i++)
	{
		const gchar * key = gebr_geoxml_program_parameter_get_keyword(
				GEBR_GEOXML_PROGRAM_PARAMETER(parameters));

		g_assert_cmpstr(key, ==, canonized[i]);
	}

	for (i = 0; keywords[i]; i++)
	{
		// DO NOT free this value.
		const gchar * value = (const gchar *)g_hash_table_lookup(keys_to_canonized, keywords[i]);
		g_assert_cmpstr(value, ==, canonized[i]);
	}

	// Canonize the second project dict
	gebr_geoxml_document_canonize_dict_parameters(
			GEBR_GEOXML_DOCUMENT(proj2),
			&keys_to_canonized);

	// Check canonized vars for the second dict
	parameters = gebr_geoxml_parameters_get_first_parameter(
			gebr_geoxml_document_get_dict_parameters(
					GEBR_GEOXML_DOCUMENT(proj2)));

	for (i = 0; parameters != NULL; gebr_geoxml_sequence_next(&parameters), i++)
	{
		const gchar * key = gebr_geoxml_program_parameter_get_keyword(
				GEBR_GEOXML_PROGRAM_PARAMETER(parameters));

		g_assert_cmpstr(key, ==, canonized[indices[i]]);
	}

}

void test_gebr_geoxml_document_canonize_program_parameters(void)
{
	GebrGeoXmlDocument * flow = NULL;
	GebrGeoXmlParameters *flow_params;
	GebrGeoXmlSequence * program = NULL;
	GHashTable * keys_to_canonized = NULL;

	gebr_geoxml_document_load(&flow, TEST_DIR"/dict_test_flow.flw", TRUE, NULL);
	flow_params = gebr_geoxml_document_get_dict_parameters (flow);
	gebr_geoxml_flow_get_program(GEBR_GEOXML_FLOW(flow), &program, 0);
	gebr_geoxml_document_canonize_dict_parameters(flow, &keys_to_canonized);

	gebr_geoxml_program_foreach_parameter(
			GEBR_GEOXML_PROGRAM(program),
			gebr_geoxml_program_parameter_update_old_dict_value,
			keys_to_canonized);

	// Freeing this hash table also frees the values stored
	// at canonized_to_keys.
	gebr_geoxml_document_free(flow);

}

static void test_gebr_geoxml_document_merge_and_split_dicts(void)
{
	GebrGeoXmlDocument *flow = GEBR_GEOXML_DOCUMENT(gebr_geoxml_flow_new());
	GebrGeoXmlDocument *line = GEBR_GEOXML_DOCUMENT(gebr_geoxml_line_new());
	GebrGeoXmlDocument *proj = GEBR_GEOXML_DOCUMENT(gebr_geoxml_project_new());

	gebr_geoxml_document_set_dict_keyword(flow, GEBR_GEOXML_PARAMETER_TYPE_INT, "foo", "1");
	gebr_geoxml_document_set_dict_keyword(flow, GEBR_GEOXML_PARAMETER_TYPE_INT, "foo2", "10");
	gebr_geoxml_document_set_dict_keyword(line, GEBR_GEOXML_PARAMETER_TYPE_INT, "bar", "2");
	gebr_geoxml_document_set_dict_keyword(line, GEBR_GEOXML_PARAMETER_TYPE_INT, "bar2", "20");
	gebr_geoxml_document_set_dict_keyword(proj, GEBR_GEOXML_PARAMETER_TYPE_INT, "baz", "3");
	gebr_geoxml_document_set_dict_keyword(proj, GEBR_GEOXML_PARAMETER_TYPE_INT, "baz2", "30");

	gebr_geoxml_document_merge_dicts(NULL, flow, line, proj, NULL);

	gebr_geoxml_document_free(line);
	gebr_geoxml_document_free(proj);
	line = GEBR_GEOXML_DOCUMENT(gebr_geoxml_line_new());
	proj = GEBR_GEOXML_DOCUMENT(gebr_geoxml_project_new());

	gebr_geoxml_document_split_dict(flow, line, proj, NULL);

	GebrGeoXmlProgramParameter *pp;
	pp = GEBR_GEOXML_PROGRAM_PARAMETER(gebr_geoxml_document_get_dict_parameter(flow));
	g_assert_cmpstr(gebr_geoxml_program_parameter_get_keyword(pp), ==, "foo");
	gebr_geoxml_sequence_next((GebrGeoXmlSequence**)&pp);
	g_assert_cmpstr(gebr_geoxml_program_parameter_get_keyword(pp), ==, "foo2");
	gebr_geoxml_sequence_next((GebrGeoXmlSequence**)&pp);
	g_assert(pp == NULL);

	pp = GEBR_GEOXML_PROGRAM_PARAMETER(gebr_geoxml_document_get_dict_parameter(line));
	g_assert_cmpstr(gebr_geoxml_program_parameter_get_keyword(pp), ==, "bar");
	gebr_geoxml_sequence_next((GebrGeoXmlSequence**)&pp);
	g_assert_cmpstr(gebr_geoxml_program_parameter_get_keyword(pp), ==, "bar2");
	gebr_geoxml_sequence_next((GebrGeoXmlSequence**)&pp);
	g_assert(pp == NULL);

	pp = GEBR_GEOXML_PROGRAM_PARAMETER(gebr_geoxml_document_get_dict_parameter(proj));
	g_assert_cmpstr(gebr_geoxml_program_parameter_get_keyword(pp), ==, "baz");
	gebr_geoxml_sequence_next((GebrGeoXmlSequence**)&pp);
	g_assert_cmpstr(gebr_geoxml_program_parameter_get_keyword(pp), ==, "baz2");
	gebr_geoxml_sequence_next((GebrGeoXmlSequence**)&pp);
	g_assert(pp == NULL);

	gebr_geoxml_document_free(flow);
	gebr_geoxml_document_free(line);
	gebr_geoxml_document_free(proj);
}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	gebr_geoxml_document_set_dtd_dir(DTD_DIR);

	g_test_add_func("/libgebr/geoxml/document/document_canonize_program_parameters", test_gebr_geoxml_document_canonize_program_parameters);
	g_test_add_func("/libgebr/geoxml/document/merge_and_split_dicts", test_gebr_geoxml_document_merge_and_split_dicts);

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
	g_test_add_func("/libgebr/geoxml/document/document_canonize_dict_parameters", test_gebr_geoxml_document_canonize_dict_parameters);

	return g_test_run();
}
