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

#include "../../intl.h"

#include "gebr-gui-help-edit-window.h"

#include <glib.h>

enum {
	PROP_0,
	PROP_HAS_REFRESH,
	PROP_HELP_EDIT_WIDGET,
};

enum {
	REFRESH_REQUESTED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

typedef struct _GebrGuiHelpEditWindowPrivate GebrGuiHelpEditWindowPrivate;

struct _GebrGuiHelpEditWindowPrivate {
	gboolean has_refresh;
	GtkWidget * help_edit_widget;
};

#define GEBR_GUI_HELP_EDIT_WINDOW_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE((o), GEBR_GUI_TYPE_HELP_EDIT_WINDOW, GebrGuiHelpEditWindowPrivate))

//==============================================================================
// PROTOTYPES								       =
//==============================================================================

static void gebr_gui_help_edit_window_constructed(GObject * self);

static void gebr_gui_help_edit_window_set_property	(GObject	*object,
							 guint		 prop_id,
							 const GValue	*value,
							 GParamSpec	*pspec);
static void gebr_gui_help_edit_window_get_property	(GObject	*object,
							 guint		 prop_id,
							 GValue		*value,
							 GParamSpec	*pspec);

static void gebr_gui_help_edit_window_destroy(GtkObject *object);

void on_save_clicked(GtkToolButton * button, GebrGuiHelpEditWindow * self);

void on_edit_toggled(GtkToggleToolButton * button, GebrGuiHelpEditWindow * self);

void on_refresh_clicked(GtkToolButton * button, GebrGuiHelpEditWindow * self);

G_DEFINE_TYPE(GebrGuiHelpEditWindow, gebr_gui_help_edit_window, GTK_TYPE_DIALOG);

//==============================================================================
// GOBJECT RELATED FUNCTIONS						       =
//==============================================================================

static void gebr_gui_help_edit_window_class_init(GebrGuiHelpEditWindowClass * klass)
{
	GObjectClass *gobject_class;
	GtkObjectClass *object_class;

	gobject_class = G_OBJECT_CLASS(klass);
	object_class = GTK_OBJECT_CLASS(klass);
	gobject_class->constructed = gebr_gui_help_edit_window_constructed;
	gobject_class->set_property = gebr_gui_help_edit_window_set_property;
	gobject_class->get_property = gebr_gui_help_edit_window_get_property;
	object_class->destroy = gebr_gui_help_edit_window_destroy;

	/**
	 * GebrGuiHelpEditWindow:has-refresh:
	 */
	g_object_class_install_property(gobject_class,
					PROP_HAS_REFRESH,
					g_param_spec_boolean("has-refresh",
							     "Has refresh",
							     "Whether to show the refresh button or not",
							     FALSE,
							     G_PARAM_READWRITE));

	/**
	 * GebrGuiHelpEditWindow:help-edit-widget:
	 * A #GebrGuiHelpEditWidget that will be packed into this window.
	 */
	g_object_class_install_property(gobject_class,
					PROP_HELP_EDIT_WIDGET,
					g_param_spec_pointer("help-edit-widget",
							     "Help Edit Widget",
							     "The GebrGuiHelpEdit widget",
							     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	/**
	 * GebrGuiHelpEditWindow::refresh-requested:
	 * @widget: The help edit window.
	 * @help: A #GString containing the help content.
	 *
	 * Emitted when the user presses the Refresh button in this window. You may modify @help string as you wish.
	 * When the callback returns, the modified string is set into the help edition widget.
	 */
	signals[REFRESH_REQUESTED] =
		g_signal_new("refresh-requested",
			     GEBR_GUI_TYPE_HELP_EDIT_WINDOW,
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrGuiHelpEditWindowClass, refresh_requested),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__POINTER,
			     G_TYPE_NONE,
			     1,
			     G_TYPE_GSTRING);

	g_type_class_add_private(klass, sizeof(GebrGuiHelpEditWindowPrivate));
}

static void gebr_gui_help_edit_window_constructed(GObject * self)
{
	GebrGuiHelpEditWindowPrivate * private;
	GtkWidget * vbox;
	GtkWidget * toolbar;
	GtkToolItem * item;

	private = GEBR_GUI_HELP_EDIT_WINDOW_GET_PRIVATE(self);

	vbox = GTK_DIALOG(self)->vbox;
	toolbar = gtk_toolbar_new();

	// Commit button
	item = gtk_tool_button_new_from_stock(GTK_STOCK_SAVE);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
	g_signal_connect(item, "clicked", G_CALLBACK(on_save_clicked), self);

	// Edit button
	item = gtk_toggle_tool_button_new_from_stock(GTK_STOCK_EDIT);
	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(item), TRUE);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
	g_signal_connect(item, "toggled", G_CALLBACK(on_edit_toggled), self);

	if (private->has_refresh) {
		// Refresh button
		item = gtk_tool_button_new_from_stock(GTK_STOCK_REFRESH);
		gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
		g_signal_connect(item, "clicked", G_CALLBACK(on_refresh_clicked), self);
	}

	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), private->help_edit_widget, TRUE, TRUE, 0);
	gtk_widget_show_all(toolbar);
	gtk_widget_show(private->help_edit_widget);
}

