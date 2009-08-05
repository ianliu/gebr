/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
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

#include <libgebr/intl.h>
#include <libgebr/gui/utils.h>

#include "line.h"
#include "gebr.h"
#include "document.h"
#include "project.h"
#include "flow.h"
#include "callbacks.h"
#include "ui_project_line.h"

/*
 * Section: Public
 * Public functions.
 */

/*
 * Function: line_new
 * Create a new line
 *
 */
gboolean
line_new(void)
{
	GtkTreeSelection *	selection;
	GtkTreeIter		project_iter, line_iter;

	gchar *			project_filename;
	gchar *                 project_title;

	GeoXmlLine *		line;

	if (!project_line_get_selected(NULL, ProjectLineSelection))
		return FALSE;

	/* get project iter */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_project_line->view));
	if (geoxml_document_get_type(gebr.project_line) == GEOXML_DOCUMENT_TYPE_PROJECT)
		gtk_tree_selection_get_selected(selection, NULL, &project_iter);
	else {
		gtk_tree_selection_get_selected(selection, NULL, &line_iter);
		gtk_tree_model_iter_parent(GTK_TREE_MODEL(gebr.ui_project_line->store), &project_iter, &line_iter);
	}

	/* create it */
	line = GEOXML_LINE(document_new(GEOXML_DOCUMENT_TYPE_LINE));
	geoxml_document_set_title(GEOXML_DOC(line), _("New Line"));
	geoxml_document_set_author(GEOXML_DOC(line), gebr.config.username->str);
	geoxml_document_set_email(GEOXML_DOC(line), gebr.config.email->str);
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->store), &project_iter,
		PL_TITLE, &project_title,
		PL_FILENAME, &project_filename,
		-1);
	gtk_tree_store_append(gebr.ui_project_line->store, &line_iter, &project_iter);
	gtk_tree_store_set(gebr.ui_project_line->store, &line_iter,
		PL_TITLE, geoxml_document_get_title(GEOXML_DOC(line)),
		PL_FILENAME, geoxml_document_get_filename(GEOXML_DOC(line)),
		-1);
	geoxml_project_append_line(gebr.project, geoxml_document_get_filename(GEOXML_DOC(line)));
	document_save(GEOXML_DOC(gebr.project));
	document_save(GEOXML_DOC(line));

	/* feedback */
	gebr_message(LOG_INFO, FALSE, TRUE, _("New line created in project '%s'"), project_title);
	project_line_set_selected(&line_iter, GEOXML_DOCUMENT(line));

	if (!on_project_line_properties_activate())
		line_delete(FALSE);

	g_free(project_title);
	g_free(project_filename);
	geoxml_document_free(GEOXML_DOC(line));

	return TRUE;
}

/*
 * Function: line_delete
 * Delete the selected line
 *
 */
gboolean
line_delete(gboolean confirm)
{
	GtkTreeIter		iter;

	GeoXmlSequence *	project_line;
	GeoXmlSequence *	line_flow;
	const gchar *		line_filename;

	if (!project_line_get_selected(&iter, LineSelection))
		return FALSE;

	if (confirm && libgebr_gui_confirm_action_dialog(_("Delete line"), _("Are you sure you want to delete line '%s' and all its flows?"),
		geoxml_document_get_title(GEOXML_DOC(gebr.line))) == FALSE)
		return FALSE;

	/* Removes its flows */
	geoxml_line_get_flow(gebr.line, &line_flow, 0);
	for (; line_flow != NULL; geoxml_sequence_next(&line_flow)) {
		GString *	path;
		const gchar *	flow_source;

		flow_source = geoxml_line_get_flow_source(GEOXML_LINE_FLOW(line_flow));
		path = document_get_path(flow_source);
		g_unlink(path->str);
		g_string_free(path, TRUE);

		/* log action */
		gebr_message(LOG_INFO, FALSE, TRUE, _("Erasing child flow '%s'"), flow_source);
	}

	/* Remove the line from its project */
	line_filename = geoxml_document_get_filename(GEOXML_DOC(gebr.line));
	geoxml_project_get_line(gebr.project, &project_line, 0);
	for (; project_line != NULL; geoxml_sequence_next(&project_line)) {
		if (strcmp(line_filename, geoxml_project_get_line_source(GEOXML_PROJECT_LINE(project_line))) == 0) {
			geoxml_sequence_remove(project_line);
			document_save(GEOXML_DOC(gebr.project));
			break;
		}
	}

	/* inform the user */
	if (confirm) {
		gebr_message(LOG_INFO, TRUE, FALSE, _("Erasing line '%s'"),
			geoxml_document_get_title(GEOXML_DOC(gebr.line)));
		gebr_message(LOG_INFO, FALSE, TRUE, _("Erasing line '%s' from project '%s'"),
			geoxml_document_get_title(GEOXML_DOC(gebr.line)),
			geoxml_document_get_title(GEOXML_DOC(gebr.project)));
	}

	/* finally, remove it from the disk */
	document_delete(line_filename);
	/* and from the GUI */
	libgebr_gui_gtk_tree_view_select_sibling(GTK_TREE_VIEW(gebr.ui_project_line->view));
	gtk_tree_store_remove(GTK_TREE_STORE(gebr.ui_project_line->store), &iter);

	return TRUE;
}

