/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see
 *   <http://www.gnu.org/licenses/>.
 */

#ifndef __DOCUMENT_H
#define __DOCUMENT_H

#include <glib.h>
#include <libgebr/geoxml/geoxml.h>

G_BEGIN_DECLS

/**
 * Create a new document with _type_
 *
 * Create a new document in the user's data diretory
 * with _type_ and set its filename.
 */
GebrGeoXmlDocument *document_new(GebrGeoXmlDocumentType type);

/**
 * Return TRUE if \p path is inside GeBR's data dir.
 */
gboolean document_path_is_at_gebr_data_dir(const gchar *path);

/**
 * Return TRUE if document \p filename exists in GeBR's data dir.
 */
gboolean document_file_is_at_gebr_data_dir(const gchar *filename);

/**
 * Return TRUE if \p document's filename exists in GeBR's data dir.
 */
gboolean document_is_at_gebr_data_dir(GebrGeoXmlDocument * document);

/**
 * Load a document (flow, line or project) located at GêBR's data directory from its filename, handling errors.
 * Calls #document_load_with_parent.
 */
int document_load(GebrGeoXmlDocument ** document, const gchar * filename, gboolean cache);
/**
 * Load a document (flow, line or project) located at GêBR's data directory from its filename, handling errors.
 * Calls #document_load_path_with_parent.
 */
int document_load_with_parent(GebrGeoXmlDocument ** document, const gchar * filename, GtkTreeIter *parent, gboolean cache);
/**
 * Load a document (flow, line or project) located at \p diretory from its with \p filename, handling errors.
 * Calls #document_load_at_with_parent.
 */
int document_load_at(GebrGeoXmlDocument ** document, const gchar * filename, const gchar * directory);
/**
 * Load a document (flow, line or project) located at \p diretory from its with \p filename, handling errors.
 * Calls #document_load_path_with_parent.
 */
int document_load_at_with_parent(GebrGeoXmlDocument ** document, const gchar * filename, const gchar * directory,
				 GtkTreeIter *parent);
/**
 * Load a document from its path, handling errors.
 * Calls #document_load_path_with_parent.
 */
int document_load_path(GebrGeoXmlDocument **document, const gchar * path);

/**
 * document_load_path_with_parent:
 * @document: Return location for the document to be loaded.
 * @path: The document path in the file system.
 * @parent: ???
 * @cache: %TRUE to cache this document.
 *
 * If the return value is %GEBR_GEOXML_RETV_SUCCESS, loads the document in
 * @path and returns it in @document.
 *
 * If the return value is %GEBR_GEOXML_RETV_FILE_NOT_FOUND or
 * %GEBR_GEOXML_RETV_PERMISSION_DENIED, a log message is shown indicating the
 * error.
 *
 * If another error ocurred while loading the document, a dialog is presented
 * to the user asking him to choose one of:
 *  a) delete the erroneous document;
 *  b) export the file.
 *
 * In both cases, this method will return %GEBR_GEOXML_RETV_FILE_NOT_FOUND.
 *
 * This function will remove the reference of @path if @parent is not %NULL.
 * The callee must take care to the removal of reference pointer (eg.
 * GebrGeoXmlProjectLine) so that the next iterator pointer is obtained before
 * calling this function.
 *
 * If @parent is non-%NULL, it must point to the parent of @document (eg. a
 * line if @document is a flow). It is used to remove child's reference if
 * @document fails to load.
 *
 * Returns: The same errors codes of gebr_geoxml_document_load().
 */
int document_load_path_with_parent(GebrGeoXmlDocument **document,
				   const gchar         *path,
				   GtkTreeIter         *parent,
				   gboolean             cache);

/**
 * Save \p document at \p path.  * Only set \p set_modified_date to TRUE if this save is a reflect of a explicit user action.
 * Returns TRUE on document save success or FALSE otherwise
 */
gboolean document_save_at(GebrGeoXmlDocument * document, const gchar * path, gboolean set_modified_date, gboolean cache, gboolean compress);
/**
 * Save \p document using its filename field at data directory.
 * @see document_save_at
 * Returns TRUE on document save success or FALSE otherwise
 */
gboolean document_save(GebrGeoXmlDocument * document, gboolean set_modified_date, gboolean cache);

/**
 * Free document and remove it from cache.
 */
void document_free(GebrGeoXmlDocument * document);

/**
 * Import \p document into data directory, saving it with a new filename.
 */
void document_import(GebrGeoXmlDocument * document, gboolean save);

/**
 * Creates a filename for a document
 *
 * Creates a filename for a document using the current date and a random
 * generated string and _extension_, ensuring that it is unique in user's data directory.
 */
GString *document_assembly_filename(const gchar * extension);

/**
 * Prefix filename with data diretory path
 */
GString *document_get_path(const gchar * filename);

/**
 * Delete document with _filename_ from data directory.
 */
void document_delete(const gchar * filename);

/**
 * gebr_document_report_get_inner_body:
 * @report: an html markup
 *
 * Returns: a newly allocated string containing the inner
 * html of the body tag from @report.
 */
gchar * gebr_document_report_get_inner_body(const gchar * report);


/**
 * gebr_document_generate_report:
 * @document:
 *
 * Returns: a newly allocated string containing the header for document
 */
gchar * gebr_document_generate_header(GebrGeoXmlDocument * document,
                                      gboolean is_internal,
                                      const gchar *index);

/**
 * gebr_document_generate_report:
 * @document:
 *
 * Returns: a newly allocated string containing the detailed report for document based on the GêBR configuration
 * structure #gebr.config.
 */
gchar * gebr_document_generate_report (GebrGeoXmlDocument *document);

/**
 * gebr_document_get_css_header_field:
 * @filename: Name of the css file. It trusts that the file is ok.
 * @field: Name of the field to be checked (ex. "title", "e-mail")
 *
 * Gets a @field from the CSS comment header. A field is defined as a JavaDoc comment. For instance, the value of the
 * field "title" in the example below is <emphasis>Foobar</emphasis>:
 * <programlisting>
 * /<!-- -->**
 *  * @title: Foobar
 *  * @author: John
 *  *<!-- -->/
 * body {
 *   color: blue;
 * }
 * </programlisting>
 *
 * Returns: A newly allocated string containing the field value.
 */
gchar *gebr_document_get_css_header_field (const gchar *filename, const gchar *field);

G_END_DECLS

#endif				//__DOCUMENT_H
