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

#include <unistd.h>
#include <string.h>
#include <zlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>

#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>
#include <gdome.h>
#include <libxml/parser.h>

#if ENABLE_TIDY
# if TIDY_HAVE_SUBDIR
#  include <tidy/tidy.h>
#  include <tidy/buffio.h>
# else
#  include <tidy.h>
#  include <buffio.h>
# endif
#endif

#include "document.h"
#include "document_p.h"
#include "xml.h"
#include "types.h"
#include "error.h"
#include "sequence.h"
#include "flow.h"
#include "parameter.h"
#include "program-parameter.h"
#include "program.h"
#include "parameters_p.h"
#include "parameter_p.h"
#include "parameter_group.h"
#include "parameter_group_p.h"
#include "value_sequence.h"
#include "../gebr-expr.h"
#include "../utils.h"
#include "object.h"

#include "parameters.h"

#define MAX_TAGNAME_SIZE 100

/* global variables */
/**
 * \internal
 * Global variable.
 */
GdomeException exception;

/**
 * \internal
 */
struct gebr_geoxml_document {
	GdomeDocument *document;
};

/**
 * \internal
 */
typedef GdomeDocument *(*createDoc_func) (GdomeDOMImplementation *, char *, unsigned int, GdomeException *);

/**
 * \internal
 */
static GdomeDOMImplementation *dom_implementation = NULL;

/**
 * \internal
 * Used at GebrGeoXmlObject's methods.
 */
GdomeDocument *clipboard_document = NULL;

static const gchar *dtd_directory = GEBR_GEOXML_DTD_DIR;

/**
 * \internal
 * Checks if \p version has the form '%d.%d.%d', ie three numbers separated by a dot.
 */
static gboolean gebr_geoxml_document_is_version_valid(const gchar * version);

/**
 * \internal
 * Checks if \p document version is less than or equal to GeBR's version.
 */
static gboolean gebr_geoxml_document_check_version(GebrGeoXmlDocument * document, const gchar * version);

/**
 * \internal
 */
static void gebr_geoxml_document_fix_header(GString * source, const gchar * tagname, const gchar * dtd_filename);

void gebr_geoxml_init(void)
{
	GdomeDOMString *string = gdome_str_mkref("gebr-geoxml-clipboard");
	dom_implementation = gdome_di_mkref();
	clipboard_document = gdome_di_createDocument(dom_implementation, NULL,
						     string, NULL, &exception);
	gdome_str_unref(string);
}

void gebr_geoxml_finalize(void)
{
	gdome_di_unref(dom_implementation, &exception);
	gdome_doc_unref(clipboard_document, &exception);
	clipboard_document = NULL;
}

static gchar *
get_document_property(GebrGeoXmlDocument *doc,
		      const gchar *prop)
{
	gchar *prop_value;
	GdomeElement *root;

	g_return_val_if_fail(doc != NULL, NULL);

	root = gebr_geoxml_document_root_element(doc);
	prop_value = __gebr_geoxml_get_tag_value(root, prop);
	gdome_el_unref(root, &exception);

	return prop_value;
}

static void
set_document_property(GebrGeoXmlDocument *doc,
                      const gchar *tag_element,
		      const gchar *tag_name,
		      const gchar *tag_value)
{
	GdomeElement *root;
	GdomeElement *element;

	g_return_if_fail(doc != NULL);
	g_return_if_fail(tag_name != NULL);
	g_return_if_fail(tag_value != NULL);
	g_return_if_fail(tag_element != NULL);

	root = gebr_geoxml_document_root_element(doc);
	element = __gebr_geoxml_get_first_element(root, tag_element);
	__gebr_geoxml_set_tag_value(element, tag_name, tag_value, __gebr_geoxml_create_TextNode);

	gdome_el_unref(element, &exception);
	gdome_el_unref(root, &exception);
}

static void
set_document_simple_property(GebrGeoXmlDocument *doc,
                             const gchar *tag_name,
                             const gchar *tag_value)
{
	GdomeElement *root;

	g_return_if_fail(doc != NULL);
	g_return_if_fail(tag_name != NULL);
	g_return_if_fail(tag_value != NULL);

	root = gebr_geoxml_document_root_element(doc);
	__gebr_geoxml_set_tag_value(root, tag_name, tag_value, __gebr_geoxml_create_TextNode);

	gdome_el_unref(root, &exception);
}

/*
 * __gebr_geoxml_document_new_data:
 * Creates the #GebrGeoXmlDocumentData for this document.
 */
static void __gebr_geoxml_document_new_data(GebrGeoXmlDocument * document, const gchar * filename)
{
	GebrGeoXmlDocumentData *data = g_new(GebrGeoXmlDocumentData, 1);
	((GdomeDocument*)document)->user_data = data;
	data->filename = g_string_new(filename);
}

/**
 * \internal
 */
GdomeDocument *__gebr_geoxml_document_clone_doc(GdomeDocument * source, GdomeDocumentType * document_type)
{
	if (source == NULL)
		return NULL;

	GdomeDocument *document;
	GdomeElement *root_element;
	GdomeElement *source_root_element;

	source_root_element = gdome_doc_documentElement(source, &exception);
	document = gdome_di_createDocument(dom_implementation,
					   NULL, gdome_el_nodeName(source_root_element, &exception), document_type,
					   &exception);
	if (document == NULL)
		return NULL;

	/* copy version attribute */
	root_element = gdome_doc_documentElement(document, &exception);
	__gebr_geoxml_set_attr_value(root_element, "version",
				     __gebr_geoxml_get_attr_value(source_root_element, "version"));

	/* import all elements */
	GdomeElement *element = __gebr_geoxml_get_first_element(source_root_element, "*");
	for (; element != NULL; element = __gebr_geoxml_next_element(element)) {
		GdomeNode *new_node = gdome_doc_importNode(document, (GdomeNode*)element, TRUE, &exception);
		GdomeNode *result = gdome_el_appendChild(root_element, new_node, &exception);
		gdome_n_unref(new_node, &exception);
		gdome_n_unref(result, &exception);
		gdome_el_unref(element, &exception);
	}

	gdome_el_unref(source_root_element, &exception);
	gdome_el_unref(root_element, &exception);

	return document;
}

/**
 * \internal
 */
