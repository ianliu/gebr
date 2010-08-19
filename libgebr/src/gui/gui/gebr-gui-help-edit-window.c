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

#include <glib.h>

#include "../../intl.h"

#include "gebr-gui-help-edit-window.h"

enum {
	PROP_0,
	PROP_HAS_REFRESH,
	PROP_HELP_EDIT_WIDGET,
	PROP_MENU_BAR,
};

enum {
	REFRESH_REQUESTED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

typedef struct _GebrGuiHelpEditWindowPrivate GebrGuiHelpEditWindowPrivate;

struct _GebrGuiHelpEditWindowPrivate {
	gboolean has_refresh;
	GtkWidget * menu_bar;
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

static void on_save_clicked(GtkToolButton * button, GebrGuiHelpEditWindow * self);

static void on_edit_toggled(GtkToggleToolButton * button, GebrGuiHelpEditWindow * self);

static void on_refresh_clicked(GtkToolButton * button, GebrGuiHelpEditWindow * self);

static void on_print_clicked(GtkToolButton * button, GebrGuiHelpEditWindow * self);

static gboolean gebr_gui_help_edit_window_delete_event(GtkWidget * self, GdkEventAny * event);

G_DEFINE_TYPE(GebrGuiHelpEditWindow, gebr_gui_help_edit_window, GTK_TYPE_WINDOW);

//==============================================================================
// GOBJECT RELATED FUNCTIONS						       =
//==============================================================================

static void gebr_gui_help_edit_window_class_init(GebrGuiHelpEditWindowClass * klass)
{
	GObjectClass *gobject_class;
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	gobject_class = G_OBJECT_CLASS(klass);
	object_class = GTK_OBJECT_CLASS(klass);
	widget_class = GTK_WIDGET_CLASS(klass);
	gobject_class->constructed = gebr_gui_help_edit_window_constructed;
	gobject_class->set_property = gebr_gui_help_edit_window_set_property;
	gobject_class->get_property = gebr_gui_help_edit_window_get_property;
	object_class->destroy = gebr_gui_help_edit_window_destroy;
	widget_class->delete_event = gebr_gui_help_edit_window_delete_event;

	/**
	 * GebrGuiHelpEditWindow:has-refresh:
	 */
	g_object_class_install_property(gobject_class,
					PROP_HAS_REFRESH,
					g_param_spec_boolean("has-refresh",
							     "Has refresh",
							     "Whether to show the refresh button or not",
							     FALSE,
							     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

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
	 * GebrGuiHelpEditWindow:menu-bar:
	 * A #GebrGuiHelpEditWidget that will be packed into this window.
	 */
	g_object_class_install_property(gobject_class,
					PROP_MENU_BAR,
					g_param_spec_pointer("menu-bar",
							     "GtkMenuBar",
							     "A GtkMenuBar for this window",
							     G_PARAM_READWRITE));

	/**
	 * GebrGuiHelpEditWindow::refresh-requested:
	 * @widget: The help edit window.
	 *
	 * Emitted when the user presses the Refresh button in this window.
	 */
	signals[REFRESH_REQUESTED] =
		g_signal_new("refresh-requested",
			     GEBR_GUI_TYPE_HELP_EDIT_WINDOW,
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrGuiHelpEditWindowClass, refresh_requested),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__VOID,
			     G_TYPE_NONE,
			     0);

	g_type_class_add_private(klass, sizeof(GebrGuiHelpEditWindowPrivate));
}

static void gebr_gui_help_edit_window_constructed(GObject * self)
{
	GebrGuiHelpEditWindowPrivate * private;
	GtkWidget * vbox;
	GtkWidget * toolbar;
	GtkToolItem * item;

	private = GEBR_GUI_HELP_EDIT_WINDOW_GET_PRIVATE(self);
	vbox = gtk_vbox_new(FALSE, 0);
	toolbar = gtk_toolbar_new();
	gtk_container_add(GTK_CONTAINER(self), vbox);

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

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);

	// Print button
	item = gtk_tool_button_new_from_stock(GTK_STOCK_PRINT);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
	g_signal_connect(item, "clicked", G_CALLBACK(on_print_clicked), self);

	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), private->help_edit_widget, TRUE, TRUE, 0);
	gtk_widget_show(vbox);
	gtk_widget_show(private->help_edit_widget);
	gtk_widget_show_all(toolbar);
}

