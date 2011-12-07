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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "../libgebr-gettext.h"

#include <regex.h>
#include <gdome.h>
#include <string.h>
#include <stdio.h>
#include <glib/gi18n-lib.h>
#include <stdlib.h>

#include "../date.h"
#include "document.h"
#include "document_p.h"
#include "error.h"
#include "flow.h"
#include "object.h"
#include "parameter.h"
#include "parameter_group.h"
#include "parameters.h"
#include "parameters_p.h"
#include "program-parameter.h"
#include "program.h"
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
 *  internal
 */

static void
set_flow_tag_property(GebrGeoXmlFlow *flow,
                      const gchar *tag_element,
                      const gchar *tag_name,
                      const gchar *tag_value)
{
	GdomeElement *root;
	GdomeElement *element;

	g_return_if_fail (flow != NULL);
	g_return_if_fail (tag_value != NULL);
	g_return_if_fail (tag_name != NULL);
	g_return_if_fail (tag_element != NULL);

	root = gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow));
	element = __gebr_geoxml_get_first_element(root, tag_element);

	__gebr_geoxml_set_tag_value(element, tag_name, tag_value, __gebr_geoxml_create_TextNode);

	gdome_el_unref(root, &exception);
	gdome_el_unref(element, &exception);
}

static gchar *
get_flow_tag_property(GebrGeoXmlFlow *flow,
                       const gchar *tag_element,
                       const gchar *tag_name)
{
	gchar *prop_value;
	GdomeElement *root;
	GdomeElement *element;

	g_return_val_if_fail(flow != NULL, NULL);

	root = gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow));
	element = __gebr_geoxml_get_first_element(root, tag_element);
	prop_value =  __gebr_geoxml_get_tag_value(element, tag_name);

	gdome_el_unref(root, &exception);
	gdome_el_unref(element, &exception);

	return prop_value;
}

static void
set_flow_attr_property(GebrGeoXmlFlow *flow,
                       const gchar *tag_element,
                       const gchar *tag_name,
                       const gchar *tag_value)
{
	GdomeElement *root;
	GdomeElement *element;

	g_return_if_fail (flow != NULL);
	g_return_if_fail (tag_value != NULL);
	g_return_if_fail (tag_name != NULL);
	g_return_if_fail (tag_element != NULL);

	root = gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow));
	element = __gebr_geoxml_get_first_element(root, tag_element);

	__gebr_geoxml_set_attr_value(element, tag_name, tag_value);

	gdome_el_unref(root, &exception);
	gdome_el_unref(element, &exception);
}

static gchar *
get_flow_attr_property(GebrGeoXmlFlow *flow,
                       const gchar *tag_element,
                       const gchar *tag_name)
{
	gchar *prop_value;
	GdomeElement *root;
	GdomeElement *element;

	g_return_val_if_fail(flow != NULL, NULL);

	root = gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow));
	element = __gebr_geoxml_get_first_element(root, tag_element);
	prop_value = __gebr_geoxml_get_attr_value((GdomeElement *) element, tag_name);

	gdome_el_unref(root, &exception);
	gdome_el_unref(element, &exception);

	return prop_value;
}
/*
 * library functions.
 */

