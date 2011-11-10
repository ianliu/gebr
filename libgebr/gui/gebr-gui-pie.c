#include "gebr-gui-pie.h"
#include <math.h>

struct _GebrGuiPiePriv {
	guint *data;
	guint sum;
	gint length;
	gint hovered;
};

enum {
	CLICKED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE(GebrGuiPie, gebr_gui_pie, GTK_TYPE_DRAWING_AREA);

static gint
get_index_at(GebrGuiPie *pie, gdouble px, gdouble py)
{
	gint i;
	GtkWidget *widget = GTK_WIDGET(pie);
	gint x = widget->allocation.x;
	gint y = widget->allocation.y;
	gint w = widget->allocation.width;
	gint h = widget->allocation.height;
	gdouble radius, alpha, beta, xc, yc, gamma, xx, yy;
	gdouble scale = 2 * G_PI / pie->priv->sum;
	guint sum = 0;

	radius = MIN(w, h) / 2. - 10 + 2;
	for (i = 0; i < pie->priv->length; i++) {
		alpha = (sum) * scale;
		beta = (sum + pie->priv->data[i]) * scale;
		xc = x + w / 2.0 + 2*cos((alpha + beta) / 2);
		yc = y + h / 2.0 + 2*sin((alpha + beta) / 2);
		yc = yc - 120;
		xx = px - xc;
		yy = py - yc;
		gamma = atan2(yy, xx);
		if (gamma < 0)
			gamma = 2*G_PI + gamma;
		if (gamma >= alpha && gamma <= beta
				&& (xx*xx + yy*yy <= radius*radius))
			return i;

		sum += pie->priv->data[i];
	}
	return -1;
}

static gboolean
gebr_gui_pie_expose(GtkWidget      *widget,
                    GdkEventExpose *event)
{
	gint i;
	GebrGuiPie *pie = GEBR_GUI_PIE(widget);
	gint x = widget->allocation.x;
	gint y = widget->allocation.y;
	gint w = widget->allocation.width;
	gint h = widget->allocation.height;
	cairo_t *cx = gdk_cairo_create(widget->window);
	gdouble xc, yc, radius, alpha, beta;
	gdouble scale = 2*G_PI / pie->priv->sum;
	gdouble gray;
	guint sum = 0;

	radius = MIN(w, h) / 2. - 10 + 2;
	for (i = 0; i < pie->priv->length; i++) {
		gray = i * 1.0 / (pie->priv->length - 1);
		if (i == pie->priv->hovered)
			cairo_set_source_rgb(cx, 1, 0, 0);
		else
			cairo_set_source_rgb(cx, gray, gray, gray);
		alpha = sum * scale;
		beta = (sum + pie->priv->data[i]) * scale;
		xc = x + w / 2. + 2*cos((alpha + beta) / 2);
		yc = y + h / 2. + 2*sin((alpha + beta) / 2);
		yc = yc - 120;
		cairo_move_to(cx, xc, yc);
		cairo_arc(cx, xc, yc, radius, alpha, beta);
		cairo_line_to(cx, xc, yc);
		cairo_close_path(cx);
		cairo_fill(cx);
		sum += pie->priv->data[i];
	}

	cairo_destroy(cx);
	return TRUE;
}

static gboolean
gebr_gui_pie_configure(GtkWidget  	 *widget,
                       GdkEventConfigure *event)
{
	return TRUE;
}

static gboolean
gebr_gui_pie_button_release(GtkWidget *widget,
                            GdkEventButton *event)
{
	gint i = get_index_at(GEBR_GUI_PIE(widget), event->x, event->y);

	if (i == -1)
		return FALSE;

	g_signal_emit(widget, signals[CLICKED], 0, i);

	return TRUE;
}

static gboolean
gebr_gui_pie_motion_notify(GtkWidget *widget,
                           GdkEventMotion *event)
{
	GebrGuiPie *pie = GEBR_GUI_PIE(widget);
	gint hover = get_index_at(GEBR_GUI_PIE(widget), event->x, event->y);
	if (pie->priv->hovered != hover) {
		pie->priv->hovered = hover;
		gtk_widget_queue_draw(widget);
	}
	return TRUE;
}

static void
gebr_gui_pie_init(GebrGuiPie *pie)
{
	gtk_widget_add_events(GTK_WIDGET(pie),
	                      GDK_BUTTON_PRESS_MASK
	                      | GDK_BUTTON_RELEASE_MASK
	                      | GDK_POINTER_MOTION_MASK);
	pie->priv = G_TYPE_INSTANCE_GET_PRIVATE(pie,
	                                        GEBR_GUI_TYPE_PIE,
	                                        GebrGuiPiePriv);
	pie->priv->hovered = -1;
}

static void
gebr_gui_pie_class_init(GebrGuiPieClass *klass)
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
	widget_class->expose_event = gebr_gui_pie_expose;
	widget_class->configure_event = gebr_gui_pie_configure;
	widget_class->button_release_event = gebr_gui_pie_button_release;
	widget_class->motion_notify_event = gebr_gui_pie_motion_notify;

	signals[CLICKED] =
			g_signal_new("clicked",
			             GEBR_GUI_TYPE_PIE,
			             G_SIGNAL_RUN_LAST,
			             G_STRUCT_OFFSET(GebrGuiPieClass, clicked),
			             NULL, NULL,
			             g_cclosure_marshal_VOID__INT,
			             G_TYPE_NONE, 1, G_TYPE_INT);

	g_type_class_add_private(klass, sizeof(GebrGuiPiePriv));
}

GtkWidget *
gebr_gui_pie_new(guint *data,
                 gint length)
{
	GtkWidget *widget = g_object_new(GEBR_GUI_TYPE_PIE, NULL);
	GebrGuiPie *pie = GEBR_GUI_PIE(widget);
	gebr_gui_pie_set_data(pie, data, length);
	return widget;
}

gint
gebr_gui_pie_get_hovered(GebrGuiPie *pie)
{
	return pie->priv->hovered;
}

void
gebr_gui_pie_set_data(GebrGuiPie *pie,
                      guint *data,
                      gint length)
{
	gint i;

	if (pie->priv->data)
		g_free(pie->priv->data);

	pie->priv->length = length;
	pie->priv->data = g_new(guint, length);
	pie->priv->sum = 0;

	for (i = 0; i < length; i++) {
		pie->priv->data[i] = data[i];
		pie->priv->sum += data[i];
	}

	gtk_widget_queue_draw(GTK_WIDGET(pie));
}
