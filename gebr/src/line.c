/*   GÃªBR - An environment for seismic processing.
 *   Copyright (C) 2007-2008 GÃªBR core team (http://gebr.sourceforge.net)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but gebr.THOUT ANY gebr.RRANTY; without even the implied warranty
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see
 *   <http://www.gnu.org/licenses/>.
 */

/* File: line.c
 * Lines manipulation functions
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <glib/gstdio.h>

#include <gui/utils.h>

#include "line.h"
#include "gebr.h"
#include "support.h"
#include "document.h"
#include "project.h"
#include "flow.h"

gchar * no_line_selected_error =		_("No line selected");
gchar * no_selection_error =			_("Nothing selected");

/*
 * Section: Public
 * Public functions.
 */

/*
 * Function: line_new
 * Create a new line
 *
 */
void
line_new(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		project_iter, line_iter;
	GtkTreePath *		path;

	gchar *			line_title;
	gchar *			project_filename;
	gchar *                 project_title;

	GeoXmlLine *		line;

	if (gebr.doc == NULL) {
		gebr_message(LOG_ERROR, TRUE, FALSE, no_selection_error);
		return;
	}

	/* get project iter */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_project_line->view));
	if (geoxml_document_get_type(gebr.doc) == GEOXML_DOCUMENT_TYPE_PROJECT)
		gtk_tree_selection_get_selected(selection, &model, &project_iter);
	else {
		gtk_tree_selection_get_selected(selection, &model, &line_iter);
		gtk_tree_model_iter_parent(model, &project_iter, &line_iter);
	}

	/* create it */
	line = GEOXML_LINE(document_new(GEOXML_DOCUMENT_TYPE_LINE));
	line_title = _("New Line");

	/* gtk stuff */
	gtk_tree_model_get(model, &project_iter,
			   PL_TITLE, &project_title,
			   PL_FILENAME, &project_filename,
			   -1);
	gtk_tree_store_append(gebr.ui_project_line->store, &line_iter, &project_iter);
	gtk_tree_store_set(gebr.ui_project_line->store, &line_iter,
			   PL_TITLE, line_title,
			   PL_FILENAME, geoxml_document_get_filename(GEOXML_DOC(line)),
			   -1);

	/* add to project */
	geoxml_project_append_line(gebr.project, geoxml_document_get_filename(GEOXML_DOC(line)));
	document_save(GEOXML_DOC(gebr.project));

	/* set line stuff, save and free */
	geoxml_document_set_title(GEOXML_DOC(line), line_title);
	geoxml_document_set_author(GEOXML_DOC(line), gebr.config.username->str);
	geoxml_document_set_email(GEOXML_DOC(line), gebr.config.email->str);
	document_save(GEOXML_DOC(line));
	geoxml_document_free(GEOXML_DOC(line));

	/* feedback */
	gebr_message(LOG_INFO, FALSE, TRUE, _("New line created in project '%s'"), project_title);

	/* UI: select it expand parent */
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(gebr.ui_project_line->store), &line_iter);
	gtk_tree_view_expand_to_path(GTK_TREE_VIEW(gebr.ui_project_line->view), path);
	gtk_tree_selection_select_iter(selection, &line_iter);
	gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(gebr.ui_project_line->view), path,
				     NULL, FALSE, 0, 0);
	g_signal_emit_by_name(gebr.ui_project_line->view, "cursor-changed");

	gtk_tree_path_free(path);
	g_free(project_title);
	g_free(project_filename);
}

/*
 * Function: line_delete
 * Delete the selected line
 *
 * TODO: ask the user about erasing all flows associated to this line.
 */
