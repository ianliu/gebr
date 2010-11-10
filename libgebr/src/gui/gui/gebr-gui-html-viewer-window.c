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
	GtkUIManager * manager;
};

#define GEBR_GUI_HTML_VIEWER_WINDOW_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE((o), GEBR_GUI_TYPE_HTML_VIEWER_WINDOW, GebrGuiHtmlViewerWindowPrivate))

//==============================================================================
// PROTOTYPES								       =
//==============================================================================

static void gebr_gui_html_viewer_window_response(GtkDialog *object, gint response_id);

static void on_print_clicked(GtkWidget * printer_item, GebrGuiHtmlViewerWindow * self);

static void on_quit_clicked(GtkWidget * quit_item, GebrGuiHtmlViewerWindow * self);

G_DEFINE_TYPE(GebrGuiHtmlViewerWindow, gebr_gui_html_viewer_window, GTK_TYPE_DIALOG);

static GtkActionEntry actions[] = {
        {"FileAction", NULL, N_("_File")},
        {"PrintAction", GTK_STOCK_PRINT, NULL, NULL,
                N_("Print content"), G_CALLBACK (on_print_clicked)},
        {"QuitAction", GTK_STOCK_CLOSE, NULL, NULL,
                N_("Quit this window"), G_CALLBACK (on_quit_clicked)}
};

static guint n_actions = G_N_ELEMENTS (actions);

static gchar * uidef =
"<ui>"
" <menubar name='menubar'>"
"  <menu action='FileAction'>"
"   <menuitem action='PrintAction' />"
"   <separator />"
"   <menuitem action='QuitAction' />"
"  </menu>"
" </menubar>"
"</ui>";

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
        GtkActionGroup *group;
        GtkWidget *vbox;
        GtkWidget *menubar;
        GError *error = NULL;

        priv = GEBR_GUI_HTML_VIEWER_WINDOW_GET_PRIVATE(self);

        priv->viewer_widget = gebr_gui_html_viewer_widget_new();
        priv->manager = gtk_ui_manager_new ();

        group = gtk_action_group_new ("Actions");
        gtk_action_group_add_actions (group, actions, n_actions, self);
        gtk_ui_manager_insert_action_group (priv->manager, group, -1);
        gtk_ui_manager_add_ui_from_string (priv->manager, uidef, -1, &error);
        g_object_unref (group);

        if (error) {
                g_warning ("%s", error->message);
                g_clear_error (&error);
        }

        menubar = gtk_ui_manager_get_widget (priv->manager, "/menubar");
        vbox = GTK_DIALOG(self)->vbox;
        gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), priv->viewer_widget, TRUE, TRUE, 0);

        gtk_window_set_default_size (GTK_WINDOW (self), 800, 600);
        gtk_window_add_accel_group (GTK_WINDOW (self), gtk_ui_manager_get_accel_group (priv->manager));

        gtk_widget_show_all (menubar);
        gtk_widget_show (priv->viewer_widget);
        gtk_widget_show (vbox);
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
