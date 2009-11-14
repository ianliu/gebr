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
#include "program_parameter.h"
#include "parameters_p.h"

/* global variables */
GdomeException			exception;

/*
 * internal stuff
 */

struct gebr_geoxml_document {
	GdomeDocument*	document;
};

typedef GdomeDocument *
(*createDoc_func)(GdomeDOMImplementation *, char *, unsigned int, GdomeException *);

static GdomeDOMImplementation*	dom_implementation;
static gint			dom_implementation_ref_count = 0;
GdomeDocument *			clipboard_document = NULL;

static void
__gebr_geoxml_ref(void)
{
	if (!dom_implementation_ref_count) {
		dom_implementation = gdome_di_mkref();
		clipboard_document = gdome_di_createDocument(dom_implementation, NULL,
			gdome_str_mkref("gebr-geoxml-clipboard"), NULL, &exception);
	} else
		gdome_di_ref(dom_implementation, &exception);
	++dom_implementation_ref_count;
}

static void
__gebr_geoxml_unref(void)
{
	gdome_di_unref(dom_implementation, &exception);
	--dom_implementation_ref_count;
	if (!dom_implementation)
		clipboard_document = NULL;
}

static GdomeDocument *
__gebr_geoxml_document_clone_doc(GdomeDocument * source, GdomeDocumentType * document_type)
{
	if (source == NULL)
		return NULL;

	GdomeDocument *		document;
	GdomeElement *		root_element;
	GdomeElement *		source_root_element;
	GdomeDOMString *	string;
	GdomeNode *		node;

	source_root_element = gdome_doc_documentElement(source, &exception);
	document = gdome_di_createDocument(dom_implementation,
		NULL, gdome_el_nodeName(source_root_element, &exception), document_type, &exception);
	if (document == NULL)
		return NULL;

	/* copy version attribute */
	root_element = gdome_doc_documentElement(document, &exception);
	__gebr_geoxml_set_attr_value(root_element, "version",
		__gebr_geoxml_get_attr_value(source_root_element, "version"));
	/* nextid */
	string = gdome_str_mkref("nextid");
	if (gdome_el_hasAttribute(source_root_element, string, &exception) == TRUE)
		__gebr_geoxml_set_attr_value(root_element, "nextid",
			__gebr_geoxml_get_attr_value(source_root_element, "nextid"));
	gdome_str_unref(string);

	node = gdome_el_firstChild(source_root_element, &exception);
	do {
		GdomeNode* new_node = gdome_doc_importNode(document, node, TRUE, &exception);
		gdome_el_appendChild(root_element, new_node, &exception);

		node = gdome_n_nextSibling(node, &exception);
	} while (node != NULL);

	return document;
}

