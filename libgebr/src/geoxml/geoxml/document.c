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

#include <unistd.h>
#include <string.h>
#include <zlib.h>
#include <stdio.h>
#include <errno.h>

#include <glib/gstdio.h>
#include <gdome.h>
#include <libxml/parser.h>

#include "document.h"
#include "document_p.h"
#include "defines.h"
#include "xml.h"
#include "types.h"
#include "error.h"
#include "sequence.h"
#include "flow.h"
#include "parameter.h"
#include "program-parameter.h"
#include "parameters_p.h"
#include "parameter_p.h"
#include "parameter_group_p.h"

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
static GdomeDOMImplementation *dom_implementation;
/**
 * \internal
 */
static gint dom_implementation_ref_count = 0;
/**
 * \internal
 * Used at GebrGeoXmlObject's methods.
 */
GdomeDocument *clipboard_document = NULL;

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

/**
 * \internal
 */
static void __gebr_geoxml_ref(void)
{
	if (!dom_implementation_ref_count) {
		dom_implementation = gdome_di_mkref();
		clipboard_document = gdome_di_createDocument(dom_implementation, NULL,
							     gdome_str_mkref("gebr-geoxml-clipboard"), NULL,
							     &exception);
	} else
		gdome_di_ref(dom_implementation, &exception);
	++dom_implementation_ref_count;
}

/**
 * \internal
 */
static void __gebr_geoxml_unref(void)
{
	gdome_di_unref(dom_implementation, &exception);
	--dom_implementation_ref_count;
	if (!dom_implementation) {
		gdome_doc_unref(clipboard_document, &exception);
		clipboard_document = NULL;
	}
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
static GdomeDocument *__gebr_geoxml_document_clone_doc(GdomeDocument * source, GdomeDocumentType * document_type)
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
		gdome_el_appendChild(root_element, new_node, &exception);
	}

	return document;
}

/**
 * \internal
 */