static int
__gebr_geoxml_document_validate_doc(GdomeDocument ** document,
				    GebrGeoXmlDiscardMenuRefCallback discard_menu_ref)
{
	if (gdome_doc_doctype(*document, &exception) != NULL)
		return GEBR_GEOXML_RETV_DTD_SPECIFIED;

	gchar *version;

	/* If there is no version attribute, the document is invalid */
	version = gebr_geoxml_document_get_version((GebrGeoXmlDocument *) *document);
	if (!gebr_geoxml_document_is_version_valid(version)) {
		g_free(version);
		return GEBR_GEOXML_RETV_INVALID_DOCUMENT;
	}

	/* Checks if the document's version is greater than GeBR's version */
	if (!gebr_geoxml_document_check_version((GebrGeoXmlDocument*)*document, version)) {
		g_free(version);
		return GEBR_GEOXML_RETV_NEWER_VERSION;
	}

	gint ret;
	gchar tagname[MAX_TAGNAME_SIZE]; // This will hold one of these: "flow", "line" or "project"

	GString *source = g_string_new(NULL);
	gchar *dtd_filename = NULL;
	GdomeElement *root_element;

	root_element = gebr_geoxml_document_root_element(*document);
	GdomeDOMString * root_name = gdome_el_nodeName(root_element, &exception);
	strncpy(tagname, root_name->str, MAX_TAGNAME_SIZE);
	tagname[MAX_TAGNAME_SIZE - 1] = '\0';
	gdome_str_unref(root_name);

	/* Find the DTD spec file. If the file doesn't exists, it may mean that this document is from newer version. */
	dtd_filename = g_strdup_printf("%s/%s-%s.dtd", dtd_directory, tagname, version);
	if (g_file_test(dtd_filename, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR) == FALSE
	    || g_access(dtd_filename, R_OK) < 0) {
		ret = GEBR_GEOXML_RETV_CANT_ACCESS_DTD;
		goto free_source_dtd_root_version;
	}

	/* Inserts the document type into the xml so we can validate and specify the ID attributes and use
	 * gdome_doc_getElementById. */
	gchar *src;
	gdome_di_saveDocToMemoryEnc(dom_implementation, *document, &src, ENCODING, GDOME_SAVE_STANDARD, &exception);
	g_string_assign(source, src);
	g_free(src);
	gebr_geoxml_document_fix_header(source, tagname, dtd_filename);

	/* Based on code by Daniel Veillard
	 * References:
	 *   http://xmlsoft.org/examples/index.html#parse2.c
	 *   http://xmlsoft.org/examples/parse2.c
	 */
	xmlParserCtxtPtr ctxt = xmlNewParserCtxt();
	xmlDocPtr doc = xmlCtxtReadMemory(ctxt, source->str, source->len, NULL, NULL,
					  XML_PARSE_NOBLANKS | XML_PARSE_DTDATTR |        /* default DTD attributes */
					  XML_PARSE_NOENT |                               /* substitute entities */
					  XML_PARSE_DTDVALID);
	if (doc == NULL) {
		xmlFreeParserCtxt(ctxt);
		ret = GEBR_GEOXML_RETV_INVALID_DOCUMENT;
		goto free_source_dtd_root_version;
	} else {
		xmlFreeDoc(doc);
		if (!ctxt->valid) {
			ret = GEBR_GEOXML_RETV_INVALID_DOCUMENT;
			xmlFreeParserCtxt(ctxt);
			goto free_source_dtd_root_version;
		}
		xmlFreeParserCtxt(ctxt);
	}

	GdomeDocument *tmp_doc;
	tmp_doc = gdome_di_createDocFromMemory(dom_implementation, source->str, GDOME_LOAD_PARSING, &exception);

	if (tmp_doc == NULL) {
		ret = GEBR_GEOXML_RETV_NO_MEMORY;
		goto free_source_dtd_root_version;
	}
	gdome_el_unref(root_element, &exception);
	gdome_doc_unref(*document, &exception);
	*document = tmp_doc;
	root_element = gebr_geoxml_document_root_element(tmp_doc);

	/*
	 * Success, now change to last version
	 */

	/**
	 * \internal
	 * Change group XML as declared in flow-0.3.5, project-0.3.2 and line-0.3.2
	 */
	void __port_to_new_group_semantics(void)
	{
		GdomeElement *element;
		GSList *list = __gebr_geoxml_get_elements_by_tag(root_element, "parameters");
		gebr_foreach_gslist_hyg(element, list, parameters) {
			__gebr_geoxml_set_attr_value(element, "default-selection",
						     __gebr_geoxml_get_attr_value(element, "exclusive"));
			__gebr_geoxml_remove_attr(element, "exclusive");
			__gebr_geoxml_set_attr_value(element, "selection",
						     __gebr_geoxml_get_attr_value(element, "selected"));
			__gebr_geoxml_remove_attr(element, "selected");
			gdome_el_unref(element, &exception);
		}

		list = __gebr_geoxml_get_elements_by_tag(root_element, "group");
		gebr_foreach_gslist_hyg(element, list, group) {
			GdomeNode *new_instance;
			GebrGeoXmlParameters *template_instance;

			template_instance = GEBR_GEOXML_PARAMETERS(__gebr_geoxml_get_first_element(element, "parameters"));
			/* encapsulate template instance into template-instance element */
			GdomeElement *template_container;
			template_container = __gebr_geoxml_insert_new_element(element, "template-instance",
									      (GdomeElement*)template_instance);
			gdome_n_unref(gdome_el_insertBefore_protected(template_container, (GdomeNode*)template_instance, NULL, &exception), &exception);

			/* move new instance after template instance */
			new_instance = gdome_el_cloneNode((GdomeElement*)template_instance, TRUE, &exception);
			GdomeNode *next = (GdomeNode*)__gebr_geoxml_next_element((GdomeElement*)template_container);
			gdome_n_unref(gdome_n_insertBefore_protected((GdomeNode*)element, new_instance, next, &exception), &exception);

			gebr_geoxml_object_unref(template_instance);
			gdome_n_unref(new_instance, &exception);
			gdome_n_unref(next, &exception);
			gdome_el_unref(template_container, &exception);
			gdome_el_unref(element, &exception);
		}
	}

	/* document 0.1.x to 0.2.0 */
	if (strcmp(version, "0.2.0") < 0) {
		GdomeElement *element;
		GdomeElement *before;

		before = __gebr_geoxml_get_first_element(root_element, "email");
		before = __gebr_geoxml_next_element(before);

		element = __gebr_geoxml_insert_new_element(root_element, "date", before);
		gdome_el_unref(__gebr_geoxml_insert_new_element(element, "created", NULL), &exception);
		gdome_el_unref(__gebr_geoxml_insert_new_element(element, "modified", NULL), &exception);
		gdome_el_unref(before, &exception);

		if (gebr_geoxml_document_get_type((GebrGeoXmlDocument *) *document) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW)
			gdome_el_unref(__gebr_geoxml_insert_new_element(element, "lastrun", NULL), &exception);
		gdome_el_unref(element, &exception);
	}
	/* document 0.2.0 to 0.2.1 */
	if (strcmp(version, "0.2.1") < 0) {
		if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument *) *document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
			GebrGeoXmlSequence *program;

			gebr_geoxml_flow_get_program(GEBR_GEOXML_FLOW(*document), &program, 0);
			while (program != NULL) {
				GdomeElement *el = __gebr_geoxml_get_first_element((GdomeElement *) program, "url");
				if (el == NULL) {
					GdomeElement *element = __gebr_geoxml_get_first_element((GdomeElement*) program, "parameters");
					gdome_el_unref(__gebr_geoxml_insert_new_element((GdomeElement *) program, "url", element), &exception);
					gdome_el_unref(element, &exception);
				}
				gdome_el_unref(el, &exception);
				gebr_geoxml_sequence_next(&program);
			}
		}
	}
	/* document 0.2.1 to 0.2.2 */
	if (strcmp(version, "0.2.2") < 0) {
		if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument *) *document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
			GdomeElement *element;

			__gebr_geoxml_set_attr_value(root_element, "version", "0.2.2");

			__gebr_geoxml_foreach_element_with_tagname_r(root_element, "option", element, option) {
				gchar *value;

				value = g_strdup(__gebr_geoxml_get_element_value(element));
				__gebr_geoxml_set_element_value(element, "", __gebr_geoxml_create_TextNode);

				gdome_el_unref(__gebr_geoxml_insert_new_element(element, "label", NULL), &exception);
				GdomeElement *el = __gebr_geoxml_insert_new_element(element, "value", NULL);
				__gebr_geoxml_set_element_value(el, value, __gebr_geoxml_create_TextNode);

				gdome_el_unref(el, &exception);
				g_free(value);
			}

			__gebr_geoxml_foreach_element_with_tagname_r(root_element, "range", element, range)
			    __gebr_geoxml_set_attr_value(element, "digits", "");
		}
	}
	/* document 0.2.2 to 0.2.3
	 * nothing changed. why? because the changes were about the removed group support
	 */
	if (strcmp(version, "0.2.3") < 0)
		__gebr_geoxml_set_attr_value(root_element, "version", "0.2.3");
	/* document 0.2.3 to 0.3.0 */
	if (strcmp(version, "0.3.0") < 0) {
		__gebr_geoxml_set_attr_value(root_element, "version", "0.3.0");
		__gebr_geoxml_set_attr_value(root_element, "nextid", "n0");

		if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument *) *document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
			GebrGeoXmlSequence *program;

			gebr_geoxml_flow_get_program(GEBR_GEOXML_FLOW(*document), &program, 0);
			for (; program; gebr_geoxml_sequence_next(&program)) {
				GebrGeoXmlParameters *parameters;
				GdomeElement *old_parameter;

				parameters = gebr_geoxml_program_get_parameters(GEBR_GEOXML_PROGRAM(program));
				__gebr_geoxml_set_attr_value((GdomeElement *) parameters, "exclusive", "0");
				old_parameter = __gebr_geoxml_get_first_element((GdomeElement *) parameters, "*");
				while (old_parameter) {
					GdomeElement *parameter;
					GdomeElement *property;

					GebrGeoXmlParameterType type;
					GdomeDOMString *tag_name;
					int i;

					type = GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN;
					tag_name = gdome_el_tagName(old_parameter, &exception);
					for (i = 1; i <= parameter_type_to_str_len; ++i) {
						if (!strcmp(parameter_type_to_str[i], tag_name->str)) {
							type = (GebrGeoXmlParameterType)i;
							break;
						}
					}
					gdome_str_unref(tag_name);

					parameter = __gebr_geoxml_insert_new_element((GdomeElement *) parameters,
										     "parameter", old_parameter);
					GdomeElement *first = __gebr_geoxml_get_first_element(old_parameter, "label");
					gdome_n_unref(gdome_el_insertBefore_protected(parameter, (GdomeNode *)first, NULL, &exception), &exception);
					gdome_el_unref(first, &exception);

					gdome_n_unref(gdome_el_insertBefore_protected(parameter, (GdomeNode *) old_parameter, NULL, &exception), &exception);

					first = (GdomeElement*)gdome_el_firstChild(old_parameter, &exception);
					property = __gebr_geoxml_insert_new_element(old_parameter, "property", first);
					gdome_el_unref(first, &exception);

					GdomeElement *el2 = __gebr_geoxml_get_first_element(old_parameter, "keyword");
					gdome_n_unref(gdome_el_insertBefore_protected(property, (GdomeNode *)el2, NULL, &exception), &exception);
					gdome_el_unref(el2, &exception);

					if (type != GEBR_GEOXML_PARAMETER_TYPE_FLAG) {
						GdomeElement *value;
						GdomeDOMString *string;
						GdomeDOMString *separator;

						string = gdome_str_mkref("required");
						GdomeDOMString *attr = gdome_el_getAttribute(old_parameter, string, &exception);
						gdome_el_setAttribute(property, string, attr, &exception);
						gdome_el_removeAttribute(old_parameter, string, &exception);
						gdome_str_unref(string);
						gdome_str_unref(attr);

						value = __gebr_geoxml_get_first_element(old_parameter, "value");
						string = gdome_str_mkref("separator");
						separator = gdome_el_getAttribute(old_parameter, string, &exception);
						if (strlen(separator->str)) {
							gdome_el_setAttribute(property, string, separator, &exception);

							gebr_geoxml_program_parameter_set_parse_list_value(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), FALSE,
							                                                   __gebr_geoxml_get_element_value(value));
							gebr_geoxml_program_parameter_set_parse_list_value(GEBR_GEOXML_PROGRAM_PARAMETER(parameter),
													   TRUE, __gebr_geoxml_get_attr_value(value, "default"));
						} else {
							gchar *str_value = __gebr_geoxml_get_element_value(value);
							GdomeElement *element1 = __gebr_geoxml_insert_new_element(property, "value", NULL);
							__gebr_geoxml_set_element_value(element1, str_value? str_value:"", __gebr_geoxml_create_TextNode);
							gdome_el_unref(element1, &exception);
							g_free(str_value);

							GdomeElement *element2 = __gebr_geoxml_insert_new_element(property, "default", NULL);

							str_value = __gebr_geoxml_get_attr_value (value, "default");
							__gebr_geoxml_set_element_value(element2, str_value?str_value:"", __gebr_geoxml_create_TextNode);
							g_free(str_value);
							gdome_el_unref(element2, &exception);
						}
						gdome_n_unref(gdome_el_removeChild(old_parameter, (GdomeNode *) value, &exception), &exception);
						gdome_el_removeAttribute(old_parameter, string, &exception);
						gdome_el_unref(value, &exception);
						gdome_str_unref(string);
						gdome_str_unref(separator);
					} else {
						GdomeElement *state;

						state = __gebr_geoxml_get_first_element(old_parameter, "state");
						GdomeElement *element1 = __gebr_geoxml_insert_new_element(property, "value", NULL);
						__gebr_geoxml_set_element_value(element1,
										__gebr_geoxml_get_element_value(state),
										__gebr_geoxml_create_TextNode);
						gdome_el_unref(element1, &exception);

						GdomeElement *element2 = __gebr_geoxml_insert_new_element(property, "default", NULL);
						__gebr_geoxml_set_element_value(element2,
										__gebr_geoxml_get_attr_value(state,
													     "default"),
										__gebr_geoxml_create_TextNode);
						gdome_el_unref(element2, &exception);
						__gebr_geoxml_set_attr_value(property, "required", "no");

						gdome_n_unref(gdome_el_removeChild(old_parameter, (GdomeNode *) state, &exception), &exception);
						gdome_el_unref(state, &exception);
					}

					gdome_el_unref(property, &exception);
					gdome_el_unref(parameter, &exception);
					GdomeElement *next_parameter = __gebr_geoxml_next_element(old_parameter);
					gdome_el_unref(old_parameter, &exception);
					old_parameter = next_parameter;
				}
				gebr_geoxml_object_unref(parameters);
			}
		}
	}
	/* document 0.3.0 to 0.3.1 */
	if (strcmp(version, "0.3.1") < 0) {
		GdomeElement *dict_element;
		GdomeElement *first;

		__gebr_geoxml_set_attr_value(root_element, "version", "0.3.1");

		first = __gebr_geoxml_get_first_element(root_element, "date");
		dict_element = __gebr_geoxml_insert_new_element(root_element, "dict", first);
		gebr_geoxml_object_unref(__gebr_geoxml_parameters_append_new(dict_element));
		gdome_el_unref(first, &exception);
		gdome_el_unref(dict_element, &exception);

		if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument *) *document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
			GdomeElement *first = __gebr_geoxml_get_first_element(root_element, "io");
			GdomeElement *next = __gebr_geoxml_next_element(first);
			gdome_el_unref(__gebr_geoxml_insert_new_element(root_element, "servers", next), &exception);
			gdome_el_unref(next, &exception);
			gdome_el_unref(first, &exception);
		}
	}
	/* 0.3.1 to 0.3.2 */
	if (strcmp(version, "0.3.2") < 0) {
		__gebr_geoxml_set_attr_value(root_element, "version", "0.3.2");

		if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument *) *document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
			GdomeElement *element;
			GSList *list = __gebr_geoxml_get_elements_by_tag(root_element, "menu");

			gebr_foreach_gslist(element, list) {
				GdomeNode *parent;
				if (discard_menu_ref != NULL) {
					parent = gdome_el_parentNode(element, &exception);
					discard_menu_ref((GebrGeoXmlProgram *)parent,
							 __gebr_geoxml_get_element_value(element) ,
							atoi(__gebr_geoxml_get_attr_value(element, "index")));
					gdome_n_unref(parent, &exception);
				}

				parent = gdome_el_parentNode(element, &exception);
				gdome_n_unref(gdome_n_removeChild(parent, (GdomeNode *)element, &exception), &exception);
				gdome_n_unref(parent, &exception);
				gdome_el_unref(element, &exception);
			}
		} else {
			/* removal of filename for project and lines */
			GdomeElement *el = __gebr_geoxml_get_first_element(root_element, "filename");
			gdome_n_unref(gdome_el_removeChild(root_element, (GdomeNode*)el, &exception), &exception);
			gdome_el_unref(el, &exception);

			__gebr_geoxml_remove_attr(root_element, "nextid");

			GSList * params;
			GSList * iter;

			params = __gebr_geoxml_get_elements_by_tag(root_element, "parameter");
			iter = params;
			while (iter) {
				__gebr_geoxml_remove_attr(iter->data, "id");
				gdome_el_unref(iter->data, &exception);
				iter = iter->next;
			}
			g_slist_free(params);

			GSList * refer;

			refer = __gebr_geoxml_get_elements_by_tag(root_element, "reference");
			iter = refer;
			while (iter) {
				__gebr_geoxml_remove_attr(iter->data, "idref");
				gdome_el_unref(iter->data, &exception);
				iter = iter->next;
			}
			g_slist_free(refer);

			__port_to_new_group_semantics();
		}
	}
	/* 0.3.2 to 0.3.3 */ 
	if (strcmp(version, "0.3.3") < 0)
	{
		GebrGeoXmlDocumentType type = gebr_geoxml_document_get_type(
				(GebrGeoXmlDocument *) *document);

		switch (type)
		{
		// Backward compatible change, nothing to be done.
		// Added #IMPLIED 'version' attribute to 'program' tag.
		case GEBR_GEOXML_DOCUMENT_TYPE_FLOW:
			__gebr_geoxml_set_attr_value(root_element, "version", "0.3.3");
			break;

		case GEBR_GEOXML_DOCUMENT_TYPE_LINE: {
			GdomeElement *pivot;
			GdomeElement *prev;

			prev = __gebr_geoxml_get_first_element(root_element, "date");
			pivot = __gebr_geoxml_next_element(prev);
			gdome_el_unref(prev, &exception);

			gdome_el_unref(__gebr_geoxml_insert_new_element(root_element, "server-group", pivot), &exception);
			__gebr_geoxml_set_attr_value(root_element, "version", "0.3.3");
			gdome_el_unref(pivot, &exception);
		} break;

		case GEBR_GEOXML_DOCUMENT_TYPE_PROJECT:	{
			GHashTable * keys_to_canonized = NULL;

			gebr_geoxml_document_canonize_dict_parameters(
					((GebrGeoXmlDocument *) *document),
					&keys_to_canonized);

			__gebr_geoxml_set_attr_value(root_element, "version", "0.3.3");
		} break;

		default:
			g_warn_if_reached();
		}
	}

	/* 0.3.3 to 0.3.4 */ 
	if (strcmp(version, "0.3.4") < 0)
	{
		GebrGeoXmlDocumentType type = gebr_geoxml_document_get_type(
				(GebrGeoXmlDocument *) *document);
		// Backward compatible change, nothing to be done.
		// Added #IMPLIED 'mpi' attribute to 'program' tag.
		switch (type)
		{
		case GEBR_GEOXML_DOCUMENT_TYPE_FLOW:
			__gebr_geoxml_set_attr_value(root_element, "version", "0.3.4");
			break;

		case GEBR_GEOXML_DOCUMENT_TYPE_LINE: {
			GHashTable * keys_to_canonized = NULL;

			gebr_geoxml_document_canonize_dict_parameters(
					((GebrGeoXmlDocument *) *document),
					&keys_to_canonized);

			__gebr_geoxml_set_attr_value(root_element, "version", "0.3.4");
		} break;

		case GEBR_GEOXML_DOCUMENT_TYPE_PROJECT:
			break;

		default:
			g_warn_if_reached();
		}
	}
	/* 0.3.4 to 0.3.5 */ 
	if (strcmp(version, "0.3.5") < 0) {
		if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument *) *document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
			__gebr_geoxml_set_attr_value(root_element, "version", "0.3.5");

			GdomeElement *el = __gebr_geoxml_get_first_element(root_element, "filename");
			/* remove flow filename */
			gdome_n_unref(gdome_el_removeChild(root_element, (GdomeNode*)el, &exception), &exception);
			gdome_el_unref(el, &exception);

			__port_to_new_group_semantics();

			__gebr_geoxml_remove_attr(root_element, "nextid");

			GSList * params;
			GSList * iter;

			params = __gebr_geoxml_get_elements_by_tag(root_element, "parameter");
			iter = params;
			while (iter) {
				__gebr_geoxml_remove_attr(iter->data, "id");
				gdome_el_unref(iter->data, &exception);
				iter = iter->next;
			}
			g_slist_free(params);

			GSList * refer;

			refer = __gebr_geoxml_get_elements_by_tag(root_element, "reference");
			iter = refer;
			while (iter) {
				__gebr_geoxml_remove_attr(iter->data, "idref");
				gdome_el_unref(iter->data, &exception);
				iter = iter->next;
			}
			g_slist_free(refer);
		}
	}
	/* 0.3.5 to 0.3.6 */ 
	if (strcmp(version, "0.3.6") < 0) {
		if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument *) *document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
			__gebr_geoxml_set_attr_value(root_element, "version", "0.3.6");

			GdomeElement *element;
			GSList *list = __gebr_geoxml_get_elements_by_tag(root_element, "group");

			gebr_foreach_gslist_hyg(element, list, group) {
				GebrGeoXmlParameterGroup *group;
				GebrGeoXmlSequence *instance;
				GebrGeoXmlSequence *iter;
				gboolean first_instance = TRUE;

				group = GEBR_GEOXML_PARAMETER_GROUP(gdome_el_parentNode(element, &exception));
				gebr_geoxml_parameter_group_get_instance(group, &instance, 0);

				for (; instance != NULL; gebr_geoxml_sequence_next(&instance)) {
					gebr_geoxml_parameters_get_parameter(GEBR_GEOXML_PARAMETERS(instance), &iter, 0);

					for (; iter != NULL; gebr_geoxml_sequence_next(&iter)) {
						__gebr_geoxml_parameter_set_label((GebrGeoXmlParameter *) iter, "");

						if (first_instance &&
						    __gebr_geoxml_parameter_get_type((GebrGeoXmlParameter *) iter, FALSE) != GEBR_GEOXML_PARAMETER_TYPE_REFERENCE)
							__gebr_geoxml_parameter_set_be_reference_with_value((GebrGeoXmlParameter *) iter);
					}
					first_instance = FALSE;
				}
				gdome_el_unref(element, &exception);
				gebr_geoxml_object_unref(group);
			}
		}
	}

	if (strcmp(version, "0.3.7") < 0) {
		if (gebr_geoxml_document_get_type(GEBR_GEOXML_DOCUMENT(*document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
			GHashTable * keys_to_canonized = NULL;
			GebrGeoXmlSequence * program = NULL;

			GdomeElement *io;
			GdomeElement *server;
			GdomeElement *servers;

			// Canonize revisions
			GebrGeoXmlSequence *seq;
			gebr_geoxml_flow_get_revision(GEBR_GEOXML_FLOW(*document), &seq, 0);

			for (; seq; gebr_geoxml_sequence_next(&seq)) {
				gchar *flow_xml;
				GebrGeoXmlDocument *revdoc;
				gebr_geoxml_flow_get_revision_data(GEBR_GEOXML_REVISION(seq), &flow_xml, NULL, NULL);

				if (gebr_geoxml_document_load_buffer(&revdoc, flow_xml) != GEBR_GEOXML_RETV_SUCCESS) {
					g_free(flow_xml);
					continue;
				}

				g_free(flow_xml);
				gebr_geoxml_document_to_string(revdoc, &flow_xml);
				gebr_geoxml_flow_set_revision_data(GEBR_GEOXML_REVISION(seq), flow_xml, NULL, NULL);
				gebr_geoxml_document_free(revdoc);
				g_free(flow_xml);
			}

			io = __gebr_geoxml_get_first_element (root_element, "io");
			servers = __gebr_geoxml_next_element (io);
			server = __gebr_geoxml_get_first_element (servers, "server");

			if (!server) {
				server = __gebr_geoxml_insert_new_element (root_element, "server", servers);
				__gebr_geoxml_set_attr_value(server, "address", "127.0.0.1");
				GdomeElement *new_io = __gebr_geoxml_insert_new_element(server, "io", NULL);
				GdomeElement *el = __gebr_geoxml_insert_new_element(new_io, "input", NULL);
				__gebr_geoxml_set_element_value(el, "", __gebr_geoxml_create_TextNode);
				gdome_el_unref(el, &exception);
				el = __gebr_geoxml_insert_new_element(new_io, "output", NULL);
				__gebr_geoxml_set_element_value(el, "", __gebr_geoxml_create_TextNode);
				gdome_el_unref(el, &exception);
				el = __gebr_geoxml_insert_new_element(new_io, "error", NULL);
				__gebr_geoxml_set_element_value(el, "", __gebr_geoxml_create_TextNode);
				gdome_el_unref(el, &exception);
				gdome_el_unref(new_io, &exception);

				GdomeElement *lastrun = __gebr_geoxml_insert_new_element(server, "lastrun", NULL);
				__gebr_geoxml_set_element_value(lastrun, "", __gebr_geoxml_create_TextNode);
				gdome_el_unref(lastrun, &exception);
			} else {
				GdomeNode *clone = gdome_el_cloneNode(server, TRUE, &exception);
				gdome_n_unref(gdome_el_insertBefore_protected(root_element, clone, (GdomeNode*)servers,
				                                              &exception), &exception);
				gdome_n_unref(clone, &exception);
			}

			gdome_n_unref(gdome_el_removeChild(root_element, (GdomeNode*) io, &exception), &exception);
			gdome_n_unref(gdome_el_removeChild(root_element, (GdomeNode*) servers, &exception), &exception);

			gebr_geoxml_document_canonize_dict_parameters(GEBR_GEOXML_DOCUMENT(*document), &keys_to_canonized);

			gebr_geoxml_flow_get_program(GEBR_GEOXML_FLOW(*document), &program, 0);
			for (; program != NULL; gebr_geoxml_sequence_next(&program)) {
				gebr_geoxml_program_foreach_parameter(
						GEBR_GEOXML_PROGRAM(program),
						gebr_geoxml_program_parameter_update_old_dict_value,
						keys_to_canonized);
			}

			__gebr_geoxml_set_attr_value(root_element, "version", "0.3.7");
			gdome_el_unref(io, &exception);
			gdome_el_unref(server, &exception);
			gdome_el_unref(servers, &exception);
		}
	}

	/* CHECKS (may impact performance) */
	if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument *) *document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
		GSList *elements = __gebr_geoxml_get_elements_by_tag(root_element, "flag");
		/* accept only on/off for flags */
		for (GSList *i = elements; i; i = i->next) {
			GdomeElement *value_element;
			GdomeElement *default_value_element;
			GdomeElement *element = i->data;
			gchar *val;

			value_element = __gebr_geoxml_get_first_element(element, "value");
			default_value_element = __gebr_geoxml_get_first_element(element, "default");

			val = __gebr_geoxml_get_element_value(value_element);
			if (strcmp(val, "on") && strcmp(val, "off"))
				__gebr_geoxml_set_element_value(value_element, "off", __gebr_geoxml_create_TextNode);
			g_free(val);

			val = __gebr_geoxml_get_element_value(default_value_element);
			if (strcmp(val, "on") && strcmp(val, "off"))
				__gebr_geoxml_set_element_value(default_value_element, "off", __gebr_geoxml_create_TextNode);
			g_free(val);

			gdome_el_unref(element, &exception);
			gdome_el_unref(value_element, &exception);
			gdome_el_unref(default_value_element, &exception);
		}
		g_slist_free(elements);
	}

	ret = GEBR_GEOXML_RETV_SUCCESS;

