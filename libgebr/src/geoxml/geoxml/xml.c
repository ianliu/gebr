/*   libgebr - GÍBR Library
 *   Copyright (C) 2007  Br·ulio Barros de Oliveira (brauliobo@gmail.com)
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
#include <glib.h>
#include <string.h>

#include "xml.h"
#include "types.h"

/*
 * Internal internal functions
 */

static gchar *
__geoxml_remove_line_break(const gchar * tag_value)
{
	gchar *		linebreak;
	GString *	value;
	gchar *		new_tag_value;

	if ((linebreak = strchr(tag_value, 0x0A)) == NULL)
		return (gchar*)tag_value;

	value = g_string_new(tag_value);
	while ((linebreak = strchr(value->str, 0x0A)) != NULL)
		g_string_erase(value, (linebreak - value->str), 1);

	new_tag_value = value->str;
	g_string_free(value, FALSE);
	return new_tag_value;
}

/*
 * Auxiliary library functions
 */

void
__geoxml_create_CDATASection(GdomeElement * parent_element, const gchar * value)
{
	GdomeNode *	node;

	node = (GdomeNode*)gdome_doc_createCDATASection(gdome_el_ownerDocument(parent_element, &exception),
			gdome_str_mkref_dup(value), &exception);
	gdome_el_appendChild(parent_element, node, &exception);
}

void
__geoxml_create_TextNode(GdomeElement * parent_element, const gchar * value)
{
	GdomeNode *	node;

	node = (GdomeNode*)gdome_doc_createTextNode(gdome_el_ownerDocument(parent_element, &exception),
			gdome_str_mkref_dup(value), &exception);
	gdome_el_appendChild(parent_element, node, &exception);
}

GdomeElement *
__geoxml_new_element(GdomeElement * parent_element, const gchar * tag_name)
{
	GdomeElement *	element;

	element = gdome_doc_createElement(gdome_el_ownerDocument(parent_element, &exception),
		gdome_str_mkref(tag_name), &exception);

	return element;
}

GdomeElement *
__geoxml_insert_new_element(GdomeElement * parent_element, const gchar * tag_name, GdomeElement * before_element)
{
	GdomeElement *	element;

	element = __geoxml_new_element(parent_element, tag_name);
	gdome_el_insertBefore(parent_element, (GdomeNode*)element, (GdomeNode*)before_element, &exception);

	return element;
}

GdomeElement *
__geoxml_get_first_element(GdomeElement * parent_element, const gchar * tag_name)
{
	GdomeNode *		node;
	GdomeNodeList *		node_list;
	GdomeDOMString *	string;

	string = gdome_str_mkref(tag_name);

	/* get the list of elements with this tag_name. */
	node_list = gdome_el_getElementsByTagName(parent_element, string, &exception);
	node = gdome_nl_item(node_list, 0, &exception);

	gdome_str_unref(string);
	gdome_nl_unref(node_list, &exception);

	return (GdomeElement*)node;
}

GdomeElement *
__geoxml_get_element_at(GdomeElement * parent_element, const gchar * tag_name, gulong index)
{
	GdomeElement *		element;
	GdomeNodeList *		node_list;
	GdomeDOMString *	string;

	string = gdome_str_mkref(tag_name);

	node_list = gdome_el_getElementsByTagName(parent_element, string, &exception);
	if (index >= gdome_nl_length(node_list, &exception))
		return NULL;
	element = (GdomeElement*)gdome_nl_item(node_list, index, &exception);

	gdome_str_unref(string);
	gdome_nl_unref(node_list, &exception);

	return element;
}

gulong
__geoxml_get_elements_number(GdomeElement * parent_element, const gchar * tag_name)
{
	GdomeNodeList *		node_list;
	GdomeDOMString *	string;
	gulong			elements_number;

	string = gdome_str_mkref(tag_name);

	node_list = gdome_el_getElementsByTagName(parent_element, string, &exception);
	elements_number = gdome_nl_length(node_list, &exception);

	gdome_str_unref(string);
	gdome_nl_unref(node_list, &exception);

	return elements_number;
}

