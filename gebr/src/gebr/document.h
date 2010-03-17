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
#include <libgebr/geoxml.h>

/**
 * Create a new document with _type_
 *
 * Create a new document in the user's data diretory
 * with _type_ and set its filename.
 */
GebrGeoXmlDocument *document_new(enum GEBR_GEOXML_DOCUMENT_TYPE type);

/**
 * Load a document (flow, line or project) located at GêBR's data directory from its filename, handling errors.
 * Calls #document_load_with_parent.
 */
int document_load(GebrGeoXmlDocument ** document, const gchar * filename);
/**
 * Load a document (flow, line or project) located at GêBR's data directory from its filename, handling errors.
 * Calls #document_load_path_with_parent.
 */
int document_load_with_parent(GebrGeoXmlDocument ** document, const gchar * filename, GtkTreeIter *parent);
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
 * Load a document from its path, handling errors.
 * Return the errors codes of #gebr_geoxml_document_load.
 * If load error is GEBR_GEOXML_RETV_CANT_ACCESS_FILE no action is requested from user and a log message is shown.
 * Otherwise, a dialog is presented to the user. If the user choose to delete or export file,
 * GEBR_GEOXML_RETV_CANT_ACCESS_FILE is returned, and the callee of this function is responsible for
 * removing references to \p path.
 *
 * If \p parent is non-NULL, it must point to the parent of \p document (eg. a line if \p document is a flow).
 * It is used to remove child's reference if \p document fails to load. 
 */
int document_load_path_with_parent(GebrGeoXmlDocument **document, const gchar * path, GtkTreeIter *parent);

/**
 * Save \p document at \p path.
 * Only set \p set_modified_date to TRUE if this save is a reflect of a explicit user action.
 * Returns TRUE on document save success or FALSE otherwise
 */
gboolean document_save_at(GebrGeoXmlDocument * document, const gchar * path, gboolean set_modified_date);
/**
 * Save \p document using its filename field at data directory.
 * @see document_save_at
 * Returns TRUE on document save success or FALSE otherwise
 */
gboolean document_save(GebrGeoXmlDocument * document, gboolean set_modified_date);

/**
 * Import \p document into data directory, saving it with a new filename.
 */
void document_import(GebrGeoXmlDocument * document);

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

#endif				//__DOCUMENT_H
