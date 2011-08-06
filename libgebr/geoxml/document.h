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

#include "gebr-geo-types.h"
#include <gebr-validator.h>

G_BEGIN_DECLS

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
 * Load a document XML file at \p path into \p document.
 * The document is validated using the proper DTD. Invalid documents are not loaded.
 * The filename is set according to \p path (see #gebr_geoxml_document_set_filename).
 *
 * Returns one of: GEBR_GEOXML_RETV_SUCCESS, GEBR_GEOXML_RETV_NO_MEMORY,
 * GEBR_GEOXML_RETV_FILE_NOT_FOUND, GEBR_GEOXML_RETV_PERMISSION_DENIED, 
 * GEBR_GEOXML_RETV_INVALID_DOCUMENT, GEBR_GEOXML_RETV_DTD_SPECIFIED, GEBR_GEOXML_RETV_CANT_ACCESS_DTD
 */
int gebr_geoxml_document_load(GebrGeoXmlDocument ** document,
			      const gchar * path,
			      gboolean validate,
			      GebrGeoXmlDiscardMenuRefCallback discard_menu_ref);

/**
 * Load a document XML buffer at \p xml into \p document.
 * The document is validated using the proper DTD. Invalid documents are not loaded.
 *
 * Returns one of: GEBR_GEOXML_RETV_SUCCESS, GEBR_GEOXML_RETV_NO_MEMORY,
 * GEBR_GEOXML_RETV_FILE_NOT_FOUND, GEBR_GEOXML_RETV_PERMISSION_DENIED, 
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
GebrGeoXmlDocumentType gebr_geoxml_document_get_type(GebrGeoXmlDocument * document);

/**
 * Returns the version string of \p document.
 *
 * Note that it can vary between flows, lines and projects, as not all
 * change version between libgeoxml versions.
 *
 * If \p document is NULL nothing is done.
 */
gchar *gebr_geoxml_document_get_version(GebrGeoXmlDocument * document);

/**
 * Validate the document specified in \p filename.
 *
 * Returns one of: GEBR_GEOXML_RETV_SUCCESS, GEBR_GEOXML_RETV_NO_MEMORY,
 * GEBR_GEOXML_RETV_FILE_NOT_FOUND, GEBR_GEOXML_RETV_PERMISSION_DENIED, 
 * GEBR_GEOXML_RETV_INVALID_DOCUMENT, GEBR_GEOXML_RETV_DTD_SPECIFIED, GEBR_GEOXML_RETV_CANT_ACCESS_DTD
 *
 * If \p filename is NULL, GEBR_GEOXML_RETV_FILE_NOT_FOUND is returned.
 */
int gebr_geoxml_document_validate(const gchar * filename);

/**
 * Save \p document to \p path.
 * The filename is set according to \p path (see #gebr_geoxml_document_set_filename).
 *
 * Returns one of: GEBR_GEOXML_RETV_SUCCESS, GEBR_GEOXML_RETV_PERMISSION_DENIED,
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
 * Set the filename of \p document to \p filename which is its correct name in the filesystem.
 * This information is not stored on the file. It is set according to the path from
 * #gebr_geoxml_document_load and #gebr_geoxml_document_save.
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
 * gebr_geoxml_document_get_dict_parameters:
 * @document: a #GebrGeoXmlDocument
 *
 * Retrieves from @document the parameters dictionary for use with program's parameters.
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
 *
 * If \p document is NULL nothing is done.
 *
 * \see gebr_geoxml_document_get_help
 */
void gebr_geoxml_document_set_help(GebrGeoXmlDocument * document, const gchar * help);

/**
 * Get the filename of \p document as it should correctly
 * be named in the filesystem.
 * This information is not stored on the file. It is set according to the path from
 * #gebr_geoxml_document_load and #gebr_geoxml_document_save.
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
gchar *gebr_geoxml_document_get_help(GebrGeoXmlDocument * document);

/**
 * gebr_geoxml_document_merge_dicts:
 * @validator: Uses validator to omit parameters with error,
 *       if NULL is passed all parameters are merged.
 * @first: The #GebrGeoXmlDocument that will contain all dictionary parameters.
 * @...: A %NULL-terminated list of #GebrGeoXmlDocument that will be merged
 *       into @first.
 *
 * Merges all dictionaries into @first separating them with a special
 * parameter. After applying this function, no one should use @first before
 * calling gebr_geoxml_document_split_dict().
 */
void gebr_geoxml_document_merge_dicts(GebrValidator *validator, GebrGeoXmlDocument *first,
				      ...) G_GNUC_NULL_TERMINATED;

/**
 * gebr_geoxml_document_split_dict:
 * @first: The #GebrGeoXmlDocument containing dictionary parameters (or not).
 * @...: A %NULL-terminated list of #GebrGeoXmlDocument that will be filled
 *       with @first's dictionary parameters.
 *
 * If @first has dictionary parameters separated by the function
 * gebr_geoxml_document_merge_dicts(), make sure the list @... have enougth
 * documents to hold them. If that is not the case, the function returns
 * %FALSE.
 *
 * Returns: %TRUE if the document list was large enougth to hold @first's
 * dictionary parameters, %FALSE otherwise.
 */
gboolean gebr_geoxml_document_split_dict(GebrGeoXmlDocument *first,
					 ...) G_GNUC_NULL_TERMINATED;

/**
 * gebr_geoxml_document_set_dict_keyword:
 * @doc: A #GebrGeoXmlDocument
 * @type: The type of the variable
 * @keyword: The name of the variable
 * @value: The value of the variable
 *
 * Creates a new variable in the dictionary variables buffer.
 *
 * Returns: The dictionary variable #GebrGeoXmlParameter.
 */
GebrGeoXmlParameter *
gebr_geoxml_document_set_dict_keyword(GebrGeoXmlDocument *doc,
				      GebrGeoXmlParameterType type,
				      const gchar *keyword,
				      const gchar *value);

/**
 * gebr_geoxml_document_get_dict_parameter:
 * @document:
 *
 * Returns:
 */
GebrGeoXmlSequence *
gebr_geoxml_document_get_dict_parameter(GebrGeoXmlDocument *doc);

/**
 * gebr_geoxml_document_set_dtd_dir:
 */
void gebr_geoxml_document_set_dtd_dir(const gchar *path);

/**
 * gebr_geoxml_document_canonize_dict_parameters:
 * @document: Document (flow/line/project) that will have its parameters canonized.
 * @list_copy: A hash table pointer to access the keywords -> canonized list.
 *
 * This function canonizes a document dictionary, changing invalid variable
 * names to a valid form.
 *
 * The list to variable names/values is freed every time you canonize a new
 * project.
 *
 * This fuction also converts the variable type to the current supported types
 * (e.g. int to float)
 *
 * Ex: "CDP EM METROS (M)" becomes "cdp_em_metros_m_".
 *
 * Important: You should not free the pointer list_copy.
 *
 * Returns: TRUE if everything when fine, FALSE otherwise.
 */
gboolean
gebr_geoxml_document_canonize_dict_parameters(GebrGeoXmlDocument * document,
					      GHashTable 	** vars_list);

G_END_DECLS

#endif				//__GEBR_GEOXML_DOCUMENT_H