GebrGeoXmlFlow *gebr_geoxml_flow_new()
{
	GebrGeoXmlDocument *document;
	GdomeElement *io;
	GdomeElement *root;
	GdomeElement *server;

	document = gebr_geoxml_document_new("flow", GEBR_GEOXML_FLOW_VERSION);

	root = gebr_geoxml_document_root_element(document);
	server = __gebr_geoxml_insert_new_element(root, "server", NULL);
	__gebr_geoxml_set_attr_value (server, "group-type", "");
	__gebr_geoxml_set_attr_value (server, "group-name", "");

	io = __gebr_geoxml_insert_new_element(server, "io", NULL);
	gdome_el_unref(__gebr_geoxml_insert_new_element(io, "input", NULL), &exception);
	gdome_el_unref(__gebr_geoxml_insert_new_element(io, "output", NULL), &exception);
	gdome_el_unref(__gebr_geoxml_insert_new_element(io, "error", NULL), &exception);
	gdome_el_unref(__gebr_geoxml_insert_new_element(server, "lastrun", NULL), &exception);

	GdomeElement *date = __gebr_geoxml_get_first_element(root, "date");
	gdome_el_unref(__gebr_geoxml_insert_new_element(date, "lastrun", NULL), &exception);

	gdome_el_unref(server, &exception);
	gdome_el_unref(date, &exception);
	gdome_el_unref(root, &exception);
	gdome_el_unref(io, &exception);

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
	GdomeElement *root = gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow2));
	string = gdome_str_mkref("program");
	flow2_node_list =
	    gdome_el_getElementsByTagName(root, string,
					  &exception);
	n = gdome_nl_length(flow2_node_list, &exception);
	gdome_el_unref(root, &exception);

	for (i = 0; i < n; ++i) {
		GdomeNode *node;
		GdomeNode *new_node;
		GdomeElement *revision;
		GdomeElement *root_element;

		node = gdome_nl_item(flow2_node_list, i, &exception);
		new_node = gdome_doc_importNode((GdomeDocument *) flow, node, TRUE, &exception);

		root_element = gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow));
		revision = __gebr_geoxml_get_first_element(root_element, "revision");
		gdome_n_unref(gdome_el_insertBefore_protected(root_element, new_node, (GdomeNode *)revision, &exception), &exception);
		gdome_n_unref(node, &exception);
		gdome_n_unref(new_node, &exception);
		gdome_el_unref(revision, &exception);
		gdome_el_unref(root_element, &exception);
	}

	gebr_geoxml_flow_get_category (flow2, &category, 0);
	while (category) {
		gchar *name;
		name = gebr_geoxml_value_sequence_get (GEBR_GEOXML_VALUE_SEQUENCE (category));
		gebr_geoxml_object_unref(gebr_geoxml_flow_append_category (flow, name));
		gebr_geoxml_sequence_next (&category);
		g_free(name);
	}

	// We are adding a control menu into flow.
	// Append the `iter' dictionary keyword.
	if (!has_control1 && has_control2)
	{
		GebrGeoXmlProgram * loop = gebr_geoxml_flow_get_control_program(flow2);
		GebrGeoXmlProgramStatus status = gebr_geoxml_program_get_status(loop);
		if (status != GEBR_GEOXML_PROGRAM_STATUS_DISABLED)
			gebr_geoxml_flow_insert_iter_dict (flow);
		gebr_geoxml_object_unref(loop);
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
	set_flow_tag_property(flow, "date", "lastrun", last_run);
}

gchar *gebr_geoxml_flow_get_date_last_run(GebrGeoXmlFlow * flow)
{
	return get_flow_tag_property(flow, "date", "lastrun");
}

void
gebr_geoxml_flow_server_set_group(GebrGeoXmlFlow *flow,
				  const gchar *type,
				  const gchar *name)
{
	set_flow_attr_property(flow, "server", "group-type", type);
	set_flow_attr_property(flow, "server", "group-name", name);
}

void
gebr_geoxml_flow_server_get_group(GebrGeoXmlFlow *flow,
				  gchar **type,
				  gchar **name)
{
	if (type)
		*type = get_flow_attr_property(flow, "server", "group-type");

	if (name)
		*name = get_flow_attr_property(flow, "server", "group-name");
}

void gebr_geoxml_flow_server_set_date_last_run(GebrGeoXmlFlow *flow, const gchar * date)
{
	set_flow_tag_property(flow, "server", "lastrun", date);
}

gchar *gebr_geoxml_flow_server_get_date_last_run(GebrGeoXmlFlow *flow)
{
	return get_flow_tag_property(flow, "server", "lastrun");
}

void gebr_geoxml_flow_io_set_input(GebrGeoXmlFlow *flow, const gchar *input)
{
	set_flow_tag_property(flow, "io", "input", input);
}

void gebr_geoxml_flow_io_set_output(GebrGeoXmlFlow *flow, const gchar *output)
{
	set_flow_tag_property(flow, "io", "output", output);
}

void gebr_geoxml_flow_io_set_error(GebrGeoXmlFlow *flow, const gchar *error)
{
	set_flow_tag_property(flow, "io", "error", error);
}

gchar *gebr_geoxml_flow_io_get_input(GebrGeoXmlFlow *flow)
{
	return get_flow_tag_property(flow, "io", "input");
}

