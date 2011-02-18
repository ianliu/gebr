#include <glib-object.h>

#include "value.h"

#ifndef PARAMETER_H
#define PARAMETER_H

GType parameter_get_type (void);
#define PARAMETER_TYPE                (parameter_get_type ())
#define PARAMETER(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), PARAMETER_TYPE, Parameter))
#define PARAMETER_IS_OBJECT(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PARAMETER_TYPE))
#define PARAMETER_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), PARAMETER_TYPE, ParameterClass))
#define PARAMETER_IS_OBJECT_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), PARAMETER_TYPE))
#define PARAMETER_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), PARAMETER_TYPE, ParameterClass))

typedef struct _Parameter              Parameter;
typedef struct _ParameterClass         ParameterClass;

struct _Parameter
{
	GObject parent_instance;

	gchar **value;
	GValue value_v2;
};

struct _ParameterClass
{
	GObjectClass parent_class;
};

#endif //PARAMETER_H
