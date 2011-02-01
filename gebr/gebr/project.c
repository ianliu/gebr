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
		if (!project_line_get_selected(&iter, DontWarnUnselection))
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

	document_properties_setup_ui(GEBR_GEOXML_DOCUMENT(gebr.project), on_properties_response);
}

gboolean project_delete(GtkTreeIter * iter, gboolean warn_user)
{
	GebrGeoXmlProject * project;
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->store), iter,
			   PL_XMLPOINTER, &project, -1);

	gchar *filename;
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->store), iter,
			   PL_FILENAME, &filename, -1);
	gint nlines = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(gebr.ui_project_line->store), iter);
	if (nlines > 0) {
		if (warn_user)
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Project '%s' still has %i lines."),
				     gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(project)), nlines);
		g_free(filename);
		return FALSE;
	}
	g_free(filename);

	gebr_remove_help_edit_window(GEBR_GEOXML_DOCUMENT(project));
	document_delete(filename);
	project_line_free();
	project_line_info_update();
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
			   PL_XMLPOINTER, project, -1);

	return iter;
}

GtkTreeIter project_append_line_iter(GtkTreeIter * project_iter, GebrGeoXmlLine * line)
{
	GtkTreeIter iter;

	gtk_tree_store_append(gebr.ui_project_line->store, &iter, project_iter);
	gtk_tree_store_set(gebr.ui_project_line->store, &iter,
			   PL_TITLE, gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(line)),
			   PL_FILENAME, gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(line)),
			   PL_XMLPOINTER, line, -1);

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

	/* free previous selection path */
	gtk_tree_store_clear(gebr.ui_project_line->store);
	gtk_list_store_clear(gebr.ui_flow_browse->store);
	gtk_list_store_clear(gebr.ui_flow_edition->fseq_store);

	project_line_free();

	gebr_directory_foreach_file(filename, gebr.config.data->str) {
		if (fnmatch("*.prj", filename, 1))
			continue;

		GebrGeoXmlProject *project;
		if (document_load((GebrGeoXmlDocument**)(&project), filename, FALSE))
			continue;

		project_load_with_lines(project);
	}

	project_line_info_update();
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
