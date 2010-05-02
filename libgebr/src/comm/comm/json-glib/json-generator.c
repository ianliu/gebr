/* json-generator.c - JSON streams generator
 * 
 * This file is part of JSON-GLib
 * Copyright (C) 2007  OpenedHand Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * Author:
 *   Emmanuele Bassi  <ebassi@openedhand.com>
 */

/**
 * SECTION:json-generator
 * @short_description: Generates JSON data streams
 *
 * #JsonGenerator provides an object for generating a JSON data stream and
 * put it into a buffer or a file.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "json-marshal.h"
#include "json-generator.h"

#define JSON_GENERATOR_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), JSON_TYPE_GENERATOR, JsonGeneratorPrivate))

struct _JsonGeneratorPrivate {
	JsonNode *root;

	guint indent;
	gunichar indent_char;

	guint pretty:1;
};

enum {
	PROP_0,

	PROP_PRETTY,
	PROP_INDENT,
	PROP_ROOT,
	PROP_INDENT_CHAR
};

static gchar *dump_value(JsonGenerator * generator, gint level, const gchar * name, JsonNode * node);
static gchar *dump_array(JsonGenerator * generator, gint level, const gchar * name, JsonArray * array, gsize * length);
static gchar *dump_object(JsonGenerator * generator,
			  gint level, const gchar * name, JsonObject * object, gsize * length);

G_DEFINE_TYPE(JsonGenerator, json_generator, G_TYPE_OBJECT);

static void json_generator_finalize(GObject * gobject)
{
	JsonGeneratorPrivate *priv = JSON_GENERATOR_GET_PRIVATE(gobject);

	if (priv->root)
		json_node_free(priv->root);

	G_OBJECT_CLASS(json_generator_parent_class)->finalize(gobject);
}

static void json_generator_set_property(GObject * gobject, guint prop_id, const GValue * value, GParamSpec * pspec)
{
	JsonGeneratorPrivate *priv = JSON_GENERATOR_GET_PRIVATE(gobject);

	switch (prop_id) {
	case PROP_PRETTY:
		priv->pretty = g_value_get_boolean(value);
		break;
	case PROP_INDENT:
		priv->indent = g_value_get_uint(value);
		break;
	case PROP_INDENT_CHAR:
		priv->indent_char = g_value_get_uint(value);
		break;
	case PROP_ROOT:
		json_generator_set_root(JSON_GENERATOR(gobject), g_value_get_boxed(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, pspec);
		break;
	}
}

static void json_generator_get_property(GObject * gobject, guint prop_id, GValue * value, GParamSpec * pspec)
{
	JsonGeneratorPrivate *priv = JSON_GENERATOR_GET_PRIVATE(gobject);

	switch (prop_id) {
	case PROP_PRETTY:
		g_value_set_boolean(value, priv->pretty);
		break;
	case PROP_INDENT:
		g_value_set_uint(value, priv->indent);
		break;
	case PROP_INDENT_CHAR:
		g_value_set_uint(value, priv->indent_char);
		break;
	case PROP_ROOT:
		g_value_set_boxed(value, priv->root);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, pspec);
		break;
	}
}

static void json_generator_class_init(JsonGeneratorClass * klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	g_type_class_add_private(klass, sizeof(JsonGeneratorPrivate));

	gobject_class->set_property = json_generator_set_property;
	gobject_class->get_property = json_generator_get_property;
	gobject_class->finalize = json_generator_finalize;

  /**
   * JsonGenerator:pretty:
   *
   * Whether the output should be "pretty-printed", with indentation and
   * newlines. The indentation level can be controlled by using the
   * JsonGenerator:indent property
   */
	g_object_class_install_property(gobject_class,
					PROP_PRETTY,
					g_param_spec_boolean("pretty",
							     "Pretty",
							     "Pretty-print the output", FALSE,
							     (GParamFlags)(G_PARAM_READWRITE)));
  /**
   * JsonGenerator:indent:
   *
   * Number of spaces to be used to indent when pretty printing.
   */
	g_object_class_install_property(gobject_class,
					PROP_INDENT,
					g_param_spec_uint("indent",
							  "Indent",
							  "Number of indentation spaces",
							  0, G_MAXUINT, 2, (GParamFlags)(G_PARAM_READWRITE)));
  /**
   * JsonGenerator:root:
   *
   * The root #JsonNode to be used when constructing a JSON data
   * stream.
   *
   * Since: 0.4
   */
	g_object_class_install_property(gobject_class,
					PROP_ROOT,
					g_param_spec_boxed("root",
							   "Root",
							   "Root of the JSON data tree",
							   JSON_TYPE_NODE, (GParamFlags)(G_PARAM_READWRITE)));
  /**
   * JsonGenerator:indent-char:
   *
   * The character that should be used when indenting in pretty print.
   *
   * Since: 0.6
   */
	g_object_class_install_property(gobject_class,
					PROP_INDENT_CHAR,
					g_param_spec_unichar("indent-char",
							     "Indent Char",
							     "Character that should be used when indenting",
							     ' ', (GParamFlags)(G_PARAM_READWRITE)));
}

