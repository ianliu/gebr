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

#ifndef __GEBR_GUI_HELP_EDIT__
#define __GEBR_GUI_HELP_EDIT__

#include <gtk/gtk.h>


G_BEGIN_DECLS


#define GEBR_GUI_TYPE_HELP_EDIT			(gebr_gui_help_edit_get_type())
#define GEBR_GUI_HELP_EDIT(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEBR_GUI_TYPE_HELP_EDIT, GebrGuiHelpEdit))
#define GEBR_GUI_HELP_EDIT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GEBR_GUI_TYPE_HELP_EDIT, GebrGuiHelpEditClass))
#define GEBR_GUI_IS_HELP_EDIT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEBR_GUI_TYPE_HELP_EDIT))
#define GEBR_GUI_IS_HELP_EDIT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEBR_GUI_TYPE_HELP_EDIT))
#define GEBR_GUI_HELP_EDIT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEBR_GUI_TYPE_HELP_EDIT, GebrGuiHelpEditClass))


typedef struct _GebrGuiHelpEdit GebrGuiHelpEdit;
typedef struct _GebrGuiHelpEditClass GebrGuiHelpEditClass;

struct _GebrGuiHelpEdit {
	GtkVBox parent;
};

struct _GebrGuiHelpEditClass {
	GtkVBoxClass parent_class;

	/* Abstract methods */
	gchar * (*get_content) (GebrGuiHelpEdit * self);
	void (*set_content) (GebrGuiHelpEdit * self, const gchar * content);
	void (*commit_changes) (GebrGuiHelpEdit * self);
};

GType gebr_gui_help_edit_get_type(void) G_GNUC_CONST;

/**
 * gebr_gui_help_edit_new:
 *
 * Creates a new help edit widget.
 */
GtkWidget * gebr_gui_help_edit_new();

/**
 * gebr_gui_help_edit_set_editing:
 *
 * Sets the editor into edit mode or preview mode based on the value of @editing parameter.
 *
 * @editing: Whether to edit or preview the help.
 */
void gebr_gui_help_edit_set_editing(GebrGuiHelpEdit * self, gboolean editing);

/**
 * gebr_gui_help_edit_commit_changes:
 *
 * Commit changes done in the Help until now.
 */
void gebr_gui_help_edit_commit_changes(GebrGuiHelpEdit * self);

/**
 * gebr_gui_help_edit_get_content:
 *
 * Fetches the current state of the help edition.
 *
 * Returns: A newly allocated string, with the edited content of this help edit widget.
 */
gchar * gebr_gui_help_edit_get_content(GebrGuiHelpEdit * self);

/**
 * gebr_gui_help_edit_set_content:
 *
 * Sets the content for this help edition session.
 *
 * @content: The new content for this help edition.
 */
void gebr_gui_help_edit_set_content(GebrGuiHelpEdit * self, const gchar * content);

/**
 * gebr_gui_help_edit_get_web_view:
 *
 * Returns: The #GtkWebKitWebView used to show the help edition.
 */
GtkWidget * gebr_gui_help_edit_get_web_view(GebrGuiHelpEdit * self);

G_END_DECLS

#endif /* __GEBR_GUI_HELP_EDIT__ */
