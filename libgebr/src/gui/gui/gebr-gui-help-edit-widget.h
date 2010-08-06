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

#ifndef __GEBR_GUI_HELP_EDIT_WIDGET__
#define __GEBR_GUI_HELP_EDIT_WIDGET__

#include <gtk/gtk.h>
#include <webkit/webkit.h>


G_BEGIN_DECLS


#define GEBR_GUI_TYPE_HELP_EDIT_WIDGET			(gebr_gui_help_edit_widget_get_type())
#define GEBR_GUI_HELP_EDIT_WIDGET(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_GUI_TYPE_HELP_EDIT_WIDGET, GebrGuiHelpEditWidget))
#define GEBR_GUI_HELP_EDIT_WIDGET_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_GUI_TYPE_HELP_EDIT_WIDGET, GebrGuiHelpEditWidgetClass))
#define GEBR_GUI_IS_HELP_EDIT_WIDGET(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_GUI_TYPE_HELP_EDIT_WIDGET))
#define GEBR_GUI_IS_HELP_EDIT_WIDGET_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_GUI_TYPE_HELP_EDIT_WIDGET))
#define GEBR_GUI_HELP_EDIT_WIDGET_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_GUI_TYPE_HELP_EDIT_WIDGET, GebrGuiHelpEditWidgetClass))


typedef struct _GebrGuiHelpEditWidget GebrGuiHelpEditWidget;
typedef struct _GebrGuiHelpEditWidgetClass GebrGuiHelpEditWidgetClass;

struct _GebrGuiHelpEditWidget {
	GtkVBox parent;
};

struct _GebrGuiHelpEditWidgetClass {
	GtkVBoxClass parent_class;

	/* Abstract methods */
	gchar * (*get_content) (GebrGuiHelpEditWidget * self);
	void (*set_content) (GebrGuiHelpEditWidget * self, const gchar * content);
	void (*commit_changes) (GebrGuiHelpEditWidget * self);
};

GType gebr_gui_help_edit_widget_get_type(void) G_GNUC_CONST;

/**
 * gebr_gui_help_edit_widget_set_editing:
 *
 * Sets the editor into edit mode or preview mode based on the value of @editing parameter.
 *
 * @editing: Whether to edit or preview the help.
 */
void gebr_gui_help_edit_widget_set_editing(GebrGuiHelpEditWidget * self, gboolean editing);

/**
 * gebr_gui_help_edit_widget_commit_changes:
 *
 * Commit changes done in the Help until now.
 */
void gebr_gui_help_edit_widget_commit_changes(GebrGuiHelpEditWidget * self);

/**
 * gebr_gui_help_edit_widget_get_content:
 *
 * Fetches the current state of the help edition.
 *
 * Returns: A newly allocated string, with the edited content of this help edit widget.
 */
gchar * gebr_gui_help_edit_widget_get_content(GebrGuiHelpEditWidget * self);

/**
 * gebr_gui_help_edit_widget_set_content:
 *
 * Sets the content for this help edition session.
 *
 * @content: The new content for this help edition.
 */
void gebr_gui_help_edit_widget_set_content(GebrGuiHelpEditWidget * self, const gchar * content);

/**
 * gebr_gui_help_edit_widget_get_web_view:
 * This is a convenience function for classes extending this abstract class.
 * That means you should not use this method if you are not extending it!
 *
 * Returns: The #WebKitWebView used to show the help edition.
 */
GtkWidget * gebr_gui_help_edit_widget_get_web_view(GebrGuiHelpEditWidget * self);

/**
 * gebr_gui_help_edit_widget_get_js_context:
 * This is a convenience function for classes extending this abstract class.
 * That means you should not use this method if you are not extending it!
 *
 * Returns: The #JSContextRef from the #WebKitWebView of this help edit widget.
 */
JSContextRef gebr_gui_help_edit_widget_get_js_context(GebrGuiHelpEditWidget * self);

G_END_DECLS

#endif /* __GEBR_GUI_HELP_EDIT_WIDGET__ */
