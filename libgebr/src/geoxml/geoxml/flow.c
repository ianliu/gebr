/*   libgebr - GeBR Library
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

#include <gdome.h>

#include "../../date.h"

#include "flow.h"
#include "defines.h"
#include "xml.h"
#include "error.h"
#include "types.h"
#include "document.h"
#include "document_p.h"
#include "program.h"
#include "parameters.h"
#include "parameters_p.h"
#include "parameter_group.h"
#include "value_sequence.h"

/*
 * internal structures and funcionts
 */

struct geoxml_flow {
	GeoXmlDocument * document;
};

struct geoxml_category {
	GdomeElement * element;
};

struct geoxml_revision {
	GdomeElement * element;
};

/*
 * library functions.
 */

GeoXmlFlow *
geoxml_flow_new()
{
	GeoXmlDocument *	document;
	GdomeElement *		element;

	document = geoxml_document_new("flow", GEOXML_FLOW_VERSION);

	element = __geoxml_insert_new_element(geoxml_document_root_element(document), "io", NULL);
	__geoxml_insert_new_element(element, "input", NULL);
	__geoxml_insert_new_element(element, "output", NULL);
	__geoxml_insert_new_element(element, "error", NULL);
	__geoxml_insert_new_element(__geoxml_get_first_element(geoxml_document_root_element(document), "date"),
		"lastrun", NULL);

	return GEOXML_FLOW(document);
}

void
geoxml_flow_add_flow(GeoXmlFlow * flow, GeoXmlFlow * flow2)
{
	if (flow == NULL || flow2 == NULL)
		return;

	GdomeNodeList *		flow2_node_list;
	GdomeDOMString *	string;
	gulong			i, n;

	/* import each program from flow2 */
	string		= gdome_str_mkref("program");
	flow2_node_list	= gdome_el_getElementsByTagName(geoxml_document_root_element(GEOXML_DOC(flow2)), string, &exception);
	n		= gdome_nl_length(flow2_node_list, &exception);

	/* clear each copied program help */
	for (i = 0; i < n; ++i) {
		GdomeNode * new_node = gdome_doc_importNode((GdomeDocument*)flow,
			gdome_nl_item(flow2_node_list, i, &exception), TRUE, &exception);

		__geoxml_element_reassign_ids((GdomeElement*)new_node);
		gdome_el_insertBefore(geoxml_document_root_element(GEOXML_DOC(flow)), new_node, (GdomeNode*)
			__geoxml_get_first_element(geoxml_document_root_element(GEOXML_DOC(flow)), "revision"),
			&exception);

		geoxml_program_set_help((GeoXmlProgram*)new_node, "");
	}

	gdome_str_unref(string);
	gdome_nl_unref(flow2_node_list, &exception);
}

void
geoxml_flow_set_date_last_run(GeoXmlFlow * flow, const gchar * last_run)
{
	if (flow == NULL || last_run == NULL)
		return;
	__geoxml_set_tag_value(__geoxml_get_first_element(geoxml_document_root_element(GEOXML_DOC(flow)), "date"),
		"lastrun", last_run, __geoxml_create_TextNode);
}

void
geoxml_flow_io_set_input(GeoXmlFlow * flow, const gchar * input)
{
	if (flow == NULL || input == NULL)
		return;
	__geoxml_set_tag_value(__geoxml_get_first_element(geoxml_document_root_element(GEOXML_DOC(flow)), "io"),
		"input", input, __geoxml_create_TextNode);
}

void
geoxml_flow_io_set_output(GeoXmlFlow * flow, const gchar * output)
{
	if (flow == NULL || output == NULL)
		return;
	__geoxml_set_tag_value(__geoxml_get_first_element(geoxml_document_root_element(GEOXML_DOC(flow)), "io"),
		"output", output, __geoxml_create_TextNode);
}

void
geoxml_flow_io_set_error(GeoXmlFlow * flow, const gchar * error)
{
	if (flow == NULL || error == NULL)
		return;
	__geoxml_set_tag_value(__geoxml_get_first_element(geoxml_document_root_element(GEOXML_DOC(flow)), "io"),
		"error", error, __geoxml_create_TextNode);
}

const gchar *
geoxml_flow_get_date_last_run(GeoXmlFlow * flow)
{
	if (flow == NULL)
		return NULL;
	return __geoxml_get_tag_value(__geoxml_get_first_element(geoxml_document_root_element(GEOXML_DOC(flow)), "date"), "lastrun");
}

const gchar *
geoxml_flow_io_get_input(GeoXmlFlow * flow)
{
	if (flow == NULL)
		return NULL;
	return __geoxml_get_tag_value(__geoxml_get_first_element(geoxml_document_root_element(GEOXML_DOC(flow)), "io"), "input");
}