static int
__gebr_geoxml_document_validate_doc(GdomeDocument * document)
{
	/* Based on code by Daniel Veillard
	 * References:
	 *   http://xmlsoft.org/examples/index.html#parse2.c
	 *   http://xmlsoft.org/examples/parse2.c
	 */

	xmlParserCtxtPtr	ctxt;
	xmlDocPtr		doc;
	gchar *			xml;

	GString *		dtd_filename;

	GdomeDocument *		tmp_doc;
	GdomeDocumentType *	tmp_document_type;

	GdomeElement *		root_element;
	gchar *			version;

	int			ret;

	ctxt = xmlNewParserCtxt();
	if (ctxt == NULL)
		return GEBR_GEOXML_RETV_NO_MEMORY;
	if (gdome_doc_doctype(document, &exception) != NULL) {
		ret = GEBR_GEOXML_RETV_DTD_SPECIFIED;
		goto out2;
	}

	/* initialization */
	dtd_filename = g_string_new(NULL);
	root_element = gebr_geoxml_document_root_element(document);

	/* find DTD */
	g_string_printf(dtd_filename, "%s/%s-%s.dtd", GEBR_GEOXML_DTD_DIR,
		gdome_el_nodeName(root_element, &exception)->str,
		gebr_geoxml_document_get_version((GebrGeoXmlDocument*)document));
	if (g_file_test(dtd_filename->str, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR) == FALSE
		|| g_access(dtd_filename->str, R_OK) < 0) {
		ret = GEBR_GEOXML_RETV_CANT_ACCESS_DTD;
		goto out;
	}

	/* include DTD with full path */
	tmp_document_type = gdome_di_createDocumentType(dom_implementation,
		gdome_el_nodeName(root_element, &exception), NULL,
		gdome_str_mkref_own(dtd_filename->str), &exception);
	tmp_doc = __gebr_geoxml_document_clone_doc(document, tmp_document_type);
	if (tmp_document_type == NULL || tmp_doc == NULL) {
		ret = GEBR_GEOXML_RETV_NO_MEMORY;
		goto out;
	}

	gdome_di_saveDocToMemoryEnc(dom_implementation, tmp_doc,
		&xml, ENCODING, GDOME_SAVE_STANDARD, &exception);
	doc = xmlCtxtReadMemory(ctxt, xml, strlen(xml), NULL, NULL,
		XML_PARSE_NOBLANKS |
		XML_PARSE_DTDATTR |  /* default DTD attributes */
		XML_PARSE_NOENT |    /* substitute entities */
		XML_PARSE_DTDVALID);

	/* frees memory before we quit. */
	gdome_doc_unref(tmp_doc, &exception);
	g_free(xml);

	if (doc == NULL) {
		ret = GEBR_GEOXML_RETV_INVALID_DOCUMENT;
		goto out;
	} else {
		xmlFreeDoc(doc);
		if (ctxt->valid == 0) {
			ret = GEBR_GEOXML_RETV_INVALID_DOCUMENT;
			goto out;
		}
	}

	/*
	 * Success, now change to last version
	 */

	version = (gchar*)gebr_geoxml_document_get_version((GebrGeoXmlDocument*)document);
	/* document 0.1.x to 0.2.0 */
	if (strcmp(version, "0.2.0") < 0) {
		GdomeElement *		element;
		GdomeElement *		before;

		before = __gebr_geoxml_get_first_element(root_element, "email");
		before = __gebr_geoxml_next_element(before);

		element = __gebr_geoxml_insert_new_element(root_element, "date", before);
		__gebr_geoxml_insert_new_element(element, "created", NULL);
		__gebr_geoxml_insert_new_element(element, "modified", NULL);

		if (gebr_geoxml_document_get_type((GebrGeoXmlDocument*)document) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW)
			__gebr_geoxml_insert_new_element(element, "lastrun", NULL);
	}
	/* document 0.2.0 to 0.2.1 */
	if (strcmp(version, "0.2.1") < 0) {
		if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument*)document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
			GebrGeoXmlSequence *	program;

			gebr_geoxml_flow_get_program(GEBR_GEOXML_FLOW(document), &program, 0);
			while (program != NULL) {
				if (__gebr_geoxml_get_first_element((GdomeElement*)program, "url") == NULL)
					__gebr_geoxml_insert_new_element((GdomeElement*)program, "url",
						__gebr_geoxml_get_first_element((GdomeElement*)program, "parameters"));

				gebr_geoxml_sequence_next(&program);
			}
		}
	}
	/* document 0.2.1 to 0.2.2 */
	if (strcmp(version, "0.2.2") < 0) {
		if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument*)document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
			GdomeElement *		element;

			__gebr_geoxml_set_attr_value(root_element, "version", "0.2.2");

			__gebr_geoxml_foreach_element_with_tagname_r(root_element, "option", element, option) {
				gchar *	value;

				value = g_strdup(__gebr_geoxml_get_element_value(element));
				__gebr_geoxml_set_element_value(element, "", __gebr_geoxml_create_TextNode);

				__gebr_geoxml_insert_new_element(element, "label", NULL);
				__gebr_geoxml_set_element_value(__gebr_geoxml_insert_new_element(element, "value", NULL),
					value, __gebr_geoxml_create_TextNode);

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

		if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument*)document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
			GebrGeoXmlSequence *		program;

			gebr_geoxml_flow_get_program(GEBR_GEOXML_FLOW(document), &program, 0);
			while (program != NULL) {
				GebrGeoXmlParameters *	parameters;
				GdomeElement *		old_parameter;

				parameters = gebr_geoxml_program_get_parameters(GEBR_GEOXML_PROGRAM(program));
				__gebr_geoxml_set_attr_value((GdomeElement*)parameters, "exclusive", "0");
				old_parameter = __gebr_geoxml_get_first_element((GdomeElement*)parameters, "*");
				while (old_parameter != NULL) {
					GdomeElement *			next_parameter;
					GdomeElement *			parameter;
					GdomeElement *			property;

					enum GEBR_GEOXML_PARAMETER_TYPE	type;
					GdomeDOMString*			tag_name;
					int				i;

					type = GEBR_GEOXML_PARAMETER_TYPE_UNKNOWN;
					tag_name = gdome_el_tagName(old_parameter, &exception);
					for (i = 1; i <= parameter_type_to_str_len; ++i)
						if (!strcmp(parameter_type_to_str[i], tag_name->str)) {
							type = (enum GEBR_GEOXML_PARAMETER_TYPE)i;
							break;
						}

					parameter = __gebr_geoxml_insert_new_element((GdomeElement*)parameters,
						"parameter", old_parameter);
					__gebr_geoxml_element_assign_new_id(parameter, NULL, FALSE);
					gdome_el_insertBefore(parameter, (GdomeNode*)
						__gebr_geoxml_get_first_element(old_parameter, "label"),
						NULL, &exception);

					next_parameter = __gebr_geoxml_next_element(old_parameter);
					gdome_el_insertBefore(parameter, (GdomeNode*)old_parameter, NULL, &exception);

					property = __gebr_geoxml_insert_new_element(old_parameter, "property",
						(GdomeElement*)gdome_el_firstChild(old_parameter, &exception));
					gdome_el_insertBefore(property,
						(GdomeNode*)__gebr_geoxml_get_first_element(old_parameter, "keyword"),
						NULL, &exception);
					if (type != GEBR_GEOXML_PARAMETER_TYPE_FLAG) {
						GdomeElement *		value;
						GdomeDOMString *	string;
						GdomeDOMString *	separator;

						string = gdome_str_mkref("required");
						gdome_el_setAttribute(property, string,
							gdome_el_getAttribute(old_parameter, string, &exception),
							&exception);
						gdome_el_removeAttribute(old_parameter, string, &exception);
						gdome_str_unref(string);

						value = __gebr_geoxml_get_first_element(old_parameter, "value");
						string = gdome_str_mkref("separator");
						separator = gdome_el_getAttribute(old_parameter, string, &exception);
						if (strlen(separator->str)) {
							gdome_el_setAttribute(property, string, separator, &exception);

							gebr_geoxml_program_parameter_set_parse_list_value(
								GEBR_GEOXML_PROGRAM_PARAMETER(parameter), FALSE,
								__gebr_geoxml_get_element_value(value));
							gebr_geoxml_program_parameter_set_parse_list_value(
								GEBR_GEOXML_PROGRAM_PARAMETER(parameter), TRUE,
								__gebr_geoxml_get_attr_value(value, "default"));
						} else {
							__gebr_geoxml_set_element_value(
								__gebr_geoxml_insert_new_element((GdomeElement*)property,
									"value", NULL),
								__gebr_geoxml_get_element_value(value),
								__gebr_geoxml_create_TextNode);
							__gebr_geoxml_set_element_value(
								__gebr_geoxml_insert_new_element((GdomeElement*)property,
									"default", NULL),
								__gebr_geoxml_get_attr_value(value, "default"),
								__gebr_geoxml_create_TextNode);
						}
						gdome_el_removeChild(old_parameter, (GdomeNode*)value, &exception);
						gdome_el_removeAttribute(old_parameter, string, &exception);
						gdome_str_unref(string);
					} else {
						GdomeElement *	state;

						state = __gebr_geoxml_get_first_element(old_parameter, "state");
						__gebr_geoxml_set_element_value(
							__gebr_geoxml_insert_new_element((GdomeElement*)property,
								"value", NULL),
							__gebr_geoxml_get_element_value(state),
							__gebr_geoxml_create_TextNode);
						__gebr_geoxml_set_element_value(
							__gebr_geoxml_insert_new_element((GdomeElement*)property,
								"default", NULL),
							__gebr_geoxml_get_attr_value(state, "default"),
							__gebr_geoxml_create_TextNode);
						__gebr_geoxml_set_attr_value(property, "required", "no");

						gdome_el_removeChild(old_parameter, (GdomeNode*)state, &exception);
					}

					old_parameter = next_parameter;
				}

				gebr_geoxml_sequence_next(&program);
			}
		}
	}
	/* document 0.3.0 to 0.3.1 */
	if (strcmp(version, "0.3.1") < 0) {
		GdomeElement *	dict_element;

		__gebr_geoxml_set_attr_value(root_element, "version", "0.3.1");

		dict_element = __gebr_geoxml_insert_new_element(root_element, "dict",
			__gebr_geoxml_get_first_element(root_element, "date"));
		__gebr_geoxml_parameters_append_new(dict_element);

		if (gebr_geoxml_document_get_type(((GebrGeoXmlDocument*)document)) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW) {
			GdomeElement *	pivot;
			pivot = __gebr_geoxml_next_element(__gebr_geoxml_get_first_element(root_element, "io"));
			__gebr_geoxml_insert_new_element(root_element, "servers", pivot);
		}
	}

	ret = GEBR_GEOXML_RETV_SUCCESS;
