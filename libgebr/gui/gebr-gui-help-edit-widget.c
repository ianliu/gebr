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
#include "gebr-gui-help-edit-widget.h"
#include "gebr-gui-html-viewer-widget.h"
#include "gebr-gui-utils.h"

enum {
	PROP_0,
	PROP_EDITING,
	PROP_SAVED
};

enum {
	COMMIT_REQUEST,
	CONTENT_LOADED,
	LAST_SIGNAL
};

enum {
	STATE_INIT,
	STATE_LOADING,
	STATE_LOADED
};

static guint signals[LAST_SIGNAL] = { 0 };

typedef struct _GebrGuiHelpEditWidgetPrivate GebrGuiHelpEditWidgetPrivate;

struct _GebrGuiHelpEditWidgetPrivate {
	guint state;
	gboolean is_editing;
	GtkWidget * edit_widget;
	GtkWidget * html_viewer;
	GtkWidget * scrolled_window;
};

#define GEBR_GUI_HELP_EDIT_WIDGET_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE((o), GEBR_GUI_TYPE_HELP_EDIT_WIDGET, GebrGuiHelpEditWidgetPrivate))

//==============================================================================
// PROTOTYPES								       =
//==============================================================================

static void gebr_gui_help_edit_widget_set_property	(GObject	*object,
							 guint		 prop_id,
							 const GValue	*value,
							 GParamSpec	*pspec);
static void gebr_gui_help_edit_widget_get_property	(GObject	*object,
							 guint		 prop_id,
							 GValue		*value,
							 GParamSpec	*pspec);

static void on_load_finished(WebKitWebView * view, WebKitWebFrame * frame, GebrGuiHelpEditWidget * self);

static WebKitNavigationResponse on_navigation_requested(WebKitWebView * web_view, WebKitWebFrame *frame,
							WebKitNetworkRequest *request, GebrGuiHelpEditWidget *self);

G_DEFINE_ABSTRACT_TYPE(GebrGuiHelpEditWidget, gebr_gui_help_edit_widget, GTK_TYPE_VBOX);

//==============================================================================
// GOBJECT RELATED FUNCTIONS						       =
//==============================================================================

static void gebr_gui_help_edit_widget_class_init(GebrGuiHelpEditWidgetClass * klass)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->set_property = gebr_gui_help_edit_widget_set_property;
	gobject_class->get_property = gebr_gui_help_edit_widget_get_property;

	/**
	 * GebrGuiHelpEditWidget:editing:
	 * You can set this property to change the state of the widget. If %TRUE, the widget enters editing mode, if
	 * %FALSE, the widget enters preview mode.
	 */
	g_object_class_install_property(gobject_class,
					PROP_EDITING,
					g_param_spec_boolean("editing",
							     "Editing",
							     "Whether editing the help or previewing it",
							     TRUE,
							     G_PARAM_READWRITE));

	/**
	 * GebrGuiHelpEditWidget:saved:
	 * This is a read only property which states if the content is saved or not.
	 */
	g_object_class_install_property(gobject_class,
					PROP_SAVED,
					g_param_spec_boolean("saved",
							     "Saved",
							     "Whether the help is saved or not",
							     FALSE,
							     G_PARAM_READABLE));

	/**
	 * GebrGuiHelpEditWidget::commit-request:
	 * Emitted when 'commit_changes' is called.
	 */
	signals[ COMMIT_REQUEST ] =
		g_signal_new("commit-request",
			     GEBR_GUI_TYPE_HELP_EDIT_WIDGET,
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GebrGuiHelpEditWidgetClass, commit_request),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__VOID,
			     G_TYPE_NONE,
			     0);

	/**
	 * GebrGuiHelpEditWidget::content-loaded:
	 * Emitted after a gebr_gui_help_edit_widget_set_content(), when it is fully loaded, but before JavaScripts
	 * calls.
	 */
	signals[ CONTENT_LOADED ] =
		g_signal_new ("content-loaded",
			      GEBR_GUI_TYPE_HELP_EDIT_WIDGET,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GebrGuiHelpEditWidgetClass, content_loaded),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);

	g_type_class_add_private(klass, sizeof(GebrGuiHelpEditWidgetPrivate));
}

