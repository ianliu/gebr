#include <glib-object.h>

#include "parameter.h" 

#ifndef DICTIONARY_H
#define DICTIONARY_H

GType dictionary_get_type (void);
#define DICTIONARY_TYPE                (dictionary_get_type ())
#define DICTIONARY(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), DICTIONARY_TYPE, Dictionary))
#define DICTIONARY_IS_OBJECT(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DICTIONARY_TYPE))
#define DICTIONARY_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), DICTIONARY_TYPE, DictionaryClass))
#define DICTIONARY_IS_OBJECT_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), DICTIONARY_TYPE))
#define DICTIONARY_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), DICTIONARY_TYPE, DictionaryClass))

typedef struct _Dictionary              Dictionary;
typedef struct _DictionaryClass         DictionaryClass;

struct _Dictionary
{
	GObject parent_instance;

	Parameter *parameter;
};

struct _DictionaryClass
{
	GObjectClass parent_class;
};

#endif //DICTIONARY_H