const gchar *
__geoxml_get_element_value(GdomeElement * element)
{
	GdomeDOMString *	string;
	GdomeNode *		child;

	child = gdome_el_firstChild(element, &exception);
	string = gdome_n_nodeValue(child, &exception);
	if (string == NULL)
		return "";
	if (gdome_n_nodeType(child, &exception) == GDOME_TEXT_NODE) {
		gchar *		protected_str;

		protected_str = __geoxml_remove_line_break(string->str);
		if (protected_str != string->str)
			gdome_n_set_nodeValue(child, gdome_str_mkref(protected_str), &exception);

		return protected_str;
	}

	return string->str;
}

void
__geoxml_set_element_value(GdomeElement * element, const gchar * tag_value,
	createValueNode_function create_func)
{
	GdomeNode *		value_node;
	const gchar *		value_str;

	/* Protection agains line-breaks inside text nodes */
	value_str = (create_func == __geoxml_create_TextNode)
		? __geoxml_remove_line_break(tag_value) : tag_value;

	value_node = gdome_el_firstChild(element, &exception);
	if (value_node == NULL)
		create_func(element, value_str);
	else
		gdome_n_set_nodeValue(value_node, gdome_str_mkref_dup(value_str), &exception);
}

const gchar *
__geoxml_get_tag_value(GdomeElement * parent_element, const gchar * tag_name)
{
	return __geoxml_get_element_value(__geoxml_get_first_element(parent_element, tag_name));
}

void
__geoxml_set_tag_value(GdomeElement * parent_element, const gchar * tag_name, const gchar * tag_value,
	createValueNode_function create_func)
{
	__geoxml_set_element_value(__geoxml_get_first_element(parent_element, tag_name), tag_value, create_func);
}

const gchar *
__geoxml_get_attr_value(GdomeElement * element, const gchar * attr_name)
{
	GdomeDOMString *	string;
	GdomeDOMString *	attr_value;

	string = gdome_str_mkref(attr_name);
	attr_value = gdome_el_getAttribute(element, string, &exception);
	gdome_str_unref(string);

	return attr_value->str;
}

void
__geoxml_set_attr_value(GdomeElement * element, const gchar * attr_name, const gchar * attr_value)
{
	GdomeDOMString *	string;

	string = gdome_str_mkref(attr_name);
	gdome_el_setAttribute(element, string, gdome_str_mkref_dup(attr_value), &exception);
	gdome_str_unref(string);
}

GdomeElement *
__geoxml_previous_element(GdomeElement * element)
{
	GdomeNode *	node;

	node = (GdomeNode*)element;
	do
		node = gdome_n_previousSibling(node, &exception);
	while ((node != NULL) && (gdome_n_nodeType(node, &exception) != GDOME_ELEMENT_NODE));

	return (GdomeElement*)node;
}

GdomeElement *
__geoxml_next_element(GdomeElement * element)
{
	GdomeNode *	node;

	node = (GdomeNode*)element;
	do
		node = gdome_n_nextSibling(node, &exception);
	while ((node != NULL) && (gdome_n_nodeType(node, &exception) != GDOME_ELEMENT_NODE));

	return (GdomeElement*)node;
}

GdomeElement *
__geoxml_previous_same_element(GdomeElement * element)
{
	GdomeElement *	previous_element;

	previous_element = __geoxml_previous_element(element);
	if (previous_element != NULL &&
	gdome_str_equal(gdome_el_nodeName(element, &exception), gdome_el_nodeName(previous_element, &exception)))
		return previous_element;

	return NULL;
}

GdomeElement *
__geoxml_next_same_element(GdomeElement * element)
{
	GdomeElement *	next_element;

	next_element = __geoxml_next_element(element);
	if (next_element != NULL &&
	gdome_str_equal(gdome_el_nodeName(element, &exception), gdome_el_nodeName(next_element, &exception)))
		return next_element;

	return NULL;
}
