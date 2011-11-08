/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2011 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __LIBGEBR_GUI_TOOL_BUTTON_H__
#define __LIBGEBR_GUI_TOOL_BUTTON_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define GEBR_GUI_TYPE_TOOL_BUTTON            (gebr_gui_tool_button_get_type())
#define GEBR_GUI_TOOL_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_GUI_TYPE_TOOL_BUTTON, GebrGuiToolButton))
#define GEBR_GUI_TOOL_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEBR_GUI_TYPE_TOOL_BUTTON, GebrGuiToolButtonClass))
#define GEBR_GUI_IS_TOOL_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_GUI_TYPE_TOOL_BUTTON))
#define GEBR_GUI_IS_TOOL_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEBR_GUI_TYPE_TOOL_BUTTON))
#define GEBR_GUI_TOOL_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEBR_GUI_TYPE_TOOL_BUTTON, GebrGuiToolButtonClass))


typedef struct _GebrGuiToolButton GebrGuiToolButton;
typedef struct _GebrGuiToolButtonPriv GebrGuiToolButtonPriv;
typedef struct _GebrGuiToolButtonClass GebrGuiToolButtonClass;

struct _GebrGuiToolButton {
	GtkToggleButton parent;

	GebrGuiToolButtonPriv *priv;
};

struct _GebrGuiToolButtonClass {
	GtkToggleButtonClass parent_class;
};

GType gebr_gui_tool_button_get_type(void) G_GNUC_CONST;

GtkWidget *gebr_gui_tool_button_new(void);

/**
 * gebr_gui_tool_button_add:
 *
 * Adds @widget to the popup window of @button.
 */
void gebr_gui_tool_button_add(GebrGuiToolButton *button,
			      GtkWidget *widget);

G_END_DECLS

#endif /* __LIBGEBR_GUI_TOOL_BUTTON_H__ */
