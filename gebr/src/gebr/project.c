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

#include <libgebr/intl.h>
#include <libgebr/utils.h>
#include <libgebr/gui/utils.h>
#include <libgebr/geoxml/project.h>

#include "project.h"
#include "gebr.h"
#include "document.h"
#include "flow.h"
#include "line.h"
#include "callbacks.h"
#include "ui_project_line.h"

void project_new(void)
{
	GtkTreeIter iter;

	GebrGeoXmlDocument *project;

	project = document_new(GEBR_GEOXML_DOCUMENT_TYPE_PROJECT);
	gebr_geoxml_document_set_title(project, _("New project"));
	gebr_geoxml_document_set_author(project, gebr.config.username->str);
	gebr_geoxml_document_set_email(project, gebr.config.email->str);

	gtk_tree_store_append(gebr.ui_project_line->store, &iter, NULL);
	gtk_tree_store_set(gebr.ui_project_line->store, &iter,
			   PL_TITLE, gebr_geoxml_document_get_title(project),
			   PL_FILENAME, gebr_geoxml_document_get_filename(project), -1);
	document_save(project, TRUE);
	gebr_geoxml_document_free(project);

	project_line_select_iter(&iter);
	if (!on_document_properties_activate())
		project_delete(FALSE);

	gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("New project created."));
}

gboolean project_delete(gboolean confirm)
{
	GtkTreeIter iter;
	gchar *title;
	gchar *filename;
	gint nlines;

	if (!project_line_get_selected(&iter, ProjectSelection))
		return FALSE;

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->store), &iter,
			   PL_TITLE, &title, PL_FILENAME, &filename, -1);

	/* TODO: If this project is not empty,
	   prompt the user to take about erasing its lines */
	/* Delete each line of the project */
	if ((nlines = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(gebr.ui_project_line->store), &iter)) > 0) {
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Project '%s' still has %i lines."), title, nlines);
		goto out;
	}

	/* message user */
	if (confirm)
		gebr_message(GEBR_LOG_INFO, TRUE, TRUE, _("Deleting project '%s'."), title);

	/* finally, remove it from the disk */
	document_delete(filename);
	project_line_free();
	project_line_info_update();

	/* Remove the project from the store (and its children) */
	if (gtk_tree_store_remove(GTK_TREE_STORE(gebr.ui_project_line->store), &iter))
		project_line_select_iter(&iter);

out:	g_free(title);
	g_free(filename);

	return TRUE;
}

GtkTreeIter project_append_iter(GebrGeoXmlProject * project)
{
	GtkTreeIter iter;

	gtk_tree_store_append(gebr.ui_project_line->store, &iter, NULL);
	gtk_tree_store_set(gebr.ui_project_line->store, &iter,
			   PL_TITLE, gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(project)),
			   PL_FILENAME, gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(project)), -1);

	return iter;
}

GtkTreeIter project_append_line_iter(GtkTreeIter * project_iter, GebrGeoXmlLine * line)
{
	GtkTreeIter iter;

	gtk_tree_store_append(gebr.ui_project_line->store, &iter, project_iter);
	gtk_tree_store_set(gebr.ui_project_line->store, &iter,
			   PL_TITLE, gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(line)),
			   PL_FILENAME, gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(line)), -1);

	return iter;
}

void project_list_populate(void)
{
	gchar *filename;

	/* free previous selection path */
	gtk_tree_store_clear(gebr.ui_project_line->store);
	gtk_list_store_clear(gebr.ui_flow_browse->store);
	gtk_list_store_clear(gebr.ui_flow_edition->fseq_store);
	project_line_free();
	flow_free();

	gebr_directory_foreach_file(filename, gebr.config.data->str) {
		GtkTreeIter project_iter;

		GebrGeoXmlProject *project;
		GebrGeoXmlSequence *project_line;

		if (fnmatch("*.prj", filename, 1))
			continue;

		project = GEBR_GEOXML_PROJECT(document_load(filename));
		if (project == NULL)
			continue;
		project_iter = project_append_iter(project);

		gebr_geoxml_project_get_line(project, &project_line, 0);
		while (project_line != NULL) {
			GebrGeoXmlLine *line;
			const gchar *line_source;

			line_source = gebr_geoxml_project_get_line_source(GEBR_GEOXML_PROJECT_LINE(project_line));
			line = GEBR_GEOXML_LINE(document_load(line_source));
			if (line == NULL) {
				GebrGeoXmlSequence * sequence;
				
				sequence = project_line;
				gebr_geoxml_sequence_next(&project_line);
				gebr_geoxml_sequence_remove(sequence);
				document_save(GEBR_GEOXML_DOCUMENT(project), FALSE);

				continue;
			}
			project_append_line_iter(&project_iter, line);
			gebr_geoxml_document_free(GEBR_GEOXML_DOC(line));

			gebr_geoxml_sequence_next(&project_line);
		}

		gebr_geoxml_document_free(GEBR_GEOXML_DOC(project));
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
	
	src_prj = GEBR_GEOXML_PROJECT(document_load(src_project));

	if (strcmp(src_project, dst_project) == 0)
		/* The line movement is inside the same project. */
		dst_prj = src_prj;
	else
		dst_prj = GEBR_GEOXML_PROJECT(document_load(dst_project));
	
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

	document_save(GEBR_GEOXML_DOCUMENT(src_prj), TRUE);

	if (strcmp(src_project, dst_project) != 0)
		document_save(GEBR_GEOXML_DOCUMENT(dst_prj), TRUE);
}
