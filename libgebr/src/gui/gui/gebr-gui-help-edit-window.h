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
	GtkDialog parent;
};

struct _GebrGuiHelpEditWindowClass {
	GtkDialogClass parent_class;

	/* Signals */
	void (*refresh_requested) (GebrGuiHelpEditWindow *self, GString *help);
};

GType gebr_gui_help_edit_window_get_type(void) G_GNUC_CONST;

/**
 * gebr_gui_help_edit_window_new:
 */
GtkWidget *gebr_gui_help_edit_window_new(GebrGuiHelpEditWidget * help_edit_widget);

/**
 * gebr_gui_help_edit_window_new_with_refresh:
 */
GtkWidget *gebr_gui_help_edit_window_new_with_refresh(GebrGuiHelpEditWidget * help_edit_widget);

G_END_DECLS

#endif /* __GEBR_GUI_HELP_EDIT_WINDOW__ */