gchar *gebr_geoxml_flow_io_get_output(GebrGeoXmlFlow * flow)
{
	return get_flow_tag_property(flow, "io", "output");
}

gchar *gebr_geoxml_flow_io_get_error(GebrGeoXmlFlow * flow)
{
	return get_flow_tag_property(flow, "io", "error");
}

GebrGeoXmlProgram *gebr_geoxml_flow_append_program(GebrGeoXmlFlow * flow)
{
	GdomeElement *element;
	GdomeElement *root;
	GdomeElement *first_el;

	root = gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow));
	first_el = __gebr_geoxml_get_first_element(root, "revision");
	element = __gebr_geoxml_insert_new_element(root, "program", first_el);

	/* elements/attibutes */
	gebr_geoxml_program_set_stdin((GebrGeoXmlProgram *) element, FALSE);
	gebr_geoxml_program_set_stdout((GebrGeoXmlProgram *) element, FALSE);
	gebr_geoxml_program_set_stderr((GebrGeoXmlProgram *) element, FALSE);
	gebr_geoxml_program_set_status((GebrGeoXmlProgram *) element, GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED);
	gdome_el_unref(__gebr_geoxml_insert_new_element(element, "title", NULL), &exception);
	gdome_el_unref(__gebr_geoxml_insert_new_element(element, "binary", NULL), &exception);
	gdome_el_unref(__gebr_geoxml_insert_new_element(element, "description", NULL), &exception);
	gdome_el_unref(__gebr_geoxml_insert_new_element(element, "help", NULL), &exception);
	gdome_el_unref(__gebr_geoxml_insert_new_element(element, "url", NULL), &exception);
	gebr_geoxml_object_unref(__gebr_geoxml_parameters_append_new(element));

	gdome_el_unref(root, &exception);
	gdome_el_unref(first_el, &exception);

	return (GebrGeoXmlProgram *) element;
}

int gebr_geoxml_flow_get_program(GebrGeoXmlFlow * flow, GebrGeoXmlSequence ** program, gulong index)
{
	GdomeElement *root;

	if (flow == NULL) {
		*program = NULL;
		return GEBR_GEOXML_RETV_NULL_PTR;
	}

	root = gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow));
	*program = (GebrGeoXmlSequence *) __gebr_geoxml_get_element_at(root, "program", index, FALSE);
	gdome_el_unref(root, &exception);

	return (*program == NULL)
	    ? GEBR_GEOXML_RETV_INVALID_INDEX : GEBR_GEOXML_RETV_SUCCESS;
}

glong gebr_geoxml_flow_get_programs_number(GebrGeoXmlFlow * flow)
{
	if (flow == NULL)
		return -1;
	GdomeElement *root = gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow));
	gulong retval =  __gebr_geoxml_get_elements_number(root, "program");
	gdome_el_unref(root, &exception);
	return retval;
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
	GdomeElement *root = gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow));
	GdomeElement *element = __gebr_geoxml_get_first_element(root, "server");
	category = (GebrGeoXmlCategory *)__gebr_geoxml_insert_new_element(root, "category", element);

	gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(category), name);

	gdome_el_unref(root, &exception);
	gdome_el_unref(element, &exception);

	return category;
}

int gebr_geoxml_flow_get_category(GebrGeoXmlFlow * flow, GebrGeoXmlSequence ** category, gulong index)
{
	if (flow == NULL) {
		*category = NULL;
		return GEBR_GEOXML_RETV_NULL_PTR;
	}
	GdomeElement *root = gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow));
	*category = (GebrGeoXmlSequence *)__gebr_geoxml_get_element_at(root, "category", index, FALSE);

	gdome_el_unref(root, &exception);

	return (*category == NULL)
	    ? GEBR_GEOXML_RETV_INVALID_INDEX : GEBR_GEOXML_RETV_SUCCESS;
}

glong
gebr_geoxml_flow_get_categories_number(GebrGeoXmlFlow * flow)
{
	if (flow == NULL)
		return -1;

	gulong retval = 0;
	GdomeElement * root = gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow));

	retval =  __gebr_geoxml_get_elements_number(root, "category");
	gdome_el_unref(root, &exception);

	return retval;	
}

