
#include "flow.h"

enum
{
	PROP_0,

	PROP_DICTIONARY,
	PROP_PARAMETER
};


static void
flow_set_property (GObject      *gobject,
		   guint         prop_id,
		   const GValue *value,
		   GParamSpec   *pspec)
{
	switch (prop_id)
	{
	case PROP_DICTIONARY:
		FLOW (gobject)->dictionary = g_value_dup_object (value);
		break;
	case PROP_PARAMETER:
		FLOW (gobject)->parameter = g_value_dup_object (value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
	}
}

static void
flow_get_property (GObject    *gobject,
		   guint       prop_id,
		   GValue     *value,
		   GParamSpec *pspec)
{
	switch (prop_id)
	{
	case PROP_DICTIONARY:
		g_value_set_object (value, FLOW (gobject)->dictionary);
		break;
	case PROP_PARAMETER:
		g_value_set_object (value, FLOW (gobject)->parameter);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
	}
}

static void
flow_init (Flow *object)
{
	object->dictionary = NULL;
	object->parameter = NULL;
}

static void
flow_finalize (GObject *gobject)
{
	if (FLOW (gobject)->dictionary != NULL)
		g_object_unref (FLOW (gobject)->dictionary);
	if (FLOW (gobject)->parameter != NULL)
		g_object_unref (FLOW (gobject)->parameter);

	//G_OBJECT_CLASS (gobject)->finalize (gobject);
}

static void
flow_class_init (FlowClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->set_property = flow_set_property;
	gobject_class->get_property = flow_get_property;
	gobject_class->finalize = flow_finalize;
	g_object_class_install_property (gobject_class,
					 PROP_DICTIONARY,
					 g_param_spec_object ("dictionary", "Dictionary", "Dictionary",
							      DICTIONARY_TYPE,
							      G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
					 PROP_PARAMETER,
					 g_param_spec_object ("parameter", "Parameter", "Parameter",
							      PARAMETER_TYPE,
							      G_PARAM_READWRITE));
}

G_DEFINE_TYPE (Flow, flow, G_TYPE_OBJECT);
