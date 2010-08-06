/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebr_guiproject/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or * (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see
 *   <http://www.gnu.org/licenses/>.
 */

#include "../../intl.h"

#include "gebr-gui-html-viewer-window.h"
#include "gebr-gui-html-viewer-widget.h"

#include <glib.h>

enum {
	PROP_0
};

typedef struct _GebrGuiHtmlViewerWindowPrivate GebrGuiHtmlViewerWindowPrivate;

struct _GebrGuiHtmlViewerWindowPrivate {
	GtkWidget * viewer_widget;
};

#define GEBR_GUI_HTML_VIEWER_WINDOW_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE((o), GEBR_GUI_TYPE_HTML_VIEWER_WINDOW, GebrGuiHtmlViewerWindowPrivate))

//==============================================================================
// PROTOTYPES								       =
//==============================================================================

static void gebr_gui_html_viewer_window_set_property	(GObject	*object,
							 guint		 prop_id,
							 const GValue	*value,
							 GParamSpec	*pspec);
static void gebr_gui_html_viewer_window_get_property	(GObject	*object,
							 guint		 prop_id,
							 GValue		*value,
							 GParamSpec	*pspec);
static void gebr_gui_html_viewer_window_destroy(GtkObject *object);

void on_print_clicked(GtkToolButton * button, GebrGuiHtmlViewerWindow * self);

G_DEFINE_TYPE(GebrGuiHtmlViewerWindow, gebr_gui_html_viewer_window, GTK_TYPE_WINDOW);

//==============================================================================
// GOBJECT RELATED FUNCTIONS						       =
//==============================================================================

static void gebr_gui_html_viewer_window_class_init(GebrGuiHtmlViewerWindowClass * klass)
{
	GObjectClass *gobject_class;
	GtkObjectClass *object_class;

	gobject_class = G_OBJECT_CLASS(klass);
	object_class = GTK_OBJECT_CLASS(klass);
	gobject_class->set_property = gebr_gui_html_viewer_window_set_property;
	gobject_class->get_property = gebr_gui_html_viewer_window_get_property;
	object_class->destroy = gebr_gui_html_viewer_window_destroy;

	g_type_class_add_private(klass, sizeof(GebrGuiHtmlViewerWindowPrivate));
}

static void gebr_gui_html_viewer_window_init(GebrGuiHtmlViewerWindow * self)
{
	GebrGuiHtmlViewerWindowPrivate * priv;
	GtkWidget * vbox;
	GtkWindow * window;
	GtkWidget * toolbar;
	GtkToolItem * item;

	priv = GEBR_GUI_HTML_VIEWER_WINDOW_GET_PRIVATE(self);
	priv->viewer_widget = gebr_gui_html_viewer_widget_new();

	vbox = gtk_vbox_new(FALSE, 0);
	toolbar = gtk_toolbar_new();
	item = gtk_tool_button_new_from_stock(GTK_STOCK_PRINT);

	g_signal_connect(item, "clicked", G_CALLBACK(on_print_clicked), self);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

	window = GTK_WINDOW(self);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), priv->viewer_widget, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	gtk_widget_show(priv->viewer_widget);
	gtk_widget_show_all(toolbar);
	gtk_widget_show(vbox);
}

static void gebr_gui_html_viewer_window_set_property(GObject		*object,
						     guint		 prop_id,
						     const GValue	*value,
						     GParamSpec		*pspec)
{
	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void gebr_gui_html_viewer_window_get_property(GObject	*object,
						     guint	 prop_id,
						     GValue	*value,
						     GParamSpec	*pspec)
{
	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void gebr_gui_html_viewer_window_destroy(GtkObject *object)
{
}

//==============================================================================
// PRIVATE FUNCTIONS							       =
//==============================================================================
void on_print_clicked(GtkToolButton * button, GebrGuiHtmlViewerWindow * self)
{
	GebrGuiHtmlViewerWindowPrivate * private;
	private = GEBR_GUI_HTML_VIEWER_WINDOW_GET_PRIVATE(self);
	gebr_gui_html_viewer_widget_print(GEBR_GUI_HTML_VIEWER_WIDGET(private->viewer_widget));
}

//==============================================================================
// PUBLIC FUNCTIONS							       =
//==============================================================================

GtkWidget *gebr_gui_html_viewer_window_new(const gchar *title)
{
	return  g_object_new(GEBR_GUI_TYPE_HTML_VIEWER_WINDOW, 
			     "title", title, 
			     NULL);
}

void gebr_gui_html_viewer_window_show_html(GebrGuiHtmlViewerWindow * self, const gchar * content)
{
	GebrGuiHtmlViewerWindowPrivate * priv = GEBR_GUI_HTML_VIEWER_WINDOW_GET_PRIVATE(self);
	gebr_gui_html_viewer_widget_show_html(GEBR_GUI_HTML_VIEWER_WIDGET(priv->viewer_widget), content);
}

void gebr_gui_html_viewer_window_set_geoxml_object(GebrGuiHtmlViewerWindow * self, GebrGeoXmlObject * object)
{
	GebrGuiHtmlViewerWindowPrivate * priv = GEBR_GUI_HTML_VIEWER_WINDOW_GET_PRIVATE(self);
	gebr_gui_html_viewer_widget_set_geoxml_object(GEBR_GUI_HTML_VIEWER_WIDGET(priv->viewer_widget), object);
}