gboolean gebr_geoxml_flow_change_to_revision(GebrGeoXmlFlow * flow, GebrGeoXmlRevision * revision, gboolean * report_merged)
{
	if (flow == NULL || revision == NULL)
		return FALSE;

	GString *merged_help;
	gchar *revision_help;
	gchar *flow_help;
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
	g_free(flow_help);
	g_free(revision_help);

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

GebrGeoXmlRevision *
gebr_geoxml_flow_append_revision(GebrGeoXmlFlow * flow,
				 const gchar * comment)
{
	GebrGeoXmlRevision *revision;
	GebrGeoXmlSequence *seq;
	GebrGeoXmlFlow *revision_flow;

	g_return_val_if_fail(flow != NULL, NULL);
	g_return_val_if_fail(comment != NULL, NULL);

	revision_flow = GEBR_GEOXML_FLOW(gebr_geoxml_document_clone(GEBR_GEOXML_DOCUMENT(flow)));
	gebr_geoxml_document_set_help (GEBR_GEOXML_DOCUMENT (revision_flow), "");

	GdomeElement * revision_root = gebr_geoxml_document_root_element(GEBR_GEOXML_DOCUMENT(revision_flow));
	/* remove revisions from the revision flow. */
	gebr_geoxml_flow_get_revision(revision_flow, &seq, 0);

	while (seq)
	{
		GebrGeoXmlSequence *aux = seq;
		gebr_geoxml_object_ref(aux);
		gebr_geoxml_sequence_next(&seq);
		gebr_geoxml_sequence_remove(aux);
	}

	gebr_geoxml_object_unref(revision_root);

	/* save to xml and free */
	gchar *revision_xml;
	gebr_geoxml_document_to_string(GEBR_GEOXML_DOCUMENT(revision_flow), &revision_xml);
	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(revision_flow));

	GebrGeoXmlSequence *first_revision;
	gebr_geoxml_flow_get_revision(flow, &first_revision, 0);

	GdomeElement * root = gebr_geoxml_document_root_element(GEBR_GEOXML_DOCUMENT(flow));
	revision = (GebrGeoXmlRevision *) __gebr_geoxml_insert_new_element(root, "revision", (GdomeElement *) first_revision);
	gebr_geoxml_object_unref(root);
	gebr_geoxml_object_unref(first_revision);

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

enum GEBR_GEOXML_RETV
gebr_geoxml_flow_get_revision(GebrGeoXmlFlow * flow,
			      GebrGeoXmlSequence ** revision,
			      gulong index)
{
	enum GEBR_GEOXML_RETV retval = GEBR_GEOXML_RETV_SUCCESS; 

	if (flow == NULL)
	{
		*revision = NULL;
		g_return_val_if_fail(flow != NULL, GEBR_GEOXML_RETV_NULL_PTR);
	}

	GdomeElement * root = gebr_geoxml_document_root_element(GEBR_GEOXML_DOCUMENT(flow));
	*revision = (GebrGeoXmlSequence *) __gebr_geoxml_get_element_at(root, "revision", index, FALSE);
	gebr_geoxml_object_unref(root);

	retval = (*revision == NULL) ? GEBR_GEOXML_RETV_INVALID_INDEX : GEBR_GEOXML_RETV_SUCCESS;

	return retval;
}

void gebr_geoxml_flow_get_revision_data(GebrGeoXmlRevision * revision, gchar ** flow, gchar ** date, gchar ** comment)
{
	g_return_if_fail(revision != NULL);

	if (flow)
		*flow = __gebr_geoxml_get_element_value((GdomeElement *) revision);

	if (date) {
		gchar *date_attr = __gebr_geoxml_get_attr_value((GdomeElement *) revision, "date");
		*date = g_strdup(gebr_localized_date(date_attr));
		g_free(date_attr);
	}

	if (comment)
		*comment = __gebr_geoxml_get_attr_value((GdomeElement *) revision, "comment");
}

glong gebr_geoxml_flow_get_revisions_number(GebrGeoXmlFlow * flow)
{
	if (flow == NULL)
		return -1;
	return __gebr_geoxml_get_elements_number(gebr_geoxml_document_root_element(GEBR_GEOXML_DOC(flow)), "revision");
}

gboolean
gebr_geoxml_flow_validate(GebrGeoXmlFlow *flow,
			  GebrValidator  *validator, 
			  GError        **err)
{
	GebrGeoXmlSequence *seq;
	gboolean first = TRUE;
	gboolean previous_stdout = FALSE;
	GebrGeoXmlProgram *last_configured = NULL;

	const gchar *program_title;
	const gchar *flow_title = gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(flow));

	/* Checking if flow has only the for loop */
	if (gebr_geoxml_flow_get_programs_number(flow) == 1
	    && gebr_geoxml_flow_has_control_program(flow))
	{
		g_set_error(err, GEBR_GEOXML_FLOW_ERROR,
			    GEBR_GEOXML_FLOW_ERROR_NO_VALID_PROGRAMS,
			    _("No configured or enabled Programs found in Flow \"%s\"."),
			    flow_title);
		return FALSE;
	}

	const gchar *input = gebr_geoxml_flow_io_get_input(flow);

	/* Checking if the flow has at least one configured program */
	gebr_geoxml_flow_get_program(flow, &seq, 0);
	for (; seq; gebr_geoxml_sequence_next(&seq))
	{
		GebrGeoXmlProgram *prog = GEBR_GEOXML_PROGRAM(seq);

		GebrGeoXmlProgramStatus status = gebr_geoxml_program_get_status(prog);

		if (last_configured)
			gebr_geoxml_object_unref(last_configured);
		last_configured = prog;
		gebr_geoxml_object_ref(last_configured);

		if (status != GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED
		    || gebr_geoxml_program_get_control(prog) == GEBR_GEOXML_PROGRAM_CONTROL_FOR)
			continue;

		program_title = gebr_geoxml_program_get_title(prog);

		if (first && gebr_geoxml_program_get_stdin(prog)) {
			if (!input || !*input) {
				g_set_error(err, GEBR_GEOXML_FLOW_ERROR,
					    GEBR_GEOXML_FLOW_ERROR_NO_INFILE,
					    _("No input file was specified for the Program "
					      "\"%s\" in Flow \"%s\"."),
					    program_title, flow_title);
				gebr_geoxml_object_unref(seq);
				gebr_geoxml_object_unref(last_configured);
				return FALSE;
			}

			if (!gebr_validator_validate_expr(validator, input,
							  GEBR_GEOXML_PARAMETER_TYPE_STRING,
							  NULL)) {
				g_set_error(err, GEBR_GEOXML_FLOW_ERROR,
					    GEBR_GEOXML_FLOW_ERROR_INVALID_INFILE,
					    _("Invalid input file specified for the Program "
					      "\"%s\" in Flow \"%s\"."),
					    program_title, flow_title);
				gebr_geoxml_object_unref(seq);
				gebr_geoxml_object_unref(last_configured);
				return FALSE;
			}
		} else {
			/* Convenience variable to check if two programs can connect with each other. */
			int chain_option = (previous_stdout << 1) + gebr_geoxml_program_get_stdin(prog);
			switch (chain_option) {
			case 0: /* Neither output nor input */
			case 3: /* Both output and input */
				break;
			case 1:	/* Previous does not write to stdin but current expect something */
				g_set_error(err, GEBR_GEOXML_FLOW_ERROR,
					    GEBR_GEOXML_FLOW_ERROR_NO_INPUT,
					    _("The Flow \"%s\" expects an input file."),
					    flow_title);
				gebr_geoxml_object_unref(seq);
				gebr_geoxml_object_unref(last_configured);
				return FALSE;
			case 2:	/* Previous does write to stdin but current does not care about */
				g_set_error(err, GEBR_GEOXML_FLOW_ERROR,
					    GEBR_GEOXML_FLOW_ERROR_NO_OUTPUT,
					    _("Unexpected output file for the Flow \"%s\"."),
					    flow_title);
				gebr_geoxml_object_unref(seq);
				gebr_geoxml_object_unref(last_configured);
				return FALSE;
			default:
				g_warn_if_reached();
			}
		}
		previous_stdout = gebr_geoxml_program_get_stdout(prog);
		first = FALSE;
	}
	
	if (first) {
		g_set_error(err, GEBR_GEOXML_FLOW_ERROR,
			    GEBR_GEOXML_FLOW_ERROR_NO_VALID_PROGRAMS,
			    _("No configured or enabled Programs found in Flow \"%s\"."),
			    flow_title);
		gebr_geoxml_object_unref(last_configured);
		return FALSE;
	}

	program_title = gebr_geoxml_program_get_title(last_configured);

	const gchar *output = gebr_geoxml_flow_io_get_output(flow);
	if (gebr_geoxml_program_get_stdout(last_configured)
	    && !gebr_validator_validate_expr(validator, output,
					    GEBR_GEOXML_PARAMETER_TYPE_STRING, NULL)) {
		g_set_error(err, GEBR_GEOXML_FLOW_ERROR,
			    GEBR_GEOXML_FLOW_ERROR_INVALID_OUTFILE,
			    _("Invalid output file file specified for"
			      " the Program \"%s\" in Flow \"%s\"."),
			    program_title, flow_title);
		gebr_geoxml_object_unref(last_configured);
		return FALSE;
	}	

	const gchar *error = gebr_geoxml_flow_io_get_error(flow);
	if (gebr_geoxml_program_get_stderr(last_configured)
	    && !gebr_validator_validate_expr(validator, error,
					    GEBR_GEOXML_PARAMETER_TYPE_STRING, NULL))
	{
		g_set_error(err, GEBR_GEOXML_FLOW_ERROR,
			    GEBR_GEOXML_FLOW_ERROR_INVALID_ERRORFILE,
			    _("Invalid error file specified for the Program \"%s\" in Flow \"%s\"."),
			    program_title, flow_title);
		gebr_geoxml_object_unref(last_configured);
		return FALSE;
	}	
	
	gebr_geoxml_object_unref(last_configured);
	return TRUE;
}

