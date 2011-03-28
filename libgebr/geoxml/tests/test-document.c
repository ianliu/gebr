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
#include "flow.h"
#include "line.h"
#include "project.h"
#include "error.h"

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
	g_assert_cmpstr(version, ==, "0.3.5");
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

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

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

	return g_test_run();
}
