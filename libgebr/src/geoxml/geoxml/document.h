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

#ifndef __LIBGEBR_GEOXML_DOCUMENT_H
#define __LIBGEBR_GEOXML_DOCUMENT_H

#include <glib.h>

/**
 * \struct GeoXmlDocument document.h geoxml/document.h
 * \brief
 * An abstraction to common data and functions found in flows, lines and projects.
 * \dot
 * digraph document {
 * 	fontname = "Bitstream Vera Sans"
 * 	fontsize = 9
 * 	size = "6"
 * 	node [
 * 		color = palegreen2, style = filled
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 9
 * 		shape = record
 * 	]
 * 	edge [
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 9
 * 	]
 *
 * 	"GeoXmlObject" [ URL = "\ref object.h" ];
 * 	"GeoXmlDocument" [ URL = "\ref document.h" ];
 * 	"GeoXmlFlow" [ URL = "\ref flow.h" ];
 * 	"GeoXmlLine" [ URL = "\ref line.h" ];
 * 	"GeoXmlProject" [ URL = "\ref project.h" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GeoXmlObject" -> "GeoXmlDocument"
 * 	"GeoXmlDocument" -> { "GeoXmlFlow" "GeoXmlLine" "GeoXmlProject" };
 * }
 * \enddot
 * \see document.h
 */

/**
 * \file document.h
 * An abstraction to common data and functions found in flows, lines and projects.
 * The data is things like author, help stuff, filename, etc. See methods below for more info.
 *
 * GeoXmlFlow, GeoXmlLine and GeoXmlProject XML files can all be loaded using geoxml_document_load.
 * Validation is done automatically by matching document's DTD, located in /usr/share/libgeoxml (specifically at ${datarootdir}/libgeoxml, run '--configure --help' for more information).
 * The same occurs with all methods in this class: they are valid for all GeoXmlDocument's derived classes. Use GEOXML_DOC to cast.
 */

/**
 * Cast to GeoXmlDocument's from its derived classes: GeoXmlFlow, GeoXmlLine and GeoXmlProject
 */
#define GEOXML_DOCUMENT(x) ((GeoXmlDocument*)(x))
#define GEOXML_DOC(x) GEOXML_DOCUMENT(x)

/**
 * The GeoXmlDocument struct contains private data only, and should be accessed using the functions below.
 */
typedef struct geoxml_document GeoXmlDocument;

#include "parameters.h"

/**
 * Document type: flow, line or project
 *
 */
enum GEOXML_DOCUMENT_TYPE {
	/**
	 * The document is a GeoXmlFlow
	 */
	GEOXML_DOCUMENT_TYPE_FLOW,
	/**
	 * The document is a GeoXmlLine
	 */
	GEOXML_DOCUMENT_TYPE_LINE,
	/**
	 * The document is a GeoXmlProject
	 */
	GEOXML_DOCUMENT_TYPE_PROJECT,
};

/**
 * Load a document XML file at \p path into \p document.
 * The document is validated using the proper DTD. Invalid documents are not loaded.
 *
 * Returns one of: GEOXML_RETV_SUCCESS, GEOXML_RETV_NO_MEMORY, GEOXML_RETV_CANT_ACCESS_FILE,
 * GEOXML_RETV_INVALID_DOCUMENT, GEOXML_RETV_DTD_SPECIFIED, GEOXML_RETV_CANT_ACCESS_DTD
 */
int
geoxml_document_load(GeoXmlDocument ** document, const gchar * path);

/**
 * Load a document XML buffer at \p xml into \p document.
 * The document is validated using the proper DTD. Invalid documents are not loaded.
 *
 * Returns one of: GEOXML_RETV_SUCCESS, GEOXML_RETV_NO_MEMORY, GEOXML_RETV_CANT_ACCESS_FILE,
 * GEOXML_RETV_INVALID_DOCUMENT, GEOXML_RETV_DTD_SPECIFIED, GEOXML_RETV_CANT_ACCESS_DTD
 */
int
geoxml_document_load_buffer(GeoXmlDocument ** document, const gchar * xml);

/**
 * Free the memory used by the GeoXmlDocument's XML structure.
 *
 * If \p document is NULL nothing is done.
 */
void
geoxml_document_free(GeoXmlDocument * document);

/**
 * Create an identical copy of \p source and returns it.
 *
 * If \p source is NULL nothing is done.
 */
GeoXmlDocument *
geoxml_document_clone(GeoXmlDocument * source);

/**
 * Return the type of \p document
 *
 * If \p document is NULL, GEOXML_DOCUMENT_TYPE_FLOW is returned
 *
 * \see GEOXML_DOCUMENT_TYPE
 */
enum GEOXML_DOCUMENT_TYPE
geoxml_document_get_type(GeoXmlDocument * document);

/**
 * Returns the version string of \p document.
 *
 * Note that it can vary between flows, lines and projects, as not all
 * change version between libgeoxml versions.
 *
 * If \p document is NULL nothing is done.
 */
const gchar *
geoxml_document_get_version(GeoXmlDocument * document);

/**
 * Validate the document specified in \p filename.
 *
 * Returns one of: GEOXML_RETV_SUCCESS, GEOXML_RETV_NO_MEMORY, GEOXML_RETV_CANT_ACCESS_FILE,
 * GEOXML_RETV_INVALID_DOCUMENT, GEOXML_RETV_DTD_SPECIFIED, GEOXML_RETV_CANT_ACCESS_DTD
 *
 * If \p filename is NULL, GEOXML_RETV_CANT_ACCESS_FILE is returned.
 */
