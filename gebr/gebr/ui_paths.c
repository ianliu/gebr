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
#include <unistd.h>
#include <gtk/gtk.h>

#include <glib/gi18n.h>
#include <libgebr/gui/gui.h>
#include <libgebr/gui/gebr-gui-sequence-edit.h>

#include "ui_paths.h"
#include "gebr.h"
#include "document.h"

/*
 *
 * Function modified from the realpath program.
 *
 * Original authors credits: 
 *
 * realpath.c -- output the real path of a filename.
 * $Id: realpath.c 229 2009-02-22 11:56:47Z robert $
 *
 * Copyright: (C) 1996 Lars Wirzenius <liw@iki.fi>
 *            (C) 1996-1998 Jim Pick <jim@jimpick.com>
 *            (C) 2001-2009 Robert Luberda <robert@debian.org>
 *
 */

gchar * gebr_stripdir(gchar * dir, gchar *buf, gint maxlen) {
	gchar * in, * out;
	gchar * last;
	gint ldots;

	in   = dir;
	out  = buf;
	last = buf + maxlen; 
	ldots = 0;
	*out  = 0;
       	
	
	if (*in != '/') {
		if (getcwd(buf, maxlen - 2) ) {
			out = buf + strlen(buf) - 1;
			if (*out != '/') *(++out) = '/';
			out++;
		}
		else
			return NULL;
	}

	while (out < last) {
		*out = *in;

		if (*in == '/')
		{
			while (*(++in) == '/') ;
			in--;
		}		

		if (*in == '/' || !*in)
		{
			if (ldots == 1 || ldots == 2) {
				while (ldots > 0 && --out > buf)
				{
					if (*out == '/')
						ldots--;
				}
				*(out+1) = 0;
			}
			ldots = 0;
			
		} else if (*in == '.' && ldots > -1) {
			ldots++;
		} else {
			ldots = -1; 
		}
		
		out++;

		if (!*in)
			break;
		
		in++;
	}

	if (*in) {
		return NULL;
	}
	
	while (--out != buf && (*out == '/' || !*out)) *out=0;
	return buf;
}
static gboolean check_duplicate (GebrGuiSequenceEdit * sequence_edit, const gchar * path)
{
	gboolean retval = FALSE;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar buf[2024];
	gchar buf_i_path[2024];
	gchar *ok = NULL;
	gchar * orig_path = NULL;

	orig_path = g_strdup(path);

	ok = gebr_stripdir( orig_path, buf, sizeof(buf));

	g_free(orig_path);

	if (!ok)
	{
		gebr_gui_message_dialog (GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
					 _("Invalid path"),
					 _("The path <i>%s</i> is invalid, the operation will be cancelled."),
					 path);
		return TRUE;
	}


	g_object_get(G_OBJECT(sequence_edit), "list-store", &model, NULL);
	
	gebr_gui_gtk_tree_model_foreach(iter, model) {
		gchar *i_path;

		gtk_tree_model_get(model, &iter, 0, &i_path, -1);

		ok = gebr_stripdir( i_path, buf_i_path, sizeof(buf_i_path));

		if (!ok)
		{
			g_warning ("Problems with path resolve function in function check_duplicate");
			return FALSE;
		}

		if (g_strcmp0(buf, buf_i_path) == 0) 
		{
			gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(sequence_edit->tree_view), &iter);
			retval = TRUE;
		}
		g_free(i_path);
	}


	if (retval) {
		if (g_strcmp0(path,buf) == 0)
			gebr_gui_message_dialog (GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
						 _("Path already exists"),
						 _("The path <i>%s</i> already exists in the list, the operation will be cancelled."),
						 path);
		else
			gebr_gui_message_dialog (GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
						 _("Path already exists"),
						 _("The path <i>%s</i> already exists in the list as %s, the operation will be cancelled."),
						 path, buf);
	}

	return retval;
}

gboolean path_save(void)
{
	document_save(GEBR_GEOXML_DOCUMENT(gebr.line), TRUE, FALSE);
	project_line_info_update();
	return TRUE;
}

void path_add(GebrGuiValueSequenceEdit * sequence_edit)
{
	const gchar *path;
	gchar buf[2024];
	gchar *ok = NULL;
	GebrGuiFileEntry *file_entry;

	g_object_get(G_OBJECT(sequence_edit), "value-widget", &file_entry, NULL);
	gtk_editable_select_region (GTK_EDITABLE (file_entry->entry), 0, -1);
	gtk_widget_grab_focus (file_entry->entry);
	path = gebr_gui_file_entry_get_path(file_entry);
	if (!strlen(path) || check_duplicate (GEBR_GUI_SEQUENCE_EDIT(sequence_edit), path))
		return;

	gchar * path_copy = NULL;
	path_copy = g_strdup(path);
	ok = gebr_stripdir(path_copy, buf, sizeof(buf));

	g_free(path_copy);

	if (!ok)
		return;

	gchar * clean_path = g_strdup_printf("%s", buf);
	gebr_gui_value_sequence_edit_add(sequence_edit,
					 GEBR_GEOXML_SEQUENCE(gebr_geoxml_line_append_path(gebr.line, clean_path)));
	g_free(clean_path);
	path_save();
}

gboolean path_renamed (GebrGuiValueSequenceEdit * self, const gchar * old_text, const gchar * new_text)
{
	gchar *ok = NULL;
	gchar buf[2024];
	gchar * path_copy = NULL;

	path_copy = g_strdup(new_text);
	ok = gebr_stripdir(path_copy, buf, sizeof(buf));
	g_free(path_copy);

	/*Checking if the duplicate are the same as the one in edit mode*/
	if (!g_strcmp0(old_text, ok)) {
		goto err;
	}

	if (!ok)
		goto err;

	gchar * clean_path = g_strdup_printf("%s", buf);
	if (check_duplicate (GEBR_GUI_SEQUENCE_EDIT(self), clean_path)){
		g_free(clean_path);
		goto err;
	}

	if (!gebr_gui_value_sequence_edit_rename(self, clean_path)){
		g_free(clean_path);
		goto err;
	}

err:
	return FALSE;
}