free_source_dtd_root_version:
	g_free(version);
	g_free (dtd_filename);
	g_string_free (source, TRUE);
	gdome_el_unref (root_element, &exception);

	return ret;
}

/**
 * \internal
 */
static int __gebr_geoxml_document_load(GebrGeoXmlDocument ** document, const gchar * src, createDoc_func func,
				       gboolean validate, GebrGeoXmlDiscardMenuRefCallback discard_menu_ref)
{
	GdomeDocument *doc;
	int ret;

	/* load */
	doc = func(dom_implementation, (gchar *) src, GDOME_LOAD_PARSING, &exception);
	if (doc == NULL) {
		ret = GEBR_GEOXML_RETV_INVALID_DOCUMENT;
		goto err;
	}

	if (validate) {
		ret = __gebr_geoxml_document_validate_doc(&doc, discard_menu_ref);
		if (ret != GEBR_GEOXML_RETV_SUCCESS) {
			gdome_doc_unref((GdomeDocument *) doc, &exception);
			goto err;
		}
	}

	*document = (GebrGeoXmlDocument *) doc;
	__gebr_geoxml_document_new_data(*document, "");
	return GEBR_GEOXML_RETV_SUCCESS;

 err:
	*document = NULL;
	return ret;
}

/**
 * \internal
 */
