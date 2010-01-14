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

#ifndef __GEBR_GEOXML_DOCUMENT_H
#define __GEBR_GEOXML_DOCUMENT_H

#include <glib.h>

/**
 * \struct GebrGeoXmlDocument document.h geoxml/document.h
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
 * 	"GebrGeoXmlObject" [ URL = "\ref object.h" ];
 * 	"GebrGeoXmlDocument" [ URL = "\ref document.h" ];
 * 	"GebrGeoXmlFlow" [ URL = "\ref flow.h" ];
 * 	"GebrGeoXmlLine" [ URL = "\ref line.h" ];
 * 	"GebrGeoXmlProject" [ URL = "\ref project.h" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GebrGeoXmlObject" -> "GebrGeoXmlDocument"
 * 	"GebrGeoXmlDocument" -> { "GebrGeoXmlFlow" "GebrGeoXmlLine" "GebrGeoXmlProject" };
 * }
 * \enddot
 * \see document.h
 */

/**
 * \file document.h
 * An abstraction to common data and functions found in flows, lines and projects.
 * The data is things like author, help stuff, filename, etc. See methods below for more info.
 *
 * GebrGeoXmlFlow, GebrGeoXmlLine and GebrGeoXmlProject XML files can all be loaded using gebr_geoxml_document_load.
 * Validation is done automatically by matching document's DTD, located in /usr/share/libgeoxml (specifically at ${datarootdir}/libgeoxml, run '--configure --help' for more information).
 * The same occurs with all methods in this class: they are valid for all GebrGeoXmlDocument's derived classes. Use GEBR_GEOXML_DOC to cast.
 */

/**
 * Cast to GebrGeoXmlDocument's from its derived classes: GebrGeoXmlFlow, GebrGeoXmlLine and GebrGeoXmlProject
 */
#define GEBR_GEOXML_DOCUMENT(x) ((GebrGeoXmlDocument*)(x))
#define GEBR_GEOXML_DOC(x) GEBR_GEOXML_DOCUMENT(x)

/**
 * The GebrGeoXmlDocument struct contains private data only, and should be accessed using the functions below.
 */
typedef struct gebr_geoxml_document GebrGeoXmlDocument;

#include "parameters.h"

/**
 * Document type: flow, line or project
 *
 */
enum GEBR_GEOXML_DOCUMENT_TYPE {
	/**
	 * The document is a GebrGeoXmlFlow
	 */
	GEBR_GEOXML_DOCUMENT_TYPE_FLOW,
	/**
	 * The document is a GebrGeoXmlLine
	 */
	GEBR_GEOXML_DOCUMENT_TYPE_LINE,
	/**
	 * The document is a GebrGeoXmlProject
	 */
	GEBR_GEOXML_DOCUMENT_TYPE_PROJECT,
};

#include "program.h"
/**
 * Used by \ref gebr_geoxml_document_load 
 */
typedef void (*GebrGeoXmlDiscardMenuRefCallback)(GebrGeoXmlProgram * program, const gchar * menu, gint index);

/**
 * Load a document XML file at \p path into \p document.
 * The document is validated using the proper DTD. Invalid documents are not loaded.
 *
 * Returns one of: GEBR_GEOXML_RETV_SUCCESS, GEBR_GEOXML_RETV_NO_MEMORY, GEBR_GEOXML_RETV_CANT_ACCESS_FILE,
 * GEBR_GEOXML_RETV_INVALID_DOCUMENT, GEBR_GEOXML_RETV_DTD_SPECIFIED, GEBR_GEOXML_RETV_CANT_ACCESS_DTD
 */
int gebr_geoxml_document_load(GebrGeoXmlDocument ** document, const gchar * path, GebrGeoXmlDiscardMenuRefCallback discard_menu_ref);