static void json_generator_init(JsonGenerator * generator)
{
	JsonGeneratorPrivate *priv;

	generator->priv = priv = JSON_GENERATOR_GET_PRIVATE(generator);

	priv->pretty = FALSE;
	priv->indent = 2;
	priv->indent_char = ' ';
}

static gchar *dump_value(JsonGenerator * generator, gint level, const gchar * name, JsonNode * node)
{
	JsonGeneratorPrivate *priv = generator->priv;
	gboolean pretty = priv->pretty;
	guint indent = priv->indent;
	GValue value = { 0, };
	GString *buffer;
	guint i;

	buffer = g_string_new("");

	if (pretty) {
		for (i = 0; i < (level * indent); i++)
			g_string_append_c(buffer, priv->indent_char);
	}

	if (name && name[0] != '\0')
		g_string_append_printf(buffer, "\"%s\" : ", name);

	json_node_get_value(node, &value);

	switch (G_VALUE_TYPE(&value)) {
	case G_TYPE_INT:
		g_string_append_printf(buffer, "%d", g_value_get_int(&value));
		break;

	case G_TYPE_STRING:
		g_string_append_printf(buffer, "\"%s\"", g_value_get_string(&value));
		break;

	case G_TYPE_DOUBLE:
		g_string_append_printf(buffer, "%f", g_value_get_double(&value));
		break;

	case G_TYPE_BOOLEAN:
		g_string_append_printf(buffer, "%s", g_value_get_boolean(&value) ? "true" : "false");
		break;

	default:
		break;
	}

	g_value_unset(&value);

	return g_string_free(buffer, FALSE);
}

static gchar *dump_array(JsonGenerator * generator, gint level, const gchar * name, JsonArray * array, gsize * length)
{
	JsonGeneratorPrivate *priv = generator->priv;
	guint array_len = json_array_get_length(array);
	guint i;
	GString *buffer;
	gboolean pretty = priv->pretty;
	guint indent = priv->indent;

	buffer = g_string_new("");

	if (pretty) {
		for (i = 0; i < (level * indent); i++)
			g_string_append_c(buffer, priv->indent_char);
	}

	if (name && name[0] != '\0')
		g_string_append_printf(buffer, "\"%s\" : ", name);

	g_string_append_c(buffer, '[');

	if (pretty)
		g_string_append_c(buffer, '\n');
	else
		g_string_append_c(buffer, ' ');

	for (i = 0; i < array_len; i++) {
		JsonNode *cur = json_array_get_element(array, i);
		guint sub_level = level + 1;
		guint j;
		gchar *value;

		switch (JSON_NODE_TYPE(cur)) {
		case JSON_NODE_NULL:
			if (pretty) {
				for (j = 0; j < (sub_level * indent); j++)
					g_string_append_c(buffer, priv->indent_char);
			}
			g_string_append(buffer, "null");
			break;

		case JSON_NODE_VALUE:
			value = dump_value(generator, sub_level, NULL, cur);
			g_string_append(buffer, value);
			g_free(value);
			break;

		case JSON_NODE_ARRAY:
			value = dump_array(generator, sub_level, NULL, json_node_get_array(cur), NULL);
			g_string_append(buffer, value);
			g_free(value);
			break;

		case JSON_NODE_OBJECT:
			value = dump_object(generator, sub_level, NULL, json_node_get_object(cur), NULL);
			g_string_append(buffer, value);
			g_free(value);
			break;
		}

		if ((i + 1) != array_len)
			g_string_append_c(buffer, ',');

		if (pretty)
			g_string_append_c(buffer, '\n');
		else
			g_string_append_c(buffer, ' ');
	}

	if (pretty) {
		for (i = 0; i < (level * indent); i++)
			g_string_append_c(buffer, priv->indent_char);
	}

	g_string_append_c(buffer, ']');

	if (length)
		*length = buffer->len;

	return g_string_free(buffer, FALSE);
}