static int __gebr_geoxml_document_validate_doc(GdomeDocument ** document, GebrGeoXmlDiscardMenuRefCallback discard_menu_ref)
{
	GString *source;
	GString *dtd_filename;

	GdomeDocument *tmp_doc;

	GdomeElement *root_element;
	const gchar *version;

	int ret;

	if (gdome_doc_doctype(*document, &exception) != NULL) {
		ret = GEBR_GEOXML_RETV_DTD_SPECIFIED;
		goto out2;
	}

	/* initialization */
	dtd_filename = g_string_new(NULL);
	root_element = gebr_geoxml_document_root_element(*document);

	/* If there is no version attribute, the document is invalid */
	version = gebr_geoxml_document_get_version((GebrGeoXmlDocument *) *document);
	if (!gebr_geoxml_document_is_version_valid(version)) {
		ret = GEBR_GEOXML_RETV_INVALID_DOCUMENT;
		goto out;
	}

	/* Checks if the document's version is greater than GeBR's version */
	if (!gebr_geoxml_document_check_version((GebrGeoXmlDocument*)*document, version)) {
		ret = GEBR_GEOXML_RETV_NEWER_VERSION;
		goto out;
	}

	/* Find the DTD spec file. If the file doesn't exists, it may mean that this document is from newer version. */
	gchar * tagname;
	tagname = g_strdup(gdome_el_nodeName(root_element, &exception)->str);
	g_string_printf(dtd_filename, "%s/%s-%s.dtd", GEBR_GEOXML_DTD_DIR, tagname, version);
	if (g_file_test(dtd_filename->str, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR) == FALSE
	    || g_access(dtd_filename->str, R_OK) < 0) {
		ret = GEBR_GEOXML_RETV_CANT_ACCESS_DTD;
		goto out;
	}

	/* Inserts the document type into the xml so we can validate and specify the ID attributes and use
	 * gdome_doc_getElementById. */
	gchar *src;
	gdome_di_saveDocToMemoryEnc(dom_implementation, *document, &src, ENCODING, GDOME_SAVE_STANDARD, &exception);
	source = g_string_new(src);
	g_free(src);
	gebr_geoxml_document_fix_header(source, tagname, dtd_filename->str);

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
		goto out;
	} else {
		xmlFreeDoc(doc);
		if (ctxt->valid == 0) {
			xmlFreeParserCtxt(ctxt);
			ret = GEBR_GEOXML_RETV_INVALID_DOCUMENT;
			goto out;
		}
	}

	tmp_doc = gdome_di_createDocFromMemory(dom_implementation, source->str, GDOME_LOAD_PARSING, &exception);

	g_free(tagname);

	if (tmp_doc == NULL) {
		ret = GEBR_GEOXML_RETV_NO_MEMORY;
		goto out;
	}
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
		gebr_foreach_gslist_hyg(element, __gebr_geoxml_get_elements_by_tag(root_element, "parameters"), parameters) {
			__gebr_geoxml_set_attr_value(element, "default-selection",
						     __gebr_geoxml_get_attr_value(element, "exclusive"));
			__gebr_geoxml_remove_attr(element, "exclusive");
			__gebr_geoxml_set_attr_value(element, "selection",
						     __gebr_geoxml_get_attr_value(element, "selected"));
			__gebr_geoxml_remove_attr(element, "selected");
		}

		gebr_foreach_gslist_hyg(element, __gebr_geoxml_get_elements_by_tag(root_element, "group"), group) {
			GdomeNode *new_instance;
			GdomeElement *parameter;
			GebrGeoXmlParameterGroup *group;
			GebrGeoXmlParameters *template_instance;

			parameter = (GdomeElement*)gdome_el_parentNode(element, &exception);
			group = GEBR_GEOXML_PARAMETER_GROUP(parameter);
			template_instance = GEBR_GEOXML_PARAMETERS(__gebr_geoxml_get_first_element(element, "parameters"));
			/* encapsulate template instance into template-instance element */
			GdomeElement *template_container;
			template_container = __gebr_geoxml_insert_new_element(element, "template-instance",
									      (GdomeElement*)template_instance);
			gdome_el_insertBefore_protected(template_container, (GdomeNode*)template_instance, NULL, &exception);

			/* move new instance after template instance */
			new_instance = gdome_el_cloneNode((GdomeElement*)template_instance, TRUE, &exception);
			GdomeNode *next = (GdomeNode*)__gebr_geoxml_next_element((GdomeElement*)template_container);
			gdome_n_insertBefore_protected((GdomeNode*)element, new_instance, next, &exception);
		}
	}

	/* document 0.1.x to 0.2.0 */
	if (strcmp(version, "0.2.0") < 0) {
		GdomeElement *element;
		GdomeElement *before;

		before = __gebr_geoxml_get_first_element(root_element, "email");
		before = __gebr_geoxml_next_element(before);

		element = __gebr_geoxml_insert_new_element(root_element, "date", before);
		__gebr_geoxml_insert_new_element(element, "created", NULL);
		__gebr_geoxml_insert_new_element(element, "modified", NULL);

		if (gebr_geoxml_document_get_type((GebrGeoXmlDocument *) *document) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW)
			__gebr_geoxml_insert_new_element(element, "lastrun", NULL);
	}
	/* document 0.2.0 to 0.2.1 */
	if (strcmp(version, "0.2.1") < 0) {
		if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument *) *document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
			GebrGeoXmlSequence *program;

			gebr_geoxml_flow_get_program(GEBR_GEOXML_FLOW(*document), &program, 0);
			while (program != NULL) {
				if (__gebr_geoxml_get_first_element((GdomeElement *) program, "url") == NULL)
					__gebr_geoxml_insert_new_element((GdomeElement *) program, "url",
									 __gebr_geoxml_get_first_element((GdomeElement
													  *) program,
													 "parameters"));

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

				__gebr_geoxml_insert_new_element(element, "label", NULL);
				__gebr_geoxml_set_element_value(__gebr_geoxml_insert_new_element
								(element, "value", NULL), value,
								__gebr_geoxml_create_TextNode);

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
			while (program != NULL) {
				GebrGeoXmlParameters *parameters;
				GdomeElement *old_parameter;

				parameters = gebr_geoxml_program_get_parameters(GEBR_GEOXML_PROGRAM(program));
				__gebr_geoxml_set_attr_value((GdomeElement *) parameters, "exclusive", "0");
				old_parameter = __gebr_geoxml_get_first_element((GdomeElement *) parameters, "*");
				while (old_parameter != NULL) {
					GdomeElement *next_parameter;
					GdomeElement *parameter;
					GdomeElement *property;

					GebrGeoXmlParameterType type;
					GdomeDOMString *tag_name;
					int i;

					type = GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN;
					tag_name = gdome_el_tagName(old_parameter, &exception);
					for (i = 1; i <= parameter_type_to_str_len; ++i)
						if (!strcmp(parameter_type_to_str[i], tag_name->str)) {
							type = (GebrGeoXmlParameterType)i;
							break;
						}

					parameter = __gebr_geoxml_insert_new_element((GdomeElement *) parameters,
										     "parameter", old_parameter);
					gdome_el_insertBefore_protected(parameter, (GdomeNode *)
							      __gebr_geoxml_get_first_element(old_parameter, "label"),
							      NULL, &exception);

					next_parameter = __gebr_geoxml_next_element(old_parameter);
					gdome_el_insertBefore_protected(parameter, (GdomeNode *) old_parameter, NULL, &exception);

					property = __gebr_geoxml_insert_new_element(old_parameter, "property",
										    (GdomeElement *)
										    gdome_el_firstChild(old_parameter,
													&exception));
					gdome_el_insertBefore_protected(property, (GdomeNode *)
							      __gebr_geoxml_get_first_element(old_parameter, "keyword"),
							      NULL, &exception);
					if (type != GEBR_GEOXML_PARAMETER_TYPE_FLAG) {
						GdomeElement *value;
						GdomeDOMString *string;
						GdomeDOMString *separator;

						string = gdome_str_mkref("required");
						gdome_el_setAttribute(property, string,
								      gdome_el_getAttribute(old_parameter, string,
											    &exception), &exception);
						gdome_el_removeAttribute(old_parameter, string, &exception);
						gdome_str_unref(string);

						value = __gebr_geoxml_get_first_element(old_parameter, "value");
						string = gdome_str_mkref("separator");
						separator = gdome_el_getAttribute(old_parameter, string, &exception);
						if (strlen(separator->str)) {
							gdome_el_setAttribute(property, string, separator, &exception);

							gebr_geoxml_program_parameter_set_parse_list_value
							    (GEBR_GEOXML_PROGRAM_PARAMETER(parameter), FALSE,
							     __gebr_geoxml_get_element_value(value));
							gebr_geoxml_program_parameter_set_parse_list_value
							    (GEBR_GEOXML_PROGRAM_PARAMETER(parameter), TRUE,
							     __gebr_geoxml_get_attr_value(value, "default"));
						} else {
							__gebr_geoxml_set_element_value(__gebr_geoxml_insert_new_element
											((GdomeElement *) property,
											 "value", NULL),
											__gebr_geoxml_get_element_value
											(value),
											__gebr_geoxml_create_TextNode);
							__gebr_geoxml_set_element_value(__gebr_geoxml_insert_new_element
											((GdomeElement *) property,
											 "default", NULL),
											__gebr_geoxml_get_attr_value
											(value, "default"),
											__gebr_geoxml_create_TextNode);
						}
						gdome_el_removeChild(old_parameter, (GdomeNode *) value, &exception);
						gdome_el_removeAttribute(old_parameter, string, &exception);
						gdome_str_unref(string);
					} else {
						GdomeElement *state;

						state = __gebr_geoxml_get_first_element(old_parameter, "state");
						__gebr_geoxml_set_element_value(__gebr_geoxml_insert_new_element
										((GdomeElement *) property, "value",
										 NULL),
										__gebr_geoxml_get_element_value(state),
										__gebr_geoxml_create_TextNode);
						__gebr_geoxml_set_element_value(__gebr_geoxml_insert_new_element
										((GdomeElement *) property, "default",
										 NULL),
										__gebr_geoxml_get_attr_value(state,
													     "default"),
										__gebr_geoxml_create_TextNode);
						__gebr_geoxml_set_attr_value(property, "required", "no");

						gdome_el_removeChild(old_parameter, (GdomeNode *) state, &exception);
					}

					old_parameter = next_parameter;
				}

				gebr_geoxml_sequence_next(&program);
			}
		}
	}
	/* document 0.3.0 to 0.3.1 */
	if (strcmp(version, "0.3.1") < 0) {
		GdomeElement *dict_element;

		__gebr_geoxml_set_attr_value(root_element, "version", "0.3.1");

		dict_element = __gebr_geoxml_insert_new_element(root_element, "dict",
								__gebr_geoxml_get_first_element(root_element, "date"));
		__gebr_geoxml_parameters_append_new(dict_element);

		if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument *) *document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
			__gebr_geoxml_insert_new_element(root_element, "servers",
							 __gebr_geoxml_next_element(__gebr_geoxml_get_first_element
										    (root_element, "io")));
		}
	}
	/* 0.3.1 to 0.3.2 */ 
	if (strcmp(version, "0.3.2") < 0) {
		__gebr_geoxml_set_attr_value(root_element, "version", "0.3.2");

		if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument *) *document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
			GdomeElement *element;

			gebr_foreach_gslist(element, __gebr_geoxml_get_elements_by_tag(root_element, "menu")) {
				if (discard_menu_ref != NULL)
					discard_menu_ref((GebrGeoXmlProgram *)gdome_el_parentNode(element, &exception), 
							__gebr_geoxml_get_element_value(element) ,
							atoi(__gebr_geoxml_get_attr_value(element, "index")));

				gdome_n_removeChild(gdome_el_parentNode(element, &exception), (GdomeNode *)element, &exception);
			}
		} else {
			/* removal of filename for project and lines */
			gdome_el_removeChild(root_element, (GdomeNode*)__gebr_geoxml_get_first_element(root_element, "filename"), &exception);

			__gebr_geoxml_remove_attr(root_element, "nextid");

			GSList * params;
			GSList * iter;

			params = __gebr_geoxml_get_elements_by_tag(root_element, "parameter");
			iter = params;
			while (iter) {
				__gebr_geoxml_remove_attr(iter->data, "id");
				iter = iter->next;
			}
			g_slist_free(params);

			GSList * refer;

			refer = __gebr_geoxml_get_elements_by_tag(root_element, "reference");
			iter = refer;
			while (iter) {
				__gebr_geoxml_remove_attr(iter->data, "idref");
				iter = iter->next;
			}
			g_slist_free(refer);

			__port_to_new_group_semantics();
		}
	}
	/* flow 0.3.2 to 0.3.3 */ 
	if (strcmp(version, "0.3.3") < 0) {
		// Backward compatible change, nothing to be done.
		// Added #IMPLIED 'version' attribute to 'program' tag.
		if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument *) *document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW)
			__gebr_geoxml_set_attr_value(root_element, "version", "0.3.3");
	}
	/* flow 0.3.3 to 0.3.4 */ 
	if (strcmp(version, "0.3.4") < 0) {
		// Backward compatible change, nothing to be done.
		// Added #IMPLIED 'mpi' attribute to 'program' tag.
		if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument *) *document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW)
			__gebr_geoxml_set_attr_value(root_element, "version", "0.3.4");
	}
	/* flow 0.3.4 to 0.3.5 */ 
	if (strcmp(version, "0.3.5") < 0) {
		if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument *) *document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
			__gebr_geoxml_set_attr_value(root_element, "version", "0.3.5");

			/* remove flow filename */
			gdome_el_removeChild(root_element, (GdomeNode*)__gebr_geoxml_get_first_element(root_element, "filename"), &exception);

			__port_to_new_group_semantics();

			__gebr_geoxml_remove_attr(root_element, "nextid");

			GSList * params;
			GSList * iter;

			params = __gebr_geoxml_get_elements_by_tag(root_element, "parameter");
			iter = params;
			while (iter) {
				__gebr_geoxml_remove_attr(iter->data, "id");
				iter = iter->next;
			}
			g_slist_free(params);

			GSList * refer;

			refer = __gebr_geoxml_get_elements_by_tag(root_element, "reference");
			iter = refer;
			while (iter) {
				__gebr_geoxml_remove_attr(iter->data, "idref");
				iter = iter->next;
			}
			g_slist_free(refer);
		}
	}
	/* flow 0.3.5 to 0.3.6 */ 
	if (strcmp(version, "0.3.6") < 0) {
		if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument *) *document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
			__gebr_geoxml_set_attr_value(root_element, "version", "0.3.6");

			GdomeElement *element;

			gebr_foreach_gslist_hyg(element, __gebr_geoxml_get_elements_by_tag(root_element, "group"), group) {
				GdomeElement *parameter;
				GebrGeoXmlParameterGroup *group;
				GebrGeoXmlSequence *instance;
				GebrGeoXmlSequence *iter;
				gboolean first_instance = TRUE;

				parameter = (GdomeElement*)gdome_el_parentNode(element, &exception);
				group = GEBR_GEOXML_PARAMETER_GROUP(parameter);
				gebr_geoxml_parameter_group_get_instance(group, &instance, 0);

				for (; instance != NULL; gebr_geoxml_sequence_next(&instance)){
					gebr_geoxml_parameters_get_parameter(GEBR_GEOXML_PARAMETERS(instance), &iter, 0);

					for (; iter != NULL; gebr_geoxml_sequence_next(&iter)){
						__gebr_geoxml_parameter_set_label((GebrGeoXmlParameter *) iter, "");

						if (first_instance){
							if( __gebr_geoxml_parameter_get_type((GebrGeoXmlParameter *) iter, FALSE) != GEBR_GEOXML_PARAMETER_TYPE_REFERENCE){
								__gebr_geoxml_parameter_set_be_reference_with_value((GebrGeoXmlParameter *) iter);
							}
						}
					}
					first_instance = FALSE;
				}
			}
		}
	}

	/* CHECKS (may impact performance) */
	if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument *) *document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
		GdomeElement *element;

		/* accept only on/off for flags */
		gebr_foreach_gslist(element, __gebr_geoxml_get_elements_by_tag(root_element, "flag")) {
			GdomeElement *value_element;
			GdomeElement *default_value_element;

			value_element = __gebr_geoxml_get_first_element(element, "value");
			default_value_element = __gebr_geoxml_get_first_element(element, "default");

			if (strcmp(__gebr_geoxml_get_element_value(value_element), "on") &&
			    strcmp(__gebr_geoxml_get_element_value(value_element), "off"))
				__gebr_geoxml_set_element_value(value_element, "off", __gebr_geoxml_create_TextNode);
			if (strcmp(__gebr_geoxml_get_element_value(default_value_element), "on") &&
			    strcmp(__gebr_geoxml_get_element_value(default_value_element), "off"))
				__gebr_geoxml_set_element_value(default_value_element, "off",
								__gebr_geoxml_create_TextNode);
		}
	}

	ret = GEBR_GEOXML_RETV_SUCCESS;

