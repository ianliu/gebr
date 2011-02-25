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

#ifndef __GEBR_COMM_JSON_CONTENT_H
#define __GEBR_COMM_JSON_CONTENT_H

#include <glib-object.h>

#include <libgebr/json/json-glib.h>

typedef struct _GebrCommJsonContent GebrCommJsonContent; 
struct _GebrCommJsonContent {
	GString *data;
};
GebrCommJsonContent *gebr_comm_json_content_new(const gchar *json_content);
GebrCommJsonContent *gebr_comm_json_content_new_from_node(JsonNode *node);
GebrCommJsonContent *gebr_comm_json_content_new_from_string(const gchar *string);
GebrCommJsonContent *gebr_comm_json_content_new_from_property(GObject * object, const gchar *property);
void gebr_comm_json_content_free(GebrCommJsonContent *content);

JsonNode *gebr_comm_json_content_to_node(GebrCommJsonContent *json);
void gebr_comm_json_content_to_property(GebrCommJsonContent *json, GObject * object, const gchar *property);
gchar* gebr_comm_json_content_to_string(GebrCommJsonContent *json);
GString *gebr_comm_json_content_to_gstring(GebrCommJsonContent *json);

#endif //__GEBR_COMM_JSON_CONTENT_H

