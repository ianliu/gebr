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
#include <string.h>

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
 * internal structures and functions
 */

struct gebr_geoxml_flow {
	GebrGeoXmlDocument *document;
};

struct gebr_geoxml_server {
	GdomeElement *element;
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
	GdomeElement *element;
	GdomeElement *servers;

	document = gebr_geoxml_document_new("flow", GEBR_GEOXML_FLOW_VERSION);

	servers = __gebr_geoxml_insert_new_element(gebr_geoxml_document_root_element(document), "servers", NULL);
	element = __gebr_geoxml_insert_new_element(gebr_geoxml_document_root_element(document), "io", servers);
	__gebr_geoxml_insert_new_element(element, "input", NULL);
	__gebr_geoxml_insert_new_element(element, "output", NULL);
	__gebr_geoxml_insert_new_element(element, "error", NULL);
	__gebr_geoxml_insert_new_element(__gebr_geoxml_get_first_element
					 (gebr_geoxml_document_root_element(document), "date"), "lastrun", NULL);

	return GEBR_GEOXML_FLOW(document);
}

void gebr_geoxml_flow_add_flow(GebrGeoXmlFlow * flow, GebrGeoXmlFlow * flow2)
{
	if (flow == NULL || flow2 == NULL)
		return;

	GdomeNodeList *flow2_node_list;
	GdomeDOMString *string;
	gulong i, n;

	/* import each program from flow2 */
	string = gdome_str_mkref("program");
	flow2_node_list =
	    gdome_el_getElementsByTagName(gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow2)), string,
					  &exception);
	n = gdome_nl_length(flow2_node_list, &exception);

	/* clear each copied program help */
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

GebrGeoXmlFlowServer *gebr_geoxml_flow_append_server(GebrGeoXmlFlow * flow)
{
	GdomeElement *server;
	GdomeElement *element;

	server =
	    __gebr_geoxml_insert_new_element(__gebr_geoxml_get_first_element
					     (gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow)), "servers"),
					     "server", NULL);
	__gebr_geoxml_set_attr_value((GdomeElement *) server, "address", "");

	element = __gebr_geoxml_insert_new_element(server, "io", NULL);
	__gebr_geoxml_insert_new_element(element, "input", NULL);
	__gebr_geoxml_insert_new_element(element, "output", NULL);
	__gebr_geoxml_insert_new_element(element, "error", NULL);
	__gebr_geoxml_insert_new_element(server, "lastrun", NULL);

	return (GebrGeoXmlFlowServer *) server;
}

int gebr_geoxml_flow_get_server(GebrGeoXmlFlow * flow, GebrGeoXmlSequence ** server, gulong index)
{
	GdomeElement *element;

	if (flow == NULL) {
		*server = NULL;
		return GEBR_GEOXML_RETV_NULL_PTR;
	}

	element = __gebr_geoxml_get_first_element(gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow)), "servers");
	*server = (GebrGeoXmlSequence *) __gebr_geoxml_get_element_at(element, "server", index, FALSE);

	return (*server == NULL)
	    ? GEBR_GEOXML_RETV_INVALID_INDEX : GEBR_GEOXML_RETV_SUCCESS;
}

void gebr_geoxml_flow_server_set_address(GebrGeoXmlFlowServer * server, const gchar * address)
{
	__gebr_geoxml_set_attr_value((GdomeElement *) server, "address", address);
}

const gchar *gebr_geoxml_flow_server_get_address(GebrGeoXmlFlowServer * server)
{
	return __gebr_geoxml_get_attr_value((GdomeElement *) server, "address");
}

GebrGeoXmlFlowServer *gebr_geoxml_flow_servers_get_last_run(GebrGeoXmlFlow * flow)
{
	GebrGeoXmlSequence *server;
	GebrGeoXmlSequence *last_server;
	const gchar *last_date;

	last_date = "";
	last_server = NULL;
	gebr_geoxml_flow_get_server(flow, &server, 0);

	while (server) {
		const gchar *date;
		date = gebr_geoxml_flow_server_get_date_last_run(GEBR_GEOXML_FLOW_SERVER(server));
		if (strcmp(last_date, date) <= 0) {
			last_date = date;
			last_server = server;
		}
		gebr_geoxml_sequence_next(&server);
	}
	g_assert(last_server != NULL);
	return GEBR_GEOXML_FLOW_SERVER(last_server);
}