static int filename_check_access(const gchar * filename)
{
	if (filename == NULL)
		return GEBR_GEOXML_RETV_FILE_NOT_FOUND;
	if (!g_file_test(filename, G_FILE_TEST_EXISTS)) 
		return GEBR_GEOXML_RETV_FILE_NOT_FOUND;
	if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR) || g_access(filename, R_OK) < 0) 
		return GEBR_GEOXML_RETV_PERMISSION_DENIED;

	return GEBR_GEOXML_RETV_SUCCESS;
}

/*
 * private functions and variables
 */

GebrGeoXmlDocument *gebr_geoxml_document_new(const gchar * name, const gchar * version)
{
	GdomeDocument *document;
	GdomeElement *root_element;
	GdomeElement *element;
	GdomeDOMString *str;

	str = gdome_str_mkref(name);
	document = gdome_di_createDocument(dom_implementation, NULL, str, NULL, &exception);
	__gebr_geoxml_document_new_data((GebrGeoXmlDocument *)document, "");
	gdome_str_unref(str);

	/* document (root) element */
	root_element = gdome_doc_documentElement(document, &exception);
	__gebr_geoxml_set_attr_value(root_element, "version", version);

	/* elements (as specified in DTD) */
	gdome_el_unref(__gebr_geoxml_insert_new_element(root_element, "title", NULL), &exception);
	gdome_el_unref(__gebr_geoxml_insert_new_element(root_element, "description", NULL), &exception);
	gdome_el_unref(__gebr_geoxml_insert_new_element(root_element, "help", NULL), &exception);
	gdome_el_unref(__gebr_geoxml_insert_new_element(root_element, "author", NULL), &exception);
	gdome_el_unref(__gebr_geoxml_insert_new_element(root_element, "email", NULL), &exception);
	element = __gebr_geoxml_insert_new_element(root_element, "dict", NULL);
	gdome_el_unref((GdomeElement*)__gebr_geoxml_parameters_append_new(element), &exception);
	gdome_el_unref(element, &exception);
	element = __gebr_geoxml_insert_new_element(root_element, "date", NULL);
	gdome_el_unref(__gebr_geoxml_insert_new_element(element, "created", NULL), &exception);
	gdome_el_unref(__gebr_geoxml_insert_new_element(element, "modified", NULL), &exception);

	gdome_el_unref(element, &exception);
	gdome_el_unref(root_element, &exception);
	return (GebrGeoXmlDocument *) document;
}

