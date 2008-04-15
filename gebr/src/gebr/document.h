/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2008 GeBR core team (http://gebr.sourceforge.net)
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
#include <geoxml.h>

GeoXmlDocument *
document_new(enum GEOXML_DOCUMENT_TYPE type);

GeoXmlDocument *
document_load(const gchar * filename);

GeoXmlDocument *
document_load_path(const gchar * path);

void
document_save(GeoXmlDocument * document);

GString *
document_assembly_filename(const gchar * extension);

GString *
document_get_path(const gchar * filename);

void
document_delete(const gchar * filename);

#endif //__DOCUMENT_H