out:	g_string_free(dtd_filename, TRUE);
out2:	xmlFreeParserCtxt(ctxt);

	return ret;
}

static int
__gebr_geoxml_document_load(GebrGeoXmlDocument ** document, const gchar * src, createDoc_func func)
{
	GdomeDocument *	doc;
	int		ret;

	/* create the implementation. */
	__gebr_geoxml_ref();

	/* load */
	doc = func(dom_implementation, (gchar*)src, GDOME_LOAD_PARSING, &exception);
	if (doc == NULL) {
		ret = GEBR_GEOXML_RETV_INVALID_DOCUMENT;
		goto err;
	}

	/* validate */
	ret = __gebr_geoxml_document_validate_doc(doc);
	if (ret != GEBR_GEOXML_RETV_SUCCESS) {
		gdome_doc_unref((GdomeDocument*)doc, &exception);
		goto err;
	}

	*document = (GebrGeoXmlDocument*)doc;
	return GEBR_GEOXML_RETV_SUCCESS;

err:	__gebr_geoxml_unref();
	*document = NULL;
	return ret;
}

/*
 * private functions and variables
 */

GebrGeoXmlDocument *
gebr_geoxml_document_new(const gchar * name, const gchar * version)
{
	GdomeDocument *	document;
	GdomeElement *	root_element;
	GdomeElement *	element;

	/* create the implementation. */
	__gebr_geoxml_ref();

	/* doc */
	document = gdome_di_createDocument(dom_implementation,
		NULL, gdome_str_mkref(name), NULL, &exception);

	/* document (root) element */
	root_element = gdome_doc_documentElement(document, &exception);
	__gebr_geoxml_set_attr_value(root_element, "version", version);
	__gebr_geoxml_set_attr_value(root_element, "nextid", "n0");

	/* elements (as specified in DTD) */
	__gebr_geoxml_insert_new_element(root_element, "filename", NULL);
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

	return (GebrGeoXmlDocument*)document;
}