/*
 * library functions.
 */

int gebr_geoxml_document_load(GebrGeoXmlDocument ** document, const gchar * path, gboolean validate,
			      GebrGeoXmlDiscardMenuRefCallback discard_menu_ref)
{
	int ret;
	if ((ret = filename_check_access(path))) {
		*document = NULL;
		return ret;
	}

	ret = __gebr_geoxml_document_load(document, path, (createDoc_func)gdome_di_createDocFromURI, validate,
					  discard_menu_ref);
	if (ret)
		return ret;

	gchar * filename = g_path_get_basename(path);
	gebr_geoxml_document_set_filename(*document, filename);
	g_free(filename);

	return ret;
}

int gebr_geoxml_document_load_buffer(GebrGeoXmlDocument ** document, const gchar * xml)
{
	return __gebr_geoxml_document_load(document, xml, (createDoc_func)gdome_di_createDocFromMemory, TRUE, NULL);
}

void gebr_geoxml_document_free(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return;

	GebrGeoXmlDocumentData *data;
	data = _gebr_geoxml_document_get_data(document);
	g_string_free(data->filename, TRUE);
	g_free(data);
	gdome_doc_unref((GdomeDocument *) document, &exception);
}

GebrGeoXmlDocument *gebr_geoxml_document_clone(GebrGeoXmlDocument * source)
{
	GebrGeoXmlDocumentData *data;
	GebrGeoXmlDocument *document;

	if (source == NULL)
		return NULL;

	data = _gebr_geoxml_document_get_data(source);
	document = (GebrGeoXmlDocument*)__gebr_geoxml_document_clone_doc((GdomeDocument*)source, NULL);
	__gebr_geoxml_document_new_data(document, data->filename->str);

	return document;
}

