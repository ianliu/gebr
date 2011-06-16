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

#include <regex.h>
#include <gdome.h>
#include <string.h>
#include <stdio.h>
#include <glib/gi18n.h>

#include "../date.h"
#include "defines.h"
#include "document.h"
#include "document_p.h"
#include "error.h"
#include "flow.h"
#include "parameter.h"
#include "parameter_group.h"
#include "parameters.h"
#include "parameters_p.h"
#include "program.h"
#include "program-parameter.h"
#include "sequence.h"
#include "types.h"
#include "value_sequence.h"
#include "xml.h"

/*
 * internal structures and functions
 */

struct gebr_geoxml_flow {
	GebrGeoXmlDocument *document;
};

struct gebr_geoxml_category {
	GdomeElement *element;
};

struct gebr_geoxml_revision {
	GdomeElement *element;
};

/*
 * library functions.
 */

GebrGeoXmlFlow *gebr_geoxml_flow_new()
{
	GebrGeoXmlDocument *document;
	GdomeElement *io;
	GdomeElement *server;

	document = gebr_geoxml_document_new("flow", GEBR_GEOXML_FLOW_VERSION);

	server = __gebr_geoxml_insert_new_element(gebr_geoxml_document_root_element(document), "server", NULL);
	__gebr_geoxml_set_attr_value (server, "address", "");

	io = __gebr_geoxml_insert_new_element(server, "io", NULL);
	__gebr_geoxml_insert_new_element(io, "input", NULL);
	__gebr_geoxml_insert_new_element(io, "output", NULL);
	__gebr_geoxml_insert_new_element(io, "error", NULL);

	__gebr_geoxml_insert_new_element(server, "lastrun", NULL);

	__gebr_geoxml_insert_new_element(__gebr_geoxml_get_first_element
					 (gebr_geoxml_document_root_element(document), "date"), "lastrun", NULL);

	return GEBR_GEOXML_FLOW(document);
}

void gebr_geoxml_flow_add_flow(GebrGeoXmlFlow * flow, GebrGeoXmlFlow * flow2)
{
	GebrGeoXmlSequence *category;
	GdomeNodeList *flow2_node_list;
	GdomeDOMString *string;
	gulong i, n;
	gboolean has_control1, has_control2;

	g_return_if_fail (flow != NULL);
	g_return_if_fail (flow2 != NULL);

	has_control1 = gebr_geoxml_flow_has_control_program (flow);
	has_control2 = gebr_geoxml_flow_has_control_program (flow2);

	/* import each program from flow2 */
	string = gdome_str_mkref("program");
	flow2_node_list =
	    gdome_el_getElementsByTagName(gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow2)), string,
					  &exception);
	n = gdome_nl_length(flow2_node_list, &exception);

	for (i = 0; i < n; ++i) {
		GdomeNode *node;
		GdomeNode *new_node;
		GdomeElement *revision;
		GdomeElement *root_element;

		node = gdome_nl_item(flow2_node_list, i, &exception);
		new_node = gdome_doc_importNode((GdomeDocument *) flow, node, TRUE, &exception);

		root_element = gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow));
		revision = __gebr_geoxml_get_first_element(root_element, "revision");
		gdome_el_insertBefore_protected(root_element, new_node, (GdomeNode *)revision, &exception);
	}

	gebr_geoxml_flow_get_category (flow2, &category, 0);
	while (category) {
		const gchar *name;
		name = gebr_geoxml_value_sequence_get (GEBR_GEOXML_VALUE_SEQUENCE (category));
		gebr_geoxml_flow_append_category (flow, name);
		gebr_geoxml_sequence_next (&category);
	}

	// We are adding a control menu into flow.
	// Append the `iter' dictionary keyword.
	if (!has_control1 && has_control2)
	{
		GebrGeoXmlProgram * loop = gebr_geoxml_flow_get_control_program(flow2);
		GebrGeoXmlProgramStatus status = gebr_geoxml_program_get_status(loop);
		if (status == GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED)
			gebr_geoxml_flow_insert_iter_dict (flow);
	}

	gdome_str_unref(string);
	gdome_nl_unref(flow2_node_list, &exception);
}