const gchar *
geoxml_flow_io_get_output(GeoXmlFlow * flow)
{
	if (flow == NULL)
		return NULL;
	return __geoxml_get_tag_value(__geoxml_get_first_element(geoxml_document_root_element(GEOXML_DOC(flow)), "io"), "output");
}

const gchar *
geoxml_flow_io_get_error(GeoXmlFlow * flow)
{
	if (flow == NULL)
		return NULL;
	return __geoxml_get_tag_value(__geoxml_get_first_element(geoxml_document_root_element(GEOXML_DOC(flow)), "io"), "error");
}

GeoXmlParameters *
geoxml_flow_get_dict_parameters(GeoXmlFlow * flow)
{
	if (flow == NULL)
		return NULL;
	return __geoxml_get_first_element(__geoxml_get_first_element(
		geoxml_document_root_element(GEOXML_DOC(flow)), "dict"), "parameters");
}

GeoXmlProgram *
geoxml_flow_append_program(GeoXmlFlow * flow)
{
	GdomeElement *	element;

	element = __geoxml_insert_new_element(
		geoxml_document_root_element(GEOXML_DOC(flow)), "program",
		__geoxml_get_first_element(geoxml_document_root_element(GEOXML_DOC(flow)), "revision"));

	/* elements/attibutes */
	geoxml_program_set_stdin((GeoXmlProgram*)element, FALSE);
	geoxml_program_set_stdout((GeoXmlProgram*)element, FALSE);
	geoxml_program_set_stderr((GeoXmlProgram*)element, FALSE);
	geoxml_program_set_status((GeoXmlProgram*)element, "unconfigured");
	__geoxml_insert_new_element(element, "menu", NULL);
	geoxml_program_set_menu((GeoXmlProgram*)element, geoxml_document_get_filename(GEOXML_DOC(flow)), -1);
	__geoxml_insert_new_element(element, "title", NULL);
	__geoxml_insert_new_element(element, "binary", NULL);
	__geoxml_insert_new_element(element, "description", NULL);
	__geoxml_insert_new_element(element, "help", NULL);
	__geoxml_insert_new_element(element, "url", NULL);
	__geoxml_parameters_append_new(element);

	return (GeoXmlProgram*)element;
}

int
geoxml_flow_get_program(GeoXmlFlow * flow, GeoXmlSequence ** program, gulong index)
{
	if (flow == NULL) {
		*program = NULL;
		return GEOXML_RETV_NULL_PTR;
	}

	*program = (GeoXmlSequence*)__geoxml_get_element_at(
		geoxml_document_root_element(GEOXML_DOC(flow)), "program", index, FALSE);

	return (*program == NULL)
		? GEOXML_RETV_INVALID_INDEX
		: GEOXML_RETV_SUCCESS;
}

glong
geoxml_flow_get_programs_number(GeoXmlFlow * flow)
{
	if (flow == NULL)
		return -1;
	return __geoxml_get_elements_number(geoxml_document_root_element(GEOXML_DOC(flow)), "program");
}

GeoXmlCategory *
geoxml_flow_append_category(GeoXmlFlow * flow, const gchar * name)
{
	if (flow == NULL || name == NULL)
		return NULL;

	GeoXmlCategory *	category;

	category = (GeoXmlCategory*)__geoxml_insert_new_element(
		geoxml_document_root_element(GEOXML_DOC(flow)), "category",
		__geoxml_get_first_element(geoxml_document_root_element(GEOXML_DOC(flow)), "io"));
	geoxml_value_sequence_set(GEOXML_VALUE_SEQUENCE(category), name);

	return category;
}

int
geoxml_flow_get_category(GeoXmlFlow * flow, GeoXmlSequence ** category, gulong index)
{
	if (flow == NULL) {
		*category = NULL;
		return GEOXML_RETV_NULL_PTR;
	}

	*category = (GeoXmlSequence*)__geoxml_get_element_at(
		geoxml_document_root_element(GEOXML_DOC(flow)), "category", index, FALSE);

	return (*category == NULL)
		? GEOXML_RETV_INVALID_INDEX
		: GEOXML_RETV_SUCCESS;
}

glong
geoxml_flow_get_categories_number(GeoXmlFlow * flow)
{
	if (flow == NULL)
		return -1;
	return __geoxml_get_elements_number(geoxml_document_root_element(GEOXML_DOC(flow)), "category");
}

