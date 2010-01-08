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

GebrGeoXmlDocument *document_new(enum GEBR_GEOXML_DOCUMENT_TYPE type);

GebrGeoXmlDocument *document_load(const gchar * filename);
GebrGeoXmlDocument *document_load_at(const gchar * filename, const gchar * directory);
GebrGeoXmlDocument *document_load_path(const gchar * path);

void document_save(GebrGeoXmlDocument * document);
void document_import(GebrGeoXmlDocument * document);

GString *document_assembly_filename(const gchar * extension);

GString *document_get_path(const gchar * filename);

void document_delete(const gchar * filename);

#endif				//__DOCUMENT_H