void gebr_geoxml_flow_foreach_parameter(GebrGeoXmlFlow * flow, GebrGeoXmlCallback callback, gpointer user_data)
{
	if (flow == NULL)
		return;

	GebrGeoXmlSequence *program;

	gebr_geoxml_flow_get_program(flow, &program, 0);
	for (; program != NULL; gebr_geoxml_sequence_next(&program))
		gebr_geoxml_program_foreach_parameter(GEBR_GEOXML_PROGRAM(program), callback, user_data);
}

void gebr_geoxml_flow_set_date_last_run(GebrGeoXmlFlow * flow, const gchar * last_run)
{
	if (flow == NULL || last_run == NULL)
		return;
	__gebr_geoxml_set_tag_value(__gebr_geoxml_get_first_element
				    (gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow)), "date"), "lastrun",
				    last_run, __gebr_geoxml_create_TextNode);
}

const gchar *gebr_geoxml_flow_get_date_last_run(GebrGeoXmlFlow * flow)
{
	if (flow == NULL)
		return NULL;
	return
	    __gebr_geoxml_get_tag_value(__gebr_geoxml_get_first_element
					(gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow)), "date"), "lastrun");
}

void gebr_geoxml_flow_server_set_address(GebrGeoXmlFlow *flow, const gchar * address)
{
	GdomeElement *root;
	GdomeElement *server;
	root = gebr_geoxml_document_root_element (flow);
	server = __gebr_geoxml_get_first_element (root, "server");
	__gebr_geoxml_set_attr_value (server, "address", address);
}

const gchar *gebr_geoxml_flow_server_get_address(GebrGeoXmlFlow *flow)
{
	GdomeElement *root;
	GdomeElement *server;
	root = gebr_geoxml_document_root_element (flow);
	server = __gebr_geoxml_get_first_element (root, "server");
	return __gebr_geoxml_get_attr_value((GdomeElement *) server, "address");
}

void gebr_geoxml_flow_server_set_date_last_run(GebrGeoXmlFlow *flow, const gchar * date)
{
	GdomeElement *root;
	GdomeElement *server;

	g_return_if_fail (flow != NULL);
	g_return_if_fail (date != NULL);

	root = gebr_geoxml_document_root_element (flow);
	server = __gebr_geoxml_get_first_element (root, "server");
	__gebr_geoxml_set_tag_value ((GdomeElement *) server, "lastrun", date,
				     __gebr_geoxml_create_TextNode);
}

const gchar *gebr_geoxml_flow_server_get_date_last_run(GebrGeoXmlFlow *flow)
{
	GdomeElement *root;
	GdomeElement *server;

	g_return_val_if_fail (flow != NULL, NULL);

	root = gebr_geoxml_document_root_element (flow);
	server = __gebr_geoxml_get_first_element (root, "server");
	return __gebr_geoxml_get_tag_value ((GdomeElement *) server, "lastrun");
}

void gebr_geoxml_flow_io_set_input(GebrGeoXmlFlow *flow, const gchar *input)
{
	GdomeElement *root;
	GdomeElement *io;

	g_return_if_fail (flow != NULL);
	g_return_if_fail (input != NULL);

	root = gebr_geoxml_document_root_element (flow);
	io = __gebr_geoxml_get_first_element (root, "io");
	__gebr_geoxml_set_tag_value (io, "input", input, __gebr_geoxml_create_TextNode);
}

void gebr_geoxml_flow_io_set_output(GebrGeoXmlFlow *flow, const gchar *output)
{
	GdomeElement *root;
	GdomeElement *io;

	g_return_if_fail (flow != NULL);
	g_return_if_fail (output != NULL);

	root = gebr_geoxml_document_root_element (flow);
	io = __gebr_geoxml_get_first_element (root, "io");
	__gebr_geoxml_set_tag_value(io, "output", output, __gebr_geoxml_create_TextNode);
}

void gebr_geoxml_flow_io_set_error(GebrGeoXmlFlow *flow, const gchar *error)
{
	GdomeElement *root;
	GdomeElement *io;

	g_return_if_fail (flow != NULL);
	g_return_if_fail (error != NULL);

	root = gebr_geoxml_document_root_element (flow);
	io = __gebr_geoxml_get_first_element (root, "io");
	__gebr_geoxml_set_tag_value(io, "error", error, __gebr_geoxml_create_TextNode);
}

