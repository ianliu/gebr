/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * File: ui_paths.c
 * Line paths UI.
 *
 * Assembly line's paths dialog.
 */

#include <string.h>
#include <gtk/gtk.h>

#include <libgebr/intl.h>
#include <libgebr/gui/gebr-gui-value-sequence-edit.h>
#include <libgebr/gui/gebr-gui-file-entry.h>
#include <libgebr/gui/gebr-gui-utils.h>

#include "ui_paths.h"
#include "gebr.h"
#include "document.h"

gboolean path_save(void)
{
	document_save(GEBR_GEOXML_DOCUMENT(gebr.line), TRUE);
	project_line_info_update();
	return TRUE;
}

void path_add(GebrGuiValueSequenceEdit * sequence_edit)
{
	const gchar *path;

	GebrGuiGtkFileEntry *file_entry;
	g_object_get(G_OBJECT(sequence_edit), "value-widget", &file_entry, NULL);
	path = gebr_gui_gtk_file_entry_get_path(file_entry);
	if (!strlen(path))
		return;

	/* check for duplication */
	GtkTreeModel *model;
	g_object_get(G_OBJECT(sequence_edit), "list-store", &model, NULL);
	GtkTreeIter iter;
	gebr_gui_gtk_tree_model_foreach(iter, model) {
		gchar *i_path;
		gtk_tree_model_get(model, &iter, 0, &i_path, -1);
		if (!strcmp(i_path, path)) {
			gebr_gui_gtk_tree_view_select_iter(GEBR_GUI_SEQUENCE_EDIT(sequence_edit)->tree_view, &iter);
			g_free(i_path);
			return;
		}
		g_free(i_path);
	}

	gebr_gui_value_sequence_edit_add(sequence_edit,
					 GEBR_GEOXML_SEQUENCE(gebr_geoxml_line_append_path(gebr.line, path)));
	path_save();
}

