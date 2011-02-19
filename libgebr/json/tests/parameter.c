
#include "parameter.h"

enum
{
	PROP_0,

	PROP_VALUE, //DEPRECATED
	PROP_VALUE_V2
};


static void
parameter_set_property (GObject      *gobject,
		   guint         prop_id,
		   const GValue *value,
		   GParamSpec   *pspec)
{
	switch (prop_id)
	{
	case PROP_VALUE:
		g_strfreev (PARAMETER (gobject)->value);
		PARAMETER (gobject)->value = g_strdupv (g_value_get_boxed (value));
		break;
	case PROP_VALUE_V2:
		//PARAMETER (gobject)->value_v2 = g_value_get_boxed (value);
		g_value_copy (value, &PARAMETER (gobject)->value_v2);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
	}
}

static void
parameter_get_property (GObject    *gobject,
		   guint       prop_id,
		   GValue     *value,
		   GParamSpec *pspec)
{
	switch (prop_id)
	{
	case PROP_VALUE:
		g_value_set_boxed (value, PARAMETER (gobject)->value);
		break;
	case PROP_VALUE_V2:
		//g_value_set_boxed (value, &PARAMETER (gobject)->value_v2);
		g_value_copy (&PARAMETER (gobject)->value_v2, value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
	}
}

static void
parameter_init (Parameter *object)
{
	object->value = NULL;
	object->value_v2 = (GValue){0,};
	g_value_init (&object->value_v2, G_TYPE_VALUE_ARRAY);
}

static void
parameter_finalize (GObject *gobject)
{
	g_strfreev (PARAMETER (gobject)->value);
  
	//G_OBJECT_CLASS (gobject)->finalize (gobject);
}

static void
parameter_class_init (ParameterClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->set_property = parameter_set_property;
	gobject_class->get_property = parameter_get_property;
	gobject_class->finalize = parameter_finalize;
	//deprecated
	g_object_class_install_property (gobject_class, PROP_VALUE,
					 g_param_spec_boxed ("value", "Value (deprecated)", "Value (deprecated)",
							     G_TYPE_STRV,
							     G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class, PROP_VALUE_V2,
					 g_param_spec_value_array ("value-v2", "Value", "Value",
								   g_param_spec_object ("value-v2-in-array", "", "",
											VALUE_TYPE,
											G_PARAM_READWRITE),
								   G_PARAM_READWRITE));
}

G_DEFINE_TYPE (Parameter, parameter, G_TYPE_OBJECT);