int
geoxml_document_validate(const gchar * filename);

/**
 * Save \p document to \p path.
 *
 * Returns one of: GEOXML_RETV_SUCCESS, GEOXML_RETV_CANT_ACCESS_FILE,
 *
 * If \p document is NULL nothing is done.
 */
int
geoxml_document_save(GeoXmlDocument * document, const gchar * path);

/**
 * Save \p document to \p xml_string. Memory needed for \p xml_string
 * is allocated. Therefore, you should free it at the approtiate time.
 *
 * If \p document is NULL nothing is done.
 */
int
geoxml_document_to_string(GeoXmlDocument * document, gchar ** xml_string);

/**
 * Set the filename of \p document to \p filename
 * which is its correct name in the filesystem
 *
 * If \p document is NULL nothing is done.
 *
 * \see geoxml_document_get_filename
 */
void
geoxml_document_set_filename(GeoXmlDocument * document, const gchar * filename);

/**
 * Change the \p document 's title to \p title
 *
 * If \p document is NULL nothing is done.
 *
 * \see geoxml_document_get_title
 */
void
geoxml_document_set_title(GeoXmlDocument * document, const gchar * title);

/**
 * Change the \p document 's author to \p author
 *
 * If \p document is NULL nothing is done.
 *
 * \see geoxml_document_get_author
 */
void
geoxml_document_set_author(GeoXmlDocument * document, const gchar * author);

/**
 * Change the \p document 's author email to \p email
 *
 * If \p document is NULL nothing is done.
 *
 * \see geoxml_document_get_email
 */
void
geoxml_document_set_email(GeoXmlDocument * document, const gchar * email);

/**
 * Retrieves from \p document. the parameters dictionary for use with program's parameters.
 *
 * If \p document is NULL returns NULL.
 */
GeoXmlParameters *
geoxml_document_get_dict_parameters(GeoXmlDocument * document);

/**
 * Change the \p document 's creation date to \p created
 *
 * If \p document is NULL nothing is done.
 *
 * \see geoxml_document_get_date_created
 */
void
geoxml_document_set_date_created(GeoXmlDocument * document, const gchar * created);

/**
 * Change the \p document 's modified date to \p created
 *
 * If \p document is NULL nothing is done.
 *
 * \see geoxml_document_get_date_modified
 */
void
geoxml_document_set_date_modified(GeoXmlDocument * document, const gchar * created);

/**
 * Set a brief description of the document, usually an one line text.
 * Only plain text is accepted. See 'geoxml_document_set_help' and 'geoxml_document_get_help'
 *
 * If \p document is NULL nothing is done.
 *
 * \see geoxml_document_get_description
 */
void
geoxml_document_set_description(GeoXmlDocument * document, const gchar * description);

/**
 * Set a help associated with \p help equal to \p help.
 * It can be both a plain text or an HTML text.
 * This enables rich text documentation of the document with
 * all the functionallity of html,
 * like images, tables, fonts, etc.
 * PS: Because of implementation HTML text in \p help
 * cannot contain the string ']]>', as it is stored in a
 * CDATA Section. If those strings are found they are
 * going to be replaced by ']] >'.
 *
 * If \p document is NULL nothing is done.
 *
 * \see geoxml_document_get_help
 */
void
geoxml_document_set_help(GeoXmlDocument * document, const gchar * help);

/**
 * Get the filename of \p document as it should correctly
 * be named in the filesystem.
 *
 * If \p document is NULL returns NULL.
 *
 * \see geoxml_document_set_filename
 */
const gchar *
geoxml_document_get_filename(GeoXmlDocument * document);

/**
 * Get a brief title summarizing \p document.
 *
 * If \p document is NULL returns NULL.
 *
 * \see geoxml_document_set_title
 */
const gchar *
geoxml_document_get_title(GeoXmlDocument * document);

/**
 * Get the \p document 's author name.
 *
 * If \p document is NULL returns NULL.
 *
 * \see geoxml_document_set_author
 */
const gchar *
geoxml_document_get_author(GeoXmlDocument * document);

/**
 * Get the \p document 's author email
 *
 * If \p document is NULL returns NULL.
 *
 * \see geoxml_document_set_email
 */
const gchar *
geoxml_document_get_email(GeoXmlDocument * document);

/**
 * Get the \p document 'c creation date
 *
 * If \p document is NULL returns NULL.
 *
 * \see geoxml_document_set_date_created
 */
const gchar *
geoxml_document_get_date_created(GeoXmlDocument * document);

/**
 * Get the \p document 's last modification date
 *
 * If \p document is NULL returns NULL.
 *
 * \see geoxml_document_set_date_modified
 */
const gchar *
geoxml_document_get_date_modified(GeoXmlDocument * document);

/**
 * Get a brief description of the document, usually an one line text.
 * No html is allowed here. See 'geoxml_document_set_help' and 'geoxml_document_get_help'
 *
 * If \p document is NULL returns NULL.
 *
 * \see geoxml_document_set_description
 */
const gchar *
geoxml_document_get_description(GeoXmlDocument * document);

/**
 * Returns the help associated with this document.
 * It can be a plain text or a HTML format.
 * This enables rich text documentation of the document with
 * all the functionallity of html,
 * like images, tables, fonts, etc.
 *
 * If \p document is NULL returns NULL.
 *
 * \see geoxml_document_set_help
 */
const gchar *
geoxml_document_get_help(GeoXmlDocument * document);

#endif //__LIBGEBR_GEOXML_DOCUMENT_H