gboolean gebr_geoxml_flow_has_control_program (GebrGeoXmlFlow *flow)
{
	GebrGeoXmlProgram *control = gebr_geoxml_flow_get_control_program(flow);

	if (!control)
		return FALSE;

	gebr_geoxml_object_unref(control);
	return TRUE;
}

GebrGeoXmlProgram * gebr_geoxml_flow_get_control_program (GebrGeoXmlFlow *flow)
{
	GebrGeoXmlSequence *seq;
	GebrGeoXmlProgramControl cont;

	gebr_geoxml_flow_get_program (flow, &seq, 0);
	while (seq) {
		cont = gebr_geoxml_program_get_control(GEBR_GEOXML_PROGRAM(seq));
		if (cont != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY
		    && cont != GEBR_GEOXML_PROGRAM_CONTROL_UNKNOWN) {
			return GEBR_GEOXML_PROGRAM(seq);
		}
		gebr_geoxml_sequence_next(&seq);
	}
	return NULL;
}

gboolean gebr_geoxml_flow_insert_iter_dict (GebrGeoXmlFlow *flow)
{
	GebrGeoXmlSequence *seq;
	GebrGeoXmlParameter *param;
	GebrGeoXmlParameters *dict;
	gchar *keyword;

	dict = gebr_geoxml_document_get_dict_parameters (GEBR_GEOXML_DOCUMENT (flow));
	seq = gebr_geoxml_parameters_get_first_parameter (dict);
	keyword = gebr_geoxml_program_parameter_get_keyword (GEBR_GEOXML_PROGRAM_PARAMETER (seq));

	if (g_strcmp0 (keyword, "iter") == 0) {
		gebr_geoxml_object_unref(dict);
		gebr_geoxml_object_unref(seq);
		g_free(keyword);
		return FALSE;
	}

	param = gebr_geoxml_parameters_append_parameter(dict, GEBR_GEOXML_PARAMETER_TYPE_FLOAT);
	gebr_geoxml_program_parameter_set_list_separator(GEBR_GEOXML_PROGRAM_PARAMETER (param), "|");

	gebr_geoxml_object_unref(dict);
	gebr_geoxml_object_unref(seq);
	g_free(keyword);

	// Append four values in iter parameter to represent the
	// current value and the 'ini', 'step' and 'n' values.
	gebr_geoxml_object_unref(gebr_geoxml_program_parameter_append_value(GEBR_GEOXML_PROGRAM_PARAMETER (param), FALSE));
	gebr_geoxml_object_unref(gebr_geoxml_program_parameter_append_value(GEBR_GEOXML_PROGRAM_PARAMETER (param), FALSE));
	gebr_geoxml_object_unref(gebr_geoxml_program_parameter_append_value(GEBR_GEOXML_PROGRAM_PARAMETER (param), FALSE));

	gebr_geoxml_program_parameter_set_keyword(GEBR_GEOXML_PROGRAM_PARAMETER (param), "iter");
	gebr_geoxml_parameter_set_label(param, _("Loop iteration counter"));
	gebr_geoxml_sequence_move_after(GEBR_GEOXML_SEQUENCE (param), NULL);

	gebr_geoxml_flow_update_iter_dict_value(flow);
	gebr_geoxml_object_unref(param);

	return TRUE;
}