out:
	g_string_free(dtd_filename, TRUE);
	g_string_free(source, TRUE);
out2:
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

	/* create the implementation. */
	__gebr_geoxml_ref();

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

 err:	__gebr_geoxml_unref();
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

	/* create the implementation. */
	__gebr_geoxml_ref();
	document = gdome_di_createDocument(dom_implementation, NULL, gdome_str_mkref(name), NULL, &exception);
	__gebr_geoxml_document_new_data((GebrGeoXmlDocument *)document, "");

	/* document (root) element */
	root_element = gdome_doc_documentElement(document, &exception);
	__gebr_geoxml_set_attr_value(root_element, "version", version);

	/* elements (as specified in DTD) */
	__gebr_geoxml_insert_new_element(root_element, "title", NULL);
	__gebr_geoxml_insert_new_element(root_element, "description", NULL);
	__gebr_geoxml_insert_new_element(root_element, "help", NULL);
	__gebr_geoxml_insert_new_element(root_element, "author", NULL);
	__gebr_geoxml_insert_new_element(root_element, "email", NULL);
	element = __gebr_geoxml_insert_new_element(root_element, "dict", NULL);
	__gebr_geoxml_parameters_append_new(element);
	element = __gebr_geoxml_insert_new_element(root_element, "date", NULL);
	__gebr_geoxml_insert_new_element(element, "created", NULL);
	__gebr_geoxml_insert_new_element(element, "modified", NULL);

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
	__gebr_geoxml_unref();
}