GebrGeoXmlFlowServer *gebr_geoxml_flow_servers_query(GebrGeoXmlFlow * flow, const gchar * address,
						     const gchar * input, const gchar * output, const gchar * error)
{
	if (flow == NULL || address == NULL)
		return NULL;

	GebrGeoXmlSequence *server;

	gebr_geoxml_flow_get_server(flow, &server, 0);
	for (; server != NULL; gebr_geoxml_sequence_next(&server))
		if (!strcmp(address,
			    gebr_geoxml_flow_server_get_address(GEBR_GEOXML_FLOW_SERVER(server))) &&
		    (input == NULL || !strcmp(input,
					      gebr_geoxml_flow_server_io_get_input(GEBR_GEOXML_FLOW_SERVER(server)))) &&
		    (output == NULL || !strcmp(output,
					       gebr_geoxml_flow_server_io_get_output(GEBR_GEOXML_FLOW_SERVER(server))))
		    && (error == NULL
			|| !strcmp(error, gebr_geoxml_flow_server_io_get_error(GEBR_GEOXML_FLOW_SERVER(server)))))
			return (GebrGeoXmlFlowServer *) server;

	return NULL;
}

glong gebr_geoxml_flow_get_servers_number(GebrGeoXmlFlow * flow)
{
	GdomeElement * root_element ;
	GdomeElement * servers_element ;

	g_return_val_if_fail(flow != NULL, -1);
	root_element = gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow));
	servers_element = __gebr_geoxml_get_first_element(root_element, "servers");

	return __gebr_geoxml_get_elements_number(servers_element, "server");
}

void gebr_geoxml_flow_server_io_set_input(GebrGeoXmlFlowServer * server, const gchar * input)
{
	if (server == NULL || input == NULL)
		return;
	__gebr_geoxml_set_tag_value(__gebr_geoxml_get_first_element((GdomeElement *) server, "io"),
				    "input", input, __gebr_geoxml_create_TextNode);
}

void gebr_geoxml_flow_server_io_set_output(GebrGeoXmlFlowServer * server, const gchar * output)
{
	if (server == NULL || output == NULL)
		return;
	__gebr_geoxml_set_tag_value(__gebr_geoxml_get_first_element((GdomeElement *) server, "io"),
				    "output", output, __gebr_geoxml_create_TextNode);
}

void gebr_geoxml_flow_server_io_set_error(GebrGeoXmlFlowServer * server, const gchar * error)
{
	if (server == NULL || error == NULL)
		return;
	__gebr_geoxml_set_tag_value(__gebr_geoxml_get_first_element((GdomeElement *) server, "io"),
				    "error", error, __gebr_geoxml_create_TextNode);
}

const gchar *gebr_geoxml_flow_server_io_get_input(GebrGeoXmlFlowServer * server)
{
	if (server == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value(__gebr_geoxml_get_first_element((GdomeElement *) server, "io"), "input");
}

const gchar *gebr_geoxml_flow_server_io_get_output(GebrGeoXmlFlowServer * server)
{
	if (server == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value(__gebr_geoxml_get_first_element((GdomeElement *) server, "io"), "output");
}

const gchar *gebr_geoxml_flow_server_io_get_error(GebrGeoXmlFlowServer * server)
{
	if (server == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value(__gebr_geoxml_get_first_element((GdomeElement *) server, "io"), "error");
}

void gebr_geoxml_flow_server_set_date_last_run(GebrGeoXmlFlowServer * server, const gchar * date)
{
	if (server == NULL || date == NULL)
		return;
	__gebr_geoxml_set_tag_value((GdomeElement *) server, "lastrun", date, __gebr_geoxml_create_TextNode);
}

const gchar *gebr_geoxml_flow_server_get_date_last_run(GebrGeoXmlFlowServer * server)
{
	if (server == NULL)
		return NULL;

	return __gebr_geoxml_get_tag_value((GdomeElement *) server, "lastrun");
}

GebrGeoXmlFlowServer *gebr_geoxml_flow_server_get_last_runned_server(GebrGeoXmlFlow * flow)
{
	if (flow == NULL)
		return NULL;

	GebrGeoXmlSequence *server_io;
	GebrGeoXmlSequence *last_run;
	const gchar *aux;

	aux = "";
	last_run = NULL;
	gebr_geoxml_flow_get_server(flow, &server_io, 0);
	for (; server_io != NULL; gebr_geoxml_sequence_next(&server_io)) {
		const gchar *date;
		date = gebr_geoxml_flow_server_get_date_last_run(GEBR_GEOXML_FLOW_SERVER(server_io));
		if (strcmp(aux, date) < 0) {
			aux = date;
			last_run = server_io;
		}
	}

	return (GebrGeoXmlFlowServer *) last_run;
}

void gebr_geoxml_flow_io_set_from_server(GebrGeoXmlFlow * flow, GebrGeoXmlFlowServer * server)
{
	gebr_geoxml_flow_io_set_input(flow, gebr_geoxml_flow_server_io_get_input(server));
	gebr_geoxml_flow_io_set_output(flow, gebr_geoxml_flow_server_io_get_output(server));
	gebr_geoxml_flow_io_set_error(flow, gebr_geoxml_flow_server_io_get_error(server));
}

void gebr_geoxml_flow_io_set_input(GebrGeoXmlFlow * flow, const gchar * input)
{
	if (flow == NULL || input == NULL)
		return;
	__gebr_geoxml_set_tag_value(__gebr_geoxml_get_first_element
				    (gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow)), "io"), "input", input,
				    __gebr_geoxml_create_TextNode);
}

void gebr_geoxml_flow_io_set_output(GebrGeoXmlFlow * flow, const gchar * output)
{
	if (flow == NULL || output == NULL)
		return;
	__gebr_geoxml_set_tag_value(__gebr_geoxml_get_first_element
				    (gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow)), "io"), "output", output,
				    __gebr_geoxml_create_TextNode);
}

