#ifndef __GEBR_GUI_PIE_H__
#define __GEBR_GUI_PIE_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GEBR_GUI_TYPE_PIE (gebr_gui_pie_get_type())
#define GEBR_GUI_PIE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GEBR_GUI_TYPE_PIE, GebrGuiPie))

typedef struct _GebrGuiPie GebrGuiPie;
typedef struct _GebrGuiPiePriv GebrGuiPiePriv;
typedef struct _GebrGuiPieClass GebrGuiPieClass;

struct _GebrGuiPie {
	GtkDrawingArea parent;
	GebrGuiPiePriv *priv;
};

struct _GebrGuiPieClass {
	GtkDrawingAreaClass parent_class;

	void (*clicked) (GebrGuiPie *pie,
			 gint index);
};

GType gebr_gui_pie_get_type(void) G_GNUC_CONST;

GtkWidget *gebr_gui_pie_new(guint *data,
			    gint length);

void gebr_gui_pie_get_color(GebrGuiPie *pie,
                            gint index,
                            gdouble *red,
                            gdouble *green,
                            gdouble *blue);

gint gebr_gui_pie_get_hovered(GebrGuiPie *pie);

void gebr_gui_pie_set_data(GebrGuiPie *pie,
			   guint *data,
			   gint length);

G_END_DECLS

#endif /* __GEBR_GUI_PIE_H__ */
