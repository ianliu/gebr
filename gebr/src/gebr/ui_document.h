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

#include <libgebr/geoxml.h>

G_BEGIN_DECLS

GebrGeoXmlDocument *document_get_current(void);

gboolean document_properties_setup_ui(GebrGeoXmlDocument * document);

void document_dict_edit_setup_ui(void);

G_END_DECLS
#endif				//__UI_DOCUMENT_H
