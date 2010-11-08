/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebr_guiproject/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or (at your option) any later version.
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

#ifndef __GEBR_GUI_HTML_VIEWER_WIDGET__
#define __GEBR_GUI_HTML_VIEWER_WIDGET__

#include <gtk/gtk.h>
#include <geoxml.h>

G_BEGIN_DECLS


#define GEBR_GUI_TYPE_HTML_VIEWER_WIDGET		(gebr_gui_html_viewer_widget_get_type())
#define GEBR_GUI_HTML_VIEWER_WIDGET(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_GUI_TYPE_HTML_VIEWER_WIDGET, GebrGuiHtmlViewerWidget))
#define GEBR_GUI_HTML_VIEWER_WIDGET_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_GUI_TYPE_HTML_VIEWER_WIDGET, GebrGuiHtmlViewerWidgetClass))
#define GEBR_GUI_IS_HTML_VIEWER_WIDGET(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_GUI_TYPE_HTML_VIEWER_WIDGET))
#define GEBR_GUI_IS_HTML_VIEWER_WIDGET_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_GUI_TYPE_HTML_VIEWER_WIDGET))
#define GEBR_GUI_HTML_VIEWER_WIDGET_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_GUI_TYPE_HTML_VIEWER_WIDGET, GebrGuiHtmlViewerWidgetClass))


typedef struct _GebrGuiHtmlViewerWidget GebrGuiHtmlViewerWidget;
typedef struct _GebrGuiHtmlViewerWidgetClass GebrGuiHtmlViewerWidgetClass;

struct _GebrGuiHtmlViewerWidget {
	GtkVBox parent;
};

struct _GebrGuiHtmlViewerWidgetClass {
	GtkVBoxClass parent_class;

	/* Signals */
	void (*title_ready) (GebrGuiHtmlViewerWidget * self, const gchar * title);
};

GType gebr_gui_html_viewer_widget_get_type(void) G_GNUC_CONST;

/**
 * gebr_gui_html_viewer_widget_new:
 *
 * Creates a new html viewer widget.
 */
GtkWidget * gebr_gui_html_viewer_widget_new();

/**
 * gebr_gui_html_viewer_widget_print:
 *
 * Prints the content of a html viewer widget
 */
void gebr_gui_html_viewer_widget_print(GebrGuiHtmlViewerWidget * self);

/**
 * gebr_gui_html_viewer_widget_show_html:
 *
 * Show the html content
 */
void gebr_gui_html_viewer_widget_show_html(GebrGuiHtmlViewerWidget * self, const gchar * content);

/**
 * gebr_gui_html_viewer_widget_generate_links:
 * @widget: The HTML viewer widget
 * @object: A #GebrGeoXmlObject for generating the links
 *
 * Schedules the generation of the links for @object, which happens when the next call to
 * gebr_gui_html_viewer_widget_show_html() is done.
 */
void gebr_gui_html_viewer_widget_generate_links(GebrGuiHtmlViewerWidget *self, GebrGeoXmlObject * object);

/**
 * gebr_gui_html_viewer_widget_get_related_object:
 *
 * Returns: the #GebrGeoXmlObject associated with the help being shown by this widget. If the help shown has nothing to
 * do with any object, than NULL is returned.
 */
const GebrGeoXmlObject *
gebr_gui_html_viewer_widget_get_related_object (GebrGuiHtmlViewerWidget *self);

G_END_DECLS

#endif /* __GEBR_GUI_HTML_VIEWER_WIDGET__ */