GebrGeoXmlDocument *gebr_geoxml_document_clone(GebrGeoXmlDocument * source)
{
	if (source == NULL)
		return NULL;

	gchar *xml;
	GebrGeoXmlDocumentData *data;
	GebrGeoXmlDocument *document;

	data = _gebr_geoxml_document_get_data(source);
	gebr_geoxml_document_to_string(source, &xml);
	gebr_geoxml_document_load_buffer(&document, xml);
	__gebr_geoxml_document_new_data(document, data->filename->str);
	g_free(xml);

	return document;
}

GebrGeoXmlDocumentType gebr_geoxml_document_get_type(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return GEBR_GEOXML_DOCUMENT_TYPE_FLOW;

	GdomeElement *root_element;

	root_element = gdome_doc_documentElement((GdomeDocument *) document, &exception);

	if (strcmp("flow", gdome_el_nodeName(root_element, &exception)->str) == 0)
		return GEBR_GEOXML_DOCUMENT_TYPE_FLOW;
	else if (strcmp("line", gdome_el_nodeName(root_element, &exception)->str) == 0)
		return GEBR_GEOXML_DOCUMENT_TYPE_LINE;
	else if (strcmp("project", gdome_el_nodeName(root_element, &exception)->str) == 0)
		return GEBR_GEOXML_DOCUMENT_TYPE_PROJECT;

	return GEBR_GEOXML_DOCUMENT_TYPE_FLOW;
}

