#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib-object.h>

#include <json-glib.h>
#include <json-gobject.h>

#include "flow.h"

static const gchar *var_test =
"{\n"
"  \"dictionary\" : {\n"
"    \"parameter\" : {\n"
"      \"value\" : [\"a\",\"b\"],\n"
"      \"value_v2\" : [{\"param-value\" : \"a\"}, {\"param-value\" : \"b\"}]\n"
"    }\n"
"  },\n"
"  \"parameter\" : {\n"
"    \"value\" : [\"c\",\"d\"],\n"
"    \"value_v2\" : [{\"param-value\" : \"c\"}, {\"param-value\" : \"d\"}]\n"
"  },\n"
"  \"garbage\"  : \"hello\"\n"
"}";

static void
test_deserialize (void)
{
  GObject *object;
  GError *error;
  gchar *str;

  error = NULL;
  object = json_gobject_from_data (FLOW_TYPE, var_test, -1, &error);
  if (error)
    g_error ("*** Unable to parse buffer: %s\n", error->message);

  Parameter *parameter = PARAMETER (FLOW (object)->parameter);
  //g_assert_cmpstr (parameter->value, ==, "456");
  GValueArray *a;

  g_assert (DICTIONARY_IS_OBJECT (FLOW (object)->dictionary));
  Dictionary *dictionary = DICTIONARY (FLOW (object)->dictionary);
  g_assert (dictionary->parameter);
  //g_assert_cmpstr (dictionary->parameter->value, ==, "123");

  gchar *json = json_gobject_to_data (object, NULL);
  puts (json);
  g_free(json);

  g_object_unref (object);
}

int
main (int   argc,
      char *argv[])
{
  g_type_init ();
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/deserialize/json-to-gobject", test_deserialize);

  return g_test_run ();
}