GebrGeoXmlDocumentType gebr_geoxml_document_get_type(GebrGeoXmlDocument * document)
{
	GebrGeoXmlDocumentType retval;
	GdomeElement *root_element;
	GdomeDOMString *name;

	if (document == NULL)
		return GEBR_GEOXML_DOCUMENT_TYPE_FLOW;

	retval = GEBR_GEOXML_DOCUMENT_TYPE_FLOW;
	root_element = gdome_doc_documentElement((GdomeDocument *) document, &exception);
	name = gdome_el_nodeName(root_element, &exception);

	if (g_strcmp0("flow", name->str) == 0) {
		retval = GEBR_GEOXML_DOCUMENT_TYPE_FLOW;
		goto out;
	}

	if (g_strcmp0("line", name->str) == 0) {
		retval = GEBR_GEOXML_DOCUMENT_TYPE_LINE;
		goto out;
	}

	if (g_strcmp0("project", name->str) == 0) {
		retval = GEBR_GEOXML_DOCUMENT_TYPE_PROJECT;
		goto out;
	}

out:
	gdome_el_unref(root_element, &exception);
	gdome_str_unref(name);
	return retval;
}

gchar *gebr_geoxml_document_get_version(GebrGeoXmlDocument * document)
{
	GdomeElement *root;
	gchar *version;

	if (document == NULL)
		return NULL;

	root = gdome_doc_documentElement((GdomeDocument *) document, &exception);
	version = __gebr_geoxml_get_attr_value(root, "version");
	gdome_el_unref(root, &exception);

	return version;
}

int gebr_geoxml_document_validate(const gchar * filename)
{
	int ret;
	if ((ret = filename_check_access(filename)))
		return ret;

	GebrGeoXmlDocument *document;

	ret = gebr_geoxml_document_load(&document, filename, TRUE, NULL);
	gebr_geoxml_document_free(document);

	return ret;
}

int gebr_geoxml_document_save(GebrGeoXmlDocument * document, const gchar * path)
{
	gzFile zfp;
	char *xml;
	int ret = 0;
	FILE *fp;

	if (document == NULL)
		return FALSE;

	gebr_geoxml_document_to_string(document, &xml);

	if ((gebr_geoxml_document_get_type(document) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) && g_str_has_suffix(path, ".mnu")) {
		fp = fopen(path, "w");
		if (fp == NULL) {
			g_free(xml);
			return GEBR_GEOXML_RETV_PERMISSION_DENIED;
		}

#if ENABLE_TIDY
		Bool ok;
		TidyBuffer output;
		TidyBuffer errbuf;
		TidyDoc tdoc = tidyCreate ();

		tidyBufInit (&output);
		tidyBufInit (&errbuf);
		ok = tidyOptSetBool (tdoc, TidyXmlTags, yes);
		ok = ok && tidyOptSetValue (tdoc, TidyIndentContent, "auto");
		ok = ok && tidyOptSetValue (tdoc, TidyCharEncoding, "UTF8");
		ok = ok && tidyOptSetValue (tdoc, TidyWrapLen, "0");
		ok = ok && tidyOptSetBool(tdoc, TidyLiteralAttribs, yes);

		if (ok) {
			ret = tidySetErrorBuffer (tdoc, &errbuf);
			if (ret >= 0)
				ret = tidyParseString (tdoc, xml);
			if (ret >= 0)
				ret = tidyCleanAndRepair (tdoc);
			if (ret >= 0)
				ret = tidySaveBuffer (tdoc, &output);
			if (ret >= 0)
				ret = fwrite (output.bp, sizeof(gchar),
					      strlen((const gchar *)output.bp), fp);
		}
		tidyBufFree(&output);
		tidyBufFree(&errbuf);
		tidyRelease(tdoc);
#else
		ret = fwrite(xml, sizeof(gchar), strlen(xml), fp);
#endif

		fclose(fp);
	} else {
		zfp = gzopen(path, "w");

		if (zfp == NULL && errno != 0) {
			g_free(xml);
			return GEBR_GEOXML_RETV_PERMISSION_DENIED;
		}

		ret = gzwrite(zfp, xml, strlen(xml));
		gzclose(zfp);
	}

	if (!ret) {
		gchar * filename = g_path_get_basename(path);
		gebr_geoxml_document_set_filename(document, filename);
		g_free(filename);
	}

	g_free(xml);

	return ret ? GEBR_GEOXML_RETV_SUCCESS : GEBR_GEOXML_RETV_PERMISSION_DENIED;
}

int gebr_geoxml_document_to_string(GebrGeoXmlDocument * document, gchar ** xml_string)
{
	if (document == NULL)
		return FALSE;

	GdomeBoolean ret;
	GdomeDocument * clone;

	/* clone to remove DOCTYPE */
	clone = __gebr_geoxml_document_clone_doc((GdomeDocument*)document, NULL);
	ret = gdome_di_saveDocToMemoryEnc(dom_implementation, clone, xml_string, ENCODING,
					  GDOME_SAVE_LIBXML_INDENT, &exception);
	gdome_doc_unref(clone, &exception);

	return ret ? GEBR_GEOXML_RETV_SUCCESS : GEBR_GEOXML_RETV_NO_MEMORY;
}

void gebr_geoxml_document_set_filename(GebrGeoXmlDocument * document, const gchar * filename)
{
	if (document == NULL || filename == NULL)
		return;
	g_string_assign(_gebr_geoxml_document_get_data(document)->filename, filename);
}

void gebr_geoxml_document_set_title(GebrGeoXmlDocument * document, const gchar * title)
{
	set_document_simple_property(document, "title", title);
}

void gebr_geoxml_document_set_author(GebrGeoXmlDocument * document, const gchar * author)
{
	set_document_simple_property(document, "author", author);
}

void gebr_geoxml_document_set_email(GebrGeoXmlDocument * document, const gchar * email)
{
	set_document_simple_property(document, "email", email);
}

GebrGeoXmlParameters *gebr_geoxml_document_get_dict_parameters(GebrGeoXmlDocument * document)
{
	g_return_val_if_fail (document != NULL, NULL);
	GdomeElement *root = gebr_geoxml_document_root_element(document);
	GdomeElement *element = __gebr_geoxml_get_first_element (root, "dict");

	GebrGeoXmlParameters *params = (GebrGeoXmlParameters *)__gebr_geoxml_get_first_element(element, "parameters");

	gdome_el_unref(root, &exception);
	gdome_el_unref(element, &exception);
	return params;
}