const gchar *gebr_geoxml_document_get_version(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return __gebr_geoxml_get_attr_value(gdome_doc_documentElement((GdomeDocument *) document, &exception),
					    "version");
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
	int ret;
	FILE *fp;

	if (document == NULL)
		return FALSE;

	gebr_geoxml_document_to_string(document, &xml);

	if ((gebr_geoxml_document_get_type(document) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) && g_str_has_suffix(path, ".mnu")) {
		fp = fopen(path, "w");

		if (fp == NULL){
			return GEBR_GEOXML_RETV_PERMISSION_DENIED;
		}
		ret = fwrite(xml, sizeof(gchar), strlen(xml) + 1, fp);
		fclose(fp);
	} else {
		zfp = gzopen(path, "w");

		if (zfp == NULL || errno != 0)
			return GEBR_GEOXML_RETV_PERMISSION_DENIED;

		ret = gzwrite(zfp, xml, strlen(xml) + 1);
		gzclose(zfp);
	}

	if (!ret) {
		gchar * filename = g_path_get_basename(path);
		gebr_geoxml_document_set_filename(document, filename);
		g_free(filename);
	}

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
	if (document == NULL || title == NULL)
		return;
	__gebr_geoxml_set_tag_value(gebr_geoxml_document_root_element(document),
				    "title", title, __gebr_geoxml_create_TextNode);
}