const gchar *gebr_geoxml_flow_io_get_input(GebrGeoXmlFlow *flow)
{
	GdomeElement *root;
	GdomeElement *io;

	g_return_val_if_fail (flow != NULL, NULL);

	root = gebr_geoxml_document_root_element (flow);
	io = __gebr_geoxml_get_first_element (root, "io");
	return __gebr_geoxml_get_tag_value (io, "input");
}

const gchar *gebr_geoxml_flow_io_get_output(GebrGeoXmlFlow * flow)
{
	GdomeElement *root;
	GdomeElement *io;

	g_return_val_if_fail (flow != NULL, NULL);

	root = gebr_geoxml_document_root_element (flow);
	io = __gebr_geoxml_get_first_element (root, "io");
	return __gebr_geoxml_get_tag_value(io, "output");
}

const gchar *gebr_geoxml_flow_io_get_error(GebrGeoXmlFlow * flow)
{
	GdomeElement *root;
	GdomeElement *io;

	g_return_val_if_fail (flow != NULL, NULL);

	root = gebr_geoxml_document_root_element (flow);
	io = __gebr_geoxml_get_first_element (root, "io");
	return __gebr_geoxml_get_tag_value(io, "error");
}

GebrGeoXmlProgram *gebr_geoxml_flow_append_program(GebrGeoXmlFlow * flow)
{
	GdomeElement *element;

	element = __gebr_geoxml_insert_new_element(gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow)), "program",
						   __gebr_geoxml_get_first_element(gebr_geoxml_document_root_element
										   (GEBR_GEOXML_DOC(flow)),
										   "revision"));

	/* elements/attibutes */
	gebr_geoxml_program_set_stdin((GebrGeoXmlProgram *) element, FALSE);
	gebr_geoxml_program_set_stdout((GebrGeoXmlProgram *) element, FALSE);
	gebr_geoxml_program_set_stderr((GebrGeoXmlProgram *) element, FALSE);
	gebr_geoxml_program_set_status((GebrGeoXmlProgram *) element, GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED);
	__gebr_geoxml_insert_new_element(element, "title", NULL);
	__gebr_geoxml_insert_new_element(element, "binary", NULL);
	__gebr_geoxml_insert_new_element(element, "description", NULL);
	__gebr_geoxml_insert_new_element(element, "help", NULL);
	__gebr_geoxml_insert_new_element(element, "url", NULL);
	__gebr_geoxml_parameters_append_new(element);

	return (GebrGeoXmlProgram *) element;
}

int gebr_geoxml_flow_get_program(GebrGeoXmlFlow * flow, GebrGeoXmlSequence ** program, gulong index)
{
	if (flow == NULL) {
		*program = NULL;
		return GEBR_GEOXML_RETV_NULL_PTR;
	}

	*program = (GebrGeoXmlSequence *)
	    __gebr_geoxml_get_element_at(gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow)), "program", index,
					 FALSE);

	return (*program == NULL)
	    ? GEBR_GEOXML_RETV_INVALID_INDEX : GEBR_GEOXML_RETV_SUCCESS;
}

glong gebr_geoxml_flow_get_programs_number(GebrGeoXmlFlow * flow)
{
	if (flow == NULL)
		return -1;
	return __gebr_geoxml_get_elements_number(gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow)), "program");
}

GebrGeoXmlProgram *gebr_geoxml_flow_get_first_mpi_program(GebrGeoXmlFlow * flow)
{
	if (flow == NULL)
		return NULL;

	GebrGeoXmlSequence *program;
	gebr_geoxml_flow_get_program(flow, &program, 0);
	for (; program != NULL; gebr_geoxml_sequence_next(&program))
		if (strlen(gebr_geoxml_program_get_mpi(GEBR_GEOXML_PROGRAM(program))))
			return GEBR_GEOXML_PROGRAM(program);

	return NULL;
}

GebrGeoXmlCategory *gebr_geoxml_flow_append_category(GebrGeoXmlFlow * flow, const gchar * name)
{
	GebrGeoXmlCategory *category;
	GebrGeoXmlSequence *sequence;

	g_return_val_if_fail (flow != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);

	gebr_geoxml_flow_get_category (flow, &sequence, 0);
	while (sequence) {
		const gchar *catname;
		catname = gebr_geoxml_value_sequence_get (GEBR_GEOXML_VALUE_SEQUENCE (sequence));
		if (strcmp (catname, name) == 0)
			return (GebrGeoXmlCategory *)(sequence);
		gebr_geoxml_sequence_next (&sequence);
	}

	category = (GebrGeoXmlCategory *)
	    __gebr_geoxml_insert_new_element(gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow)), "category",
					     __gebr_geoxml_get_first_element(gebr_geoxml_document_root_element
									     (GEBR_GEOXML_DOC(flow)), "server"));
	gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(category), name);

	return category;
}

