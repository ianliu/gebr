
#include "value.h"

enum
{
	PROP_0,

	PROP_DICTIONARY_KEYWORD,
	PROP_VALUE
};


#include <gobject/gvaluecollector.h>
static void
g_value_object_init (GValue *value)
{
  value->data[0].v_pointer = NULL;
}

static void
g_value_object_free_value (GValue *value)
{
  if (value->data[0].v_pointer)
    g_object_unref (value->data[0].v_pointer);
}

static void
g_value_object_copy_value (const GValue *src_value,
			   GValue	*dest_value)
{
  if (src_value->data[0].v_pointer)
    dest_value->data[0].v_pointer = g_object_ref (src_value->data[0].v_pointer);
  else
    dest_value->data[0].v_pointer = NULL;
}

static void
g_value_object_transform_value (const GValue *src_value,
				GValue       *dest_value)
{
  if (src_value->data[0].v_pointer && g_type_is_a (G_OBJECT_TYPE (src_value->data[0].v_pointer), G_VALUE_TYPE (dest_value)))
    dest_value->data[0].v_pointer = g_object_ref (src_value->data[0].v_pointer);
  else
    dest_value->data[0].v_pointer = NULL;
}

static gpointer
g_value_object_peek_pointer (const GValue *value)
{
  return value->data[0].v_pointer;
}

static gchar*
g_value_object_collect_value (GValue	  *value,
			      guint        n_collect_values,
			      GTypeCValue *collect_values,
			      guint        collect_flags)
{
  if (collect_values[0].v_pointer)
    {
      GObject *object = collect_values[0].v_pointer;
      
      if (object->g_type_instance.g_class == NULL)
	return g_strconcat ("invalid unclassed object pointer for value type `",
			    G_VALUE_TYPE_NAME (value),
			    "'",
			    NULL);
      else if (!g_value_type_compatible (G_OBJECT_TYPE (object), G_VALUE_TYPE (value)))
	return g_strconcat ("invalid object type `",
			    G_OBJECT_TYPE_NAME (object),
			    "' for value type `",
			    G_VALUE_TYPE_NAME (value),
			    "'",
			    NULL);
      /* never honour G_VALUE_NOCOPY_CONTENTS for ref-counted types */
      value->data[0].v_pointer = g_object_ref (object);
    }
  else
    value->data[0].v_pointer = NULL;
  
  return NULL;
}

static gchar*
g_value_object_lcopy_value (const GValue *value,
			    guint        n_collect_values,
			    GTypeCValue *collect_values,
			    guint        collect_flags)
{
  GObject **object_p = collect_values[0].v_pointer;
  
  if (!object_p)
    return g_strdup_printf ("value location for `%s' passed as NULL", G_VALUE_TYPE_NAME (value));

  if (!value->data[0].v_pointer)
    *object_p = NULL;
  else if (collect_flags & G_VALUE_NOCOPY_CONTENTS)
    *object_p = value->data[0].v_pointer;
  else
    *object_p = g_object_ref (value->data[0].v_pointer);
  
  return NULL;
}

GObject* value_object_copy(GObject *gobject)
{
	GObject *new_object;

	guint n_pspecs, i;
	GParamSpec **pspecs;
	pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (gobject), &n_pspecs);
	GParameter parameters[n_pspecs];
	for (i = 0; i < n_pspecs; i++)
	{
		GParamSpec *pspec = pspecs[i];

		GValue value = { 0, };
		g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
		g_object_get_property (gobject, pspec->name, &value);
		parameters[i].name = pspec->name;
		g_value_copy(&value, &parameters[i].value);

		g_value_unset (&value);
	}

	new_object = g_object_newv (value_get_type(), n_pspecs, parameters);
	return new_object;
}

void value_object_free(GObject *gobject)
{
	g_object_unref(gobject);
}

static void
value_set_property (GObject      *gobject,
		   guint         prop_id,
		   const GValue *value,
		   GParamSpec   *pspec)
{
	switch (prop_id)
	{
	case PROP_DICTIONARY_KEYWORD:
		g_free (VALUE (gobject)->dictkeyword);
		VALUE (gobject)->dictkeyword = g_value_dup_string (value);
		break;
	case PROP_VALUE:
		g_free (VALUE (gobject)->value);
		VALUE (gobject)->value = g_value_dup_string (value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
	}
}

static void
value_get_property (GObject    *gobject,
		   guint       prop_id,
		   GValue     *value,
		   GParamSpec *pspec)
{
	switch (prop_id)
	{
	case PROP_DICTIONARY_KEYWORD:
		g_value_set_string (value, VALUE (gobject)->dictkeyword);
		break;
	case PROP_VALUE:
		g_value_set_string (value, VALUE (gobject)->value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
	}
}

static void
value_init (Value *object)
{
	object->dictkeyword = NULL;
	object->value = NULL;
}

static void
value_finalize (GObject *gobject)
{
	g_free(VALUE (gobject)->dictkeyword);
	g_free(VALUE (gobject)->value);

	//G_OBJECT_CLASS (gobject)->finalize (gobject);
}

static void
value_class_init (ValueClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->set_property = value_set_property;
	gobject_class->get_property = value_get_property;
	gobject_class->finalize = value_finalize;
	g_object_class_install_property (gobject_class,
					 PROP_DICTIONARY_KEYWORD,
					 g_param_spec_string ("dictkeyword", "Keyword from Dictionary", "Keyword from Dictionary",
							      NULL,
							      G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
					 PROP_VALUE,
					 g_param_spec_string ("param-value", "Value", "Value",
							      NULL,
							      G_PARAM_READWRITE));
}

//G_DEFINE_TYPE (Value, value, G_TYPE_OBJECT);
GType value_get_type (void)
{
	static GType type = 0;

	if (!type) {
		GTypeValueTable value_table;
		GTypeInfo info;
		value_table.value_init = g_value_object_init;
		value_table.value_free = g_value_object_free_value;
		value_table.value_copy = g_value_object_copy_value;
		value_table.value_peek_pointer = g_value_object_peek_pointer;
		value_table.collect_format = "p";
		value_table.collect_value = g_value_object_collect_value;
		value_table.lcopy_format = "p";
		value_table.lcopy_value = g_value_object_lcopy_value;

		info.class_size = sizeof (ValueClass);
		info.base_init = NULL;
		info.base_finalize = NULL;
		info.class_init = value_class_init;
		info.class_finalize = NULL;
		info.class_data = NULL;
		info.instance_size = sizeof (Value);
		info.n_preallocs = 0;
		info.instance_init = value_init;
		info.value_table = &value_table;

//		g_boxed_type_register_static("Value", value_object_copy, value_object_free);
		type = g_type_register_static(G_TYPE_OBJECT, g_intern_static_string ("Value"), &info, 0);
	}

	return type;
}
