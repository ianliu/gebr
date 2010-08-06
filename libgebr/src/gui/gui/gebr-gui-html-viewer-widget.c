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
#include "../../utils.h"
#include <webkit/webkit.h>

#include "gebr-gui-html-viewer-widget.h"

#include <glib.h>

enum {
	PROP_0
};

typedef struct _GebrGuiHtmlViewerWidgetPrivate GebrGuiHtmlViewerWidgetPrivate;

struct _GebrGuiHtmlViewerWidgetPrivate {
	GtkWidget * web_view;
};

#define GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE((o), GEBR_GUI_TYPE_HTML_VIEWER_WIDGET, GebrGuiHtmlViewerWidgetPrivate))

//==============================================================================
// PROTOTYPES								       =
//==============================================================================

static void gebr_gui_html_viewer_widget_set_property	(GObject	*object,
							 guint		 prop_id,
							 const GValue	*value,
							 GParamSpec	*pspec);
static void gebr_gui_html_viewer_widget_get_property	(GObject	*object,
							 guint		 prop_id,
							 GValue		*value,
							 GParamSpec	*pspec);
static void gebr_gui_html_viewer_widget_destroy(GtkObject *object);

G_DEFINE_TYPE(GebrGuiHtmlViewerWidget, gebr_gui_html_viewer_widget, GTK_TYPE_VBOX);

//==============================================================================
// GOBJECT RELATED FUNCTIONS						       =
//==============================================================================

static void gebr_gui_html_viewer_widget_class_init(GebrGuiHtmlViewerWidgetClass * klass)
{
	GObjectClass *gobject_class;
	GtkObjectClass *object_class;

	gobject_class = G_OBJECT_CLASS(klass);
	object_class = GTK_OBJECT_CLASS(klass);
	gobject_class->set_property = gebr_gui_html_viewer_widget_set_property;
	gobject_class->get_property = gebr_gui_html_viewer_widget_get_property;
	object_class->destroy = gebr_gui_html_viewer_widget_destroy;

	g_type_class_add_private(klass, sizeof(GebrGuiHtmlViewerWidgetPrivate));
}

static void gebr_gui_html_viewer_widget_init(GebrGuiHtmlViewerWidget * self)
{
	GtkWidget * vbox;
	GebrGuiHtmlViewerWidgetPrivate * priv;

	priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);

	priv->web_view = webkit_web_view_new();

	vbox = GTK_WIDGET(self);
	gtk_box_pack_start(GTK_BOX(vbox), priv->web_view, TRUE, TRUE, 0);
	gtk_widget_show(priv->web_view);
}

static void gebr_gui_html_viewer_widget_set_property(GObject		*object,
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

static void gebr_gui_html_viewer_widget_get_property(GObject	*object,
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

static void gebr_gui_html_viewer_widget_destroy(GtkObject *object)
{
}

//==============================================================================
// PRIVATE FUNCTIONS							       =
//==============================================================================

//==============================================================================
// PUBLIC FUNCTIONS							       =
//==============================================================================

GtkWidget *gebr_gui_html_viewer_widget_new(void)
{
	return  g_object_new(GEBR_GUI_TYPE_HTML_VIEWER_WIDGET, NULL);
}

void gebr_gui_html_viewer_widget_print(GebrGuiHtmlViewerWidget * self)
{
	GebrGuiHtmlViewerWidgetPrivate * priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);
	WebKitWebFrame * frame = webkit_web_view_get_main_frame(WEBKIT_WEB_VIEW(priv->web_view));
	webkit_web_frame_print(frame);
}
void gebr_gui_html_viewer_widget_show_html(GebrGuiHtmlViewerWidget * self, const gchar * content)
{

	GebrGuiHtmlViewerWidgetPrivate * priv = GEBR_GUI_HTML_VIEWER_WIDGET_GET_PRIVATE(self);
	GString *_content = g_string_new(content);
	GString *tmp_file;

	tmp_file = gebr_make_temp_filename("XXXXXX.html");
	g_string_assign(_content, tmp_file->str);

	/* some webkit versions crash to open an empty file... */
	if (!_content->len)
		g_string_assign(_content, " ");

	/* write current _content to temporary file */
	FILE *fp;
	fp = fopen(tmp_file->str, "w");
	fputs(_content->str, fp);
	fclose(fp);

	webkit_web_view_open(WEBKIT_WEB_VIEW(priv->web_view), tmp_file->str);

	g_string_free(tmp_file, TRUE);
	g_string_free(_content, TRUE);

}
