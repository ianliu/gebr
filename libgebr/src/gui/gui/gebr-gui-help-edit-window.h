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


/*
 * These macros defines the names for the xml ui definition.
 */
#define GEBR_GUI_HELP_EDIT_WINDOW_MENU_BAR_NAME "help_edit_window_menu_bar"
#define GEBR_GUI_HELP_EDIT_WINDOW_MENU_BAR_MARK "help_edit_window_menu_bar_place_holder"
#define GEBR_GUI_HELP_EDIT_WINDOW_TOOL_BAR_NAME "help_edit_window_tool_bar"
#define GEBR_GUI_HELP_EDIT_WINDOW_TOOL_BAR_MARK "help_edit_window_tool_bar_place_holder"


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
 * Creates a new window containing a tool bar and a menu bar, which you can modify
 * using the GtkUIManager.
 */
GtkWidget *gebr_gui_help_edit_window_new(GebrGuiHelpEditWidget * help_edit_widget);

/**
 * gebr_gui_help_edit_window_quit:
 * @window: a #GebrGuiHelpEditWindow
 *
 * Closes @window, showing a message dialog if the content has uncommitted changes.
 */
void gebr_gui_help_edit_window_quit(GebrGuiHelpEditWindow * self);

/**
 * gebr_gui_help_edit_window_set_has_menu_bar:
 * @window:
 * @has_menu_bar:
 */
void gebr_gui_help_edit_window_set_has_menu_bar(GebrGuiHelpEditWindow * self, gboolean has_menu_bar);

/**
 * gebr_gui_help_edit_window_set_auto_save:
 * @window:
 * @auto_save:
 */
void gebr_gui_help_edit_window_set_auto_save(GebrGuiHelpEditWindow * self, gboolean auto_save);

/**
 * gebr_gui_help_edit_window_get_ui_manager:
 * @window: this window.
 * Returns: A #GtkUIManager.
 */
GtkUIManager * gebr_gui_help_edit_window_get_ui_manager(GebrGuiHelpEditWindow * self);

/*
 * UI Manager path accessors
 */
const gchar * gebr_gui_help_edit_window_get_menu_bar_path(GebrGuiHelpEditWindow * self);

const gchar * gebr_gui_help_edit_window_get_file_menu_path(GebrGuiHelpEditWindow * self);

const gchar * gebr_gui_help_edit_window_get_edit_menu_path(GebrGuiHelpEditWindow * self);

const gchar * gebr_gui_help_edit_window_get_menu_mark(GebrGuiHelpEditWindow * self);

const gchar * gebr_gui_help_edit_window_get_tool_bar_path(GebrGuiHelpEditWindow * self);

const gchar * gebr_gui_help_edit_window_get_tool_bar_mark(GebrGuiHelpEditWindow * self);

G_END_DECLS

#endif /* __GEBR_GUI_HELP_EDIT_WINDOW__ */
