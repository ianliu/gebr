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

/* PiePartIter {{{1 */
/*
 * This struct is used to iterate
 * over the values of a pie chart.
 */
typedef struct {
	/* private */
	GebrGuiPie *pie;
	gdouble scale;
	gdouble sum;
	gdouble xc, yc;

	/* public - variables */
	int i;           /* Current iteration */
	gdouble alpha;   /* First angle of the current part */
	gdouble beta;    /* Second angle of the current part */
	gdouble x, y;    /* Center of the current part */
} PiePartIter;

/*
 * pie_part_iter_init:
 *
 * Initializes a PiePartIter with an undefined value.
 * The first usable value is computed when calling pie_part_iter_next().
 *
 * Example:
 *
 * PiePartIter iter;
 * pie_part_iter_init(&iter, pie, 10);
 * for (; pie_part_next(&iter);)
 *   // Do stuff with iter.alpha, iter.beta, iter.xc and iter.yc!
 */
static void
pie_part_iter_init(PiePartIter *iter,
		   GebrGuiPie *pie)
{
	iter->pie = pie;
	iter->scale = 2*G_PI / pie->priv->sum;
	iter->sum = 0;
	iter->xc = GTK_WIDGET(iter->pie)->allocation.width / 2.;
	iter->yc = GTK_WIDGET(iter->pie)->allocation.height / 2.;

	iter->i = -1;
	iter->alpha = 0;
	iter->beta = 0;
	iter->x = 0;
	iter->y = 0;
}

/*
 * pie_part_iter_next:
 *
 * Calculates the next values of @iter if this is not the last part. In that
 * case, %TRUE is returned.
 */
static gboolean
pie_part_iter_next(PiePartIter *iter)
{
	iter->i++;

	if (iter->i == iter->pie->priv->length)
		return FALSE;

	iter->alpha = iter->sum * iter->scale;
	iter->beta = (iter->sum + iter->pie->priv->data[iter->i]) * iter->scale;
	iter->x = iter->xc + 2 * cos((iter->alpha + iter->beta) / 2);
	iter->y = iter->yc + 2 * sin((iter->alpha + iter->beta) / 2);
	iter->sum += iter->pie->priv->data[iter->i];
	return TRUE;
}

/* Private methods {{{1 */
static gint
get_index_at(GebrGuiPie *pie, gdouble px, gdouble py)
{
	GtkWidget *widget = GTK_WIDGET(pie);
	gdouble radius2, gamma, xx, yy;
	gint w = widget->allocation.width;
	gint h = widget->allocation.height;

	radius2 = MIN(w, h) / 2. - 10 + 2;
	radius2 *= radius2;

	PiePartIter iter;
	pie_part_iter_init(&iter, pie);
	while (pie_part_iter_next(&iter))
	{
		xx = px - iter.x;
		yy = py - iter.y;

		/* Get the angle of the point (px, py) relative to the current
		 * center */
		gamma = atan2(yy, xx);

		/* atan2 discontinuity is at 9 o'clock. We must bring it to 3
		 * o'clock so its equal cairo's discontinuity. */
		if (gamma < 0)
			gamma = 2*G_PI + gamma;

		if (gamma >= iter.alpha && gamma <= iter.beta
		    && (xx*xx + yy*yy <= radius2))
			return iter.i;
	}

	return -1;
}

/* Override Gtk.Widget.expose_event {{{1 */
static gboolean
gebr_gui_pie_expose(GtkWidget      *widget,
                    GdkEventExpose *event)
{
	GebrGuiPie *pie = GEBR_GUI_PIE(widget);
	cairo_t *cx = gdk_cairo_create(widget->window);
	gint w = widget->allocation.width;
	gint h = widget->allocation.height;
	gdouble radius = MIN(w, h) / 2. - 10 + 2;
	gdouble gray;

	PiePartIter iter;
	pie_part_iter_init(&iter, pie);
	while (pie_part_iter_next(&iter))
	{
		gray = iter.i * 1.0 / (pie->priv->length - 1);
		if (iter.i == pie->priv->hovered)
			cairo_set_source_rgb(cx, 1, 0, 0);
		else
			cairo_set_source_rgb(cx, gray, gray, gray);
		cairo_arc(cx, iter.x, iter.y, radius, iter.alpha, iter.beta);
		cairo_line_to(cx, iter.x, iter.y);
		cairo_close_path(cx);
		cairo_fill(cx);
	}

	cairo_destroy(cx);
	return TRUE;
}

/* Override Gtk.Widget.configure {{{1 */
static gboolean
gebr_gui_pie_configure(GtkWidget  	 *widget,
                       GdkEventConfigure *event)
{
	return TRUE;
}

/* Override Gtk.Widget.button_release {{{1 */
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

/* Override Gtk.Widget.motion_notify {{{1 */
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

/* Class & Instance initializations {{{1 */
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

/* Public methods {{{1 */
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
