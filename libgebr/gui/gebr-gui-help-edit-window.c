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

#include "../intl.h"

#include "gebr-gui-help-edit-window.h"

enum {
	PROP_0,
	PROP_HELP_EDIT_WIDGET,
	PROP_AUTO_SAVE,
	PROP_HAS_MENU_BAR,
};

typedef struct _GebrGuiHelpEditWindowPrivate GebrGuiHelpEditWindowPrivate;

struct _GebrGuiHelpEditWindowPrivate {
	gboolean auto_save;
	gboolean has_menu_bar;

	GtkUIManager * ui_manager;
	GtkWidget * action_area;
	GtkWidget * help_edit_widget;
};

#define GEBR_GUI_HELP_EDIT_WINDOW_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE((o), GEBR_GUI_TYPE_HELP_EDIT_WINDOW, GebrGuiHelpEditWindowPrivate))

//==============================================================================
// PROTOTYPES AND STATIC VARIABLES					       =
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

static void gebr_gui_help_edit_window_dispose(GObject * object);

static void on_preview_toggled(GtkToggleAction * button, GebrGuiHelpEditWindow * self);

static void on_print_clicked(GtkAction * action, GebrGuiHelpEditWindow * self);

static gboolean gebr_gui_help_edit_window_delete_event(GtkWidget * widget, GdkEventAny * event);

static gboolean gebr_gui_help_edit_window_focus_out_event(GtkWidget * widget, GdkEventFocus * event);

static void on_quit_clicked(GtkAction * action, GebrGuiHelpEditWindow * self);

G_DEFINE_TYPE(GebrGuiHelpEditWindow, gebr_gui_help_edit_window, GTK_TYPE_WINDOW);

static const GtkActionEntry action_entries[] = {
	{"FileMenu", NULL, N_("_File")},
	{"EditMenu", NULL, N_("_Edit")},

	{"PrintAction", GTK_STOCK_PRINT, NULL, NULL,
		N_("Prints the content of this window"), G_CALLBACK(on_print_clicked)},
	{"QuitAction", GTK_STOCK_QUIT, NULL, NULL,
		N_("Quits the window"), G_CALLBACK(on_quit_clicked)},
};

static const guint n_action_entries = G_N_ELEMENTS(action_entries);

static const GtkToggleActionEntry toggle_entries[] = {
	{"PreviewAction", GTK_STOCK_PRINT_PREVIEW, N_("Preview"), NULL,
		N_("Toggles between edit and preview modes"), G_CALLBACK(on_preview_toggled), FALSE},
};

static const guint n_toggle_entries = G_N_ELEMENTS(toggle_entries);

static const gchar * ui_def =
"<ui>"
" <menubar name='" GEBR_GUI_HELP_EDIT_WINDOW_MENU_BAR_NAME "'>"
"  <menu action='FileMenu'>"
"    <separator />"
"    <menuitem action='PrintAction' />"
"    <separator />"
"    <menuitem action='QuitAction' />"
"  </menu>"
"  <menu action='EditMenu'>"
"   <menuitem action='PreviewAction' />"
"  </menu>"
"  <placeholder name='" GEBR_GUI_HELP_EDIT_WINDOW_MENU_BAR_MARK "' />"
" </menubar>"
" <toolbar name='" GEBR_GUI_HELP_EDIT_WINDOW_TOOL_BAR_NAME "'>"
"  <placeholder name='" GEBR_GUI_HELP_EDIT_WINDOW_TOOL_BAR_MARK "'>"
"   <separator />"
"   <toolitem action='PreviewAction' />"
"   <separator />"
"   <toolitem action='PrintAction' />"
"   <separator />"
"  </placeholder>"
" </toolbar>"
"</ui>";

//==============================================================================
// GOBJECT RELATED FUNCTIONS						       =
//==============================================================================

