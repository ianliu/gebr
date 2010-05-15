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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "xml.h"
#include "types.h"

/*
 * Internal internal functions
 */

const static gchar *id_tags[] = {
	"parameter", NULL,
};

static gchar *__gebr_geoxml_remove_line_break(const gchar * tag_value)
{
	gchar *linebreak;
	GString *value;
	gchar *new_tag_value;

	if (strchr(tag_value, 0x0A) == NULL)
		return (gchar *) tag_value;

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

void __gebr_geoxml_create_CDATASection(GdomeElement * parent_element, const gchar * value)
{
	GdomeNode *node;

	node = (GdomeNode *) gdome_doc_createCDATASection(gdome_el_ownerDocument(parent_element, &exception),
							  gdome_str_mkref_dup(value), &exception);
	gdome_el_appendChild(parent_element, node, &exception);
}

void __gebr_geoxml_create_TextNode(GdomeElement * parent_element, const gchar * value)
{
	GdomeNode *node;

	node = (GdomeNode *) gdome_doc_createTextNode(gdome_el_ownerDocument(parent_element, &exception),
						      gdome_str_mkref_dup(value), &exception);
	gdome_el_appendChild(parent_element, node, &exception);
}

GdomeElement *__gebr_geoxml_new_element(GdomeElement * parent_element, const gchar * tag_name)
{
	GdomeElement *element;

	element = gdome_doc_createElement(gdome_el_ownerDocument(parent_element, &exception),
					  gdome_str_mkref(tag_name), &exception);

	return element;
}

GdomeNode *gdome_n_insertBefore_protected(GdomeNode * self, GdomeNode *newChild,
					   GdomeNode *refChild, GdomeException *exc)
{
	if (newChild == refChild) {
		*exc = GDOME_NOEXCEPTION_ERR;
		return newChild;
	}
	return gdome_n_insertBefore(self, newChild, refChild, exc);
}

GdomeNode *gdome_el_insertBefore_protected(GdomeElement * self, GdomeNode *newChild,
					   GdomeNode *refChild, GdomeException *exc)
{
	return gdome_n_insertBefore_protected((GdomeNode*)self, newChild, refChild, exc);
}

GdomeElement *__gebr_geoxml_insert_new_element(GdomeElement * parent_element, const gchar * tag_name,
					       GdomeElement * before_element)
{
	GdomeElement *element;

	element = __gebr_geoxml_new_element(parent_element, tag_name);
	gdome_el_insertBefore_protected(parent_element, (GdomeNode *) element, (GdomeNode *) before_element, &exception);

	return element;
}

GdomeElement *__gebr_geoxml_get_first_element(GdomeElement * parent_element, const gchar * tag_name)
{
	GdomeNode *node;
	GdomeNodeList *node_list;
	GdomeDOMString *string;

	string = gdome_str_mkref(tag_name);

	/* get the list of elements with this tag_name. */
	node_list = gdome_el_getElementsByTagName(parent_element, string, &exception);
	node = gdome_nl_item(node_list, 0, &exception);

	gdome_str_unref(string);
	gdome_nl_unref(node_list, &exception);

	return (GdomeElement *) node;
}

GdomeElement *__gebr_geoxml_get_element_at(GdomeElement * parent_element, const gchar * tag_name, gulong index,
					   gboolean recursive)
{
	if (recursive == FALSE) {
		GString *expression;
		GdomeElement *child;
		GdomeXPathResult *xpath_result;

		expression = g_string_new(NULL);

		g_string_printf(expression, "child::%s[%lu]", tag_name, index + 1);
		xpath_result = __gebr_geoxml_xpath_evaluate(parent_element, expression->str);
		child = (GdomeElement *) gdome_xpresult_singleNodeValue(xpath_result, &exception);

		g_string_free(expression, TRUE);
		gdome_xpresult_unref(xpath_result, &exception);

		return child;
	} else {
		GdomeElement *element;
		GdomeNodeList *node_list;
		GdomeDOMString *string;

		string = gdome_str_mkref(tag_name);

		node_list = gdome_el_getElementsByTagName(parent_element, string, &exception);
		if (index >= gdome_nl_length(node_list, &exception))
			return NULL;
		element = (GdomeElement *) gdome_nl_item(node_list, index, &exception);

		gdome_str_unref(string);
		gdome_nl_unref(node_list, &exception);

		return element;
	}
}

GdomeElement *__gebr_geoxml_get_element_by_id(GdomeElement * base, const gchar * id)
{
	/* gdome_doc_getElementById only work with xml:id element */
	GdomeElement *document_element;
	gint i;

	document_element = gdome_doc_documentElement(gdome_el_ownerDocument(base, &exception), &exception);
	for (i = 0; id_tags[i] != NULL; ++i) {

		GdomeDOMString *string = gdome_str_mkref(id_tags[i]);
		GdomeNodeList *node_list;
		node_list = gdome_el_getElementsByTagName(document_element, string, &exception);
		gdome_nl_unref(node_list, &exception);
		gdome_str_unref(string);

		GSList * list;
		for (GSList *i = list; i != NULL; i = g_slist_next(i)) {
			GdomeElement *element;

			element = (GdomeElement *)i->data;
			if (!strcmp(__gebr_geoxml_get_attr_value(element, "id"), id))
				return element;
		}
	}

	return NULL;

/* only work with xml:id att. name */
//      GString *               expression;
//      GdomeXPathResult *      xpath_result;
//      GdomeElement *          document_element;
//      GdomeElement *          element;
// 
//      expression = g_string_new(NULL);
//      document_element = gdome_doc_documentElement(gdome_el_ownerDocument(base, &exception), &exception);
//      g_string_printf(expression, "id('%s')", id);

//      xpath_result = __gebr_geoxml_xpath_evaluate(document_element, expression->str);
//      element = (GdomeElement*)gdome_xpresult_singleNodeValue(xpath_result, &exception);
// 
//      /* frees */
//      g_string_free(expression, TRUE);
//      gdome_xpresult_unref(xpath_result, &exception);
// 
//      return element;
/* simple, but doesn't work :( */
//      GdomeDOMString *        string;
//      GdomeElement *          element;
// 
//      string = gdome_str_mkref(id);
//      element = gdome_doc_getElementById(gdome_el_ownerDocument(base, &exception), string, &exception);
//      gdome_str_unref(string);
// 
//      return element;
}

GSList *__gebr_geoxml_get_elements_by_tag(GdomeElement * base, const gchar * tag)
{
	GSList *element_list;
	GdomeDOMString *string;
	GdomeNodeList *node_list;
	gint l;

	element_list = NULL;
	string = gdome_str_mkref(tag);
	node_list = gdome_el_getElementsByTagName(base, string, &exception);
	l = gdome_nl_length(node_list, &exception);
	for (int i = 0; i < l; ++i) {
		GdomeElement *element;

		element = (GdomeElement *) gdome_nl_item(node_list, i, &exception);
		element_list = g_slist_prepend(element_list, element);
	}

	gdome_str_unref(string);
	gdome_nl_unref(node_list, &exception);

	return element_list;
}

GSList *__gebr_geoxml_get_elements_by_idref(GdomeElement * base, const gchar * idref, gboolean global)
{
	const static gchar *reference_tags[] = {
		"reference", NULL,
	};
	GSList *idref_list;
	GdomeDOMString *string, *idref_string;
	GdomeNodeList *node_list;
	gint k, i, l;

	idref_list = NULL;
	if (global)
		base = gdome_doc_documentElement(gdome_el_ownerDocument(base, &exception), &exception);
	idref_string = gdome_str_mkref("idref");

	for (k = 0; reference_tags[k] != NULL; ++k) {
		string = gdome_str_mkref(reference_tags[k]);
		node_list = gdome_el_getElementsByTagName(base, string, &exception);

		l = gdome_nl_length(node_list, &exception);
		/* get the list of elements with this tag_name. */
		for (i = 0; i < l; ++i) {
			GdomeElement *element;

			element = (GdomeElement *) gdome_nl_item(node_list, i, &exception);
			if (strcmp(gdome_el_getAttribute(element, idref_string, &exception)->str, idref) == 0)
				idref_list = g_slist_prepend(idref_list, element);
		}

		gdome_str_unref(string);
		gdome_nl_unref(node_list, &exception);
	}

	gdome_str_unref(idref_string);

	idref_list = g_slist_reverse(idref_list);

	return idref_list;
}

/* XPath 2.0 support waiting version */
// GdomeXPathResult *
// __gebr_geoxml_get_elements_by_idref(GdomeElement * base, const gchar * idref)
// {
//      GString *               expression;
//      GdomeXPathResult *      xpath_result;
//      GdomeElement *          document_element;
// 
//      expression = g_string_new(NULL);
//      document_element = gdome_doc_documentElement(gdome_el_ownerDocument(base, &exception), &exception);
// 
//      g_string_printf(expression, "idref(('%s'))", idref);
//      xpath_result = __gebr_geoxml_xpath_evaluate(document_element, expression->str);
// 
//      /* the result may contain document's element because of nextid attribute
//       * if so, ignore it */
//      if ((GdomeElement*)gdome_xpresult_singleNodeValue(xpath_result, &exception) == document_element) {
//              gdome_xpresult_iterateNext(xpath_result, &exception);
//      }
// 
//      g_string_free(expression, TRUE);
// 
//      return xpath_result;
// }

gulong __gebr_geoxml_get_element_index(GdomeElement * element)
{
	gulong index;
	GdomeElement *i;

	/* TODO: try XPath position() */

	index = 0;
	i = element;
	while ((i = __gebr_geoxml_previous_same_element(i)) != NULL)
		++index;

	return index;
}

gulong __gebr_geoxml_get_elements_number(GdomeElement * parent_element, const gchar * tag_name)
{
	GdomeElement *child;
	gulong elements_number;

	elements_number = 0;
	for (child = __gebr_geoxml_get_element_at(parent_element, tag_name, 0, FALSE); child != NULL; elements_number++)
		child = __gebr_geoxml_next_same_element(child);

	return elements_number;
}

const gchar *__gebr_geoxml_get_element_value(GdomeElement * element)
{
	GdomeDOMString *string;
	GdomeNode *child;

	if (element == NULL)
		return NULL;
	child = gdome_el_firstChild(element, &exception);
	string = gdome_n_nodeValue(child, &exception);
	if (string == NULL)
		return "";
	if (gdome_n_nodeType(child, &exception) == GDOME_TEXT_NODE) {
		gchar *protected_str;

		protected_str = __gebr_geoxml_remove_line_break(string->str);
		if (protected_str != string->str)
			gdome_n_set_nodeValue(child, gdome_str_mkref(protected_str), &exception);

		return protected_str;
	}

	return string->str;
}

void
__gebr_geoxml_set_element_value(GdomeElement * element, const gchar * tag_value, createValueNode_function create_func)
{
	GdomeNode *value_node;
	const gchar *value_str;

	/* delete all childs elements */
	while ((value_node = gdome_el_firstChild(element, &exception)) != NULL)
		gdome_el_removeChild(element, value_node, &exception);

	/* Protection agains line-breaks inside text nodes */
	value_str = (create_func == __gebr_geoxml_create_TextNode)
	    ? __gebr_geoxml_remove_line_break(tag_value) : tag_value;

	create_func(element, value_str);
}

const gchar *__gebr_geoxml_get_tag_value(GdomeElement * parent_element, const gchar * tag_name)
{
	return __gebr_geoxml_get_element_value(__gebr_geoxml_get_first_element(parent_element, tag_name));
}

void
__gebr_geoxml_set_tag_value(GdomeElement * parent_element, const gchar * tag_name, const gchar * tag_value,
			    createValueNode_function create_func)
{
	__gebr_geoxml_set_element_value(__gebr_geoxml_get_first_element(parent_element, tag_name), tag_value,
					create_func);
}

void __gebr_geoxml_set_attr_value(GdomeElement * element, const gchar * name, const gchar * value)
{
	GdomeDOMString *string;

	string = gdome_str_mkref(name);
	gdome_el_setAttribute(element, string, gdome_str_mkref_dup(value), &exception);
	gdome_str_unref(string);
}

const gchar *__gebr_geoxml_get_attr_value(GdomeElement * element, const gchar * name)
{
	GdomeDOMString *string;
	GdomeDOMString *attr_value;

	if (element == NULL)
		return NULL;
	string = gdome_str_mkref(name);
	attr_value = gdome_el_getAttribute(element, string, &exception);
	gdome_str_unref(string);

	return attr_value->str;
}

void __gebr_geoxml_remove_attr(GdomeElement * element, const gchar * name)
{
	GdomeDOMString *string;

	string = gdome_str_mkref(name);
	gdome_el_removeAttribute(element, string, &exception);
	gdome_str_unref(string);
}

GdomeElement *__gebr_geoxml_previous_element(GdomeElement * element)
{
	GdomeNode *node;

	node = (GdomeNode *) element;
	do
		node = gdome_n_previousSibling(node, &exception);
	while ((node != NULL) && (gdome_n_nodeType(node, &exception) != GDOME_ELEMENT_NODE));

	return (GdomeElement *) node;
}

GdomeElement *__gebr_geoxml_next_element(GdomeElement * element)
{
	GdomeNode *node;

	node = (GdomeNode *) element;
	do
		node = gdome_n_nextSibling(node, &exception);
	while ((node != NULL) && (gdome_n_nodeType(node, &exception) != GDOME_ELEMENT_NODE));

	return (GdomeElement *) node;
}

GdomeElement *__gebr_geoxml_previous_same_element(GdomeElement * element)
{
	GdomeElement *previous_element;

	previous_element = __gebr_geoxml_previous_element(element);
	if (previous_element != NULL &&
	    gdome_str_equal(gdome_el_nodeName(element, &exception), gdome_el_nodeName(previous_element, &exception)))
		return previous_element;

	return NULL;
}

GdomeElement *__gebr_geoxml_next_same_element(GdomeElement * element)
{
	GdomeElement *next_element;

	next_element = __gebr_geoxml_next_element(element);
	if (next_element != NULL &&
	    gdome_str_equal(gdome_el_nodeName(element, &exception), gdome_el_nodeName(next_element, &exception)))
		return next_element;

	return NULL;
}

void
__gebr_geoxml_element_assign_new_id(GdomeElement * element, GdomeElement * reassign_context,
				    gboolean reassign_refereceds)
{
	GdomeElement *document_element;
	gulong nextid;
	gchar *nextid_str;
	gchar *newid;
	GdomeElement *reference_element;

	document_element = gdome_doc_documentElement(gdome_el_ownerDocument(element, &exception), &exception);
	nextid_str = (gchar *) __gebr_geoxml_get_attr_value(document_element, "nextid");
	if (strlen(nextid_str))
		sscanf(nextid_str, "n%lu", &nextid);
	else
		nextid = 0;

	newid = g_strdup(nextid_str);
	nextid_str = g_strdup_printf("n%lu", ++nextid);
	__gebr_geoxml_set_attr_value(document_element, "nextid", nextid_str);

	/* change referenced elements */
	if (reassign_refereceds) {
		const gchar *oldid;

		oldid = __gebr_geoxml_get_attr_value(element, "id");
		if (reassign_context == NULL)
			reassign_context = document_element;
		__gebr_geoxml_foreach_element(reference_element,
					      __gebr_geoxml_get_elements_by_idref(reassign_context, oldid, FALSE))
		    __gebr_geoxml_set_attr_value(reference_element, "idref", newid);
	}
	__gebr_geoxml_set_attr_value(element, "id", newid);

	g_free(nextid_str);
	g_free(newid);
}

void __gebr_geoxml_element_reassign_ids(GdomeElement * element)
{
	gint i;

	for (i = 0; id_tags[i] != NULL; ++i) {
		GdomeDOMString *string;
		GdomeNodeList *node_list;
		gint j, l;

		if (!strcmp(gdome_el_tagName(element, &exception)->str, id_tags[i]))
			__gebr_geoxml_element_assign_new_id(element, element, TRUE);

		string = gdome_str_mkref(id_tags[i]);
		node_list = gdome_el_getElementsByTagName(element, string, &exception);

		l = gdome_nl_length(node_list, &exception);
		/* get the list of elements with this tag_name. */
		for (j = 0; j < l; ++j)
			__gebr_geoxml_element_assign_new_id((GdomeElement *) gdome_nl_item(node_list, j, &exception),
							    element, TRUE);

		gdome_str_unref(string);
		gdome_nl_unref(node_list, &exception);
	}
}

void __gebr_geoxml_element_assign_reference_id(GdomeElement * element, GdomeElement * referencee)
{
	__gebr_geoxml_set_attr_value(element, "idref", __gebr_geoxml_get_attr_value(referencee, "id"));
}

GdomeXPathResult *__gebr_geoxml_xpath_evaluate(GdomeElement * context, const gchar * expression)
{
	GdomeDOMString *string;
	GdomeXPathEvaluator *xpath_evaluator;
	GdomeXPathResult *xpath_result;

	string = gdome_str_mkref(expression);
	xpath_evaluator = gdome_xpeval_mkref();
	xpath_result = gdome_xpeval_evaluate(xpath_evaluator, string, (GdomeNode *) context, NULL, 0, NULL, &exception);

	/* frees */
	gdome_str_unref(string);
	gdome_xpeval_unref(xpath_evaluator, &exception);

	return xpath_result;
}

GSList *__gebr_geoxml_nodelist_to_gslist(GdomeNodeList *node_list)
{
	GSList *list = NULL;

	gint l = gdome_nl_length(node_list, &exception);
	/* get the list of elements with this tag_name. */
	for (gint j = 0; j < l; ++j)
		list = g_slist_prepend(list, gdome_nl_item(node_list, j, &exception));
	list = g_slist_reverse(list);

	return list;
}

