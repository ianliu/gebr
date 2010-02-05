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

#include <glib.h>
#include <geoxml.h>

/** 
 * Show HTML at \p uri with WebKit (if enabled) or with \p browser executable specified
 */
void gebr_gui_help_show(const gchar * uri, const gchar * title, const gchar * browser);

typedef void (*GebrGuiHelpEdited)(GebrGeoXmlObject * object, const gchar * help);

/**
 * Edit help HTML from \p document with WebKit and CKEDITOR (if enabled) or with \p editor executable specified.
 * \p edited_callback is called each time the content is edited. 
 */
void gebr_gui_help_edit(GebrGeoXmlDocument * document, const gchar * editor, GebrGuiHelpEdited edited_callback);

/**
 * Edit help HTML from \p program with WebKit and CKEDITOR (if enabled) or with \p editor executable specified.
 * \p edited_callback is called each time the content is edited. 
 */
void gebr_gui_program_help_edit(GebrGeoXmlProgram * program, const gchar * editor, GebrGuiHelpEdited edited_callback);


#endif				//__GEBR_GUI_HELP_H
