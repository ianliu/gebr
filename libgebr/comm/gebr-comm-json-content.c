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

#include "gebr-comm-json-content.h"

GebrCommJsonContent *gebr_comm_json_content_new(const gchar *json_content)
{
	GebrCommJsonContent *content = g_new(GebrCommJsonContent, 1);
	content->data = g_string_new(json_content);
	return content;
}
GebrCommJsonContent *gebr_comm_json_content_new_from_node(JsonNode *node)
{
	GebrCommJsonContent *content = gebr_comm_json_content_new("");
	if (node == NULL)
		return content;

	JsonGenerator *generator = json_generator_new();
	JsonObject * object = json_object_new();
	json_object_set_member(object, "value", node);
	JsonNode *root = json_node_new(JSON_NODE_OBJECT);
	json_node_take_object(root, object);
	json_generator_set_root(generator, root);

	gchar *tmp = json_generator_to_data(generator, NULL);
	g_string_assign(content->data, tmp);
	g_free(tmp);

	g_object_unref(generator);

	return content;
}
GebrCommJsonContent *gebr_comm_json_content_new_from_string(const gchar *string)
{
	JsonNode *node = json_node_new(JSON_NODE_VALUE);
	json_node_set_string(node, string);
	GebrCommJsonContent *json = gebr_comm_json_content_new_from_node(node);
	json_node_free(node);

	return json;
}
#include "../json/json-gobject-private.h"
GebrCommJsonContent *gebr_comm_json_content_new_from_property(GObject * object, const gchar *property)
{
	GParamSpec *pspec = g_object_class_find_property(G_OBJECT_GET_CLASS(object), property);
	if (!pspec)
		return gebr_comm_json_content_new(NULL);

	GValue value = {0, };
	g_value_init(&value, pspec->value_type);
	g_object_get_property(object, property, &value);
	JsonNode *node = json_serialize_pspec(&value, pspec);
	g_value_unset(&value);

	GebrCommJsonContent *json = gebr_comm_json_content_new_from_node(node);
	if (node)
		json_node_free(node);

	return json;
}
void gebr_comm_json_content_free(GebrCommJsonContent *content)
{
	if (content == NULL)
		return;
	g_string_free(content->data, TRUE);
	g_free(content);
}

JsonNode *gebr_comm_json_content_to_node(GebrCommJsonContent *json)
{
	JsonNode *node = NULL;

	JsonParser *parser = json_parser_new();
	GError *error = NULL;
	gboolean ret = json_parser_load_from_data(parser, json->data->str, -1, &error);
	if (!ret)
		goto out;
	
	JsonNode *root = json_parser_get_root(parser);
	JsonObject *object = json_node_get_object(root);
	if (!object)
		goto out;
	node = json_node_copy(json_object_get_member(object, "value"));

out:
	g_object_unref(parser);
	return node;
}
void gebr_comm_json_content_to_property(GebrCommJsonContent *json, GObject * object, const gchar *property)
{
	GParamSpec *pspec = g_object_class_find_property(G_OBJECT_GET_CLASS(object), property);
	if (!pspec)
		return;

	GValue value = {0, };
	g_value_init(&value, pspec->value_type);
	JsonNode *node = gebr_comm_json_content_to_node(json);
	gboolean ret = json_deserialize_pspec(&value, pspec, node);
	json_node_free(node);

	if (ret)
		g_object_set_property(object, property, &value);
	g_value_unset(&value);
}
gchar *gebr_comm_json_content_to_string(GebrCommJsonContent *json)
{
	JsonNode *node = gebr_comm_json_content_to_node(json);
	gchar *string = json_node_dup_string(node);
	json_node_free(node);
	return string;
}
GString *gebr_comm_json_content_to_gstring(GebrCommJsonContent *json)
{
	JsonNode *node = gebr_comm_json_content_to_node(json);
	GString *string = g_string_new(json_node_get_string(node));
	json_node_free(node);
	return string;
}
