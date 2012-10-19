/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
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

#include <stdio.h>
#include <unistd.h>

#include <glib/gstdio.h>

#include <glib/gi18n.h>
#include <libgebr/utils.h>
#include <libgebr/gui/gebr-gui-utils.h>
#include <libgebr/geoxml/project.h>

#include "project.h"
#include "gebr.h"
#include "document.h"
#include "flow.h"
#include "line.h"
#include "callbacks.h"
#include "ui_project_line.h"
#include "ui_document.h"

static void on_properties_response(gboolean accept)
{
	if (accept)
		gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("New project created."));
	else {
		GtkTreeIter iter;
		if (project_line_get_selected(&iter, DontWarnUnselection))
			project_delete(&iter, FALSE);
	}
}

void project_new(void)
{
	GtkTreeIter iter;

	GebrGeoXmlDocument *project;

	project = document_new(GEBR_GEOXML_DOCUMENT_TYPE_PROJECT);
	gebr_geoxml_document_set_title(project, _("New project"));
	gebr_geoxml_document_set_author(project, gebr.config.username->str);
	gebr_geoxml_document_set_email(project, gebr.config.email->str);
	iter = project_append_iter(GEBR_GEOXML_PROJECT(project));
	document_save(project, TRUE, FALSE);
	project_line_select_iter(&iter);

	document_properties_setup_ui(GEBR_GEOXML_DOCUMENT(gebr.project), on_properties_response, TRUE);
}

gboolean project_delete(GtkTreeIter * iter, gboolean warn_user)
{
	GebrGeoXmlProject * project;
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->store), iter,
			   PL_XMLPOINTER, &project, -1);

	gint nlines = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(gebr.ui_project_line->store), iter);
	if (nlines > 0) {
		const gchar *header = _("Selection error:");
		if (warn_user)
			gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
						NULL, header, header,
						_("The project has lines.\nThese Lines should also be selected so as to be deleted along with the project."));
		return FALSE;
	}

	gchar *filename;
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->store), iter,
			   PL_FILENAME, &filename, -1);
	document_delete(filename);
	g_free(filename);
	project_line_free();
	project_line_info_update();
	gebr_remove_help_edit_window(GEBR_GEOXML_DOCUMENT(project));
	gtk_tree_store_remove(GTK_TREE_STORE(gebr.ui_project_line->store), iter);

	/* message user */
	if (warn_user)
		gebr_message(GEBR_LOG_INFO, TRUE, TRUE, _("Deleting project '%s'."),
			     gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(project)));

	return TRUE;
}

GtkTreeIter project_append_iter(GebrGeoXmlProject * project)
{
	GtkTreeIter iter;

	gtk_tree_store_append(gebr.ui_project_line->store, &iter, NULL);
	gtk_tree_store_set(gebr.ui_project_line->store, &iter,
			   PL_TITLE, gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(project)),
			   PL_FILENAME, gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(project)),
			   PL_XMLPOINTER, project,
			   PL_SENSITIVE, TRUE, -1);

	return iter;
}

GtkTreeIter project_append_line_iter(GtkTreeIter * project_iter, GebrGeoXmlLine * line)
{
	GtkTreeIter iter;

	gboolean sensitive;
	GebrMaestroServer *m = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, line);
	if (m && gebr_maestro_server_get_state(m) == SERVER_STATE_LOGGED)
		sensitive = TRUE;
	else
		sensitive = FALSE;

	gtk_tree_store_append(gebr.ui_project_line->store, &iter, project_iter);
	gtk_tree_store_set(gebr.ui_project_line->store, &iter,
			   PL_TITLE, gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(line)),
			   PL_FILENAME, gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(line)),
			   PL_XMLPOINTER, line,
			   PL_SENSITIVE, sensitive,
			   -1);

	return iter;
}

