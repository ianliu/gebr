#include <glib-object.h>

#ifndef VALUE_H
#define VALUE_H

GType value_get_type (void);
#define VALUE_TYPE                (value_get_type ())
#define VALUE(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), VALUE_TYPE, Value))
#define VALUE_IS_OBJECT(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VALUE_TYPE))
#define VALUE_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), VALUE_TYPE, ValueClass))
#define VALUE_IS_OBJECT_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), VALUE_TYPE))
#define VALUE_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), VALUE_TYPE, ValueClass))

typedef struct _Value              Value;
typedef struct _ValueClass         ValueClass;

struct _Value
{
	GObject parent_instance;

	gchar *dictkeyword;
	gchar *value;
};

struct _ValueClass
{
	GObjectClass parent_class;
};

#endif //VALUE_H
