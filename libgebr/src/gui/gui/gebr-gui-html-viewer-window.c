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

typedef struct _GebrGuiHtmlViewerWindowPrivate GebrGuiHtmlViewerWindowPrivate;

struct _GebrGuiHtmlViewerWindowPrivate {
	GtkWidget * viewer_widget;
};

#define GEBR_GUI_HTML_VIEWER_WINDOW_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE((o), GEBR_GUI_TYPE_HTML_VIEWER_WINDOW, GebrGuiHtmlViewerWindowPrivate))

//==============================================================================
// PROTOTYPES								       =
//==============================================================================

static void gebr_gui_html_viewer_window_response(GtkDialog *object, gint response_id);

static void on_print_clicked(GtkWidget * printer_item, GebrGuiHtmlViewerWindow * self);

static void on_quit_clicked(GtkWidget * quit_item, GebrGuiHtmlViewerWindow * self);

static void on_title_ready(GebrGuiHtmlViewerWidget * widget, const gchar * title, GebrGuiHtmlViewerWindow * self);

G_DEFINE_TYPE(GebrGuiHtmlViewerWindow, gebr_gui_html_viewer_window, GTK_TYPE_DIALOG);

//==============================================================================
// GOBJECT RELATED FUNCTIONS						       =
//==============================================================================

static void gebr_gui_html_viewer_window_class_init(GebrGuiHtmlViewerWindowClass * klass)
{
	GtkDialogClass *dialog_class;

	dialog_class = GTK_DIALOG_CLASS(klass);
	dialog_class->response = gebr_gui_html_viewer_window_response;

	g_type_class_add_private(klass, sizeof(GebrGuiHtmlViewerWindowPrivate));
}

static void gebr_gui_html_viewer_window_init(GebrGuiHtmlViewerWindow * self)
{
	GebrGuiHtmlViewerWindowPrivate * priv;
	GtkDialog * dialog;
	GtkWidget *menu_bar;
	GtkWidget *file_item;
	GtkWidget *print_item;
	GtkWidget *quit_item;
	GtkWidget *file_menu;

	priv = GEBR_GUI_HTML_VIEWER_WINDOW_GET_PRIVATE(self);
	priv->viewer_widget = gebr_gui_html_viewer_widget_new();

	g_signal_connect (priv->viewer_widget, "title-ready",
			  G_CALLBACK (on_title_ready),
			  self);

	dialog = GTK_DIALOG(self);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 800, 600);

	gtk_widget_show(priv->viewer_widget);

	file_menu = gtk_menu_new ();    /* Don't need to show menus */

	/* Create the menu items */
	print_item = gtk_menu_item_new_with_label (_("Print"));
	quit_item = gtk_menu_item_new_with_label (_("Quit"));

	/* Add them to the menu */
	gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), print_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (file_menu), quit_item);

	/* Attach the callback functions to the activate signal */
	g_signal_connect (G_OBJECT (print_item), "activate",
			  G_CALLBACK (on_print_clicked),
			  self);

	/* We can attach the Quit menu item to our exit function */
	g_signal_connect (G_OBJECT (quit_item), "activate",
			  G_CALLBACK (on_quit_clicked),
			  self);

	/* We do need to show menu items */
	gtk_widget_show (print_item);
	gtk_widget_show (quit_item);

	menu_bar = gtk_menu_bar_new ();
	gtk_box_pack_start (GTK_BOX (dialog->vbox), menu_bar, FALSE, TRUE, 0);
	gtk_widget_show (menu_bar);

	file_item = gtk_menu_item_new_with_label (_("File"));
	gtk_widget_show (file_item);

	gtk_menu_item_set_submenu (GTK_MENU_ITEM (file_item), file_menu);

	gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), file_item);

	gtk_box_pack_start(GTK_BOX(dialog->vbox), priv->viewer_widget, TRUE, TRUE, 0);

}

//==============================================================================
// PRIVATE FUNCTIONS							       =
//==============================================================================
static void gebr_gui_html_viewer_window_response(GtkDialog *object, gint response_id)
{
	gtk_widget_destroy(GTK_WIDGET(object));
}

static void on_print_clicked(GtkWidget * print_item, GebrGuiHtmlViewerWindow * self)
{
	GebrGuiHtmlViewerWindowPrivate * private;
	private = GEBR_GUI_HTML_VIEWER_WINDOW_GET_PRIVATE(self);
	gebr_gui_html_viewer_widget_print(GEBR_GUI_HTML_VIEWER_WIDGET(private->viewer_widget));
}

static void on_quit_clicked(GtkWidget * quit_item, GebrGuiHtmlViewerWindow * self)
{
	GebrGuiHtmlViewerWindowPrivate * private;
	private = GEBR_GUI_HTML_VIEWER_WINDOW_GET_PRIVATE(self);

	gtk_widget_destroy(GTK_WIDGET(self));
}

static void on_title_ready(GebrGuiHtmlViewerWidget * widget, const gchar * title, GebrGuiHtmlViewerWindow * self)
{
	gtk_window_set_title(GTK_WINDOW (self), title);
}

//==============================================================================
// PUBLIC FUNCTIONS							       =
//==============================================================================
GtkWidget *gebr_gui_html_viewer_window_new()
{
	return g_object_new(GEBR_GUI_TYPE_HTML_VIEWER_WINDOW, NULL);
}

void gebr_gui_html_viewer_window_show_html(GebrGuiHtmlViewerWindow * self, const gchar * content)
{
	g_return_if_fail(GEBR_GUI_IS_HTML_VIEWER_WINDOW(self));

	GebrGuiHtmlViewerWindowPrivate * priv = GEBR_GUI_HTML_VIEWER_WINDOW_GET_PRIVATE(self);
	gebr_gui_html_viewer_widget_show_html(GEBR_GUI_HTML_VIEWER_WIDGET(priv->viewer_widget), content);
}

GebrGuiHtmlViewerWidget* gebr_gui_html_viewer_window_get_widget(GebrGuiHtmlViewerWindow * self)
{
	g_return_val_if_fail(GEBR_GUI_IS_HTML_VIEWER_WINDOW(self), NULL);

	GebrGuiHtmlViewerWindowPrivate * priv = GEBR_GUI_HTML_VIEWER_WINDOW_GET_PRIVATE(self);
	return GEBR_GUI_HTML_VIEWER_WIDGET(priv->viewer_widget);
}

void gebr_gui_html_viewer_window_set_custom_tab(GebrGuiHtmlViewerWindow * self, const gchar * label, GebrGuiHtmlViewerCustomTab callback)
{
	g_return_if_fail(GEBR_GUI_IS_HTML_VIEWER_WINDOW(self));

	GebrGuiHtmlViewerWindowPrivate * priv = GEBR_GUI_HTML_VIEWER_WINDOW_GET_PRIVATE(self);
	gebr_gui_html_viewer_widget_set_custom_tab(GEBR_GUI_HTML_VIEWER_WIDGET(priv->viewer_widget), label, callback);
}
