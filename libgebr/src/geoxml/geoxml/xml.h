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

#ifndef __GEBR_GEOXML_XML_H
#define __GEBR_GEOXML_XML_H

#include <gdome.h>
#include <gdome-xpath.h>
#include <glib.h>

#include "../../utils.h"

G_BEGIN_DECLS

/**
 * \internal
 *
 */
typedef void (*createValueNode_function) (GdomeElement *, const gchar *);

/**
 * \internal
 *
 */
void __gebr_geoxml_create_CDATASection(GdomeElement * parent_element, const gchar * value);

/**
 * \internal
 *
 */
void __gebr_geoxml_create_TextNode(GdomeElement * parent_element, const gchar * value);

/**
 * \internal
 * Create a new element.
 * \p parent_element is only used to know the associated document.
 */
GdomeElement *__gebr_geoxml_new_element(GdomeElement * parent_element, const gchar * tag_name);

/**
 * \internal
 * Create a new element and insert it before \p before_element.
 * If \p before_element the element is appended.
 * \see __gebr_geoxml_insert_new_element
 */
GdomeElement *__gebr_geoxml_insert_new_element(GdomeElement * parent_element, const gchar * tag_name,
					       GdomeElement * before_element);

/**
 * \internal
 * Protect against strange behaviou of insertBefore, when \p newChild is equal to \p refChild.
 */
GdomeNode *gdome_n_insertBefore_protected(GdomeNode * self, GdomeNode *newChild,
					   GdomeNode *refChild, GdomeException *exc);
GdomeNode *gdome_el_insertBefore_protected(GdomeElement * self, GdomeNode *newChild,
					   GdomeNode *refChild, GdomeException *exc);

/**
 * \internal
 * Same as gdome_el_cloneNode but generating new ids when needed
 */
GdomeElement* gdome_el_cloneNode_protected(GdomeElement * el);

/**
 * \internal
 * Same as gdome_doc_importNode but generating new ids when needed
 */
GdomeNode* gdome_doc_importNode_protected(GdomeDocument * self, GdomeElement * source);

/**
 * \internal
 * Get the first child element of \p parent_element with tag name
 * \p tag_name. The child might not be a direct child of \p parent_element
 * (the search done is recursive)
 */
GdomeElement *__gebr_geoxml_get_first_element(GdomeElement * parent_element, const gchar * tag_name);

/**
 * \internal
 * Get the child element of \p parent_element at \p index position.
 *
 * If recursive is TRUE, all the children (direct or not) are considered
 */
GdomeElement *__gebr_geoxml_get_element_at(GdomeElement * parent_element, const gchar * tag_name, gulong index,
					   gboolean recursive);

/**
 * \internal
 *
 */
GdomeElement *__gebr_geoxml_get_element_by_id(GdomeElement * base, const gchar * id);

/**
 * \internal
 * Use with gebr_foreach_gslist, e.g.
 * gebr_foreach_gslist(element, __gebr_geoxml_get_elements_by_tag) { }
 */
GSList *__gebr_geoxml_get_elements_by_tag(GdomeElement * base, const gchar * tag);

/**
 * \internal
 *
 */
GSList *__gebr_geoxml_get_elements_by_idref(GdomeElement * base, const gchar * idref, gboolean global);

/**
 * \internal
 *
 */
gulong __gebr_geoxml_get_element_index(GdomeElement * element);

/**
 * \internal
 * Search only for childs elements named \p tag_name of \p parent_element, not recursevely
 */
gulong __gebr_geoxml_get_elements_number(GdomeElement * parent_element, const gchar * tag_name);

/**
 * \internal
 *
 */
const gchar *__gebr_geoxml_get_element_value(GdomeElement * element);

/**
 * \internal
 *
 */
void

 __gebr_geoxml_set_element_value(GdomeElement * element, const gchar * tag_value, createValueNode_function create_func);

/**
 * \internal
 *
 */
const gchar *__gebr_geoxml_get_tag_value(GdomeElement * parent_element, const gchar * tag_name);

/**
 * \internal
 *
 */
void


__gebr_geoxml_set_tag_value(GdomeElement * parent_element, const gchar * tag_name, const gchar * tag_value,
			    createValueNode_function create_func);

/**
 * \internal
 * Set attribute with name \p attr_name of \p element to \p attr_value.
 * If it doesn't exist it is created.
 */