static gchar *dump_object(JsonGenerator * generator,
			  gint level, const gchar * name, JsonObject * object, gsize * length)
{
	JsonGeneratorPrivate *priv = generator->priv;
	GList *members, *l;
	GString *buffer;
	gboolean pretty = priv->pretty;
	guint indent = priv->indent;
	guint i;

	buffer = g_string_new("");

	if (pretty) {
		for (i = 0; i < (level * indent); i++)
			g_string_append_c(buffer, priv->indent_char);
	}

	if (name && name[0] != '\0')
		g_string_append_printf(buffer, "\"%s\" : ", name);

	g_string_append_c(buffer, '{');

	if (pretty)
		g_string_append_c(buffer, '\n');
	else
		g_string_append_c(buffer, ' ');

	members = json_object_get_members(object);

	for (l = members; l != NULL; l = l->next) {
		const gchar *member_name = l->data;
		JsonNode *cur = json_object_get_member(object, member_name);
		guint sub_level = level + 1;
		guint j;
		gchar *value;

		switch (JSON_NODE_TYPE(cur)) {
		case JSON_NODE_NULL:
			if (pretty) {
				for (j = 0; j < (sub_level * indent); j++)
					g_string_append_c(buffer, priv->indent_char);
			}
			g_string_append_printf(buffer, "\"%s\" : null", member_name);
			break;

		case JSON_NODE_VALUE:
			value = dump_value(generator, sub_level, member_name, cur);
			g_string_append(buffer, value);
			g_free(value);
			break;

		case JSON_NODE_ARRAY:
			value = dump_array(generator, sub_level, member_name, json_node_get_array(cur), NULL);
			g_string_append(buffer, value);
			g_free(value);
			break;

		case JSON_NODE_OBJECT:
			value = dump_object(generator, sub_level, member_name, json_node_get_object(cur), NULL);
			g_string_append(buffer, value);
			g_free(value);
			break;
		}

		if (l->next != NULL)
			g_string_append_c(buffer, ',');

		if (pretty)
			g_string_append_c(buffer, '\n');
		else
			g_string_append_c(buffer, ' ');
	}

	g_list_free(members);

	if (pretty) {
		for (i = 0; i < (level * indent); i++)
			g_string_append_c(buffer, priv->indent_char);
	}

	g_string_append_c(buffer, '}');

	if (length)
		*length = buffer->len;

	return g_string_free(buffer, FALSE);
}

/**
 * json_generator_new:
 * 
 * Creates a new #JsonGenerator. You can use this object to generate a
 * JSON data stream starting from a data object model composed by
 * #JsonNode<!-- -->s.
 *
 * Return value: the newly created #JsonGenerator instance
 */
JsonGenerator *json_generator_new(void)
{
	return g_object_new(JSON_TYPE_GENERATOR, NULL);
}

/**
 * json_generator_to_data:
 * @generator: a #JsonGenerator
 * @length: return location for the length of the returned buffer, or %NULL
 *
 * Generates a JSON data stream from @generator and returns it as a
 * buffer.
 *
 * Return value: a newly allocated buffer holding a JSON data stream. Use
 *   g_free() to free the allocated resources.
 */
gchar *json_generator_to_data(JsonGenerator * generator, gsize * length)
{
	JsonNode *root;
	gchar *retval = NULL;

	g_return_val_if_fail(JSON_IS_GENERATOR(generator), NULL);

	root = generator->priv->root;
	if (!root) {
		if (length)
			*length = 0;

		return NULL;
	}

	switch (JSON_NODE_TYPE(root)) {
	case JSON_NODE_ARRAY:
		retval = dump_array(generator, 0, NULL, json_node_get_array(root), length);
		break;

	case JSON_NODE_OBJECT:
		retval = dump_object(generator, 0, NULL, json_node_get_object(root), length);
		break;

	case JSON_NODE_NULL:
		retval = g_strdup("null");
		if (length)
			*length = 4;
		break;

	case JSON_NODE_VALUE:
		retval = NULL;
		break;
	}

	return retval;
}

/**
 * json_generator_to_file:
 * @generator: a #JsonGenerator
 * @filename: path to the target file
 * @error: return location for a #GError, or %NULL
 *
 * Creates a JSON data stream and puts it inside @filename, overwriting the
 * current file contents. This operation is atomic.
 *
 * Return value: %TRUE if saving was successful.
 */
gboolean json_generator_to_file(JsonGenerator * generator, const gchar * filename, GError ** error)
{
	gchar *buffer;
	gsize len;
	gboolean retval;

	g_return_val_if_fail(JSON_IS_GENERATOR(generator), FALSE);
	g_return_val_if_fail(filename != NULL, FALSE);

	buffer = json_generator_to_data(generator, &len);
	retval = g_file_set_contents(filename, buffer, len, error);
	g_free(buffer);

	return retval;
}

/**
 * json_generator_set_root:
 * @generator: a #JsonGenerator
 * @node: a #JsonNode
 *
 * Sets @node as the root of the JSON data stream to be serialized by
 * the #JsonGenerator.
 *
 * Note: the node is copied by the generator object, so it can be safely
 * freed after calling this function.
 */
void json_generator_set_root(JsonGenerator * generator, JsonNode * node)
{
	g_return_if_fail(JSON_IS_GENERATOR(generator));

	if (generator->priv->root) {
		json_node_free(generator->priv->root);
		generator->priv->root = NULL;
	}

	if (node)
		generator->priv->root = json_node_copy(node);
}
