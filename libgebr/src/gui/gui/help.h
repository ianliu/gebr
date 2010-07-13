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

#ifndef __GEBR_GUI_HELP_H
#define __GEBR_GUI_HELP_H

/**
 * \file help.h API for displaying help's
 */

#include <gtk/gtk.h>
#include <geoxml.h>

G_BEGIN_DECLS

/** 
 * Show HTML \p help within a dialog with \p title in its title bar.
 * \p object is used for help navition within the xml if \p menu is TRUE.
 */
void gebr_gui_help_show(GebrGeoXmlObject * object, gboolean menu, const gchar *help, const gchar * title);

/**
 * Callback for when 'Refresh' button is pressed.
 * When the refresh button is pressed, this callback is called so \p help is altered. When this callback returns, the
 * help content is updated with the modified \p help.
 */
typedef void (*GebrGuiHelpRefresh)(GString * help, GebrGeoXmlObject * object);

/**
 * Callback for \ref gebr_gui_help_edit and \ref gebr_gui_program_help_edit; it is called whenever the help is saved.
 */
typedef void (*GebrGuiHelpEdited)(GebrGeoXmlObject * object, const gchar * help);

/**
 * Edit help HTML from \p document with WebKit and CKEDITOR (if enabled) or with \p editor executable specified.
 * If \p menu_edition is TRUE then enabled menu specific features edition (for DéBR) \p edited_callback is called each
 * time the content is edited. 
 */
GtkWidget* gebr_gui_help_edit(GebrGeoXmlDocument * document, GString * tmpl, GebrGuiHelpEdited edited_callback,
			      GebrGuiHelpRefresh refresh_callback, gboolean menu_edition);

/**
 * Edit help HTML from \p program with WebKit and CKEDITOR (if enabled) or with \p editor executable specified.
 * \p edited_callback is called each time the content is edited. 
 */
GtkWidget* gebr_gui_program_help_edit(GebrGeoXmlProgram * program, GString * tmpl, GebrGuiHelpEdited edited_callback,
				      GebrGuiHelpRefresh refresh_callback);

G_END_DECLS

#endif				//__GEBR_GUI_HELP_H
