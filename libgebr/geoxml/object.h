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

#ifndef __GEBR_GEOXML_OBJECT_H
#define __GEBR_GEOXML_OBJECT_H

#include <glib.h>

G_BEGIN_DECLS

/**
 * \struct GebrGeoXmlObject object.h geoxml/object.h
 * \brief
 * The base class for all GebrGeoXml.
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
 * 	"GebrGeoXmlObject" [ URL = "\ref object.h" ];
 * 	"GebrGeoXmlSequence" [ URL = "\ref sequence.h" ];
 * 	"GebrGeoXmlDocument" [ URL = "\ref flow.h" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GebrGeoXmlObject" -> "GebrGeoXmlSequence"
 * 	"GebrGeoXmlObject" -> "GebrGeoXmlDocument"
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
#define GEBR_GEOXML_OBJECT(object) ((GebrGeoXmlObject*)(object))


/**
 * The GebrGeoXmlObject struct contains private data only, and should be accessed using the functions below.
 */
typedef struct gebr_geoxml_object GebrGeoXmlObject;

/**
 * GebrGeoXml basic object types
 */
typedef enum {
	GEBR_GEOXML_OBJECT_TYPE_UNKNOWN = 0,
	GEBR_GEOXML_OBJECT_TYPE_PROJECT,
	GEBR_GEOXML_OBJECT_TYPE_LINE,
	GEBR_GEOXML_OBJECT_TYPE_FLOW,
	GEBR_GEOXML_OBJECT_TYPE_PROGRAM,
	GEBR_GEOXML_OBJECT_TYPE_PARAMETERS,
	GEBR_GEOXML_OBJECT_TYPE_PARAMETER,
	GEBR_GEOXML_OBJECT_TYPE_ENUM_OPTION,
} GebrGeoXmlObjectType;

/**
 * Return the type of \p object
 */
GebrGeoXmlObjectType gebr_geoxml_object_get_type(GebrGeoXmlObject * object);

/**
 * \see gebr_geoxml_program_foreach_parameter
 */
typedef void (*GebrGeoXmlCallback)(GebrGeoXmlObject * object, gpointer user_data);

/**
 * Set the user pointer data associated with _object_
 *
 * \see gebr_geoxml_object_get_user_data
 */
void gebr_geoxml_object_set_user_data(GebrGeoXmlObject * object, gpointer user_data);

/**
 * Get the user pointer data associated with _object_
 *
 * Returns NULL if _object_ is NULL
 * \see gebr_geoxml_object_set_user_data
 */
gpointer gebr_geoxml_object_get_user_data(GebrGeoXmlObject * object);

#include "document.h"

/**
 * Get the owner document of \p object
 *
 * Returns NULL if _object_ is NULL
 */
GebrGeoXmlDocument *gebr_geoxml_object_get_owner_document(GebrGeoXmlObject * object);

/**
 * Copy _object_  for parallel use (e.g. when you don't want to modify _object_)
 *
 * Returns NULL if _object_ is NULL
 */
GebrGeoXmlObject *gebr_geoxml_object_copy(GebrGeoXmlObject * object);

G_END_DECLS
#endif				//__GEBR_GEOXML_OBJECT_H