gboolean
geoxml_flow_change_to_revision(GeoXmlFlow * flow, GeoXmlRevision * revision)
{
	if (flow == NULL || revision == NULL)
		return FALSE;

	GeoXmlDocument *	revision_flow;
	GeoXmlSequence *	first_revision;
	GdomeElement *		child;

	if (geoxml_document_load_buffer(&revision_flow,
	__geoxml_get_element_value((GdomeElement*)revision)))
		return FALSE;

	geoxml_flow_get_revision(flow, &first_revision, 0);
	/* remove all elements till first_revision
	 * WARNING: for this implementation to work revision must be
	 * the last child of flow. Be careful when changing the DTD!
	 */
	child = __geoxml_get_first_element(geoxml_document_root_element(GEOXML_DOCUMENT(flow)), "*");
	while (child != NULL) {
		GdomeElement *	aux;

		if (child == (GdomeElement*)first_revision)
			break;
		aux = __geoxml_next_element(child);
		gdome_el_removeChild(
			geoxml_document_root_element(GEOXML_DOCUMENT(flow)),
			(GdomeNode*)child, &exception);
		child = aux;
	}

	/* add all revision elements */
	child = __geoxml_get_first_element(geoxml_document_root_element(GEOXML_DOCUMENT(revision_flow)), "*");
	for (; child != NULL; child = __geoxml_next_element(child)) {
		GdomeNode *	new_node;

		new_node = gdome_doc_importNode((GdomeDocument*)flow,
			(GdomeNode*)child, TRUE, &exception);
		gdome_el_insertBefore(geoxml_document_root_element(GEOXML_DOCUMENT(flow)),
			(GdomeNode*)new_node, (GdomeNode*)first_revision, &exception);
	}

	return TRUE;
}

GeoXmlRevision *
geoxml_flow_append_revision(GeoXmlFlow * flow, const gchar * comment)
{
	if (flow == NULL)
		return NULL;

	GeoXmlSequence *	first_revision;
	GeoXmlRevision *	revision;
	GeoXmlFlow *		revision_flow;
	gchar *			revision_xml;
	GeoXmlSequence *	i;

	geoxml_flow_get_revision(flow, &first_revision, 0);
	revision = (GeoXmlRevision*)__geoxml_insert_new_element(
		geoxml_document_root_element(GEOXML_DOCUMENT(flow)),
		"revision", (GdomeElement*)first_revision);
	__geoxml_set_attr_value((GdomeElement*)revision, "date", iso_date());
	__geoxml_set_attr_value((GdomeElement*)revision, "comment", comment);

	revision_flow = GEOXML_FLOW(geoxml_document_clone(GEOXML_DOCUMENT(flow)));
	geoxml_document_to_string(GEOXML_DOCUMENT(revision_flow), &revision_xml);
	/* remove revisions from the revision flow. */
	geoxml_flow_get_revision(revision_flow, &i, 0);
	while (i != NULL) {
		GeoXmlSequence *	aux;

		aux = (GeoXmlSequence*)__geoxml_next_element((GdomeElement*)i);
		gdome_el_removeChild(geoxml_document_root_element(GEOXML_DOCUMENT(revision_flow)),
			(GdomeNode*)i, &exception);

		i = aux;
	}

	/* save the xml */
	geoxml_document_to_string(GEOXML_DOCUMENT(revision_flow), &revision_xml);
	__geoxml_set_element_value((GdomeElement*)revision,
		revision_xml, __geoxml_create_CDATASection);

	/* frees */
	g_free(revision_xml);
	geoxml_document_free(GEOXML_DOCUMENT(revision_flow));

	return revision;
}

int
geoxml_flow_get_revision(GeoXmlFlow * flow, GeoXmlSequence ** revision, gulong index)
{
	if (flow == NULL) {
		*revision = NULL;
		return GEOXML_RETV_NULL_PTR;
	}

	*revision = (GeoXmlSequence*)__geoxml_get_element_at(
		geoxml_document_root_element(GEOXML_DOCUMENT(flow)),
		"revision", index, FALSE);

	return (*revision == NULL)
		? GEOXML_RETV_INVALID_INDEX
		: GEOXML_RETV_SUCCESS;
}

void
geoxml_flow_get_revision_data(GeoXmlRevision * revision, gchar ** flow, gchar ** date, gchar ** comment)
{
	if (revision == NULL)
		return;

	if (flow != NULL)
		*flow = (gchar*)__geoxml_get_element_value((GdomeElement*)revision);
	if (date != NULL)
		*date = (gchar*)localized_date(__geoxml_get_attr_value((GdomeElement*)revision, "date"));
	if (comment != NULL)
		*comment = (gchar*)__geoxml_get_attr_value((GdomeElement*)revision, "comment");
}

glong
geoxml_flow_get_revisions_number(GeoXmlFlow * flow)
{
	if (flow == NULL)
		return -1;
	return __geoxml_get_elements_number(geoxml_document_root_element(GEOXML_DOC(flow)), "revision");
}

