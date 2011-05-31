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

#include "gebr-geo-types.h"

G_BEGIN_DECLS

/**
 * Return the type of \p object
 */
GebrGeoXmlObjectType gebr_geoxml_object_get_type(GebrGeoXmlObject * object);

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

/**
 * gebr_geoxml_object_generate_help:
 * @object: either a #GebrGeoXmlFlow or a #GebrGeoXmlProgram
 * @content: the content which the user edited
 *
 * Generates the help HTML from the help-template.html file by inserting
 * some properties of @object and inserting the @content.
 *
 * Returns: a newly allocated c-string containing the generated help for this object.
 */
gchar *gebr_geoxml_object_generate_help (GebrGeoXmlObject *object, const gchar *content);

/**
 * gebr_geoxml_object_get_help_content:
 * @object:
 *
 * Returns: A newly allocated string containing the content of @object<!-- --!>'s help.
 */
gchar *gebr_geoxml_object_get_help_content (GebrGeoXmlObject *object);

/**
 * gebr_geoxml_object_get_help_content_from_str:
 * @str:
 *
 * Returns: A newly allocated string containing the content of @str.
 */
gchar *gebr_geoxml_object_get_help_content_from_str (const gchar *str);

/**
 * gebr_geoxml_object_set_help:
 * @object:
 *
 * Set help in document or program
 */
void gebr_geoxml_object_set_help (GebrGeoXmlObject *object, const gchar *help);

G_END_DECLS
#endif				//__GEBR_GEOXML_OBJECT_H
