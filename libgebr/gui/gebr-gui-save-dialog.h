/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
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
 * @file gebr-gui-save-dialog.h Widget to choose a file to save
 * @ingroup libgebr-gui
 */

#ifndef __GEBR_GUI_SAVE_DIALOG_H__
#define __GEBR_GUI_SAVE_DIALOG_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GEBR_GUI_TYPE_SAVE_DIALOG		(gebr_gui_save_dialog_get_type())
#define GEBR_GUI_SAVE_DIALOG(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), GEBR_GUI_TYPE_SAVE_DIALOG, GebrGuiSaveDialog))
#define GEBR_GUI_SAVE_DIALOG_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass),  GEBR_GUI_TYPE_SAVE_DIALOG, GebrGuiSaveDialogClass))
#define GEBR_GUI_IS_SAVE_DIALOG(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), GEBR_GUI_TYPE_SAVE_DIALOG))
#define GEBR_GUI_IS_SAVE_DIALOG_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),  GEBR_GUI_TYPE_SAVE_DIALOG))
#define GEBR_GUI_SAVE_DIALOG_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj),  GEBR_GUI_TYPE_SAVE_DIALOG, GebrGuiSaveDialogClass))

typedef struct _GebrGuiSaveDialog GebrGuiSaveDialog;
typedef struct _GebrGuiSaveDialogClass GebrGuiSaveDialogClass;

struct _GebrGuiSaveDialog {
	GtkFileChooserDialog parent;

	/* private */
	gchar *extension;
};

struct _GebrGuiSaveDialogClass {
	GtkFileChooserDialogClass parent_class;
};

GType gebr_gui_save_dialog_get_type(void) G_GNUC_CONST;

/**
 * Creates a new save dialog.
 */
GtkWidget *gebr_gui_save_dialog_new(const gchar *title, GtkWindow *parent);

/**
 * Sets the default file extension to use with this save dialog.
 * If the file choosen by the user doesn't have the extension, it is appended.
 */
void gebr_gui_save_dialog_set_default_extension(GebrGuiSaveDialog *self, const gchar *extension);

/**
 * Gets the default file extension that is added to the file when saving.
 */
const gchar *gebr_gui_save_dialog_get_default_extension(GebrGuiSaveDialog *self);

/**
 * Shows the dialog so the user can choose a file.
 * \return NULL if the operation was canceled. Otherwise returns a newly allocated string containing the file path
 * chosen by the user.
 */
gchar *gebr_gui_save_dialog_run(GebrGuiSaveDialog *self);

G_END_DECLS

#endif /* __GEBR_GUI_SAVE_DIALOG_H__ */