/*
 * library functions.
 */

int
gebr_geoxml_document_load(GebrGeoXmlDocument ** document, const gchar * path)
{
	if (g_file_test(path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR) == FALSE
		|| g_access(path, R_OK) < 0) {
		*document = NULL;
		return GEBR_GEOXML_RETV_CANT_ACCESS_FILE;
	}

	return __gebr_geoxml_document_load(document, path, (createDoc_func)gdome_di_createDocFromURI);
}

int
gebr_geoxml_document_load_buffer(GebrGeoXmlDocument ** document, const gchar * xml)
{
	return __gebr_geoxml_document_load(document, xml, (createDoc_func)gdome_di_createDocFromMemory);
}

void
gebr_geoxml_document_free(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return;

	gdome_doc_unref((GdomeDocument*)document, &exception);
	__gebr_geoxml_unref();
}

GebrGeoXmlDocument *
gebr_geoxml_document_clone(GebrGeoXmlDocument * source)
{
	if (source == NULL)
		return NULL;

	GdomeDocument *	document;

	document = __gebr_geoxml_document_clone_doc((GdomeDocument*)source, NULL);

	__gebr_geoxml_ref();

	return (GebrGeoXmlDocument*)document;
}

enum GEBR_GEOXML_DOCUMENT_TYPE
gebr_geoxml_document_get_type(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return GEBR_GEOXML_DOCUMENT_TYPE_FLOW;

	GdomeElement *	root_element;

	root_element = gdome_doc_documentElement((GdomeDocument*)document, &exception);

	if (strcmp("flow", gdome_el_nodeName(root_element, &exception)->str) == 0)
		return GEBR_GEOXML_DOCUMENT_TYPE_FLOW;
	else if (strcmp("line", gdome_el_nodeName(root_element, &exception)->str) == 0)
		return GEBR_GEOXML_DOCUMENT_TYPE_LINE;
	else if (strcmp("project", gdome_el_nodeName(root_element, &exception)->str) == 0)
		return GEBR_GEOXML_DOCUMENT_TYPE_PROJECT;

	return GEBR_GEOXML_DOCUMENT_TYPE_FLOW;
}