void gebr_geoxml_flow_remove_iter_dict (GebrGeoXmlFlow *flow)
{
	GebrGeoXmlSequence *seq;
	GebrGeoXmlParameters *dict;
	gchar *keyword;

	dict = gebr_geoxml_document_get_dict_parameters (GEBR_GEOXML_DOCUMENT (flow));
	seq = gebr_geoxml_parameters_get_first_parameter (dict);
	keyword = gebr_geoxml_program_parameter_get_keyword (GEBR_GEOXML_PROGRAM_PARAMETER (seq));

	if (g_strcmp0 (keyword, "iter") == 0)
		gebr_geoxml_sequence_remove(seq);

	gebr_geoxml_object_unref(dict);
	g_free(keyword);
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
	gchar *keyword;
	gchar *step;
	gchar *ini;
	gchar *n;
	gchar *current;

	doc = GEBR_GEOXML_DOCUMENT(flow);
	iter = GEBR_GEOXML_PROGRAM_PARAMETER(gebr_geoxml_document_get_dict_parameter(doc));
	keyword = gebr_geoxml_program_parameter_get_keyword(iter);

	if (g_strcmp0 (keyword, "iter") != 0) {
		gebr_geoxml_object_unref(iter);
		g_free(keyword);
		return;
	}
	g_free(keyword);

	program = gebr_geoxml_flow_get_control_program(flow);
	n = gebr_geoxml_program_control_get_n(program, &step, &ini);
	current = g_strdup_printf("(%s)+(%s)*((%s)-1)", ini, step, n);

	// Set the 'ini+step*(n-1)' value for 'current'
	gebr_geoxml_program_parameter_get_value(iter, FALSE, &seq, 0);
	gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(seq), current);

	// Set the 'ini' value for 'ini'
	gebr_geoxml_sequence_next(&seq);
	gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(seq), ini);
	g_free(ini);

	// Set 'step'
	gebr_geoxml_sequence_next(&seq);
	gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(seq), step);
	g_free(step);

	// Set 'n'
	gebr_geoxml_sequence_next(&seq);
	gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(seq), n);
	g_free(n);

	gebr_geoxml_object_unref(iter);
	gebr_geoxml_object_unref(program);
	if (seq)
		gebr_geoxml_object_unref(seq);

	g_free(current);
}