void gebr_geoxml_document_set_author(GebrGeoXmlDocument * document, const gchar * author)
{
	if (document == NULL || author == NULL)
		return;
	__gebr_geoxml_set_tag_value(gebr_geoxml_document_root_element(document),
				    "author", author, __gebr_geoxml_create_TextNode);
}

void gebr_geoxml_document_set_email(GebrGeoXmlDocument * document, const gchar * email)
{
	if (document == NULL || email == NULL)
		return;
	__gebr_geoxml_set_tag_value(gebr_geoxml_document_root_element(document),
				    "email", email, __gebr_geoxml_create_TextNode);
}

GebrGeoXmlParameters *gebr_geoxml_document_get_dict_parameters(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return (GebrGeoXmlParameters *)
	    __gebr_geoxml_get_first_element(__gebr_geoxml_get_first_element
					    (gebr_geoxml_document_root_element(document), "dict"), "parameters");
}

void gebr_geoxml_document_set_date_created(GebrGeoXmlDocument * document, const gchar * created)
{
	if (document == NULL || created == NULL)
		return;
	__gebr_geoxml_set_tag_value(__gebr_geoxml_get_first_element
				    (gebr_geoxml_document_root_element(document), "date"), "created", created,
				    __gebr_geoxml_create_TextNode);
}