/**
 * Load a document XML buffer at \p xml into \p document.
 * The document is validated using the proper DTD. Invalid documents are not loaded.
 *
 * Returns one of: GEBR_GEOXML_RETV_SUCCESS, GEBR_GEOXML_RETV_NO_MEMORY, GEBR_GEOXML_RETV_CANT_ACCESS_FILE,
 * GEBR_GEOXML_RETV_INVALID_DOCUMENT, GEBR_GEOXML_RETV_DTD_SPECIFIED, GEBR_GEOXML_RETV_CANT_ACCESS_DTD
 */
int gebr_geoxml_document_load_buffer(GebrGeoXmlDocument ** document, const gchar * xml);

/**
 * Free the memory used by the GebrGeoXmlDocument's XML structure.
 *
 * If \p document is NULL nothing is done.
 */
void gebr_geoxml_document_free(GebrGeoXmlDocument * document);

/**
 * Create an identical copy of \p source and returns it.
 *
 * If \p source is NULL nothing is done.
 */
GebrGeoXmlDocument *gebr_geoxml_document_clone(GebrGeoXmlDocument * source);

/**
 * Return the type of \p document
 *
 * If \p document is NULL, GEBR_GEOXML_DOCUMENT_TYPE_FLOW is returned
 *
 * \see GEBR_GEOXML_DOCUMENT_TYPE
 */
enum GEBR_GEOXML_DOCUMENT_TYPE gebr_geoxml_document_get_type(GebrGeoXmlDocument * document);

/**
 * Returns the version string of \p document.
 *
 * Note that it can vary between flows, lines and projects, as not all
 * change version between libgeoxml versions.
 *
 * If \p document is NULL nothing is done.
 */
const gchar *gebr_geoxml_document_get_version(GebrGeoXmlDocument * document);

/**
 * Validate the document specified in \p filename.
 *
 * Returns one of: GEBR_GEOXML_RETV_SUCCESS, GEBR_GEOXML_RETV_NO_MEMORY, GEBR_GEOXML_RETV_CANT_ACCESS_FILE,
 * GEBR_GEOXML_RETV_INVALID_DOCUMENT, GEBR_GEOXML_RETV_DTD_SPECIFIED, GEBR_GEOXML_RETV_CANT_ACCESS_DTD
 *
 * If \p filename is NULL, GEBR_GEOXML_RETV_CANT_ACCESS_FILE is returned.
 */
int gebr_geoxml_document_validate(const gchar * filename);

/**
 * Save \p document to \p path.
 *
 * Returns one of: GEBR_GEOXML_RETV_SUCCESS, GEBR_GEOXML_RETV_CANT_ACCESS_FILE,
 *
 * If \p document is NULL nothing is done.
 */
int gebr_geoxml_document_save(GebrGeoXmlDocument * document, const gchar * path);

/**
 * Save \p document to \p xml_string. Memory needed for \p xml_string
 * is allocated. Therefore, you should free it at the approtiate time.
 *
 * If \p document is NULL nothing is done.
 */
int gebr_geoxml_document_to_string(GebrGeoXmlDocument * document, gchar ** xml_string);

/**
 * Set the filename of \p document to \p filename
 * which is its correct name in the filesystem
 *
 * If \p document is NULL nothing is done.
 *
 * \see gebr_geoxml_document_get_filename
 */
void gebr_geoxml_document_set_filename(GebrGeoXmlDocument * document, const gchar * filename);

/**
 * Change the \p document 's title to \p title
 *
 * If \p document is NULL nothing is done.
 *
 * \see gebr_geoxml_document_get_title
 */
void gebr_geoxml_document_set_title(GebrGeoXmlDocument * document, const gchar * title);

/**
 * Change the \p document 's author to \p author
 *
 * If \p document is NULL nothing is done.
 *
 * \see gebr_geoxml_document_get_author
 */
void gebr_geoxml_document_set_author(GebrGeoXmlDocument * document, const gchar * author);

/**
 * Change the \p document 's author email to \p email
 *
 * If \p document is NULL nothing is done.
 *
 * \see gebr_geoxml_document_get_email
 */