GQuark gebr_geoxml_flow_error_quark(void)
{
	return g_quark_from_static_string("gebr-geoxml-flow-error-quark");
}

void
gebr_geoxml_flow_revalidate(GebrGeoXmlFlow *flow, GebrValidator *validator)
{
	GebrGeoXmlProgram *prog;
	GebrGeoXmlSequence *seq;

	gebr_geoxml_flow_get_program(flow, &seq, 0);
	for (; seq; gebr_geoxml_sequence_next(&seq)) {
		prog = GEBR_GEOXML_PROGRAM(seq);
		if (gebr_geoxml_program_get_status(prog) == GEBR_GEOXML_PROGRAM_STATUS_DISABLED)
			continue;
		if (!gebr_geoxml_program_is_valid(prog, validator, NULL))
			gebr_geoxml_program_set_status(prog, GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED);
		else
			gebr_geoxml_program_set_status(prog, GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED);
	}
}

gboolean
gebr_geoxml_flow_is_parallelizable(GebrGeoXmlFlow *flow,
                                   GebrValidator *validator)
{
	GebrGeoXmlProgram *prog = gebr_geoxml_flow_get_control_program(flow);

	if (!prog || (prog && gebr_geoxml_program_get_status(prog) != GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED)) {
		gebr_geoxml_object_unref(prog);
		return FALSE;
	}
	gebr_geoxml_object_unref(prog);

	const gchar *output = gebr_geoxml_flow_io_get_output(flow);

	if (!strlen(output) || gebr_validator_use_iter(validator, output, GEBR_GEOXML_PARAMETER_TYPE_STRING, GEBR_GEOXML_DOCUMENT_TYPE_FLOW))
		return TRUE;

	return FALSE;
}