void
line_delete(void)
{
	GtkTreeSelection  *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		line_iter;

	GeoXmlSequence *	project_line;
	GeoXmlSequence *	line_flow;
	const gchar *		line_filename;

	if (gebr.line == NULL) {
		gebr_message(LOG_ERROR, TRUE, FALSE, no_selection_error);
		return;
	}
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_project_line->view));
	gtk_tree_selection_get_selected(selection, &model, &line_iter);

	if (confirm_action_dialog(_("Delete line"), _("Are you sure you want to delete line '%s' and all its flows?"),
		geoxml_document_get_title(GEOXML_DOC(gebr.line))) == FALSE)
		return;

	/* Removes its flows */
	geoxml_line_get_flow(gebr.line, &line_flow, 0);
	while (line_flow != NULL) {
		GString *	path;
		const gchar *	flow_source;

		flow_source = geoxml_line_get_flow_source(GEOXML_LINE_FLOW(line_flow));
		path = document_get_path(flow_source);
		g_unlink(path->str);
		g_string_free(path, TRUE);

		/* log action */
		gebr_message(LOG_INFO, FALSE, TRUE, _("Erasing child flow '%s'"), flow_source);

		geoxml_sequence_next(&line_flow);
	}

	/* Remove the line from its project */
	line_filename = geoxml_document_get_filename(GEOXML_DOC(gebr.line));
	geoxml_project_get_line(gebr.project, &project_line, 0);
	while (project_line != NULL) {
		if (strcmp(line_filename, geoxml_project_get_line_source(GEOXML_PROJECT_LINE(project_line))) == 0) {
			geoxml_sequence_remove(project_line);
			document_save(GEOXML_DOC(gebr.project));
			break;
		}

		geoxml_sequence_next(&project_line);
	}

	/* finally, remove it from the disk */
	document_delete(line_filename);
	/* and from the GUI */
	gtk_tree_store_remove(GTK_TREE_STORE(gebr.ui_project_line->store), &line_iter);
	gtk_list_store_clear(gebr.ui_flow_browse->store);
	flow_free();
	document_free();
	project_line_info_update();

	/* make the user happy */
	gebr_message(LOG_INFO, TRUE, FALSE, _("Erasing line '%s'"), geoxml_document_get_title(GEOXML_DOC(gebr.line)));
	gebr_message(LOG_INFO, FALSE, TRUE, _("Erasing line '%s' from project '%s'"),
		     geoxml_document_get_title(GEOXML_DOC(gebr.line)),
		     geoxml_document_get_title(GEOXML_DOC(gebr.project)));
}

/*
 * Function: line_load_flows
 * Load flows associated to the selected line.
 *
 */
void
line_load_flows(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;
	GtkTreePath *		path;
	GtkTreePath *		selection_path;

	gchar *			line_title;
	gchar *			line_filename;

	GeoXmlSequence*		line_flow;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(gebr.ui_project_line->view));
	if (gtk_tree_selection_get_selected (selection, &model, &iter) == FALSE) {
		gebr_message(LOG_ERROR, TRUE, FALSE, no_selection_error);
		flow_free();
	}

	selection_path = gtk_tree_model_get_path(model, &iter);
	if (gebr.ui_project_line->selection_path != NULL && !gtk_tree_path_compare(selection_path, gebr.ui_project_line->selection_path)) {
		/* uhm, the same line/project is selected, don't need to do nothing */
		gtk_tree_path_free(selection_path);
		return;
	}
	gtk_tree_path_free(gebr.ui_project_line->selection_path);
	gebr.ui_project_line->selection_path = selection_path;

	/* reset part of GUI */
	gtk_list_store_clear(gebr.ui_flow_browse->store);
	gtk_list_store_clear(gebr.ui_flow_edition->fseq_store);
	flow_free();

	/* check if the item is a project */
	path = gtk_tree_model_get_path (model, &iter);
	if (gtk_tree_path_get_depth(path) == 1) {
		gtk_tree_path_free(path);
		return;
	}

	gtk_tree_model_get(model, &iter,
		PL_TITLE, &line_title,
		PL_FILENAME, &line_filename,
		-1);

	/* iterate over its flows */
	geoxml_line_get_flow(gebr.line, &line_flow, 0);
	while (line_flow != NULL) {
		GtkTreeIter	flow_iter;
		GeoXmlFlow *	flow;
		gchar *		flow_filename;

		flow_filename = (gchar*)geoxml_line_get_flow_source(GEOXML_LINE_FLOW(line_flow));
		flow = GEOXML_FLOW(document_load(flow_filename));
		if (flow == NULL) {
			gebr_message(LOG_ERROR, TRUE, TRUE, _("Flow file %s corrupted. Ignoring"), flow_filename);
			geoxml_sequence_next(&line_flow);
			continue;
		}

		/* add to the flow browser. */
		gtk_list_store_append(gebr.ui_flow_browse->store, &flow_iter);
		gtk_list_store_set(gebr.ui_flow_browse->store, &flow_iter,
				FB_TITLE, geoxml_document_get_title(GEOXML_DOC(flow)),
				FB_FILENAME, flow_filename,
				-1);

		geoxml_document_free(GEOXML_DOC(flow));
		geoxml_sequence_next(&line_flow);
	}

	gebr_message(LOG_INFO, TRUE, FALSE, _("Flows loaded"));

	gtk_tree_path_free(path);
	g_free(line_title);
	g_free(line_filename);
}
