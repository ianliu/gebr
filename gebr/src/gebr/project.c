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

/* File: project.c
 * Functions for projects manipulation
 */

#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <fnmatch.h>

#include <glib/gstdio.h>

#include <gui/utils.h>

#include "project.h"
#include "gebr.h"
#include "support.h"
#include "document.h"
#include "flow.h"
#include "line.h"
#include "callbacks.h"

gchar * no_project_selected_error = _("No project selected");

/*
 * Function: project_new
 * Create a new project
 *
 */
void
project_new(void)
{
	GtkTreeSelection *	selection;
	GtkTreeIter		iter;
	GtkTreePath *           path;

	GeoXmlDocument *	project;
	gchar *			title;

	title = _("New project");
	project = document_new(GEOXML_DOCUMENT_TYPE_PROJECT);
	geoxml_document_set_title(project, title);
	geoxml_document_set_author(project, gebr.config.username->str);
	geoxml_document_set_email(project, gebr.config.email->str);
	document_save(project);
	geoxml_document_free(project);

	gtk_tree_store_append(gebr.ui_project_line->store, &iter, NULL);
	gtk_tree_store_set(gebr.ui_project_line->store, &iter,
		PL_TITLE, title,
		PL_FILENAME, geoxml_document_get_filename(project),
		-1);

	/* feedback */
	gebr_message(LOG_INFO, FALSE, TRUE, _("New project created"));

	/* select it */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_project_line->view));
	gtk_tree_selection_select_iter(selection, &iter);
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(gebr.ui_project_line->store), &iter);
	gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(gebr.ui_project_line->view), path,
				     NULL, FALSE, 0, 0);
	g_signal_emit_by_name(gebr.ui_project_line->view, "cursor-changed");
        on_project_line_properties_activate();

	gtk_tree_path_free(path);
}

/*
 * Function: project_delete
 * Delete the selected project
 *
 * TODO:
 * * If a project is not empty, the user user should be
 *   warned. Besides, it should be asked about erasing
 *   all project's lines.
 * * Project's line files should be deleted as well.
 */
gboolean
project_delete(void)
{
	GtkTreeIter		iter;
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreePath *		path;

	gchar *			title;
	gchar *			filename;

	int			nlines;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (gebr.ui_project_line->view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE) {
		gebr_message(LOG_ERROR, TRUE, FALSE, no_project_selected_error);
		return FALSE;
	}

	gtk_tree_model_get(model, &iter,
		PL_TITLE, &title,
		PL_FILENAME, &filename,
		-1);
	path = gtk_tree_model_get_path(model, &iter);

	if (gtk_tree_path_get_depth(path) == 2) {
		gebr_message(LOG_ERROR, TRUE, FALSE, no_project_selected_error);
		goto out;
	}

	/* TODO: If this project is not empty,
	   prompt the user to take about erasing its lines */
	/* Delete each line of the project */

	if ((nlines = gtk_tree_model_iter_n_children(model, &iter)) > 0) {
		gebr_message(LOG_ERROR, TRUE, FALSE, _("Project '%s' still has %i lines"), title, nlines);
		goto out;
	}

	/* message user */
	gebr_message(LOG_INFO, TRUE, TRUE, _("Erasing project '%s'"), title);

	/* finally, remove it from the disk */
	document_delete(filename);
	project_line_free();
	project_line_info_update();

	/* Remove the project from the store (and its children) */
	gtk_tree_view_select_sibling(GTK_TREE_VIEW(gebr.ui_project_line->view));
	gtk_tree_store_remove(GTK_TREE_STORE(gebr.ui_project_line->store), &iter);

out:	g_free(title);
	g_free(filename);
	gtk_tree_path_free(path);

	return TRUE;
}

/*
 * Function: project_list_populate
 * Reload the projets from the data directory
 */
void
project_list_populate(void)
{
	struct dirent *	file;
	DIR *		dir;

	if (g_access(gebr.config.data->str, F_OK | R_OK)) {
		gebr_message(LOG_ERROR, TRUE, FALSE, _("Unable to access data directory"));
		return;
	}
	if ((dir = opendir(gebr.config.data->str)) == NULL)
		return;

	/* free previous selection path */
	gtk_tree_store_clear(gebr.ui_project_line->store);
	gtk_list_store_clear(gebr.ui_flow_browse->store);
	gtk_list_store_clear(gebr.ui_flow_edition->fseq_store);
	project_line_free();
	flow_free();

	while ((file = readdir(dir)) != NULL) {
		GtkTreeIter		project_iter, line_iter;

		GeoXmlProject *		project;
		GeoXmlSequence *	project_line;

		if (fnmatch("*.prj", file->d_name, 1))
			continue;

		project = GEOXML_PROJECT(document_load(file->d_name));
		if (project == NULL)
			continue;

		/* Gtk stuff */
		gtk_tree_store_append(gebr.ui_project_line->store, &project_iter, NULL);
		gtk_tree_store_set(gebr.ui_project_line->store, &project_iter,
				PL_TITLE, geoxml_document_get_title(GEOXML_DOC(project)),
				PL_FILENAME, geoxml_document_get_filename(GEOXML_DOC(project)),
				-1);

		geoxml_project_get_line(project, &project_line, 0);
		while (project_line != NULL) {
			GeoXmlLine *	line;
			const gchar *	line_source;

			line_source = geoxml_project_get_line_source(GEOXML_PROJECT_LINE(project_line));
			line = GEOXML_LINE(document_load(line_source));
			if (line == NULL) {
				gebr_message(LOG_ERROR, TRUE, TRUE, _("Line file %s corrupted. Ignoring."),
					     line_source);
				geoxml_sequence_next(&project_line);
				continue;
			}

			gtk_tree_store_append(gebr.ui_project_line->store, &line_iter, &project_iter);
			gtk_tree_store_set(gebr.ui_project_line->store, &line_iter,
					PL_TITLE, geoxml_document_get_title(GEOXML_DOC(line)),
					PL_FILENAME, line_source,
					-1);

			geoxml_document_free(GEOXML_DOC(line));
			geoxml_sequence_next(&project_line);
		}

		geoxml_document_free(GEOXML_DOC(project));
	}

	closedir (dir);
	project_line_info_update();
}