static void gebr_gui_help_edit_window_init(GebrGuiHelpEditWindow * self)
{
	GebrGuiHelpEditWindowPrivate * priv;
       
	priv = GEBR_GUI_HELP_EDIT_WINDOW_GET_PRIVATE(self);
	priv->menu_bar = NULL;
}

static void gebr_gui_help_edit_window_set_property(GObject	*object,
						   guint	 prop_id,
						   const GValue	*value,
						   GParamSpec	*pspec)
{
	GebrGuiHelpEditWindow * self = GEBR_GUI_HELP_EDIT_WINDOW(object);
	GebrGuiHelpEditWindowPrivate * priv = GEBR_GUI_HELP_EDIT_WINDOW_GET_PRIVATE(object);

	switch (prop_id) {
	case PROP_HAS_REFRESH:
		priv->has_refresh = g_value_get_boolean(value);
		break;
	case PROP_HELP_EDIT_WIDGET:
		priv->help_edit_widget = g_value_get_pointer(value);
		break;
	case PROP_MENU_BAR:
		gebr_gui_help_edit_window_set_menu_bar(self, g_value_get_pointer(value));
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
	case PROP_HELP_EDIT_WIDGET:
		g_value_set_pointer(value, priv->help_edit_widget);
		break;
	case PROP_MENU_BAR:
		g_value_set_pointer(value, priv->menu_bar);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

//==============================================================================
// PRIVATE FUNCTIONS							       =
//==============================================================================
static void on_save_clicked(GtkToolButton * button, GebrGuiHelpEditWindow * self)
{
	GebrGuiHelpEditWindowPrivate * priv;
	GebrGuiHelpEditWidget * help_edit_widget;

	priv = GEBR_GUI_HELP_EDIT_WINDOW_GET_PRIVATE(self);
	help_edit_widget = GEBR_GUI_HELP_EDIT_WIDGET(priv->help_edit_widget);
	gebr_gui_help_edit_widget_commit_changes(help_edit_widget);
}

static void on_edit_toggled(GtkToggleToolButton * button, GebrGuiHelpEditWindow * self)
{
	GebrGuiHelpEditWindowPrivate * priv;
	GebrGuiHelpEditWidget * help_edit_widget;
	gboolean toggled;

	priv = GEBR_GUI_HELP_EDIT_WINDOW_GET_PRIVATE(self);
	help_edit_widget = GEBR_GUI_HELP_EDIT_WIDGET(priv->help_edit_widget);
	toggled = gtk_toggle_tool_button_get_active(button);
	gebr_gui_help_edit_widget_set_editing(help_edit_widget, toggled);
}

static void on_refresh_clicked(GtkToolButton * button, GebrGuiHelpEditWindow * self)
{
	g_signal_emit(self, signals[REFRESH_REQUESTED], 0);
}

static void on_print_clicked(GtkToolButton * button, GebrGuiHelpEditWindow * self)
{
	GebrGuiHelpEditWindowPrivate * private;
	GebrGuiHelpEditWidget * help_edit;
	GebrGuiHtmlViewerWidget * html_viewer;

	private = GEBR_GUI_HELP_EDIT_WINDOW_GET_PRIVATE(self);
	help_edit = GEBR_GUI_HELP_EDIT_WIDGET(private->help_edit_widget);
	html_viewer = gebr_gui_help_edit_widget_get_html_viewer(help_edit);

	gebr_gui_html_viewer_widget_print(html_viewer);
}

static gint confirmation_dialog(GebrGuiHelpEditWindow * self)
{
	gint response;
	GtkWidget * dialog;

	dialog = gtk_message_dialog_new_with_markup(GTK_WINDOW(self),
						    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						    GTK_MESSAGE_WARNING,
						    GTK_BUTTONS_NONE,
						    _("<span size='larger' weight='bold'>There are unsaved changes."
						      " Do you want to save them now?</span>"));

	gtk_dialog_add_buttons(GTK_DIALOG(dialog),
			       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			       GTK_STOCK_DISCARD, GTK_RESPONSE_REJECT,
			       GTK_STOCK_SAVE, GTK_RESPONSE_OK,
			       NULL);

	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
						 _("If you do not save the changes, they will be permanently lost."));

	response = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	return response;
}

static void gebr_gui_help_edit_window_destroy(GtkObject *object)
{
}

static gboolean gebr_gui_help_edit_window_quit_real(GebrGuiHelpEditWindow * self)
{
	gint response;
	GebrGuiHelpEditWidget * help_edit_widget;
	GebrGuiHelpEditWindowPrivate * private;

	private = GEBR_GUI_HELP_EDIT_WINDOW_GET_PRIVATE(self);
	help_edit_widget = GEBR_GUI_HELP_EDIT_WIDGET(private->help_edit_widget);

	if (gebr_gui_help_edit_widget_is_content_saved(help_edit_widget))
		return FALSE;

	// Return TRUE to maintain the window alive, FALSE to destroy it.
	response = confirmation_dialog(self);
	switch(response) {
	case GTK_RESPONSE_OK:
		gebr_gui_help_edit_widget_commit_changes(help_edit_widget);
		return FALSE;

	case GTK_RESPONSE_REJECT:
		return FALSE;

	default:
		return TRUE;
	}
}

static gboolean gebr_gui_help_edit_window_delete_event(GtkWidget * widget, GdkEventAny * event)
{
	GebrGuiHelpEditWindow * self;
	self = GEBR_GUI_HELP_EDIT_WINDOW(widget);
	return gebr_gui_help_edit_window_quit_real(self);
}

//==============================================================================
// PUBLIC FUNCTIONS							       =
//==============================================================================

GtkWidget *gebr_gui_help_edit_window_new(GebrGuiHelpEditWidget * help_edit_widget)
{
	g_return_val_if_fail(GEBR_GUI_IS_HELP_EDIT_WIDGET(help_edit_widget), NULL);

	return g_object_new(GEBR_GUI_TYPE_HELP_EDIT_WINDOW,
			    "help-edit-widget", help_edit_widget,
			    NULL);
}

GtkWidget *gebr_gui_help_edit_window_new_with_refresh(GebrGuiHelpEditWidget * help_edit_widget)
{
	g_return_val_if_fail(GEBR_GUI_IS_HELP_EDIT_WIDGET(help_edit_widget), NULL);

	return g_object_new(GEBR_GUI_TYPE_HELP_EDIT_WINDOW,
			    "help-edit-widget", help_edit_widget,
			    "has-refresh", TRUE,
			    NULL);
}

void gebr_gui_help_edit_window_set_menu_bar(GebrGuiHelpEditWindow * self, GtkMenuBar * menu_bar)
{
	GtkWidget * vbox;
	GebrGuiHelpEditWindowPrivate * priv;

	g_return_if_fail(GEBR_GUI_IS_HELP_EDIT_WINDOW(self));

	priv = GEBR_GUI_HELP_EDIT_WINDOW_GET_PRIVATE(self);
	vbox = gtk_bin_get_child(GTK_BIN(self));

	if (priv->menu_bar != NULL)
		gtk_container_remove(GTK_CONTAINER(vbox), priv->menu_bar);

	if (menu_bar != NULL) {
		gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(menu_bar), FALSE, TRUE, 0);
		gtk_box_reorder_child(GTK_BOX(vbox), GTK_WIDGET(menu_bar), 0);
	}
}

void gebr_gui_help_edit_window_quit(GebrGuiHelpEditWindow * self)
{
	if (!gebr_gui_help_edit_window_quit_real(self))
		gtk_widget_destroy(GTK_WIDGET(self));
}
