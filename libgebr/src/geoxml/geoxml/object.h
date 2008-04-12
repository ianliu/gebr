/*   libgebr - GeBR Library
 *   Copyright (C) 2007 GeBR core team (http://gebr.sourceforge.net)
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

#ifndef __LIBGEBR_GEOXML_OBJECT_H
#define __LIBGEBR_GEOXML_OBJECT_H

#include <glib.h>

/**
 * \struct GeoXmlObject object.h geoxml/object.h
 * \brief
 * The base class for all GeoXml.
 * \dot
 * digraph enum_option {
 * 	fontlabel = "Bitstream Vera Sans"
 * 	fontsize = 9
 * 	size = "6"
 * 	node [
 * 		color = palegreen2, style = filled
 * 		fontlabel = "Bitstream Vera Sans"
 * 		fontsize = 9
 * 		shape = record
 * 	]
 * 	edge [
 * 		fontlabel = "Bitstream Vera Sans"
 * 		fontsize = 9
 * 	]
 *
 * 	"GeoXmlObject" [ URL = "\ref object.h" ];
 * 	"GeoXmlSequence" [ URL = "\ref sequence.h" ];
 * 	"GeoXmlDocument" [ URL = "\ref flow.h" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GeoXmlObject" -> "GeoXmlSequence"
 * 	"GeoXmlObject" -> "GeoXmlDocument"
 * }
 * \enddot
 * \see object.h
 */

/**
 * \file object.h
 */

/**
 * Get base object class from a sequence or a document.
 */
#define GEOXML_OBJECT(object) ((GeoXmlObject*)(object))

/**
 * The GeoXmlObject struct contains private data only, and should be accessed using the functions below.
 */
typedef struct geoxml_object GeoXmlObject;

/**
 *
 * \see geoxml_object_get_user_data
 */
void
geoxml_object_set_user_data(GeoXmlObject * object, gpointer user_data);

/**
 *
 * \see geoxml_object_set_user_data
 */
gpointer
geoxml_object_get_user_data(GeoXmlObject * object);

#endif //__LIBGEBR_GEOXML_OBJECT_H