static gint *
gebr_geoxml_flow_calculate_proportional_n(gint total_n,
                                          gdouble *weights,
                                          gint n_weights)
{
	gint *distributed_n = g_new(int, n_weights);
	gint aux_total_n = total_n;

	for (gint i = 0; i < n_weights; i++)
	{
		gint partial_n;
		partial_n = total_n * weights[i];
		aux_total_n -= partial_n;

		distributed_n[i] = partial_n;
	}

	gint k = 0;
	while (aux_total_n > 0)
	{
		distributed_n[k]++;
		aux_total_n--;

		if (k == n_weights - 1)
			k = 0;
		else
			k++;
	}
	for (gint i = 0; i < n_weights; i++)
		weights[i] = (gdouble)(distributed_n[i])/(gdouble)total_n;

	return distributed_n;
}

gdouble *gebr_geoxml_flow_calulate_weights(gint n_servers,
                                           gdouble *scores,
                                           gdouble acc_scores)
{
	gdouble *weigths = g_new(double, n_servers);

	for (gint i = 0; i < n_servers; i++)
	{
		gdouble score = scores[i];
		weigths[i] = score/acc_scores;
	}

	return weigths;
}

GList *gebr_geoxml_flow_divide_flows(GebrGeoXmlFlow *flow,
                                     GebrValidator *validator,
                                     gdouble *weights,
                                     gint n_weights)
{
	if (!gebr_geoxml_flow_is_parallelizable(flow, validator)) {
		GList *l = NULL;
		return g_list_prepend(l, gebr_geoxml_document_ref(GEBR_GEOXML_DOCUMENT(flow)));
	}

	GList *flows = NULL;
	GebrGeoXmlProgram *loop;
	gchar *n, *step, *ini;
	gchar *eval_n, *eval_step, *eval_ini;
	gint total_n;
	gint *distributed_n;
	gint end, step_int, ini_int;

	loop = gebr_geoxml_flow_get_control_program(flow);
	n = gebr_geoxml_program_control_get_n(loop, &step, &ini);
	gebr_geoxml_object_unref(loop);

	gebr_validator_evaluate(validator, n, GEBR_GEOXML_PARAMETER_TYPE_INT, GEBR_GEOXML_DOCUMENT_TYPE_LINE, &eval_n, NULL);
	gebr_validator_evaluate(validator, step, GEBR_GEOXML_PARAMETER_TYPE_INT, GEBR_GEOXML_DOCUMENT_TYPE_LINE, &eval_step, NULL);
	gebr_validator_evaluate(validator, ini, GEBR_GEOXML_PARAMETER_TYPE_INT, GEBR_GEOXML_DOCUMENT_TYPE_LINE, &eval_ini, NULL);


	total_n = (gint) g_strtod(eval_n, NULL);
	distributed_n = gebr_geoxml_flow_calculate_proportional_n(total_n, weights, n_weights);

	step_int = (gint) g_strtod(eval_step, NULL);
	ini_int = (gint) g_strtod(eval_ini, NULL);

	for (gint i = 0; i < n_weights; i++)
	{
		GebrGeoXmlFlow *div_flow;
		GebrGeoXmlProgram *div_loop;
		const gchar *div_n;

		if (distributed_n[i] <= 0)
			break;

		ini = g_strdup_printf("%d", ini_int);
		end = (distributed_n[i]-1)*step_int+ini_int;
		div_n = g_strdup_printf("%d", distributed_n[i]);

		div_flow = GEBR_GEOXML_FLOW(gebr_geoxml_document_clone(GEBR_GEOXML_DOCUMENT(flow)));
		div_loop = gebr_geoxml_flow_get_control_program(div_flow);
		gebr_geoxml_program_control_set_n(div_loop, eval_step, ini, div_n);
		flows = g_list_append(flows, div_flow);
		gebr_geoxml_object_unref(div_loop);
		ini_int = end+1;
	}

	g_free(eval_n);
	g_free(ini);
	g_free(step);
	g_free(eval_ini);
	g_free(eval_step);
	g_free(distributed_n);

	return flows;
}
