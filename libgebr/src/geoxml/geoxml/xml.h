/*   libgebr - GÍBR Library
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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
 * \p tag_name.
 */
GdomeElement *
__geoxml_get_first_element(GdomeElement * parent_element, const gchar * tag_name);

/**
 * \internal
 *
 */
GdomeElement *
__geoxml_get_element_at(GdomeElement * parent_element, const gchar * tag_name, gulong index);

/**
 * \internal
 *
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
 * Get attribute with name \p attr_name of \p element value.
 * If it doesn't exist it an empty string is returned.
 */
const gchar *
__geoxml_get_attr_value(GdomeElement * element, const gchar * attr_name);

/**
 * \internal
 * Set attribute with name \p attr_name of \p element to \p attr_value.
 * If it doesn't exist it created.
 */
void
__geoxml_set_attr_value(GdomeElement * element, const gchar * attr_name, const gchar * attr_value);

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

#endif //__LIBGEBR_GEOXML_XML_H
