/*   GeBR - An environment for seismic processing.
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

#ifndef __UI_DOCUMENT_H
#define __UI_DOCUMENT_H

#include <gtk/gtk.h>

#include <libgebr/geoxml/geoxml.h>
#include "gebr-maestro-server.h"

G_BEGIN_DECLS

/**
 * GebrPropertiesResponseFunc:
 * @accept: %TRUE if the user clicked Ok on properties window.
 *
 * See document_properties_setup_ui().
 */
typedef void (*GebrPropertiesResponseFunc) (gboolean accept);

/**
 * document_get_current:
 * Returns: current selected and active project, line or flow.
 */
GebrGeoXmlDocument *document_get_current(void);

/**
 * document_properties_setup_ui:
 * @document: The document which will have its properties modified.
 * @func: A callback function which is called when the dialog exits.
 * @is_new: Set to %TRUE if you are creating a new document.
 *
 * Create the user interface for editing @document, which maybe a #GebrGeoXmlFlow, #GebrGeoXmlLine or
 * #GebrGeoXmlProject.
 */
void document_properties_setup_ui (GebrGeoXmlDocument * document,
				   GebrPropertiesResponseFunc func,
				   gboolean is_new);

/**
 * document_dict_edit_setup_ui:
 * Opens a dialog for editing dictionary keywords.
 */
void document_dict_edit_setup_ui(void);

/**
 * gebr_ui_document_set_properties_from_builder:
 *
 * Sets the @document properties based on @builder interface state.
 */
void gebr_ui_document_set_properties_from_builder(GebrGeoXmlDocument *document,
						  GtkBuilder *builder);

void gebr_ui_document_send_paths_messages(GebrGeoXmlDocument *document,
					  const gchar *old_title);

void gebr_ui_document_send_paths_to_maestro(GebrMaestroServer *maestro,
					    gint option,
					    const gchar *oldmsg,
					    const gchar *newmsg);

void validate_entry(GtkEntry *entry, gboolean error, const gchar *err_text, const gchar *clean_text);

void gebr_ui_document_remove_iter_and_update_complete(GebrGeoXmlDocument *flow);

void gebr_ui_document_add_iter_and_update_complete(GebrGeoXmlDocument *flow);


G_END_DECLS

#endif				//__UI_DOCUMENT_H