static void gebr_gui_help_edit_widget_init(GebrGuiHelpEditWidget * self)
{
	GtkBox * box;
	GebrGuiHelpEditWidgetPrivate * priv;

	priv = GEBR_GUI_HELP_EDIT_WIDGET_GET_PRIVATE (self);
	priv->state = STATE_INIT;
	priv->edit_widget = webkit_web_view_new();
	priv->html_viewer = gebr_gui_html_viewer_widget_new();
	priv->is_editing = TRUE;

#if WEBKIT_CHECK_VERSION(1,1,16)
	WebKitWebSettings *settings;
	settings = webkit_web_view_get_settings (WEBKIT_WEB_VIEW (priv->edit_widget));
	g_object_set (settings, "enable-dom-paste", TRUE);
#endif

	/* a reasonable minimum size, considering the toolbar */
	gtk_widget_set_size_request(priv->edit_widget, 800, -1);

	box = GTK_BOX(self);
	priv->scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(priv->scrolled_window),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(priv->scrolled_window), priv->edit_widget);

	gtk_box_pack_start(box, priv->scrolled_window, TRUE, TRUE, 0);
	gtk_box_pack_start(box, priv->html_viewer, TRUE, TRUE, 0);
	gtk_widget_hide(priv->html_viewer);
	gtk_widget_show_all(priv->scrolled_window);
}

