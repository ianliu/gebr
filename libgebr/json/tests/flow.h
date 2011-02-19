#include <glib-object.h>

#include "dictionary.h" 
#include "parameter.h" 

#ifndef FLOW_H
#define FLOW_H

GType flow_get_type (void);
#define FLOW_TYPE                (flow_get_type ())
#define FLOW(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), FLOW_TYPE, Flow))
#define FLOW_IS_OBJECT(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FLOW_TYPE))
#define FLOW_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), FLOW_TYPE, FlowClass))
#define FLOW_IS_OBJECT_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), FLOW_TYPE))
#define FLOW_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), FLOW_TYPE, FlowClass))

typedef struct _Flow              Flow;
typedef struct _FlowClass         FlowClass;

struct _Flow
{
	GObject parent_instance;

	Dictionary *dictionary;
	Parameter *parameter;
};

struct _FlowClass
{
	GObjectClass parent_class;
};

#endif //FLOW_H
