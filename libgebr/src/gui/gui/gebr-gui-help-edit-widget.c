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
#include "gebr-gui-help-edit-widget.h"
#include "gebr-gui-html-viewer-widget.h"

enum {
	PROP_0,
	PROP_EDITING,
	PROP_SAVED
};

typedef struct _GebrGuiHelpEditWidgetPrivate GebrGuiHelpEditWidgetPrivate;

struct _GebrGuiHelpEditWidgetPrivate {
	gboolean is_editing;
	gboolean is_saved;
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
static void gebr_gui_help_edit_widget_destroy(GtkObject *object);

G_DEFINE_ABSTRACT_TYPE(GebrGuiHelpEditWidget, gebr_gui_help_edit_widget, GTK_TYPE_VBOX);

//==============================================================================
// GOBJECT RELATED FUNCTIONS						       =
//==============================================================================

static void gebr_gui_help_edit_widget_class_init(GebrGuiHelpEditWidgetClass * klass)
{
	GObjectClass *gobject_class;
	GtkObjectClass *object_class;

	gobject_class = G_OBJECT_CLASS(klass);
	object_class = GTK_OBJECT_CLASS(klass);
	gobject_class->set_property = gebr_gui_help_edit_widget_set_property;
	gobject_class->get_property = gebr_gui_help_edit_widget_get_property;
	object_class->destroy = gebr_gui_help_edit_widget_destroy;

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

	g_type_class_add_private(klass, sizeof(GebrGuiHelpEditWidgetPrivate));
}

static void gebr_gui_help_edit_widget_init(GebrGuiHelpEditWidget * self)
{
	GtkBox * box;
	GebrGuiHelpEditWidgetPrivate * priv;

	priv = GEBR_GUI_HELP_EDIT_WIDGET_GET_PRIVATE(self);
	priv->edit_widget = webkit_web_view_new();
	priv->html_viewer = gebr_gui_html_viewer_widget_new();
	priv->is_editing = TRUE;
	priv->is_saved = FALSE;

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

	switch (prop_id) {
	case PROP_EDITING:
		g_value_set_boolean(value, priv->is_editing);
		break;
	case PROP_SAVED:
		g_value_set_boolean(value, priv->is_saved);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void gebr_gui_help_edit_widget_destroy(GtkObject *object)
{
}

//==============================================================================
// PRIVATE FUNCTIONS							       =
//==============================================================================

//==============================================================================
// PUBLIC FUNCTIONS							       =
//==============================================================================

void gebr_gui_help_edit_widget_set_editing(GebrGuiHelpEditWidget * self, gboolean editing)
{
	GebrGuiHelpEditWidgetPrivate * priv = GEBR_GUI_HELP_EDIT_WIDGET_GET_PRIVATE(self);

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
	GEBR_GUI_HELP_EDIT_WIDGET_GET_CLASS(self)->commit_changes(self);
}

gchar * gebr_gui_help_edit_widget_get_content(GebrGuiHelpEditWidget * self)
{
	return GEBR_GUI_HELP_EDIT_WIDGET_GET_CLASS(self)->get_content(self);
}

void gebr_gui_help_edit_widget_set_content(GebrGuiHelpEditWidget * self, const gchar * content)
{
	GEBR_GUI_HELP_EDIT_WIDGET_GET_CLASS(self)->set_content(self, content);
}

GtkWidget * gebr_gui_help_edit_widget_get_web_view(GebrGuiHelpEditWidget * self)
{
	GebrGuiHelpEditWidgetPrivate * priv;
	priv = GEBR_GUI_HELP_EDIT_WIDGET_GET_PRIVATE(self);
	return priv->edit_widget;
}

JSContextRef gebr_gui_help_edit_widget_get_js_context(GebrGuiHelpEditWidget * self)
{
	WebKitWebView * view;
	WebKitWebFrame * frame;
	GebrGuiHelpEditWidgetPrivate * private;

	private = GEBR_GUI_HELP_EDIT_WIDGET_GET_PRIVATE(self);
	view = WEBKIT_WEB_VIEW(private->edit_widget);
	frame = webkit_web_view_get_main_frame(view);

	return webkit_web_frame_get_global_context(frame);
}
