/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
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

#ifndef __LIBGEBR_GEOXML_XML_H
#define __LIBGEBR_GEOXML_XML_H

#include <gdome.h>
#include <gdome-xpath.h>
#include <glib.h>

/**
 * \internal
 *
 */
typedef void (*createValueNode_function)(GdomeElement *, const gchar *);

/**
 * \internal
 *
 */
void __geoxml_create_CDATASection(GdomeElement * parent_element, const gchar * value);

/**
 * \internal
 *
 */
void __geoxml_create_TextNode(GdomeElement * parent_element, const gchar * value);

/**
 * \internal
 * Create a new element.
 * \p parent_element is only used to know the associated document.
 */
GdomeElement *
__geoxml_new_element(GdomeElement * parent_element, const gchar * tag_name);

/**
 * \internal
 * Create a new element and insert it before \p before_element.
 * If \p before_element the element is appended.
 * \see __geoxml_insert_new_element
 */
GdomeElement *
__geoxml_insert_new_element(GdomeElement * parent_element, const gchar * tag_name, GdomeElement * before_element);

/**
 * \internal
 * Get the first child element of \p parent_element with tag name
 * \p tag_name. The child might not be a direct child of \p parent_element
 * (the search done is recursive)
 */
GdomeElement *
__geoxml_get_first_element(GdomeElement * parent_element, const gchar * tag_name);

/**
 * \internal
 * Get the child element of \p parent_element at \p index position. If recursive is TRUE,
 * all the childs (direct or not) are considered
 */
GdomeElement *
__geoxml_get_element_at(GdomeElement * parent_element, const gchar * tag_name, gulong index, gboolean recursive);

/**
 * \internal
 *
 */
GdomeElement *
__geoxml_get_element_by_id(GdomeElement * base, const gchar * id);

/**
 * \internal
 *
 */
GSList *
__geoxml_get_elements_by_idref(GdomeElement * base, const gchar * idref, gboolean global);

/**
 * \internal
 *
 */
glong
__geoxml_get_element_index(GdomeElement * element);

/**
 * \internal
 * Search only for childs elements named \p tag_name of \p parent_element, not recursevely
 */
gulong
__geoxml_get_elements_number(GdomeElement * parent_element, const gchar * tag_name);

/**
 * \internal
 *
 */
const gchar *
__geoxml_get_element_value(GdomeElement * element);

/**
 * \internal
 *
 */
void
__geoxml_set_element_value(GdomeElement * element, const gchar * tag_value,
	createValueNode_function create_func);

/**
 * \internal
 *
 */
const gchar *
__geoxml_get_tag_value(GdomeElement * parent_element, const gchar * tag_name);

/**
 * \internal
 *
 */
void
__geoxml_set_tag_value(GdomeElement * parent_element, const gchar * tag_name, const gchar * tag_value,
	createValueNode_function create_func);

/**
 * \internal
 * Set attribute with name \p attr_name of \p element to \p attr_value.
 * If it doesn't exist it is created.
 */
void
__geoxml_set_attr_value(GdomeElement * element, const gchar * name, const gchar * value);

/**
 * \internal
 * Get attribute with name \p attr_name of \p element value.
 * If it doesn't exist it an empty string is returned.
 */
const gchar *
__geoxml_get_attr_value(GdomeElement * element, const gchar * name);

/**
 * \internal
 */
void
__geoxml_remove_attr(GdomeElement * element, const gchar * name);

/**
 * \internal
 *
 */
GdomeElement *
__geoxml_previous_element(GdomeElement * element);

/**
 * \internal
 *
 */
GdomeElement *
__geoxml_next_element(GdomeElement * element);

/**
 * \internal
 *
 */
GdomeElement *
__geoxml_previous_same_element(GdomeElement * element);

/**
 * \internal
 *
 */
GdomeElement *
__geoxml_next_same_element(GdomeElement * element);

/**
 * \internal
 * Assign to \p element a new unique ID based on document nextid attibute.
 * Use nextid and use it as ID
 * Automatically change references ids.
 */
void
__geoxml_element_assign_new_id(GdomeElement * element, GdomeElement * context, gboolean reassign_refereceds);

/**
 * \internal
 * Recursively reassign all elements in \p element (including it)
 * using __geoxml_element_assign_new_id.
 */
void
__geoxml_element_reassign_ids(GdomeElement * element);

/**
 * \internal
 * Assign \p reference's ID to \p element
 */
void
__geoxml_element_assign_reference_id(GdomeElement * element, GdomeElement * referencee);

/**
 * \internal
 * Easy function to evaluate a XPath expression. Use in combination with
 * \ref __geoxml_foreach_xpath_result or use .
 */
GdomeXPathResult *
__geoxml_xpath_evaluate(GdomeElement * context, const gchar * expression);

/**
 * \internal
 * Iterates elements of a GdomeXPathResult at \p result
 * This macro will auto free \p result and you can use it
 * as __geoxml_foreach_xpath_result(element, __geoxml_xpath_evaluate(context, expression)).
 * If you use 'break' then you have to free it yourself.
 */
#define __geoxml_foreach_xpath_result(element, xpath_result) \
	GdomeXPathResult * __xpath_result = xpath_result; \
	if (__xpath_result != NULL) \
		for (element = (GdomeElement*)gdome_xpresult_singleNodeValue(__xpath_result, &exception); \
		element != NULL || (gdome_xpresult_unref(__xpath_result, &exception), 0); \
		element = (GdomeElement*)gdome_xpresult_iterateNext(__xpath_result, &exception))

/**
 * \internal
 * Iterates elements of a GSList at \p list
 * This macro will auto free \p list and you can use it
 * as __geoxml_foreach_element(element, __geoxml_get_elements_by_idref(base, id)).
 * If you use 'break' then you have to free it yourself.
 */
#define __geoxml_foreach_element(element, list) \
	GSList * __i = list; \
	if (__i != NULL || (g_slist_free(list), 0)) \
		for (element = (GdomeElement*)__i->data; \
		(__i != NULL && (element = (GdomeElement*)__i->data, 1)) || (g_slist_free(list), 0); \
		__i = g_slist_next(__i))

/**
 * hygienizing the macro...
 */
#define __geoxml_foreach_element_with_tagname_r_aux(base, tagname, element) \
	GdomeNodeList *		__node_list; \
	GdomeDOMString *	__string; \
	int			__i, __l; \
	__string = gdome_str_mkref(tagname); \
	__node_list = gdome_el_getElementsByTagName(base, __string, &exception); \
	gdome_str_unref(__string); \
	__l = gdome_nl_length(__node_list, &exception); \
	for (__i = 0, element = (GdomeElement*)gdome_nl_item(__node_list, 0, &exception); \
		__i < __l || (gdome_nl_unref(__node_list, &exception), 0); \
		element = (GdomeElement*)gdome_nl_item(__node_list, ++__i, &exception))
/**
 * \internal
 * Look for each _element_ child of base (recursive search, not necessarily a direct child)
 * and with tag name _tagname_
 * A break in the loop will leak memory
 */
#define __geoxml_foreach_element_with_tagname_r(base, tagname, element) \
	{ __geoxml_foreach_element_with_tagname_r_aux(base, tagname, element) {} }

#endif //__LIBGEBR_GEOXML_XML_H