void gebr_geoxml_document_set_date_modified(GebrGeoXmlDocument * document, const gchar * modified)
{
	if (document == NULL || modified == NULL)
		return;
	__gebr_geoxml_set_tag_value(__gebr_geoxml_get_first_element
				    (gebr_geoxml_document_root_element(document), "date"), "modified", modified,
				    __gebr_geoxml_create_TextNode);
}

void gebr_geoxml_document_set_description(GebrGeoXmlDocument * document, const gchar * description)
{
	if (document == NULL || description == NULL)
		return;
	__gebr_geoxml_set_tag_value(gebr_geoxml_document_root_element(document),
				    "description", description, __gebr_geoxml_create_TextNode);
}

void gebr_geoxml_document_set_help(GebrGeoXmlDocument * document, const gchar * help)
{
	if (document == NULL || help == NULL)
		return;
	__gebr_geoxml_set_tag_value(gebr_geoxml_document_root_element(document),
				    "help", help, __gebr_geoxml_create_CDATASection);
}

const gchar *gebr_geoxml_document_get_filename(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return _gebr_geoxml_document_get_data(document)->filename->str;
}

const gchar *gebr_geoxml_document_get_title(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value(gebr_geoxml_document_root_element(document), "title");
}

const gchar *gebr_geoxml_document_get_author(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value(gebr_geoxml_document_root_element(document), "author");
}

const gchar *gebr_geoxml_document_get_email(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value(gebr_geoxml_document_root_element(document), "email");
}

const gchar *gebr_geoxml_document_get_date_created(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return
	    __gebr_geoxml_get_tag_value(__gebr_geoxml_get_first_element
					(gebr_geoxml_document_root_element(document), "date"), "created");
}

const gchar *gebr_geoxml_document_get_date_modified(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return
	    __gebr_geoxml_get_tag_value(__gebr_geoxml_get_first_element
					(gebr_geoxml_document_root_element(document), "date"), "modified");
}

const gchar *gebr_geoxml_document_get_description(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value(gebr_geoxml_document_root_element(document), "description");
}

const gchar *gebr_geoxml_document_get_help(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value(gebr_geoxml_document_root_element(document), "help");
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