int gebr_geoxml_flow_get_category(GebrGeoXmlFlow * flow, GebrGeoXmlSequence ** category, gulong index)
{
	if (flow == NULL) {
		*category = NULL;
		return GEBR_GEOXML_RETV_NULL_PTR;
	}

	*category = (GebrGeoXmlSequence *)
	    __gebr_geoxml_get_element_at(gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow)), "category", index,
					 FALSE);

	return (*category == NULL)
	    ? GEBR_GEOXML_RETV_INVALID_INDEX : GEBR_GEOXML_RETV_SUCCESS;
}

glong gebr_geoxml_flow_get_categories_number(GebrGeoXmlFlow * flow)
{
	if (flow == NULL)
		return -1;
	return __gebr_geoxml_get_elements_number(gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow)), "category");
}

gboolean gebr_geoxml_flow_change_to_revision(GebrGeoXmlFlow * flow, GebrGeoXmlRevision * revision, gboolean * report_merged)
{
	if (flow == NULL || revision == NULL)
		return FALSE;

	GString *merged_help;
	const gchar *revision_help;
	const gchar *flow_help;
	GebrGeoXmlDocument *revision_flow;
	GebrGeoXmlSequence *first_revision;
	GdomeElement *child;
	if(report_merged)
		*report_merged = FALSE;

	/* load document validating it */
	if (gebr_geoxml_document_load_buffer(&revision_flow,
					     __gebr_geoxml_get_element_value((GdomeElement *) revision)))
		return FALSE;

	flow_help = gebr_geoxml_document_get_help (GEBR_GEOXML_DOCUMENT (flow));
	revision_help = gebr_geoxml_document_get_help (revision_flow);
	merged_help = g_string_new (flow_help);
	if (strlen (revision_help) > 1) {
		gchar *revision_xml;
		regex_t regexp;
		regmatch_t matchptr[2];
		gssize start = -1;
		gssize length = -1;
		gssize flow_i = -1;

		regcomp(&regexp, "<body[^>]*>\\(.*\\?\\)<\\/body>", REG_ICASE);
		if (!regexec(&regexp, revision_help, 2, matchptr, 0)) {
			start = matchptr[1].rm_so;
			length = matchptr[1].rm_eo - matchptr[1].rm_so;
		}

		regcomp (&regexp, "</body>", REG_NEWLINE | REG_ICASE);
		if (!regexec (&regexp, flow_help, 1, matchptr, 0)) {
			flow_i = matchptr[0].rm_so;
		}

		if (start != -1 && length != -1 && flow_i != -1){
			gchar * date = NULL;
			gchar * comment = NULL;
			GString * revision_data;

			revision_data = g_string_new(NULL);

			gebr_geoxml_flow_get_revision_data(revision, NULL, &date, &comment);
			g_string_append_printf (revision_data, "<hr /><p>%s</p><p>%s</p>",
						comment, date);
			g_string_insert (merged_help, flow_i, revision_data->str); 
			g_string_insert_len (merged_help, flow_i + revision_data->len, revision_help + start, length); 
		}

		gebr_geoxml_document_set_help (revision_flow, "");
		gebr_geoxml_document_to_string (revision_flow, &revision_xml);
		__gebr_geoxml_set_element_value((GdomeElement *) revision, revision_xml, __gebr_geoxml_create_CDATASection);
		g_free (revision_xml);
		if(report_merged)
			*report_merged = TRUE;
	}

	gebr_geoxml_flow_get_revision(flow, &first_revision, 0);
	/* remove all elements till first_revision
	 * WARNING: for this implementation to work revision must be
	 * the last child of flow. Be careful when changing the DTD!
	 */
	child = __gebr_geoxml_get_first_element(gebr_geoxml_document_root_element(GEBR_GEOXML_DOCUMENT(flow)), "*");
	while (child != NULL) {
		GdomeElement *aux;

		if (child == (GdomeElement *) first_revision)
			break;
		aux = __gebr_geoxml_next_element(child);
		gdome_el_removeChild(gebr_geoxml_document_root_element(GEBR_GEOXML_DOCUMENT(flow)),
				     (GdomeNode *) child, &exception);
		child = aux;
	}

	/* add all revision elements */
	child = __gebr_geoxml_get_first_element(gebr_geoxml_document_root_element(GEBR_GEOXML_DOCUMENT(revision_flow)),
						"*");
	for (; child != NULL; child = __gebr_geoxml_next_element(child)) {
		GdomeNode *new_node;

		new_node = gdome_doc_importNode((GdomeDocument *) flow, (GdomeNode *) child, TRUE, &exception);
		gdome_el_insertBefore_protected(gebr_geoxml_document_root_element(GEBR_GEOXML_DOCUMENT(flow)),
				      (GdomeNode *) new_node, (GdomeNode *) first_revision, &exception);
	}

	gebr_geoxml_document_set_help (GEBR_GEOXML_DOCUMENT (flow), merged_help->str);
	g_string_free (merged_help, TRUE);

	return TRUE;
}