GtkTreeIter project_load_with_lines(GebrGeoXmlProject *project)
{
	GtkTreeIter project_iter;
	GebrGeoXmlSequence *project_line;

	project_iter = project_append_iter(project);

	gebr_geoxml_project_get_line(project, &project_line, 0);
	while (project_line != NULL) {
		GebrGeoXmlLine *line;
		const gchar *line_source;

		GebrGeoXmlSequence * next = project_line;
		gebr_geoxml_sequence_next(&next);

		line_source = gebr_geoxml_project_get_line_source(GEBR_GEOXML_PROJECT_LINE(project_line));
		int ret = document_load_with_parent((GebrGeoXmlDocument**)(&line), line_source, &project_iter, FALSE);
		gchar *proj_maestro = gebr_geoxml_line_get_maestro(line);
		if (g_strcmp0(proj_maestro, "") == 0) {
			GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);
			if (maestro) {
				const gchar *actual_maestro_addr = gebr_maestro_server_get_address(maestro);
				gebr_geoxml_line_set_maestro(line, actual_maestro_addr);

				const gchar *maestro_home = gebr_maestro_server_get_home_dir(maestro);
				gebr_geoxml_line_append_path(line, "HOME", maestro_home);
			}
			document_save(GEBR_GEOXML_DOC(line), TRUE,FALSE);
		} else {
			gchar *line_home = gebr_geoxml_line_get_path_by_name(line, "HOME");
			if (!line_home) {
				GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_address(gebr.maestro_controller,
				                                                                             proj_maestro);
				if (maestro) {
					const gchar *maestro_home = gebr_maestro_server_get_home_dir(maestro);
					gebr_geoxml_line_append_path(line, "HOME", maestro_home);
				}
			} else {
				g_free(line_home);
			}
		}
		if (ret) {
			project_line = next;
			continue;
		}
		project_append_line_iter(&project_iter, line);

		project_line = next;
	}

	return project_iter;
}

void project_list_populate(void)
{
	gchar *filename;
	gsize length;
	gchar **key_array;
	GError *error = NULL;
	GebrGeoXmlProject *project;

	/* free previous selection path */
	gtk_tree_store_clear(gebr.ui_project_line->store);
	gtk_tree_store_clear(gebr.ui_flow_browse->store);

	project_line_free();

	key_array = g_key_file_get_keys(gebr.config.key_file, "projects", &length, &error);

	if (!key_array) {
		key_array = g_strsplit("", ",", 0);
	}

	for (gint i = 0; key_array[i] != NULL; ++i) {
		if (!g_str_has_prefix(key_array[i], "project-"))
			continue;

		filename = g_key_file_get_string (gebr.config.key_file, "projects", key_array[i], &error);

		if (document_load((GebrGeoXmlDocument**)(&project), filename, FALSE)){
			g_free(filename);
			continue;
		}

		project_load_with_lines(project);
		g_free(filename);
	}

	gebr_directory_foreach_file(filename, gebr.config.data->str) {
		gboolean already_loaded = FALSE;

		if (fnmatch("*.prj", filename, 1))
			continue;

		for (gint i = 0; key_array[i] != NULL; ++i) {
			gchar * filename_loaded = g_key_file_get_string (gebr.config.key_file, "projects", key_array[i], &error);

			if (!g_strcmp0(filename_loaded, filename)){
				already_loaded = TRUE;
				g_free(filename_loaded);
				break;
			}
			g_free(filename_loaded);
		}

		if (already_loaded || document_load((GebrGeoXmlDocument**)(&project), filename, FALSE))
			continue;

		project_load_with_lines(project);
	}

	project_line_info_update();
	g_strfreev(key_array);
}

void project_line_move(const gchar * src_project, const gchar * src_line,
		       const gchar * dst_project, const gchar * dst_line, gboolean before)
{
	GebrGeoXmlProject * src_prj;
	GebrGeoXmlProject * dst_prj;
	GebrGeoXmlProjectLine * src_lne;
	GebrGeoXmlProjectLine * dst_lne;
	GebrGeoXmlProjectLine * clone;
	
	document_load((GebrGeoXmlDocument**)&src_prj, src_project, FALSE);

	if (strcmp(src_project, dst_project) == 0)
		/* The line movement is inside the same project. */
		dst_prj = src_prj;
	else
		document_load((GebrGeoXmlDocument**)&dst_prj, dst_project, FALSE);
	
	src_lne = gebr_geoxml_project_get_line_from_source(src_prj, src_line);
	clone = gebr_geoxml_project_append_line(dst_prj, src_line);

	gebr_geoxml_sequence_remove(GEBR_GEOXML_SEQUENCE(src_lne));

	if (dst_line) {
		dst_lne = gebr_geoxml_project_get_line_from_source(dst_prj, dst_line);
		if (before)
			gebr_geoxml_sequence_move_before(GEBR_GEOXML_SEQUENCE(clone), GEBR_GEOXML_SEQUENCE(dst_lne));
		else
			gebr_geoxml_sequence_move_after(GEBR_GEOXML_SEQUENCE(clone), GEBR_GEOXML_SEQUENCE(dst_lne));
	}

	document_save(GEBR_GEOXML_DOCUMENT(src_prj), TRUE, FALSE);

	if (strcmp(src_project, dst_project) != 0)
		document_save(GEBR_GEOXML_DOCUMENT(dst_prj), TRUE, FALSE);
}