gboolean
gebr_geoxml_document_canonize_dict_parameters(GebrGeoXmlDocument *document,
					      GHashTable 	**list_copy)
{
	g_return_val_if_fail(document != NULL, FALSE);
	g_return_val_if_fail(list_copy != NULL, FALSE);

	static GHashTable * vars_list = NULL;

	GebrGeoXmlSequence * parameters = NULL;
	gint i = 0;
	GHashTable * values_to_canonized = NULL;

	values_to_canonized = g_hash_table_new_full(g_str_hash, g_str_equal,
						    g_free, g_free);

	if (gebr_geoxml_document_get_type(document) == GEBR_GEOXML_DOCUMENT_TYPE_PROJECT)
	{
		if (vars_list != NULL)
			g_hash_table_destroy(vars_list);
		vars_list = NULL;
	}

	if (vars_list == NULL)
		vars_list = g_hash_table_new_full(g_str_hash, g_str_equal,
						  g_free, g_free);

	parameters = gebr_geoxml_document_get_dict_parameter(document);

	for (i = 0; parameters != NULL; gebr_geoxml_sequence_next(&parameters), i++)
	{
		gchar * new_value = NULL;
		gchar * key = gebr_geoxml_program_parameter_get_keyword(
				GEBR_GEOXML_PROGRAM_PARAMETER(parameters));

		GebrGeoXmlParameterType type = gebr_geoxml_parameter_get_type(
				GEBR_GEOXML_PARAMETER(parameters));

		gebr_geoxml_program_parameter_set_required(GEBR_GEOXML_PROGRAM_PARAMETER(parameters), TRUE);

		switch(type)
		{
		case	GEBR_GEOXML_PARAMETER_TYPE_STRING:
			break;
		case	GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
			break;
		default:
			gebr_geoxml_parameter_set_type(
					GEBR_GEOXML_PARAMETER(parameters),
					GEBR_GEOXML_PARAMETER_TYPE_FLOAT);
		}

		gchar * spaces = g_strdup(key);
		if(g_strcmp0(g_strstrip(spaces),"") == 0)
		{
			g_free(spaces);
			continue;
		}
		g_free(spaces);

		gebr_str_canonical_var_name(key, &new_value, NULL);
		gchar * duplicated_key = NULL;

		duplicated_key = g_hash_table_lookup(vars_list, key);
		if (duplicated_key)
		{
			gebr_geoxml_program_parameter_set_keyword(
					GEBR_GEOXML_PROGRAM_PARAMETER(parameters),
					(const gchar *)duplicated_key);
			g_free(new_value);
			continue;
		}

		duplicated_key = g_hash_table_lookup(values_to_canonized,
						     new_value);

		if(!duplicated_key)
		{
			g_hash_table_insert(values_to_canonized,
					    g_strdup(new_value),
					    g_strdup(key));

			g_hash_table_insert(vars_list,
					    g_strdup(key),
					    g_strdup(new_value));
		}
		else
		{	
			if(g_strcmp0(key, duplicated_key) == 0)
			{
				g_hash_table_insert(vars_list,
						    g_strdup(key),
						    g_strdup(new_value));

			}
			else
			{

				gint j = 1;
				gchar * concat = g_strdup_printf("%s_%d", new_value, j);

				while (g_hash_table_lookup(values_to_canonized,
						     concat) != NULL)
				{
					g_free(concat);
					concat = NULL;
					j += 1;
					concat = g_strdup_printf("%s_%d", new_value, j);
				}

				new_value = concat;

				g_hash_table_insert(vars_list,
						    g_strdup(key),
						    g_strdup(new_value));

				g_hash_table_insert(values_to_canonized,
						    g_strdup(new_value),
						    g_strdup(key));
			}
		}

		gebr_geoxml_program_parameter_set_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(parameters),
							  (const gchar *)new_value);
		g_free(new_value);
		g_free(key);
	}

	*list_copy = vars_list;
	g_hash_table_destroy(values_to_canonized); 
	return TRUE;
}


void gebr_geoxml_document_set_date_created(GebrGeoXmlDocument * document, const gchar * created)
{
	set_document_property(document, "date", "created", created);
}

void gebr_geoxml_document_set_date_modified(GebrGeoXmlDocument * document, const gchar * modified)
{
	set_document_property(document, "date", "modified", modified);
}

void gebr_geoxml_document_set_description(GebrGeoXmlDocument * document, const gchar * description)
{
	set_document_simple_property(document, "description", description);
}

void gebr_geoxml_document_set_help(GebrGeoXmlDocument * document, const gchar * help)
{
	set_document_simple_property(document, "help", help);
}

const gchar *gebr_geoxml_document_get_filename(GebrGeoXmlDocument * document)
{
	g_return_val_if_fail(document != NULL, NULL);

	return _gebr_geoxml_document_get_data(document)->filename->str;
}

gchar *gebr_geoxml_document_get_title(GebrGeoXmlDocument * document)
{
	return get_document_property(document, "title");
}

gchar *gebr_geoxml_document_get_author(GebrGeoXmlDocument * document)
{
	return get_document_property(document, "author");
}

gchar *gebr_geoxml_document_get_email(GebrGeoXmlDocument * document)
{
	return get_document_property(document, "email");
}

gchar *
gebr_geoxml_document_get_date_created(GebrGeoXmlDocument * document)
{
	gchar *retval;
	GdomeElement *first;
	GdomeElement *root;

	g_return_val_if_fail(document != NULL, NULL);

	root = gebr_geoxml_document_root_element(document);
	first = __gebr_geoxml_get_first_element(root, "date");
	retval = __gebr_geoxml_get_tag_value(first, "created");
	gdome_el_unref(root, &exception);
	gdome_el_unref(first, &exception);

	return retval;
}

gchar *
gebr_geoxml_document_get_date_modified(GebrGeoXmlDocument * document)
{
	gchar *retval;
	GdomeElement *first;
	GdomeElement *root;

	g_return_val_if_fail(document != NULL, NULL);

	root = gebr_geoxml_document_root_element(document);
	first = __gebr_geoxml_get_first_element(root, "date");
	retval = __gebr_geoxml_get_tag_value(first, "modified");
	gdome_el_unref(root, &exception);
	gdome_el_unref(first, &exception);

	return retval;
}

gchar *gebr_geoxml_document_get_description(GebrGeoXmlDocument * document)
{
	return get_document_property(document, "description");
}

gchar *gebr_geoxml_document_get_help(GebrGeoXmlDocument * document)
{
	return get_document_property(document, "help");
}

static gboolean gebr_geoxml_document_is_version_valid(const gchar * version)
{
	if (!version)
		return FALSE;

	guint major, minor, micro;
	return sscanf(version, "%d.%d.%d", &major, &minor, &micro) == 3;
}

static gboolean gebr_geoxml_document_check_version(GebrGeoXmlDocument * document, const gchar * version)
{
	guint major1, minor1, micro1;
	guint major2, minor2, micro2;
	const gchar * doc_version;

	switch (gebr_geoxml_document_get_type(document)) {
	case GEBR_GEOXML_DOCUMENT_TYPE_FLOW:
		doc_version = GEBR_GEOXML_FLOW_VERSION;
		break;
	case GEBR_GEOXML_DOCUMENT_TYPE_LINE:
		doc_version = GEBR_GEOXML_LINE_VERSION;
		break;
	case GEBR_GEOXML_DOCUMENT_TYPE_PROJECT:
		doc_version = GEBR_GEOXML_PROJECT_VERSION;
		break;
	default:
		return FALSE;
	}

	if (sscanf(version, "%d.%d.%d", &major1, &minor1, &micro1) != 3)
		return FALSE;

	if (sscanf(doc_version, "%d.%d.%d", &major2, &minor2, &micro2) != 3)
		return FALSE;

	return major2 < major1
		|| (major2 == major1 && minor2 < minor1)
		|| (major2 == major1 && minor2 == minor1 && micro2 < micro1) ? FALSE : TRUE;
}

