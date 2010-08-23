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

/**
 * SECTION:gebr-gui-help-edit-window
 * @short_description: A window presenting a toolbar with actions for the help edit widget
 * @title: GebrGuiHelpEditWindow class
 * @see_also: #GebrGuiHelpEditWidget
 * @include: libgebr/gui/gebr-gui-help-edit-window.h
 * @image: gebr-gui-help-edit-window.png
 *
 * This widget packs a #GebrGuiHelpEditWidget for editing a help
 * string. It may or may not have a Refresh button, depending on which
 * constructor you use to instanciate this class.
 */

#ifndef __GEBR_GUI_HELP_EDIT_WINDOW__
#define __GEBR_GUI_HELP_EDIT_WINDOW__

#include <gtk/gtk.h>
#include "gebr-gui-help-edit-widget.h"


G_BEGIN_DECLS


#define GEBR_GUI_TYPE_HELP_EDIT_WINDOW			(gebr_gui_help_edit_window_get_type())
#define GEBR_GUI_HELP_EDIT_WINDOW(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_GUI_TYPE_HELP_EDIT_WINDOW, GebrGuiHelpEditWindow))
#define GEBR_GUI_HELP_EDIT_WINDOW_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_GUI_TYPE_HELP_EDIT_WINDOW, GebrGuiHelpEditWindowClass))
#define GEBR_GUI_IS_HELP_EDIT_WINDOW(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_GUI_TYPE_HELP_EDIT_WINDOW))
#define GEBR_GUI_IS_HELP_EDIT_WINDOW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_GUI_TYPE_HELP_EDIT_WINDOW))
#define GEBR_GUI_HELP_EDIT_WINDOW_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_GUI_TYPE_HELP_EDIT_WINDOW, GebrGuiHelpEditWindowClass))


typedef struct _GebrGuiHelpEditWindow GebrGuiHelpEditWindow;
typedef struct _GebrGuiHelpEditWindowClass GebrGuiHelpEditWindowClass;

struct _GebrGuiHelpEditWindow {
	GtkWindow parent;
};

struct _GebrGuiHelpEditWindowClass {
	GtkWindowClass parent_class;

	/* Signals */
	void (*refresh_requested) (GebrGuiHelpEditWindow *self, GString *help);
};

GType gebr_gui_help_edit_window_get_type(void) G_GNUC_CONST;

/**
 * gebr_gui_help_edit_window_new:
 * @help_edit_widget: A #GebrGuiHelpEditWidget to perform the help edition.
 *
 * Creates a new window containing a tool bar with buttons for commiting
 * changes and toggling between preview and edit mode. You do not need
 * to destroy the widget on #GtkObject::destroy signal, since this is
 * already done.
 *
 * Returns: A new #GebrGuiHelpEditWindow containing a tool bar with
 * buttons for commiting changes and toggling between preview and edit
 * mode.
 */
GtkWidget *gebr_gui_help_edit_window_new(GebrGuiHelpEditWidget * help_edit_widget);

/**
 * gebr_gui_help_edit_window_new_with_refresh:
 * @help_edit_widget: A #GebrGuiHelpEditWidget to perform the help edition.
 *
 * Creates a new window containing a tool bar with buttons for commiting
 * changes, toggling between preview and edit mode and refreshing
 * content.
 *
 * Returns: A new #GebrGuiHelpEditWindow containing a tool bar with
 * buttons for commiting changes, toggling between preview and edit mode
 * and refreshing the editor content.
 */
GtkWidget *gebr_gui_help_edit_window_new_with_refresh(GebrGuiHelpEditWidget * help_edit_widget);

/**
 * gebr_gui_help_edit_window_set_menu_bar:
 * @window: a #GebrGuiHelpEditWindow
 * @menubar: a #GtkMenuBar to be placed on top of the window.
 *
 * Sets the menu bar for this window. If there is already a menu bar, it is removed from the interface and replaced with
 * @menu_bar. Notice that by removing the old menu will destroy it unless you g_object_ref() it before this operation.
 * See gtk_container_remove() for more information on widget removal.
 */
void gebr_gui_help_edit_window_set_menu_bar(GebrGuiHelpEditWindow * self, GtkMenuBar * menu_bar);

/**
 * gebr_gui_help_edit_window_quit:
 * @window: a #GebrGuiHelpEditWindow
 *
 * Closes @window, showing a message dialog if the content has uncommitted changes.
 */
void gebr_gui_help_edit_window_quit(GebrGuiHelpEditWindow * self);

G_END_DECLS

#endif /* __GEBR_GUI_HELP_EDIT_WINDOW__ */
