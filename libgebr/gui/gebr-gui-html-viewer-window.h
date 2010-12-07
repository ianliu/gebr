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

#ifndef __GEBR_GUI_HTML_VIEWER_WINDOW__
#define __GEBR_GUI_HTML_VIEWER_WINDOW__

#include <gtk/gtk.h>
#include <geoxml.h>
#include "gebr-gui-html-viewer-widget.h"

G_BEGIN_DECLS


#define GEBR_GUI_TYPE_HTML_VIEWER_WINDOW		(gebr_gui_html_viewer_window_get_type())
#define GEBR_GUI_HTML_VIEWER_WINDOW(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_GUI_TYPE_HTML_VIEWER_WINDOW, GebrGuiHtmlViewerWindow))
#define GEBR_GUI_HTML_VIEWER_WINDOW_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_GUI_TYPE_HTML_VIEWER_WINDOW, GebrGuiHtmlViewerWindowClass))
#define GEBR_GUI_IS_HTML_VIEWER_WINDOW(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_GUI_TYPE_HTML_VIEWER_WINDOW))
#define GEBR_GUI_IS_HTML_VIEWER_WINDOW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_GUI_TYPE_HTML_VIEWER_WINDOW))
#define GEBR_GUI_HTML_VIEWER_WINDOW_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_GUI_TYPE_HTML_VIEWER_WINDOW, GebrGuiHtmlViewerWindowClass))


#define GEBR_GUI_HTML_VIEWER_WINDOW_MENU_BAR "menubar"

typedef struct _GebrGuiHtmlViewerWindow GebrGuiHtmlViewerWindow;
typedef struct _GebrGuiHtmlViewerWindowClass GebrGuiHtmlViewerWindowClass;

struct _GebrGuiHtmlViewerWindow {
	GtkWindow parent;
};

struct _GebrGuiHtmlViewerWindowClass {
	GtkWindowClass parent_class;
};

GType gebr_gui_html_viewer_window_get_type(void) G_GNUC_CONST;

/**
 * gebr_gui_html_viewer_window_new:
 *
 * Creates a new html viewer.
 */
GtkWidget * gebr_gui_html_viewer_window_new();

/**
 * gebr_gui_html_viewer_window_show_html:
 *
 * Show the html content
 */
void gebr_gui_html_viewer_window_show_html(GebrGuiHtmlViewerWindow * self, const gchar * content);

/**
 * gebr_gui_html_viewer_window_get_widget:
 *
 * Get widget
 */
GebrGuiHtmlViewerWidget* gebr_gui_html_viewer_window_get_widget(GebrGuiHtmlViewerWindow * self);

/**
 * gebr_gui_html_viewer_window_set_custom_tab:
 * @self: A #GebrGuiHtmlViewerWindow
 * @label: Name the new custom tab
 * @widget: The widget to attach to the print dialog, as the only custom tab
 *
 * a proxy to create the custom print tab
 */
void gebr_gui_html_viewer_window_set_custom_tab (GebrGuiHtmlViewerWindow * self,
						 const gchar * label,
						 GebrGuiHtmlViewerCustomTab callback);

/**
 * gebr_gui_html_viewer_window_get_ui_manager:
 * @self: A #GebrGuiHtmlViewerWindow
 *
 * You can get this ui manager to customize this window's interface.
 *
 * Returns: the #GtkUIManager this window is using.
 */
GtkUIManager *gebr_gui_html_viewer_window_get_ui_manager (GebrGuiHtmlViewerWindow * self);

G_END_DECLS

#endif /* __GEBR_GUI_HTML_VIEWER_WINDOW__ */