static void gebr_geoxml_document_fix_header(GString * source, const gchar * tagname, const gchar * dtd_filename)
{
	gssize c = 0;
	gchar * doctype;

	while (source->str[c] != '>')
		c++;

	doctype = g_strdup_printf("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>"
				  "<!DOCTYPE %s SYSTEM \"%s\">", tagname, dtd_filename);
	g_string_erase(source, 0, c + 1);
	g_string_prepend(source, doctype);
	g_free(doctype);
}

void
gebr_geoxml_document_merge_dicts(GebrValidator *validator, GebrGeoXmlDocument *first, ...)
{
	va_list args;
	GebrGeoXmlDocument *doc;
	GebrGeoXmlParameters *dict;

	g_return_if_fail(first != NULL);

	va_start(args, first);
	doc = va_arg(args, GebrGeoXmlDocument*);

	if (!doc)
		return;

	dict = gebr_geoxml_document_get_dict_parameters(first);

	while (doc) {
		GebrGeoXmlSequence *seq = gebr_geoxml_document_get_dict_parameter(doc);
		GebrGeoXmlParameter *separator = gebr_geoxml_parameters_append_parameter(dict, GEBR_GEOXML_PARAMETER_TYPE_GROUP);
		gebr_geoxml_object_unref(separator);
		while (seq) {
			gchar *value;
			gchar *keyword;
			gchar *comment;
			GebrGeoXmlParameterType type;
			GebrGeoXmlParameter *param =  GEBR_GEOXML_PARAMETER(seq);
			GebrGeoXmlProgramParameter *pparam = GEBR_GEOXML_PROGRAM_PARAMETER(seq);

			// Strip parameters with error
			if (validator && !gebr_validator_validate_param(validator, param, NULL, NULL)) {
				gebr_geoxml_sequence_next(&seq);
				continue;
			}

			// copy data
			value = gebr_geoxml_program_parameter_get_first_value(pparam, FALSE);
			keyword = gebr_geoxml_program_parameter_get_keyword(pparam);
			comment = gebr_geoxml_parameter_get_label(param);
			type = gebr_geoxml_parameter_get_type(param);

			// paste data
			GebrGeoXmlParameter *copy;
			copy = gebr_geoxml_parameters_append_parameter(dict, type);
			param = GEBR_GEOXML_PARAMETER(copy);
			pparam = GEBR_GEOXML_PROGRAM_PARAMETER(copy);
			gebr_geoxml_program_parameter_set_keyword(pparam, keyword);
			gebr_geoxml_program_parameter_set_first_value(pparam, FALSE, value);
			gebr_geoxml_parameter_set_label(param, comment);

			gebr_geoxml_object_unref(copy);
			gebr_geoxml_sequence_next(&seq);
			g_free(comment);
			g_free(keyword);
			g_free(value);
		}
		doc = va_arg(args, GebrGeoXmlDocument*);
	}
	gebr_geoxml_object_unref(dict);
	va_end(args);
}

gboolean
gebr_geoxml_document_split_dict(GebrGeoXmlDocument *first, ...)
{
	va_list args;
	gboolean retval = TRUE;
	GebrGeoXmlSequence *seq;
	GebrGeoXmlSequence *clean = NULL;
	GebrGeoXmlDocument *doc = NULL;
	GebrGeoXmlParameterType type;

	va_start(args, first);

	/**
	 * A document can contain dictionary information of its parents
	 * documents, ie a line can contain its projects variables, and a flow
	 * its line and project variables. Internally, they are separated by
	 * parameters of type "GROUP". See the example below
	 *
	 * FLOW
	 *   DICT-PARAMS
	 *     flow parameter
	 *     flow parameter
	 *     ...
	 *     PARAMETER_TYPE_GROUP
	 *     line parameter
	 *     line parameter
	 *     ...
	 *     PARAMETER_TYPE_GROUP
	 *     proj parameter
	 *     proj parameter
	 *     ...
	 */

	seq = gebr_geoxml_document_get_dict_parameter(GEBR_GEOXML_DOCUMENT(first));
	for (; seq; gebr_geoxml_sequence_next(&seq)) {
		GebrGeoXmlParameter *param = GEBR_GEOXML_PARAMETER(seq);
		type = gebr_geoxml_parameter_get_type(param);
		if (type == GEBR_GEOXML_PARAMETER_TYPE_GROUP) {
			doc = va_arg(args, GebrGeoXmlDocument*);
			if (!doc) {
				/* The list of documents isn't large enough to
				 * hold the parameters. */
				retval = FALSE;
				clean = seq;
				goto clean;
			}
			if (!clean) {
				gebr_geoxml_object_ref(seq);
				clean = seq;
			}
			continue;
		} else if (!doc)
			continue; // skip flow parameters

		// Time to split them
		gchar *value;
		gchar *keyword;
		gchar *comment;
		GebrGeoXmlParameter *copy;
		GebrGeoXmlProgramParameter *pparam = GEBR_GEOXML_PROGRAM_PARAMETER(seq);

		value = gebr_geoxml_program_parameter_get_first_value(pparam, FALSE);
		keyword = gebr_geoxml_program_parameter_get_keyword(pparam);
		comment = gebr_geoxml_parameter_get_label(param);
		copy = gebr_geoxml_document_set_dict_keyword(doc, type, keyword, value);
		gebr_geoxml_parameter_set_label(copy, comment);
		gebr_geoxml_object_unref(copy);
		g_free(value);
		g_free(keyword);
		g_free(comment);
	}

clean:
	while (clean)
	{
		GebrGeoXmlSequence *aux = clean;
		gebr_geoxml_object_ref(aux);
		gebr_geoxml_sequence_next(&clean);
		gebr_geoxml_sequence_remove(aux);
	}

	va_end(args);
	return retval;
}

GebrGeoXmlSequence *
gebr_geoxml_document_get_dict_parameter(GebrGeoXmlDocument *doc)
{
	g_return_val_if_fail(doc != NULL, NULL);
	GebrGeoXmlSequence *seq;
	GebrGeoXmlParameters *params;

	params = gebr_geoxml_document_get_dict_parameters(doc);
	gebr_geoxml_parameters_get_parameter(params, &seq, 0);
	gebr_geoxml_object_unref(params);
	return seq;
}

GebrGeoXmlParameter *
gebr_geoxml_document_set_dict_keyword(GebrGeoXmlDocument *doc,
				      GebrGeoXmlParameterType type,
				      const gchar *keyword,
				      const gchar *value)
{
	GebrGeoXmlParameter *param;
	GebrGeoXmlParameters *params;

	g_return_val_if_fail(doc != NULL, NULL);
	g_return_val_if_fail(keyword != NULL, NULL);
	g_return_val_if_fail(value != NULL, NULL);
	g_return_val_if_fail(type == GEBR_GEOXML_PARAMETER_TYPE_STRING ||
			     type == GEBR_GEOXML_PARAMETER_TYPE_FLOAT ||
			     type == GEBR_GEOXML_PARAMETER_TYPE_INT,
			     NULL);

	params = gebr_geoxml_document_get_dict_parameters(doc);
	param = gebr_geoxml_parameters_append_parameter(params, type);
	gebr_geoxml_program_parameter_set_required(GEBR_GEOXML_PROGRAM_PARAMETER(param), TRUE);
	gebr_geoxml_program_parameter_set_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(param), keyword);
	gebr_geoxml_program_parameter_set_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(param),
						      FALSE, value);

	gebr_geoxml_object_unref(params);
	return param;
}

void gebr_geoxml_document_set_dtd_dir(const gchar *path)
{
	dtd_directory = path;
}

void gebr_geoxml_document_ref(GebrGeoXmlDocument *self)
{
	gdome_doc_ref((GdomeDocument*)self, &exception);
}

void gebr_geoxml_document_unref(GebrGeoXmlDocument *self)
{
	gdome_doc_unref((GdomeDocument*)self, &exception);
}
