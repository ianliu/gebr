/*   libgeoxml - An interface to describe seismic software in XML
 *   Copyright (C) 2007  Bráulio Barros de Oliveira (brauliobo@gmail.com)
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

#include "flow.h"
#include "document.h"
#include "document_p.h"
#include "program.h"
#include "xml.h"
#include "error.h"
#include "types.h"
#include "sequence.h"

/*
 * internal structures and funcionts
 */

struct geoxml_flow {
	GeoXmlDocument * document;
};

/*
 * library functions.
 */

GeoXmlFlow *
geoxml_flow_new()
{
	GeoXmlDocument *	document;
	GdomeElement *		element;

	document = geoxml_document_new("flow", "0.1.0");

	element = __geoxml_new_element(geoxml_document_root_element(document), NULL, "io");
	__geoxml_new_element(element, NULL, "input");
	__geoxml_new_element(element, NULL, "output");
	__geoxml_new_element(element, NULL, "error");

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
		GdomeNode* new_node = gdome_doc_importNode((GdomeDocument*)flow,
			gdome_nl_item(flow2_node_list, i, &exception), TRUE, &exception);

		gdome_el_appendChild(geoxml_document_root_element(GEOXML_DOC(flow)), new_node, &exception);

		geoxml_program_set_help((GeoXmlProgram*)new_node, "");
	}

	gdome_str_unref(string);
	gdome_nl_unref(flow2_node_list, &exception);
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

GeoXmlProgram *
geoxml_flow_new_program(GeoXmlFlow * flow)
{
	if (flow == NULL)
		return NULL;

	GdomeElement *	program_element;

	program_element = __geoxml_new_element(geoxml_document_root_element(GEOXML_DOC(flow)), NULL, "program");

	/* elements/attibutes */
	geoxml_program_set_stdin((GeoXmlProgram*)program_element, FALSE);
	geoxml_program_set_stdout((GeoXmlProgram*)program_element, FALSE);
	geoxml_program_set_stderr((GeoXmlProgram*)program_element, FALSE);
	geoxml_program_set_status((GeoXmlProgram*)program_element, "unconfigured");
	__geoxml_new_element(program_element, NULL, "menu");
	geoxml_program_set_menu((GeoXmlProgram*)program_element, geoxml_document_get_filename(GEOXML_DOC(flow)), -1);
	__geoxml_new_element(program_element, NULL, "title");
	__geoxml_new_element(program_element, NULL, "binary");
	__geoxml_new_element(program_element, NULL, "description");
	__geoxml_new_element(program_element, NULL, "help");
	__geoxml_new_element(program_element, NULL, "parameters");

	return (GeoXmlProgram*)program_element;
}

int
geoxml_flow_get_program(GeoXmlFlow * flow, GeoXmlProgram ** program, gulong index)
{
	if (flow == NULL) {
		*program = NULL;
		return GEOXML_RETV_NULL_PTR;
	}

	*program = (GeoXmlProgram*)__geoxml_get_element_at(geoxml_document_root_element(GEOXML_DOC(flow)), "program", index);

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
geoxml_flow_new_category(GeoXmlFlow * flow, const gchar * name)
{
	if (flow == NULL)
		return NULL;

	GeoXmlCategory *	category;

	category = (GeoXmlCategory*)__geoxml_new_element(geoxml_document_root_element(GEOXML_DOC(flow)),
		__geoxml_get_first_element(geoxml_document_root_element(GEOXML_DOC(flow)), "io"), "category");
	geoxml_category_set_name(category, name);

	return category;
}

int
geoxml_flow_get_category(GeoXmlFlow * flow, GeoXmlCategory ** category, gulong index)
{
	if (flow == NULL) {
		*category = NULL;
		return GEOXML_RETV_NULL_PTR;
	}

	*category = (GeoXmlCategory*)__geoxml_get_element_at(geoxml_document_root_element(GEOXML_DOC(flow)), "category", index);

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

void
geoxml_flow_remove_program(GeoXmlFlow * flow, GeoXmlProgram * program)
{
	geoxml_sequence_remove((GeoXmlSequence*)program);
}

void
geoxml_flow_move_program(GeoXmlFlow * flow, GeoXmlProgram * program, GeoXmlProgram * before_program)
{
	geoxml_sequence_move((GeoXmlSequence*)program, (GeoXmlSequence*)before_program);
}

int
geoxml_flow_move_program_up(GeoXmlFlow * flow, GeoXmlProgram * program)
{
	return geoxml_sequence_move_up((GeoXmlSequence*)program);
}

int
geoxml_flow_move_program_down(GeoXmlFlow * flow, GeoXmlProgram * program)
{
	return geoxml_sequence_move_down((GeoXmlSequence*)program);
}

GeoXmlCategory *
geoxml_flow_append_category(GeoXmlFlow * flow, const gchar * name)
{
	return geoxml_flow_new_category(flow, name);
}

void
geoxml_flow_remove_category(GeoXmlFlow * flow, GeoXmlCategory * category)
{
	geoxml_sequence_remove((GeoXmlSequence*)category);
}