void gebr_geoxml_flow_io_set_error(GebrGeoXmlFlow * flow, const gchar * error)
{
	if (flow == NULL || error == NULL)
		return;
	__gebr_geoxml_set_tag_value(__gebr_geoxml_get_first_element
				    (gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow)), "io"), "error", error,
				    __gebr_geoxml_create_TextNode);
}

const gchar *gebr_geoxml_flow_io_get_input(GebrGeoXmlFlow * flow)
{
	if (flow == NULL)
		return NULL;
	return
	    __gebr_geoxml_get_tag_value(__gebr_geoxml_get_first_element
					(gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow)), "io"), "input");
}

const gchar *gebr_geoxml_flow_io_get_output(GebrGeoXmlFlow * flow)
{
	if (flow == NULL)
		return NULL;
	return
	    __gebr_geoxml_get_tag_value(__gebr_geoxml_get_first_element
					(gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow)), "io"), "output");
}

const gchar *gebr_geoxml_flow_io_get_error(GebrGeoXmlFlow * flow)
{
	if (flow == NULL)
		return NULL;
	return
	    __gebr_geoxml_get_tag_value(__gebr_geoxml_get_first_element
					(gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow)), "io"), "error");
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
	if (flow == NULL || name == NULL)
		return NULL;

	GebrGeoXmlCategory *category;

	category = (GebrGeoXmlCategory *)
	    __gebr_geoxml_insert_new_element(gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow)), "category",
					     __gebr_geoxml_get_first_element(gebr_geoxml_document_root_element
									     (GEBR_GEOXML_DOC(flow)), "io"));
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

gboolean gebr_geoxml_flow_change_to_revision(GebrGeoXmlFlow * flow, GebrGeoXmlRevision * revision)
{
	if (flow == NULL || revision == NULL)
		return FALSE;

	GebrGeoXmlDocument *revision_flow;
	GebrGeoXmlSequence *first_revision;
	GdomeElement *child;

	/* load document validating it */
	if (gebr_geoxml_document_load_buffer(&revision_flow,
					     __gebr_geoxml_get_element_value((GdomeElement *) revision)))
		return FALSE;

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

	return TRUE;
}

GebrGeoXmlRevision *gebr_geoxml_flow_append_revision(GebrGeoXmlFlow * flow, const gchar * comment)
{
	g_return_val_if_fail(flow != NULL, NULL);
	g_return_val_if_fail(comment != NULL, NULL);

	GebrGeoXmlRevision *revision;

	GebrGeoXmlFlow *revision_flow = GEBR_GEOXML_FLOW(gebr_geoxml_document_clone(GEBR_GEOXML_DOCUMENT(flow)));
	/* remove revisions from the revision flow. */
	GebrGeoXmlSequence *i;
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