GebrGeoXmlRevision *gebr_geoxml_flow_append_revision(GebrGeoXmlFlow * flow, const gchar * comment)
{
	GebrGeoXmlRevision *revision;
	GebrGeoXmlSequence *i;
	GebrGeoXmlFlow *revision_flow;

	g_return_val_if_fail(flow != NULL, NULL);
	g_return_val_if_fail(comment != NULL, NULL);

	revision_flow = GEBR_GEOXML_FLOW(gebr_geoxml_document_clone(GEBR_GEOXML_DOCUMENT(flow)));
	gebr_geoxml_document_set_help (GEBR_GEOXML_DOCUMENT (revision_flow), "");

	/* remove revisions from the revision flow. */
	gebr_geoxml_flow_get_revision(revision_flow, &i, 0);
	while (i != NULL) {
		GebrGeoXmlSequence *aux = (GebrGeoXmlSequence *) __gebr_geoxml_next_element((GdomeElement *) i);
		gdome_el_removeChild(gebr_geoxml_document_root_element(GEBR_GEOXML_DOCUMENT(revision_flow)),
				     (GdomeNode *) i, &exception);

		i = aux;
	}

	/* save to xml and free */
	gchar *revision_xml;
	gebr_geoxml_document_to_string(GEBR_GEOXML_DOCUMENT(revision_flow), &revision_xml);
	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(revision_flow));

	GebrGeoXmlSequence *first_revision;
	gebr_geoxml_flow_get_revision(flow, &first_revision, 0);
	revision = (GebrGeoXmlRevision *)
	    __gebr_geoxml_insert_new_element(gebr_geoxml_document_root_element(GEBR_GEOXML_DOCUMENT(flow)), "revision",
					     (GdomeElement *) first_revision);

	gebr_geoxml_flow_set_revision_data(revision, revision_xml, gebr_iso_date(), comment);
	g_free(revision_xml);

	return revision;
}

void gebr_geoxml_flow_set_revision_data(GebrGeoXmlRevision * revision, const gchar * flow, const gchar * date, const gchar * comment)
{
	g_return_if_fail(revision != NULL);
	if (flow != NULL)
		__gebr_geoxml_set_element_value((GdomeElement *) revision, flow, __gebr_geoxml_create_CDATASection);
	if (date != NULL)
		__gebr_geoxml_set_attr_value((GdomeElement *) revision, "date", date);
	if (comment != NULL)
		__gebr_geoxml_set_attr_value((GdomeElement *) revision, "comment", comment);
}

int gebr_geoxml_flow_get_revision(GebrGeoXmlFlow * flow, GebrGeoXmlSequence ** revision, gulong index)
{
	if (flow == NULL) {
		*revision = NULL;
		return GEBR_GEOXML_RETV_NULL_PTR;
	}

	*revision = (GebrGeoXmlSequence *)
		__gebr_geoxml_get_element_at(gebr_geoxml_document_root_element(GEBR_GEOXML_DOCUMENT(flow)), "revision",
					     index, FALSE);

	return (*revision == NULL)
	    ? GEBR_GEOXML_RETV_INVALID_INDEX : GEBR_GEOXML_RETV_SUCCESS;
}