const gchar *
gebr_geoxml_document_get_version(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return __gebr_geoxml_get_attr_value(gdome_doc_documentElement((GdomeDocument*)document, &exception), "version");
}

int
gebr_geoxml_document_validate(const gchar * filename)
{
	if (g_file_test(filename, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR) == FALSE
		|| g_access(filename, R_OK) < 0)
		return GEBR_GEOXML_RETV_CANT_ACCESS_FILE;

	GebrGeoXmlDocument *	document;
	int			ret;

	ret = gebr_geoxml_document_load(&document, filename);
	gebr_geoxml_document_free(document);

	return ret;
}

int
gebr_geoxml_document_save(GebrGeoXmlDocument * document, const gchar * path)
{
        gzFile    zfp;
        char *    xml;
        int       ret;
 
	if (document == NULL)
		return FALSE;

        zfp = gzopen (path, "w");
        if (zfp == NULL)
                return GEBR_GEOXML_RETV_CANT_ACCESS_FILE;

        gebr_geoxml_document_to_string(document, &xml);
        ret = gzwrite (zfp, xml, strlen(xml)+1); 
        gzclose(zfp);

	return ret ? GEBR_GEOXML_RETV_SUCCESS
                : GEBR_GEOXML_RETV_CANT_ACCESS_FILE;
}

int
gebr_geoxml_document_to_string(GebrGeoXmlDocument * document, gchar ** xml_string)
{
	if (document == NULL)
		return FALSE;

	return gdome_di_saveDocToMemoryEnc(dom_implementation, (GdomeDocument*)document,
		xml_string, ENCODING, GDOME_SAVE_LIBXML_INDENT, &exception)
			? GEBR_GEOXML_RETV_SUCCESS
			: GEBR_GEOXML_RETV_NO_MEMORY;
}