static void gebr_gui_help_edit_widget_set_property(GObject	*object,
						   guint	 prop_id,
						   const GValue	*value,
						   GParamSpec	*pspec)
{
	GebrGuiHelpEditWidget * self;

	self = GEBR_GUI_HELP_EDIT_WIDGET(object);

	switch (prop_id) {
	case PROP_EDITING:
		gebr_gui_help_edit_widget_set_editing(self, g_value_get_boolean(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void gebr_gui_help_edit_widget_get_property(GObject	*object,
						   guint	 prop_id,
						   GValue	*value,
						   GParamSpec	*pspec)
{
	GebrGuiHelpEditWidgetPrivate * priv = GEBR_GUI_HELP_EDIT_WIDGET_GET_PRIVATE(object);
	GebrGuiHelpEditWidget * self = GEBR_GUI_HELP_EDIT_WIDGET(object);

	switch (prop_id) {
	case PROP_EDITING:
		g_value_set_boolean(value, priv->is_editing);
		break;
	case PROP_SAVED:
		g_value_set_boolean(value, gebr_gui_help_edit_widget_is_content_saved(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

//==============================================================================
// PRIVATE FUNCTIONS							       =
//==============================================================================
static void on_load_finished(WebKitWebView * view, WebKitWebFrame * frame, GebrGuiHelpEditWidget * self)
{
	GebrGuiHelpEditWidgetPrivate * priv;

	priv = GEBR_GUI_HELP_EDIT_WIDGET_GET_PRIVATE (self);
	priv->state = STATE_LOADED;

	g_signal_handlers_disconnect_by_func (priv->edit_widget,
					      on_load_finished,
					      self);

	g_signal_connect (priv->edit_widget, "navigation-requested",
			  G_CALLBACK (on_navigation_requested), self);

	g_signal_emit (self, signals[ CONTENT_LOADED ], 0);
}

static WebKitNavigationResponse on_navigation_requested(WebKitWebView * web_view, WebKitWebFrame *frame,
							WebKitNetworkRequest *request, GebrGuiHelpEditWidget *self)
{
	GebrGuiHelpEditWidgetPrivate *priv;
	const gchar *uri;
	const gchar *path;

	priv = GEBR_GUI_HELP_EDIT_WIDGET_GET_PRIVATE (self);
	uri = webkit_network_request_get_uri (request);
	path = gebr_gui_help_edit_widget_get_uri (self);

	if (g_str_has_prefix(uri + strlen("file://"), path) || g_str_has_prefix(uri, "about:"))
		return WEBKIT_NAVIGATION_RESPONSE_ACCEPT;
	
	return WEBKIT_NAVIGATION_RESPONSE_IGNORE;
}
//==============================================================================
// PUBLIC FUNCTIONS							       =
//==============================================================================

void gebr_gui_help_edit_widget_set_editing(GebrGuiHelpEditWidget * self, gboolean editing)
{
	GebrGuiHelpEditWidgetPrivate * priv;
	
	g_return_if_fail (GEBR_GUI_IS_HELP_EDIT_WIDGET (self));

	priv = GEBR_GUI_HELP_EDIT_WIDGET_GET_PRIVATE (self);

	priv->is_editing = editing;

	if (editing) {
		gtk_widget_show(priv->scrolled_window);
		gtk_widget_hide(priv->html_viewer);
	} else {
		gchar * content;
		content = gebr_gui_help_edit_widget_get_content(self);
		gebr_gui_html_viewer_widget_show_html(GEBR_GUI_HTML_VIEWER_WIDGET(priv->html_viewer), content);
		gtk_widget_show(priv->html_viewer);
		gtk_widget_hide(priv->scrolled_window);
		g_free(content);
	}
}

void gebr_gui_help_edit_widget_commit_changes(GebrGuiHelpEditWidget * self)
{
	GebrGuiHelpEditWidgetPrivate * priv;

	g_return_if_fail (GEBR_GUI_IS_HELP_EDIT_WIDGET (self));

	priv = GEBR_GUI_HELP_EDIT_WIDGET_GET_PRIVATE (self);

	if (priv->state == STATE_LOADED) {
		GEBR_GUI_HELP_EDIT_WIDGET_GET_CLASS(self)->commit_changes(self);
		g_signal_emit(self, signals[COMMIT_REQUEST], 0);
	}
}

gchar * gebr_gui_help_edit_widget_get_content(GebrGuiHelpEditWidget * self)
{
	g_return_val_if_fail (GEBR_GUI_IS_HELP_EDIT_WIDGET (self), NULL);

	return GEBR_GUI_HELP_EDIT_WIDGET_GET_CLASS(self)->get_content(self);
}

void gebr_gui_help_edit_widget_set_content(GebrGuiHelpEditWidget * self, const gchar * content)
{
	GebrGuiHelpEditWidgetPrivate * priv;

	g_return_if_fail (GEBR_GUI_IS_HELP_EDIT_WIDGET (self));

	priv = GEBR_GUI_HELP_EDIT_WIDGET_GET_PRIVATE (self);

	if (priv->state == STATE_LOADING)
		return;

	priv->state = STATE_LOADING;

	g_signal_connect (priv->edit_widget, "load-finished",
			  G_CALLBACK (on_load_finished), self);

	GEBR_GUI_HELP_EDIT_WIDGET_GET_CLASS(self)->set_content(self, content);
}

gboolean gebr_gui_help_edit_widget_is_content_saved(GebrGuiHelpEditWidget * self)
{
	g_return_val_if_fail (GEBR_GUI_IS_HELP_EDIT_WIDGET (self), FALSE);

	return GEBR_GUI_HELP_EDIT_WIDGET_GET_CLASS(self)->is_content_saved(self);
}

GtkWidget * gebr_gui_help_edit_widget_get_web_view(GebrGuiHelpEditWidget * self)
{
	GebrGuiHelpEditWidgetPrivate * priv;

	g_return_val_if_fail (GEBR_GUI_IS_HELP_EDIT_WIDGET (self), NULL);

	priv = GEBR_GUI_HELP_EDIT_WIDGET_GET_PRIVATE(self);
	return priv->edit_widget;
}

JSContextRef gebr_gui_help_edit_widget_get_js_context(GebrGuiHelpEditWidget * self)
{
	WebKitWebView * view;
	WebKitWebFrame * frame;
	GebrGuiHelpEditWidgetPrivate * priv;

	g_return_val_if_fail (GEBR_GUI_IS_HELP_EDIT_WIDGET (self), NULL);

	priv = GEBR_GUI_HELP_EDIT_WIDGET_GET_PRIVATE(self);
	view = WEBKIT_WEB_VIEW(priv->edit_widget);
	frame = webkit_web_view_get_main_frame(view);

	return webkit_web_frame_get_global_context(frame);
}

GebrGuiHtmlViewerWidget * gebr_gui_help_edit_widget_get_html_viewer(GebrGuiHelpEditWidget * self)
{
	GebrGuiHelpEditWidgetPrivate * priv;

	g_return_val_if_fail (GEBR_GUI_IS_HELP_EDIT_WIDGET (self), NULL);

	priv = GEBR_GUI_HELP_EDIT_WIDGET_GET_PRIVATE(self);
	return GEBR_GUI_HTML_VIEWER_WIDGET(priv->html_viewer);
}

void gebr_gui_help_edit_widget_set_loaded(GebrGuiHelpEditWidget * self)
{
	GebrGuiHelpEditWidgetPrivate * priv;

	g_return_if_fail (GEBR_GUI_IS_HELP_EDIT_WIDGET (self));

	priv = GEBR_GUI_HELP_EDIT_WIDGET_GET_PRIVATE(self);
	priv->state = STATE_LOADED;
}

const gchar *gebr_gui_help_edit_widget_get_uri (GebrGuiHelpEditWidget * self)
{
	g_return_val_if_fail (GEBR_GUI_IS_HELP_EDIT_WIDGET (self), NULL);

	return GEBR_GUI_HELP_EDIT_WIDGET_GET_CLASS (self)->get_uri (self);
}