void gebr_geoxml_flow_get_revision_data(GebrGeoXmlRevision * revision, gchar ** flow, gchar ** date, gchar ** comment)
{
	if (revision == NULL)
		return;

	if (flow != NULL)
		*flow = (gchar *) __gebr_geoxml_get_element_value((GdomeElement *) revision);
	if (date != NULL)
		*date = (gchar *) gebr_localized_date(__gebr_geoxml_get_attr_value((GdomeElement *) revision, "date"));
	if (comment != NULL)
		*comment = (gchar *) __gebr_geoxml_get_attr_value((GdomeElement *) revision, "comment");
}

glong gebr_geoxml_flow_get_revisions_number(GebrGeoXmlFlow * flow)
{
	if (flow == NULL)
		return -1;
	return __gebr_geoxml_get_elements_number(gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow)), "revision");
}

GebrGeoXmlFlowError gebr_geoxml_flow_validade(GebrGeoXmlFlow * flow, 
					      GebrGeoXmlLine * line,  
					      GebrGeoXmlProject *project, 
					      gchar ** program_title)
{
	const gchar *input = NULL;
	GebrGeoXmlSequence *first_program;
	gulong i = 0, max = 0;

	input = gebr_geoxml_flow_io_get_input(flow);

	/*
	 * Checking if the flow has at least one configured program
	 */
	max = gebr_geoxml_flow_get_programs_number(flow); 
	gint status = 0;
	gboolean first_configured = TRUE;
	gint previous_stdout = 0;
	gboolean find_control = FALSE;
	gint last_configured = 0;

	/*
	 * Checking if flow has only the for loop
	 */

	if (max == 1 && gebr_geoxml_flow_has_control_program (flow))
		return GEBR_GEOXML_FLOW_ERROR_LOOP_ONLY;


	for (i = 0; i < max; i++)
	{
		gebr_geoxml_flow_get_program(flow, &first_program, i);
		status = gebr_geoxml_program_get_status(GEBR_GEOXML_PROGRAM(first_program));

		if (status != GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED)
			continue;

		if (status == GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED)
			last_configured = i;

		if (!find_control && gebr_geoxml_program_get_control (GEBR_GEOXML_PROGRAM(first_program)) == GEBR_GEOXML_PROGRAM_CONTROL_FOR){
			find_control = TRUE;
			continue;
		}

		int chain_option = gebr_geoxml_program_get_stdin(GEBR_GEOXML_PROGRAM(first_program)) 
				   + (previous_stdout << 1);
		if (!first_configured)
			switch (chain_option) 
			{
			case 0:
				break;
			case 1:	/* Previous does not write to stdin but current expect something */
				*program_title = g_strdup(gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(first_program)));
				return GEBR_GEOXML_FLOW_ERROR_NO_INPUT;
			case 2:	/* Previous does write to stdin but current does not carry about */
				*program_title = g_strdup(gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(first_program)));
				return GEBR_GEOXML_FLOW_ERROR_NO_OUTPUT;
			default:
				break;
			}

		else 
		{
			if (gebr_geoxml_program_get_stdin(GEBR_GEOXML_PROGRAM(first_program))) {
				if(input == NULL || g_strcmp0("", input) == 0)
				{
					*program_title = g_strdup(gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(first_program)));
					return GEBR_GEOXML_FLOW_ERROR_NO_INFILE;
				}
				else if (!gebr_geoxml_document_validate_str (input,
									     GEBR_GEOXML_DOCUMENT (flow),
									     GEBR_GEOXML_DOCUMENT (line),
									     GEBR_GEOXML_DOCUMENT (project),
									     NULL))
				{
					*program_title = g_strdup(gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(first_program)));
					return GEBR_GEOXML_FLOW_ERROR_INVALID_INFILE;
				}
			}
		}
		previous_stdout = gebr_geoxml_program_get_stdout(GEBR_GEOXML_PROGRAM(first_program));
		first_configured = FALSE;
	}
	
	if (first_configured)
	{
		return GEBR_GEOXML_FLOW_ERROR_NO_VALID_PROGRAMS;
	}

	/*
	 * Checking the output file
	 */
	const gchar *output = NULL;
	output = gebr_geoxml_flow_io_get_output(flow);

	gebr_geoxml_flow_get_program(flow, &first_program, last_configured);

	if (gebr_geoxml_program_get_stdout(GEBR_GEOXML_PROGRAM(first_program))
	    && !gebr_geoxml_document_validate_str (output,
						   GEBR_GEOXML_DOCUMENT (flow),
						   GEBR_GEOXML_DOCUMENT (line),
						   GEBR_GEOXML_DOCUMENT (project),
						   NULL))
	{
		*program_title = g_strdup(gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(first_program)));
		return GEBR_GEOXML_FLOW_ERROR_INVALID_OUTFILE;
	}	

	/*
	 * Checking the error log file
	 */
	const gchar *error = NULL;
	error = gebr_geoxml_flow_io_get_error(flow);

	if (gebr_geoxml_program_get_stderr(GEBR_GEOXML_PROGRAM(first_program))
	    && !gebr_geoxml_document_validate_str (error,
						   GEBR_GEOXML_DOCUMENT (flow),
						   GEBR_GEOXML_DOCUMENT (line),
						   GEBR_GEOXML_DOCUMENT (project),
						   NULL))
	{
		*program_title = g_strdup(gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(first_program)));
		return GEBR_GEOXML_FLOW_ERROR_INVALID_ERRORFILE;
	}	
	
	return GEBR_GEOXML_FLOW_ERROR_NONE;


}