void __gebr_geoxml_set_attr_value(GdomeElement * element, const gchar * name, const gchar * value);

/**
 * \internal
 * Get attribute with name \p attr_name of \p element value.
 * If it doesn't exist it an empty string is returned.
 */
const gchar *__gebr_geoxml_get_attr_value(GdomeElement * element, const gchar * name);

/**
 * \internal
 */
void __gebr_geoxml_remove_attr(GdomeElement * element, const gchar * name);

/**
 * \internal
 *
 */
GdomeElement *__gebr_geoxml_previous_element(GdomeElement * element);

/**
 * \internal
 *
 */
GdomeElement *__gebr_geoxml_next_element(GdomeElement * element);

/**
 * \internal
 *
 */
GdomeElement *__gebr_geoxml_previous_same_element(GdomeElement * element);

/**
 * \internal
 *
 */
GdomeElement *__gebr_geoxml_next_same_element(GdomeElement * element);

/**
 * \internal
 * Assign to \p element a new unique ID based on document nextid attibute.
 * Use nextid and use it as ID
 * Automatically change references ids.
 */
void __gebr_geoxml_element_assign_new_id(GdomeElement * element, const gchar *oldid, GdomeElement * reassign_context);

/**
 * \internal
 */
void __gebr_geoxml_element_reassign_id(GdomeElement * element, GdomeElement * reassign_context);

/**
 * \internal
 * Assign \p reference's ID to \p element
 */
void __gebr_geoxml_element_assign_reference_id(GdomeElement * element, GdomeElement * referencee);

/**
 * \internal
 * Easy function to evaluate a XPath expression. Use in combination with
 * \ref __gebr_geoxml_foreach_xpath_result or use .
 */
GdomeXPathResult *__gebr_geoxml_xpath_evaluate(GdomeElement * context, const gchar * expression);

/**
 * \internal
 * Prints an xml node to stdout.
 */
void __gebr_geoxml_to_string(GdomeElement * el);

/**
 * \internal
 * Iterates elements of a GdomeXPathResult at \p result
 * This macro will auto free \p result and you can use it
 * as __gebr_geoxml_foreach_xpath_result(element, __gebr_geoxml_xpath_evaluate(context, expression)).
 * If you use 'break' then you have to free it yourself.
 */
#define __gebr_geoxml_foreach_xpath_result(element, xpath_result) \
	GdomeXPathResult * __xpath_result = xpath_result; \
	if (__xpath_result != NULL) \
		for (element = (GdomeElement*)gdome_xpresult_singleNodeValue(__xpath_result, &exception); \
		element != NULL || (gdome_xpresult_unref(__xpath_result, &exception), 0); \
		element = (GdomeElement*)gdome_xpresult_iterateNext(__xpath_result, &exception))

/**
 * \internal
 * Iterates elements of a GSList at \p list
 * This macro will auto free \p list and you can use it
 * as __gebr_geoxml_foreach_element(element, __gebr_geoxml_get_elements_by_idref(base, id)).
 * If you use 'break' then you have to free it yourself.
 */
#define __gebr_geoxml_foreach_element(element, list) \
	gebr_foreach_gslist(element, list)

/**
 * \internal
 * Look for each _element_ child of base (recursive search, not necessarily a direct child)
 * and with tag name _tagname_
 * A break in the loop will leak memory
 */
#define __gebr_geoxml_foreach_element_with_tagname_r(base, tagname, element, hygid) \
	GdomeNodeList *		__node_list##hygid; \
	GdomeDOMString *	__string##hygid; \
	int			__i##hygid, __l##hygid; \
	__string##hygid = gdome_str_mkref(tagname); \
	__node_list##hygid = gdome_el_getElementsByTagName(base, __string##hygid, &exception); \
	gdome_str_unref(__string##hygid); \
	__l##hygid = gdome_nl_length(__node_list##hygid, &exception); \
	for (__i##hygid = 0, element = (GdomeElement*)gdome_nl_item(__node_list##hygid, 0, &exception); \
		__i##hygid < __l##hygid || (gdome_nl_unref(__node_list##hygid, &exception), 0); \
		element = (GdomeElement*)gdome_nl_item(__node_list##hygid, ++__i##hygid, &exception))

G_END_DECLS
#endif				//__GEBR_GEOXML_XML_H