static void gebr_gui_help_edit_window_init(GebrGuiHelpEditWindow * self)
{
}

static void gebr_gui_help_edit_window_set_property(GObject	*object,
						   guint	 prop_id,
						   const GValue	*value,
						   GParamSpec	*pspec)
{
	GebrGuiHelpEditWindowPrivate * priv = GEBR_GUI_HELP_EDIT_WINDOW_GET_PRIVATE(object);

	switch (prop_id) {
	case PROP_HAS_REFRESH:
		priv->has_refresh = g_value_get_boolean(value);
		break;
	case PROP_HELP_EDIT_WIDGET:
		priv->help_edit_widget = g_value_get_pointer(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void gebr_gui_help_edit_window_get_property(GObject	*object,
						   guint	 prop_id,
						   GValue	*value,
						   GParamSpec	*pspec)
{
	GebrGuiHelpEditWindowPrivate * priv = GEBR_GUI_HELP_EDIT_WINDOW_GET_PRIVATE(object);

	switch (prop_id) {
	case PROP_HAS_REFRESH:
		g_value_set_boolean(value, priv->has_refresh);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void gebr_gui_help_edit_window_destroy(GtkObject *object)
{
}

//==============================================================================
// PRIVATE FUNCTIONS							       =
//==============================================================================
void on_save_clicked(GtkToolButton * button, GebrGuiHelpEditWindow * self)
{
	GebrGuiHelpEditWindowPrivate * priv;
	GebrGuiHelpEditWidget * help_edit_widget;

	priv = GEBR_GUI_HELP_EDIT_WINDOW_GET_PRIVATE(self);
	help_edit_widget = GEBR_GUI_HELP_EDIT_WIDGET(priv->help_edit_widget);
	gebr_gui_help_edit_widget_commit_changes(help_edit_widget);
}

void on_edit_toggled(GtkToggleToolButton * button, GebrGuiHelpEditWindow * self)
{
	GebrGuiHelpEditWindowPrivate * priv;
	GebrGuiHelpEditWidget * help_edit_widget;
	gboolean toggled;

	priv = GEBR_GUI_HELP_EDIT_WINDOW_GET_PRIVATE(self);
	help_edit_widget = GEBR_GUI_HELP_EDIT_WIDGET(priv->help_edit_widget);
	toggled = gtk_toggle_tool_button_get_active(button);
	gebr_gui_help_edit_widget_set_editing(help_edit_widget, toggled);
}

void on_refresh_clicked(GtkToolButton * button, GebrGuiHelpEditWindow * self)
{
	GebrGuiHelpEditWindowPrivate * priv;
	GebrGuiHelpEditWidget * help_edit_widget;
	GString * content_string;
	gchar * content;

	priv = GEBR_GUI_HELP_EDIT_WINDOW_GET_PRIVATE(self);
	help_edit_widget = GEBR_GUI_HELP_EDIT_WIDGET(priv->help_edit_widget);
	content = gebr_gui_help_edit_widget_get_content(help_edit_widget);
	content_string = g_string_new(content);
	g_free(content);

	g_signal_emit(self, signals[REFRESH_REQUESTED], 0, content_string);

	// After the signal callback returns, content_string should be updated.
	gebr_gui_help_edit_widget_set_content(help_edit_widget, content_string->str);
	g_string_free(content_string, TRUE);
}

//==============================================================================
// PUBLIC FUNCTIONS							       =
//==============================================================================

GtkWidget *gebr_gui_help_edit_window_new(GebrGuiHelpEditWidget * help_edit_widget)
{
	return g_object_new(GEBR_GUI_TYPE_HELP_EDIT_WINDOW,
			    "help-edit-widget", help_edit_widget,
			    "has-separator", FALSE,
			    NULL);
}

GtkWidget *gebr_gui_help_edit_window_new_with_refresh(GebrGuiHelpEditWidget * help_edit_widget)
{
	return g_object_new(GEBR_GUI_TYPE_HELP_EDIT_WINDOW,
			    "help-edit-widget", help_edit_widget,
			    "has-refresh", TRUE,
			    "has-separator", FALSE,
			    NULL);
}