gboolean gebr_geoxml_flow_has_control_program (GebrGeoXmlFlow *flow)
{
	return (gebr_geoxml_flow_get_control_program(flow) == NULL ? FALSE : TRUE);
}

GebrGeoXmlProgram * gebr_geoxml_flow_get_control_program (GebrGeoXmlFlow *flow)
{
	GebrGeoXmlSequence *seq;
	GebrGeoXmlProgram *prog;
	GebrGeoXmlProgramControl cont;

	gebr_geoxml_flow_get_program (flow, &seq, 0);
	while (seq) {
		prog = GEBR_GEOXML_PROGRAM (seq);
		cont = gebr_geoxml_program_get_control (prog);
		if (cont != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY
		    && cont != GEBR_GEOXML_PROGRAM_CONTROL_UNKNOWN)
			return prog;
		gebr_geoxml_sequence_next (&seq);
	}
	return NULL;
}

gboolean gebr_geoxml_flow_insert_iter_dict (GebrGeoXmlFlow *flow)
{
	GebrGeoXmlSequence *seq;
	GebrGeoXmlParameter *param;
	GebrGeoXmlParameters *dict;
	const gchar *keyword;

	dict = gebr_geoxml_document_get_dict_parameters (GEBR_GEOXML_DOCUMENT (flow));
	seq = gebr_geoxml_parameters_get_first_parameter (dict);
	keyword = gebr_geoxml_program_parameter_get_keyword (GEBR_GEOXML_PROGRAM_PARAMETER (seq));

	if (g_strcmp0 (keyword, "iter") == 0)
		return FALSE;

	param = gebr_geoxml_parameters_append_parameter(dict, GEBR_GEOXML_PARAMETER_TYPE_FLOAT);
	gebr_geoxml_program_parameter_set_list_separator(GEBR_GEOXML_PROGRAM_PARAMETER (param), "|");

	// Append four values in iter parameter to represent the
	// current value and the 'ini', 'step' and 'n' values.
	gebr_geoxml_program_parameter_append_value(GEBR_GEOXML_PROGRAM_PARAMETER (param), FALSE);
	gebr_geoxml_program_parameter_append_value(GEBR_GEOXML_PROGRAM_PARAMETER (param), FALSE);
	gebr_geoxml_program_parameter_append_value(GEBR_GEOXML_PROGRAM_PARAMETER (param), FALSE);
	gebr_geoxml_program_parameter_append_value(GEBR_GEOXML_PROGRAM_PARAMETER (param), FALSE);

	gebr_geoxml_program_parameter_set_keyword(GEBR_GEOXML_PROGRAM_PARAMETER (param), "iter");
	gebr_geoxml_parameter_set_label(param, _("Loop iteration counter"));
	gebr_geoxml_sequence_move_after(GEBR_GEOXML_SEQUENCE (param), NULL);

	gebr_geoxml_flow_update_iter_dict_value(flow);

	return TRUE;
}

