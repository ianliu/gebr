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

#ifndef __UI_HELP_H
#define __UI_HELP_H

#include <gtk/gtk.h>
#include <libgebr/geoxml.h>

G_BEGIN_DECLS

/**
 * gebr_help_show_selected_program_help:
 * Show help for the currently selected program.
 */
void gebr_help_show_selected_program_help(void);

/**
 * gebr_help_show:
 * @object: an Xml object to get the help from.
 * @menu: %TRUE if we are showing a menu's help, %FALSE otherwise.
 * @title: the window title.
 *
 * Show help of \p object (program or flow) HTML. 
 */
void gebr_help_show(GebrGeoXmlObject * object, gboolean menu, const gchar * title);

/**
 * gebr_help_edit_document:
 */
void gebr_help_edit_document(GebrGeoXmlDocument * document);

/**
 * gebr_generate_report:
 * Returns: a newly allocated string containing the generated report.
 */
gchar * gebr_generate_report(const gchar * title, const gchar * styles, const gchar * header, const gchar * table);

/**
 * Set \p help on XML and enable/disable "View help" accordingly
 */
void gebr_help_set_on_xml(GebrGeoXmlDocument *document, const gchar *help);


G_END_DECLS

#endif				//__UI_HELP_H