static void gebr_gui_help_edit_window_class_init(GebrGuiHelpEditWindowClass * klass)
{
	GObjectClass *gobject_class;
	GtkWidgetClass *widget_class;

	gobject_class = G_OBJECT_CLASS(klass);
	widget_class = GTK_WIDGET_CLASS(klass);
	gobject_class->constructed = gebr_gui_help_edit_window_constructed;
	gobject_class->set_property = gebr_gui_help_edit_window_set_property;
	gobject_class->get_property = gebr_gui_help_edit_window_get_property;
	gobject_class->dispose = gebr_gui_help_edit_window_dispose;
	widget_class->delete_event = gebr_gui_help_edit_window_delete_event;
	widget_class->focus_out_event = gebr_gui_help_edit_window_focus_out_event;

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
	 * GebrGuiHelpEditWindow:auto-save:
	 */
	g_object_class_install_property(gobject_class,
					PROP_AUTO_SAVE,
					g_param_spec_boolean("auto-save",
							     "Auto save",
							     "Whether to automatic save",
							     FALSE,
							     G_PARAM_READWRITE));

	/**
	 * GebrGuiHelpEditWindow:has-menu-bar:
	 */
	g_object_class_install_property(gobject_class,
					PROP_HAS_MENU_BAR,
					g_param_spec_boolean("has-menu-bar",
							     "Has GtkMenuBar",
							     "Whether to show the menu bar",
							     TRUE,
							     G_PARAM_READWRITE));

	g_type_class_add_private(klass, sizeof(GebrGuiHelpEditWindowPrivate));
}

static void gebr_gui_help_edit_window_constructed(GObject * self)
{
	GebrGuiHelpEditWindowPrivate * priv;
	priv = GEBR_GUI_HELP_EDIT_WINDOW_GET_PRIVATE(self);
	gtk_box_pack_start(GTK_BOX(priv->action_area), priv->help_edit_widget, TRUE, TRUE, 0);
	gtk_widget_show(priv->help_edit_widget);
}

static void gebr_gui_help_edit_window_init(GebrGuiHelpEditWindow * self)
{
	GebrGuiHelpEditWindowPrivate * priv;
	GtkActionGroup * action_group;
	GtkAccelGroup * accel_group;
	GtkWidget * menu_bar;
	GtkWidget * tool_bar;
	GError * error = NULL;

	const gchar * menu_bar_path;
	const gchar * tool_bar_path;
       
	priv = GEBR_GUI_HELP_EDIT_WINDOW_GET_PRIVATE(self);
	priv->action_area      = gtk_vbox_new(FALSE, 0);
	priv->auto_save        = FALSE;
	priv->has_menu_bar     = FALSE;
	priv->help_edit_widget = NULL;
	priv->ui_manager       = gtk_ui_manager_new();

	action_group = gtk_action_group_new("HelpEditWindowGroup");
	gtk_action_group_add_actions(action_group, action_entries, n_action_entries, self);
	gtk_action_group_add_toggle_actions(action_group, toggle_entries, n_toggle_entries, self);
	gtk_ui_manager_insert_action_group(priv->ui_manager, action_group, 0);
	gtk_ui_manager_add_ui_from_string(priv->ui_manager, ui_def, -1, &error);
	g_object_unref(action_group);

	if (error != NULL) {
		g_warning("%s\n", error->message);
		g_clear_error(&error);
	}

	accel_group = gtk_ui_manager_get_accel_group(priv->ui_manager);
	gtk_window_add_accel_group(GTK_WINDOW(self), accel_group);

	menu_bar_path = gebr_gui_help_edit_window_get_menu_bar_path(self);
	tool_bar_path = gebr_gui_help_edit_window_get_tool_bar_path(self);
	menu_bar = gtk_ui_manager_get_widget(priv->ui_manager, menu_bar_path);
	tool_bar = gtk_ui_manager_get_widget(priv->ui_manager, tool_bar_path);

	gtk_box_pack_start(GTK_BOX(priv->action_area), menu_bar, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(priv->action_area), tool_bar, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(self), priv->action_area);
	gtk_widget_show(priv->action_area);
	gtk_widget_show(tool_bar);
}

