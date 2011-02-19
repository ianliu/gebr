
#include "dictionary.h"

enum
{
	PROP_0,

	PROP_PARAMETER
};


static void
dictionary_set_property (GObject      *gobject,
		   guint         prop_id,
		   const GValue *value,
		   GParamSpec   *pspec)
{
	switch (prop_id)
	{
	case PROP_PARAMETER:
		DICTIONARY (gobject)->parameter = g_value_dup_object (value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
	}
}

static void
dictionary_get_property (GObject    *gobject,
		   guint       prop_id,
		   GValue     *value,
		   GParamSpec *pspec)
{
	switch (prop_id)
	{
	case PROP_PARAMETER:
		g_value_set_object (value, DICTIONARY (gobject)->parameter);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
	}
}

static void
dictionary_init (Dictionary *object)
{
	object->parameter = NULL;
}

static void
dictionary_finalize (GObject *gobject)
{
  if (DICTIONARY (gobject)->parameter != NULL)
    g_object_unref (DICTIONARY (gobject)->parameter);

  //G_OBJECT_CLASS (gobject)->finalize (gobject);
}

static void
dictionary_class_init (DictionaryClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->set_property = dictionary_set_property;
	gobject_class->get_property = dictionary_get_property;
	gobject_class->finalize = dictionary_finalize;
	g_object_class_install_property (gobject_class,
					 PROP_PARAMETER,
					 g_param_spec_object ("parameter", "Parameter", "Parameter",
							      PARAMETER_TYPE,
							      G_PARAM_READWRITE));
}

G_DEFINE_TYPE (Dictionary, dictionary, G_TYPE_OBJECT);
