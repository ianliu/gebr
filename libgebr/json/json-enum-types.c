
/* Generated data (by glib-mkenums) */

#include "json-enum-types.h"

/* enumerations from "json-parser.h" */
#include "json-parser.h"

GType
json_parser_error_get_type(void) {
  static volatile gsize g_enum_type_id__volatile = 0;

  if (g_once_init_enter (&g_enum_type_id__volatile))
    {
      static const GEnumValue values[] = {
        { JSON_PARSER_ERROR_PARSE, "JSON_PARSER_ERROR_PARSE", "parse" },
        { JSON_PARSER_ERROR_UNKNOWN, "JSON_PARSER_ERROR_UNKNOWN", "unknown" },
        { 0, NULL, NULL }
      };
      GType g_enum_type_id;

      g_enum_type_id =
        g_enum_register_static (g_intern_static_string ("JsonParserError"), values);

      g_once_init_leave (&g_enum_type_id__volatile, g_enum_type_id);
    }

  return g_enum_type_id__volatile;
}

/* enumerations from "json-reader.h" */
#include "json-reader.h"

GType
json_reader_error_get_type(void) {
  static volatile gsize g_enum_type_id__volatile = 0;

  if (g_once_init_enter (&g_enum_type_id__volatile))
    {
      static const GEnumValue values[] = {
        { JSON_READER_ERROR_NO_ARRAY, "JSON_READER_ERROR_NO_ARRAY", "no-array" },
        { JSON_READER_ERROR_INVALID_INDEX, "JSON_READER_ERROR_INVALID_INDEX", "invalid-index" },
        { JSON_READER_ERROR_NO_OBJECT, "JSON_READER_ERROR_NO_OBJECT", "no-object" },
        { JSON_READER_ERROR_INVALID_MEMBER, "JSON_READER_ERROR_INVALID_MEMBER", "invalid-member" },
        { 0, NULL, NULL }
      };
      GType g_enum_type_id;

      g_enum_type_id =
        g_enum_register_static (g_intern_static_string ("JsonReaderError"), values);

      g_once_init_leave (&g_enum_type_id__volatile, g_enum_type_id);
    }

  return g_enum_type_id__volatile;
}

/* enumerations from "json-types.h" */
#include "json-types.h"

GType
json_node_type_get_type(void) {
  static volatile gsize g_enum_type_id__volatile = 0;

  if (g_once_init_enter (&g_enum_type_id__volatile))
    {
      static const GEnumValue values[] = {
        { JSON_NODE_OBJECT, "JSON_NODE_OBJECT", "object" },
        { JSON_NODE_ARRAY, "JSON_NODE_ARRAY", "array" },
        { JSON_NODE_VALUE, "JSON_NODE_VALUE", "value" },
        { JSON_NODE_NULL, "JSON_NODE_NULL", "null" },
        { 0, NULL, NULL }
      };
      GType g_enum_type_id;

      g_enum_type_id =
        g_enum_register_static (g_intern_static_string ("JsonNodeType"), values);

      g_once_init_leave (&g_enum_type_id__volatile, g_enum_type_id);
    }

  return g_enum_type_id__volatile;
}

/* Generated data ends here */