/*
 * Function: line_import
 * Import line with basename _line_filename_ inside _at_dir_.
 * Also import its flows.
 */
GeoXmlLine *
line_import(const gchar * line_filename, const gchar * at_dir)
{
	GeoXmlLine *		line;
	GeoXmlSequence *	i;

	line = GEOXML_LINE(document_load_at(line_filename, at_dir));
	if (line == NULL)
		return NULL;
	document_import(GEOXML_DOCUMENT(line));

	geoxml_line_get_flow(line, &i, 0);
	for (; i != NULL; geoxml_sequence_next(&i)) {
		GeoXmlFlow *		flow;

		flow = GEOXML_FLOW(document_load_at(
			geoxml_line_get_flow_source(GEOXML_LINE_FLOW(i)), at_dir));
		if (flow == NULL)
			continue;
		document_import(GEOXML_DOCUMENT(flow));
		geoxml_line_set_flow_source(GEOXML_LINE_FLOW(i),
			geoxml_document_get_filename(GEOXML_DOCUMENT(flow)));
		document_save(GEOXML_DOCUMENT(flow));
		geoxml_document_free(GEOXML_DOCUMENT(flow));
	}
	document_save(GEOXML_DOCUMENT(line));

	return line;
}

/* Function: line_append_flow
 * Append _line_flow_ to flow browse
 */
GtkTreeIter
line_append_flow(GeoXmlLineFlow * line_flow)
{
	GeoXmlFlow *	flow;
	const gchar *	flow_filename;

	GtkTreeIter	iter;

	flow_filename = geoxml_line_get_flow_source(GEOXML_LINE_FLOW(line_flow));
	flow = GEOXML_FLOW(document_load(flow_filename));
	if (flow == NULL)
		return iter;

	/* add to the flow browser. */
	gtk_list_store_append(gebr.ui_flow_browse->store, &iter);
	gtk_list_store_set(gebr.ui_flow_browse->store, &iter,
		FB_TITLE, geoxml_document_get_title(GEOXML_DOC(flow)),
		FB_FILENAME, flow_filename,
		FB_LINE_FLOW_POINTER, line_flow,
		-1);

	geoxml_document_free(GEOXML_DOC(flow));

	return iter;
}

/* Function: line_load_flows
 * Load flows associated to the selected line.
 * Called only by project_line_load
 */
void
line_load_flows(void)
{
	GeoXmlSequence *	line_flow;
	GtkTreeIter		iter;

	flow_free();

	/* iterate over its flows */
	geoxml_line_get_flow(gebr.line, &line_flow, 0);
	for (; line_flow != NULL; geoxml_sequence_next(&line_flow))
		iter = line_append_flow(GEOXML_LINE_FLOW(line_flow));

	gebr_message(LOG_INFO, TRUE, FALSE, _("Flows loaded"));

	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter) == TRUE)
		flow_browse_select_iter(&iter);
}

/* Function: line_move_flow_top
 * Move flow top
 */
void
line_move_flow_top(void)
{
	GtkTreeIter		iter;
	GeoXmlSequence *	line_flow;

	project_line_get_selected(&iter, DontWarnUnselection);

	/* Update line XML */
	geoxml_line_get_flow(gebr.line, &line_flow,
		libgebr_gui_gtk_list_store_get_iter_index(gebr.ui_flow_browse->store, &iter));
	geoxml_sequence_move_after(line_flow, NULL);
	document_save(GEOXML_DOC(gebr.line));
	/* GUI */
	gtk_list_store_move_after(GTK_LIST_STORE(gebr.ui_flow_browse->store), &iter, NULL);
}

/* Function: line_move_flow_bottom
 * Move flow bottom
 */
void
line_move_flow_bottom(void)
{
	GtkTreeIter		iter;
	GeoXmlSequence *	line_flow;

	project_line_get_selected(&iter, DontWarnUnselection);
	/* Update line XML */
	geoxml_line_get_flow(gebr.line, &line_flow,
		libgebr_gui_gtk_list_store_get_iter_index(gebr.ui_flow_browse->store, &iter));
	geoxml_sequence_move_before(line_flow, NULL);
	document_save(GEOXML_DOC(gebr.line));
	/* GUI */
	gtk_list_store_move_before(GTK_LIST_STORE(gebr.ui_flow_browse->store), &iter, NULL);
}