void
gebr_geoxml_document_set_filename(GebrGeoXmlDocument * document, const gchar * filename)
{
	if (document == NULL || filename == NULL)
		return;
	__gebr_geoxml_set_tag_value(gebr_geoxml_document_root_element(document),
		"filename", filename, __gebr_geoxml_create_TextNode);
}

void
gebr_geoxml_document_set_title(GebrGeoXmlDocument * document, const gchar * title)
{
	if (document == NULL || title == NULL)
		return;
	__gebr_geoxml_set_tag_value(gebr_geoxml_document_root_element(document),
		"title", title, __gebr_geoxml_create_TextNode);
}

void
gebr_geoxml_document_set_author(GebrGeoXmlDocument * document, const gchar * author)
{
	if (document == NULL || author == NULL)
		return;
	__gebr_geoxml_set_tag_value(gebr_geoxml_document_root_element(document),
		"author", author, __gebr_geoxml_create_TextNode);
}

void
gebr_geoxml_document_set_email(GebrGeoXmlDocument * document, const gchar * email)
{
	if (document == NULL || email == NULL)
		return;
	__gebr_geoxml_set_tag_value(gebr_geoxml_document_root_element(document),
		"email", email, __gebr_geoxml_create_TextNode);
}

GebrGeoXmlParameters *
gebr_geoxml_document_get_dict_parameters(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return (GebrGeoXmlParameters*)__gebr_geoxml_get_first_element(__gebr_geoxml_get_first_element(
		gebr_geoxml_document_root_element(document), "dict"), "parameters");
}

void
gebr_geoxml_document_set_date_created(GebrGeoXmlDocument * document, const gchar * created)
{
	if (document == NULL || created == NULL)
		return;
	__gebr_geoxml_set_tag_value(__gebr_geoxml_get_first_element(gebr_geoxml_document_root_element(document), "date"),
		"created", created, __gebr_geoxml_create_TextNode);
}

void
gebr_geoxml_document_set_date_modified(GebrGeoXmlDocument * document, const gchar * modified)
{
	if (document == NULL || modified == NULL)
		return;
	__gebr_geoxml_set_tag_value(__gebr_geoxml_get_first_element(gebr_geoxml_document_root_element(document), "date"),
		"modified", modified, __gebr_geoxml_create_TextNode);
}

void
gebr_geoxml_document_set_description(GebrGeoXmlDocument * document, const gchar * description)
{
	if (document == NULL || description == NULL)
		return;
	__gebr_geoxml_set_tag_value(gebr_geoxml_document_root_element(document),
		"description", description, __gebr_geoxml_create_TextNode);
}

void
gebr_geoxml_document_set_help(GebrGeoXmlDocument * document, const gchar * help)
{
	if (document == NULL || help == NULL)
		return;
	__gebr_geoxml_set_tag_value(gebr_geoxml_document_root_element(document),
		"help", help, __gebr_geoxml_create_CDATASection);
}

const gchar *
gebr_geoxml_document_get_filename(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value(gebr_geoxml_document_root_element(document), "filename");
}

const gchar *
gebr_geoxml_document_get_title(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value(gebr_geoxml_document_root_element(document), "title");
}

const gchar *
gebr_geoxml_document_get_author(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value(gebr_geoxml_document_root_element(document), "author");
}

const gchar *
gebr_geoxml_document_get_email(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value(gebr_geoxml_document_root_element(document), "email");
}

const gchar *
gebr_geoxml_document_get_date_created(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value(__gebr_geoxml_get_first_element(gebr_geoxml_document_root_element(document), "date"), "created");
}

const gchar *
gebr_geoxml_document_get_date_modified(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value(__gebr_geoxml_get_first_element(gebr_geoxml_document_root_element(document), "date"), "modified");
}

const gchar *
gebr_geoxml_document_get_description(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value(gebr_geoxml_document_root_element(document), "description");
}

const gchar *
gebr_geoxml_document_get_help(GebrGeoXmlDocument * document)
{
	if (document == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value(gebr_geoxml_document_root_element(document), "help");
}