void gebr_geoxml_flow_remove_iter_dict (GebrGeoXmlFlow *flow)
{
	GebrGeoXmlSequence *seq;
	GebrGeoXmlParameters *dict;
	const gchar *keyword;

	dict = gebr_geoxml_document_get_dict_parameters (GEBR_GEOXML_DOCUMENT (flow));
	seq = gebr_geoxml_parameters_get_first_parameter (dict);
	keyword = gebr_geoxml_program_parameter_get_keyword (GEBR_GEOXML_PROGRAM_PARAMETER (seq));

	if (g_strcmp0 (keyword, "iter") == 0)
		gebr_geoxml_sequence_remove(seq);
}

void gebr_geoxml_flow_io_set_output_append(GebrGeoXmlFlow *flow, gboolean setting)
{
	GdomeElement *root;
	GdomeElement *io;
	GdomeElement *output;

	g_return_if_fail (flow != NULL);

	root = gebr_geoxml_document_root_element (flow);
	io = __gebr_geoxml_get_first_element (root, "io");
	output = __gebr_geoxml_get_first_element (io, "output");

	__gebr_geoxml_set_attr_value (output, "append", setting? "yes":"no");
}

gboolean gebr_geoxml_flow_io_get_output_append(GebrGeoXmlFlow *flow)
{
	GdomeElement *root;
	GdomeElement *io;
	GdomeElement *output;
	const gchar *setting;

	g_return_val_if_fail (flow != NULL, FALSE);

	root = gebr_geoxml_document_root_element (flow);
	io = __gebr_geoxml_get_first_element (root, "io");
	output = __gebr_geoxml_get_first_element (io, "output");

	setting = __gebr_geoxml_get_attr_value (output, "append");

	return g_strcmp0 (setting, "yes") == 0;
}

void gebr_geoxml_flow_io_set_error_append(GebrGeoXmlFlow *flow, gboolean setting)
{
	GdomeElement *root;
	GdomeElement *io;
	GdomeElement *error;

	g_return_if_fail (flow != NULL);

	root = gebr_geoxml_document_root_element (flow);
	io = __gebr_geoxml_get_first_element (root, "io");
	error = __gebr_geoxml_get_first_element (io, "error");

	__gebr_geoxml_set_attr_value (error, "append", setting? "yes":"no");
}

gboolean gebr_geoxml_flow_io_get_error_append(GebrGeoXmlFlow *flow)
{
	GdomeElement *root;
	GdomeElement *io;
	GdomeElement *error;
	const gchar *setting;

	g_return_val_if_fail (flow != NULL, FALSE);

	root = gebr_geoxml_document_root_element (flow);
	io = __gebr_geoxml_get_first_element (root, "io");
	error = __gebr_geoxml_get_first_element (io, "error");

	setting = __gebr_geoxml_get_attr_value (error, "append");

	return g_strcmp0 (setting, "no") != 0;
}

void gebr_geoxml_flow_update_iter_dict_value(GebrGeoXmlFlow *flow)
{
	GebrGeoXmlDocument *doc;
	GebrGeoXmlSequence *seq;
	GebrGeoXmlProgramParameter *iter;
	GebrGeoXmlProgram *program;
	const gchar *keyword;
	gchar *step, *ini, *nstr;
	guint n;

	doc = GEBR_GEOXML_DOCUMENT(flow);
	iter = GEBR_GEOXML_PROGRAM_PARAMETER(gebr_geoxml_document_get_dict_parameter(doc));
	keyword = gebr_geoxml_program_parameter_get_keyword(iter);

	if (g_strcmp0 (keyword, "iter") != 0)
		return;

	program = gebr_geoxml_flow_get_control_program(flow);
	n = gebr_geoxml_program_control_get_n(program, &step, &ini);
	nstr = g_strdup_printf("%d", n);

	// Set the 'ini' value for current and 'ini'
	gebr_geoxml_program_parameter_get_value(iter, FALSE, &seq, 0);
	gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(seq), ini);
	gebr_geoxml_sequence_next(&seq);
	gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(seq), ini);

	// Set 'step'
	gebr_geoxml_sequence_next(&seq);
	gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(seq), step);

	// Set 'n'
	gebr_geoxml_sequence_next(&seq);
	gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(seq), nstr);

	g_free(nstr);
}
