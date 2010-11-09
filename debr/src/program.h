/*   DeBR - GeBR Designer
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
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

/**
 * \file program.c Construct interfaces for programs.
 */

#ifndef __PROGRAM_H
#define __PROGRAM_H

#include <gtk/gtk.h>
#include <libgebr/geoxml.h>

G_BEGIN_DECLS

enum {
	PROGRAM_STATUS = 0,
	PROGRAM_TITLE,
	PROGRAM_XMLPOINTER,
	PROGRAM_N_COLUMN
};

struct ui_program {
	GtkWidget *widget;
	GtkWidget * help_validate_image;

	GtkListStore *list_store;
	GtkWidget *tree_view;

	struct ui_program_details {
		GtkWidget *frame;
		GtkWidget *title_label;
		GtkWidget *description_label;
		GtkWidget *nparams_label;
		GtkWidget *binary_label;
		GtkWidget *version_label;
		GtkWidget *mpi_label;
		GtkWidget *url_label;
		GtkWidget *url_button;
		GtkWidget *help_edit;
		GtkWidget *help_view;
	} details;
};

/**
 * Set interface and its callbacks.
 */
void program_setup_ui(void);

/**
 * Load programs of the current menu into the tree view.
 */
void program_load_menu(void);

/**
 * Append a new program and selects it
 */
void program_new();

/**
 * Show GeBR's program edit view.
 */
void program_preview(void);

/**
 * Confirm action and if confirmed removed selected program from XML and UI.
 */
void program_remove(gboolean confirm);

/**
 * Action move top.
 */
void program_top(void);

/**
 * Move bottom current selected program.
 */
void program_bottom(void);

/**
 * Copy selected(s) program to clipboard.
 */
void program_copy(void);

/**
 * Paste programs on clipboard.
 */
void program_paste(void);

/**
 * Open dialog to configure current program.
 */
gboolean program_dialog_setup_ui(gboolean new_program);

/**
 * Sets \p iter to point to the selected program.
 *
 * \param warn_user Whether to warn if there is no selection.
 * \return TRUE if there is a program selected.
 */
gboolean program_get_selected(GtkTreeIter * iter, gboolean warn_user);

/**
 * Selects the program pointed by \p iter, loading it into UI.
 */
void program_select_iter(GtkTreeIter iter);

/*
 *
 */
void program_help_show(void);

/*
 *
 */
void program_help_edit(void);

/**
 * debr_program_get_backup_help_from_pointer:
 */
gchar * debr_program_get_backup_help_from_pointer(gpointer program);

/**
 * debr_program_sync_help_backups:
 * Synchronize the backed up programs help.
 */
void debr_program_sync_help_backups();

G_END_DECLS
#endif				//__PROGRAM_H