static void gebr_gui_help_edit_window_set_property(GObject	*object,
						   guint	 prop_id,
						   const GValue	*value,
						   GParamSpec	*pspec)
{
	GebrGuiHelpEditWindow * self;
	GebrGuiHelpEditWindowPrivate * priv;

	self = GEBR_GUI_HELP_EDIT_WINDOW(object);
	priv = GEBR_GUI_HELP_EDIT_WINDOW_GET_PRIVATE(object);

	switch (prop_id) {
	case PROP_HELP_EDIT_WIDGET:
		priv->help_edit_widget = g_value_get_pointer(value);
		break;
	case PROP_AUTO_SAVE:
		gebr_gui_help_edit_window_set_auto_save(self, g_value_get_boolean(value));
		break;
	case PROP_HAS_MENU_BAR:
		gebr_gui_help_edit_window_set_has_menu_bar(self, g_value_get_boolean(value));
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
	case PROP_HELP_EDIT_WIDGET:
		g_value_set_pointer(value, priv->help_edit_widget);
		break;
	case PROP_AUTO_SAVE:
		g_value_set_boolean(value, priv->auto_save);
		break;
	case PROP_HAS_MENU_BAR:
		g_value_set_boolean(value, priv->has_menu_bar);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void gebr_gui_help_edit_window_dispose(GObject * object)
{
	GebrGuiHelpEditWindowPrivate * priv;
	priv = GEBR_GUI_HELP_EDIT_WINDOW_GET_PRIVATE(object);

	if (priv->ui_manager != NULL) {
		g_object_unref(priv->ui_manager);
		priv->ui_manager = NULL;
	}

	G_OBJECT_CLASS(gebr_gui_help_edit_window_parent_class)->dispose(object);
}

//==============================================================================
// PRIVATE FUNCTIONS							       =
//==============================================================================
static void on_preview_toggled(GtkToggleAction * action, GebrGuiHelpEditWindow * self)
{
	gboolean is_editing;
	GebrGuiHelpEditWindowPrivate * priv;
	GebrGuiHelpEditWidget * help_edit_widget;

	priv = GEBR_GUI_HELP_EDIT_WINDOW_GET_PRIVATE(self);
	help_edit_widget = GEBR_GUI_HELP_EDIT_WIDGET(priv->help_edit_widget);
	is_editing = !gtk_toggle_action_get_active(action);
	gebr_gui_help_edit_widget_set_editing(help_edit_widget, is_editing);
}

static void on_print_clicked(GtkAction * action, GebrGuiHelpEditWindow * self)
{
	gboolean editing;
	GebrGuiHelpEditWindowPrivate * private;
	GebrGuiHelpEditWidget * help_edit;
	GebrGuiHtmlViewerWidget * html_viewer;

	private = GEBR_GUI_HELP_EDIT_WINDOW_GET_PRIVATE(self);
	help_edit = GEBR_GUI_HELP_EDIT_WIDGET(private->help_edit_widget);
	html_viewer = gebr_gui_help_edit_widget_get_html_viewer(help_edit);

	g_object_get(help_edit, "editing", &editing, NULL);

	/* If we are in edit mode, refresh the Html Viewer content */
	if (editing) {
		gchar * content;
		content = gebr_gui_help_edit_widget_get_content(help_edit);
		gebr_gui_html_viewer_widget_show_html(html_viewer, content);
		g_free(content);
	}

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

static gboolean gebr_gui_help_edit_window_quit_real(GebrGuiHelpEditWindow * self)
{
	gint response;
	GebrGuiHelpEditWidget * help_edit_widget;
	GebrGuiHelpEditWindowPrivate * priv;

	priv = GEBR_GUI_HELP_EDIT_WINDOW_GET_PRIVATE(self);
	help_edit_widget = GEBR_GUI_HELP_EDIT_WIDGET(priv->help_edit_widget);

	// Return TRUE to maintain the window alive, FALSE to destroy it.
	if (gebr_gui_help_edit_widget_is_content_saved(help_edit_widget))
		return FALSE;

	if (priv->auto_save) {
		gebr_gui_help_edit_widget_commit_changes(help_edit_widget);
		return FALSE;
	} 

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

static gboolean gebr_gui_help_edit_window_focus_out_event(GtkWidget * widget, GdkEventFocus * event)
{
	GebrGuiHelpEditWindow * self;
	GebrGuiHelpEditWindowPrivate * priv;
	GebrGuiHelpEditWidget * help_edit_widget;

	self = GEBR_GUI_HELP_EDIT_WINDOW(widget);
	priv = GEBR_GUI_HELP_EDIT_WINDOW_GET_PRIVATE(self);
	help_edit_widget = GEBR_GUI_HELP_EDIT_WIDGET(priv->help_edit_widget);
	
	// Don't call is_content_saved before checking auto_save, since
	// is_content_saved might be expensive!
	if (priv->auto_save && !gebr_gui_help_edit_widget_is_content_saved(help_edit_widget))
		gebr_gui_help_edit_widget_commit_changes(help_edit_widget);

	return FALSE;
}

static void on_quit_clicked(GtkAction * action, GebrGuiHelpEditWindow * self)
{
	gebr_gui_help_edit_window_quit(self);
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

gboolean gebr_gui_help_edit_window_quit(GebrGuiHelpEditWindow * self)
{
	if (!gebr_gui_help_edit_window_quit_real(self)) {
		gtk_widget_destroy(GTK_WIDGET(self));
		return TRUE;
	}

	return FALSE;
}

void gebr_gui_help_edit_window_set_has_menu_bar(GebrGuiHelpEditWindow * self, gboolean has_menu_bar)
{
	GtkWidget * menu_bar;
	const gchar * menu_bar_path;
	GebrGuiHelpEditWindowPrivate * priv;

	priv = GEBR_GUI_HELP_EDIT_WINDOW_GET_PRIVATE(self);
	priv->has_menu_bar = has_menu_bar;
	menu_bar_path = gebr_gui_help_edit_window_get_menu_bar_path(self);
	menu_bar = gtk_ui_manager_get_widget(priv->ui_manager, menu_bar_path);

	if (has_menu_bar)
		gtk_widget_show(menu_bar);
	else
		gtk_widget_hide(menu_bar);
}

void gebr_gui_help_edit_window_set_auto_save(GebrGuiHelpEditWindow * self, gboolean auto_save)
{
	GebrGuiHelpEditWindowPrivate * priv;
	priv = GEBR_GUI_HELP_EDIT_WINDOW_GET_PRIVATE(self);
	priv->auto_save = auto_save;
}

GtkUIManager * gebr_gui_help_edit_window_get_ui_manager(GebrGuiHelpEditWindow * self)
{
	GebrGuiHelpEditWindowPrivate * priv;
	priv = GEBR_GUI_HELP_EDIT_WINDOW_GET_PRIVATE(self);
	return priv->ui_manager;
}

/*
 * UI Manager paths accessors
 */

const gchar * gebr_gui_help_edit_window_get_menu_bar_path(GebrGuiHelpEditWindow * self)
{
	return "/" GEBR_GUI_HELP_EDIT_WINDOW_MENU_BAR_NAME;
}

const gchar * gebr_gui_help_edit_window_get_file_menu_path(GebrGuiHelpEditWindow * self)
{
	return "/" GEBR_GUI_HELP_EDIT_WINDOW_MENU_BAR_NAME "/FileMenu";
}

const gchar * gebr_gui_help_edit_window_get_edit_menu_path(GebrGuiHelpEditWindow * self)
{
	return "/" GEBR_GUI_HELP_EDIT_WINDOW_MENU_BAR_NAME "/EditMenu";
}

const gchar * gebr_gui_help_edit_window_get_menu_mark(GebrGuiHelpEditWindow * self)
{
	return "/" GEBR_GUI_HELP_EDIT_WINDOW_MENU_BAR_NAME "/" GEBR_GUI_HELP_EDIT_WINDOW_MENU_BAR_MARK;
}

const gchar * gebr_gui_help_edit_window_get_tool_bar_path(GebrGuiHelpEditWindow * self)
{
	return "/" GEBR_GUI_HELP_EDIT_WINDOW_TOOL_BAR_NAME;
}

const gchar * gebr_gui_help_edit_window_get_tool_bar_mark(GebrGuiHelpEditWindow * self)
{
	return "/" GEBR_GUI_HELP_EDIT_WINDOW_TOOL_BAR_NAME "/" GEBR_GUI_HELP_EDIT_WINDOW_TOOL_BAR_MARK;
}

