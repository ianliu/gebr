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

#include "../intl.h"

#include "gebr-gui-html-viewer-window.h"
#include "gebr-gui-html-viewer-widget.h"
#include "gebr-gui-save-dialog.h"
#include "gebr-gui-utils.h"

#include <glib.h>

typedef struct _GebrGuiHtmlViewerWindowPrivate GebrGuiHtmlViewerWindowPrivate;

struct _GebrGuiHtmlViewerWindowPrivate {
	GtkWidget *viewer_widget;
	GtkUIManager * manager;
};

#define GEBR_GUI_HTML_VIEWER_WINDOW_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE((o), GEBR_GUI_TYPE_HTML_VIEWER_WINDOW, GebrGuiHtmlViewerWindowPrivate))

//==============================================================================
// PROTOTYPES								       =
//==============================================================================
static void gebr_gui_html_viewer_window_finalize (GObject *object);

static void on_save_activate (GtkAction * action, GebrGuiHtmlViewerWindow * self);

static void on_print_activate (GtkAction * action, GebrGuiHtmlViewerWindow * self);

static void on_quit_activate (GtkAction * action, GebrGuiHtmlViewerWindow * self);

G_DEFINE_TYPE(GebrGuiHtmlViewerWindow, gebr_gui_html_viewer_window, GTK_TYPE_WINDOW);

static GtkActionEntry actions[] = {
	{"FileAction", NULL, N_("_File")},

	{"SaveAction", GTK_STOCK_SAVE, NULL, NULL,
		N_("Save content into file"), G_CALLBACK (on_save_activate)},
	{"PrintAction", GTK_STOCK_PRINT, NULL, NULL,
		N_("Print content"), G_CALLBACK (on_print_activate)},
	{"QuitAction", GTK_STOCK_QUIT, NULL, NULL,
		N_("Quits this window"), G_CALLBACK (on_quit_activate)}
};

static guint n_actions = G_N_ELEMENTS (actions);

static gchar * uidef =
"<ui>"
" <menubar name='menubar'>"
"  <menu action='FileAction'>"
"   <menuitem action='SaveAction' />"
"   <separator />"
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
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = gebr_gui_html_viewer_window_finalize;

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
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), priv->viewer_widget, TRUE, TRUE, 0);

	gtk_window_set_default_size (GTK_WINDOW (self), 800, 600);
	gtk_window_add_accel_group (GTK_WINDOW (self), gtk_ui_manager_get_accel_group (priv->manager));
	gtk_container_add (GTK_CONTAINER (self), vbox);

	gtk_widget_show_all (menubar);
	gtk_widget_show (priv->viewer_widget);
	gtk_widget_show (vbox);
}

static void gebr_gui_html_viewer_window_finalize (GObject *object)
{
	GebrGuiHtmlViewerWindowPrivate *priv;

	priv = GEBR_GUI_HTML_VIEWER_WINDOW_GET_PRIVATE (object);

	g_object_unref (priv->manager);

	G_OBJECT_CLASS (gebr_gui_html_viewer_window_parent_class)->finalize (object);
}


//==============================================================================
// PRIVATE FUNCTIONS							       =
//==============================================================================
static void on_save_activate (GtkAction *action, GebrGuiHtmlViewerWindow * self)
{
	GebrGuiHtmlViewerWindowPrivate *priv;
	FILE *file;
	GtkWidget *dialog;
	const gchar *content;
	gchar *path;

	priv = GEBR_GUI_HTML_VIEWER_WINDOW_GET_PRIVATE (self);
	dialog = gebr_gui_save_dialog_new (_("Choose file to save content"), GTK_WINDOW (self));
	path = gebr_gui_save_dialog_run (GEBR_GUI_SAVE_DIALOG (dialog));

	if (!path)
		return;

	file = fopen (path, "w");
	if (!file) {
		gebr_gui_message_dialog (GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
					 _("Could not save file"),
					 _("You do not have the permissions necessary to save the file %s."
					   "Check that you choose the location correctly."),
					 path);
		return;
	}

	content = gebr_gui_html_viewer_widget_get_html (GEBR_GUI_HTML_VIEWER_WIDGET (priv->viewer_widget));
	fputs (content, file);
	g_free (path);
	fclose (file);
}

static void on_print_activate (GtkAction *action, GebrGuiHtmlViewerWindow * self)
{
	GebrGuiHtmlViewerWindowPrivate * priv;

	priv = GEBR_GUI_HTML_VIEWER_WINDOW_GET_PRIVATE(self);

	gebr_gui_html_viewer_widget_print(GEBR_GUI_HTML_VIEWER_WIDGET(priv->viewer_widget));
}

static void on_quit_activate (GtkAction *action, GebrGuiHtmlViewerWindow * self)
{
	gtk_widget_destroy (GTK_WIDGET (self));
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