void gebr_geoxml_document_set_email(GebrGeoXmlDocument * document, const gchar * email);

/**
 * Retrieves from \p document. the parameters dictionary for use with program's parameters.
 *
 * If \p document is NULL returns NULL.
 */
GebrGeoXmlParameters *gebr_geoxml_document_get_dict_parameters(GebrGeoXmlDocument * document);

/**
 * Change the \p document 's creation date to \p created
 *
 * If \p document is NULL nothing is done.
 *
 * \see gebr_geoxml_document_get_date_created
 */
void gebr_geoxml_document_set_date_created(GebrGeoXmlDocument * document, const gchar * created);

/**
 * Change the \p document 's modified date to \p created
 *
 * If \p document is NULL nothing is done.
 *
 * \see gebr_geoxml_document_get_date_modified
 */
void gebr_geoxml_document_set_date_modified(GebrGeoXmlDocument * document, const gchar * created);

/**
 * Set a brief description of the document, usually an one line text.
 * Only plain text is accepted. See 'gebr_geoxml_document_set_help' and 'gebr_geoxml_document_get_help'
 *
 * If \p document is NULL nothing is done.
 *
 * \see gebr_geoxml_document_get_description
 */
void gebr_geoxml_document_set_description(GebrGeoXmlDocument * document, const gchar * description);

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
 * \see gebr_geoxml_document_get_help
 */
void gebr_geoxml_document_set_help(GebrGeoXmlDocument * document, const gchar * help);

/**
 * Get the filename of \p document as it should correctly
 * be named in the filesystem.
 *
 * If \p document is NULL returns NULL.
 *
 * \see gebr_geoxml_document_set_filename
 */
const gchar *gebr_geoxml_document_get_filename(GebrGeoXmlDocument * document);

/**
 * Get a brief title summarizing \p document.
 *
 * If \p document is NULL returns NULL.
 *
 * \see gebr_geoxml_document_set_title
 */
const gchar *gebr_geoxml_document_get_title(GebrGeoXmlDocument * document);

/**
 * Get the \p document 's author name.
 *
 * If \p document is NULL returns NULL.
 *
 * \see gebr_geoxml_document_set_author
 */
const gchar *gebr_geoxml_document_get_author(GebrGeoXmlDocument * document);

/**
 * Get the \p document 's author email
 *
 * If \p document is NULL returns NULL.
 *
 * \see gebr_geoxml_document_set_email
 */
const gchar *gebr_geoxml_document_get_email(GebrGeoXmlDocument * document);

/**
 * Get the \p document 'c creation date
 *
 * If \p document is NULL returns NULL.
 *
 * \see gebr_geoxml_document_set_date_created
 */
const gchar *gebr_geoxml_document_get_date_created(GebrGeoXmlDocument * document);

/**
 * Get the \p document 's last modification date
 *
 * If \p document is NULL returns NULL.
 *
 * \see gebr_geoxml_document_set_date_modified
 */
const gchar *gebr_geoxml_document_get_date_modified(GebrGeoXmlDocument * document);

/**
 * Get a brief description of the document, usually an one line text.
 * No html is allowed here. See 'gebr_geoxml_document_set_help' and 'gebr_geoxml_document_get_help'
 *
 * If \p document is NULL returns NULL.
 *
 * \see gebr_geoxml_document_set_description
 */
const gchar *gebr_geoxml_document_get_description(GebrGeoXmlDocument * document);

/**
 * Returns the help associated with this document.
 * It can be a plain text or a HTML format.
 * This enables rich text documentation of the document with
 * all the functionallity of html,
 * like images, tables, fonts, etc.
 *
 * If \p document is NULL returns NULL.
 *
 * \see gebr_geoxml_document_set_help
 */
const gchar *gebr_geoxml_document_get_help(GebrGeoXmlDocument * document);

#endif				//__GEBR_GEOXML_DOCUMENT_H
