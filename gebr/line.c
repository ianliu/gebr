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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <glib/gstdio.h>

#include <glib/gi18n.h>
#include <libgebr/utils.h>
#include <libgebr/gui/gebr-gui-utils.h>

#include "line.h"
#include "gebr.h"
#include "document.h"
#include "project.h"
#include "flow.h"
#include "callbacks.h"
#include "ui_project_line.h"
#include "ui_document.h"

static void
on_entry_changed(GtkEntry *entry,
		 GtkAssistant *assistant)
{
	GtkWidget *current_page;
	gint page_number;
	const gchar *text;

	page_number = gtk_assistant_get_current_page(assistant);
	current_page = gtk_assistant_get_nth_page(assistant, page_number);
	text = gtk_entry_get_text(entry);

	if (text && *text)
		gtk_assistant_set_page_complete(assistant, current_page, TRUE);
	else
		gtk_assistant_set_page_complete(assistant, current_page, FALSE);
}

static void
on_assistant_cancel(GtkWidget *widget)
{
	gtk_widget_destroy(widget);
	GtkTreeIter iter;
	if (project_line_get_selected(&iter, DontWarnUnselection))
		line_delete(&iter, FALSE);
}

static void
on_assistant_close(GtkWidget *widget)
{
	GtkAssistant *assistant = GTK_ASSISTANT(widget);
	gint n = gtk_assistant_get_n_pages(assistant);
	gint i = gtk_assistant_get_current_page(assistant);

	if (i != n - 1) {
		GtkTreeIter iter;
		if (project_line_get_selected(&iter, DontWarnUnselection))
			line_delete(&iter, FALSE);
	}

	gtk_widget_destroy(widget);
}

static void
on_assistant_apply(GtkAssistant *assistant,
		   GtkBuilder *builder)
{
	gint n = gtk_assistant_get_n_pages(assistant);
	gint i = gtk_assistant_get_current_page(assistant);

	if (i == n - 1)
		gebr_ui_document_set_properties_from_builder(GEBR_GEOXML_DOCUMENT(gebr.line), builder);
}

static void
line_setup_wizard(GebrGeoXmlLine *line)
{
	GtkBuilder *builder = gtk_builder_new();

	if (!gtk_builder_add_from_file(builder, GEBR_GLADE_DIR "/document-properties.glade", NULL))
		return;

	GtkWidget *page1 = GTK_WIDGET(gtk_builder_get_object(builder, "table"));
	GtkWidget *page2 = GTK_WIDGET(gtk_builder_get_object(builder, "widget_paths"));
	GtkWidget *assistant = gtk_assistant_new();
	gtk_window_set_title(GTK_WINDOW(assistant), _("Creating a new Line"));
	g_signal_connect(assistant, "cancel", G_CALLBACK(on_assistant_cancel), NULL);
	g_signal_connect(assistant, "close", G_CALLBACK(on_assistant_close), NULL);
	g_signal_connect(assistant, "apply", G_CALLBACK(on_assistant_apply), builder);

	gtk_assistant_append_page(GTK_ASSISTANT(assistant), page1);
	gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page1, TRUE);
	gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page1, GTK_ASSISTANT_PAGE_CONTENT);
	gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page1, _("Basic line information"));

	gtk_assistant_append_page(GTK_ASSISTANT(assistant), page2);
	gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant), page2, TRUE);
	gtk_assistant_set_page_type(GTK_ASSISTANT(assistant), page2, GTK_ASSISTANT_PAGE_CONFIRM);
	gtk_assistant_set_page_title(GTK_ASSISTANT(assistant), page2, _("Line paths"));

	GtkTreeStore *store_paths = gebr_ui_document_create_paths_tree();
	GtkTreeView *view_paths = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview_paths"));
	gtk_tree_view_set_model(view_paths, GTK_TREE_MODEL(store_paths));
	gtk_tree_view_expand_all(view_paths);

	GObject *entry_title = gtk_builder_get_object(builder, "entry_title");
	GObject *entry_base = gtk_builder_get_object(builder, "entry_base");
	g_signal_connect(entry_title, "changed", G_CALLBACK(on_entry_changed), assistant);
	g_signal_connect(entry_base, "changed", G_CALLBACK(on_entry_changed), assistant);

	gtk_widget_show(assistant);
}

void line_new(void)
{
	GtkTreeIter iter;
	GtkTreeIter parent;
	GtkTreeModel *model;
	GebrGeoXmlLine *line;
	GebrGeoXmlDocument *doc;

	if (!project_line_get_selected (&parent, ProjectLineSelection))
		return;

	model = GTK_TREE_MODEL (gebr.ui_project_line->store);
	gtk_tree_model_get (model, &parent, PL_XMLPOINTER, &doc, -1);

	if (gebr_geoxml_document_get_type (doc) == GEBR_GEOXML_DOCUMENT_TYPE_LINE) {
		iter = parent;
		gtk_tree_model_iter_parent (model, &parent, &iter);
	}

	line = GEBR_GEOXML_LINE(document_new(GEBR_GEOXML_DOCUMENT_TYPE_LINE));
	gebr_geoxml_document_set_title(GEBR_GEOXML_DOC(line), _("New Line"));
	gebr_geoxml_document_set_author(GEBR_GEOXML_DOC(line), gebr.config.username->str);
	gebr_geoxml_document_set_email(GEBR_GEOXML_DOC(line), gebr.config.email->str);
	iter = project_append_line_iter(&parent, line);
	gebr_geoxml_project_append_line(gebr.project, gebr_geoxml_document_get_filename(GEBR_GEOXML_DOC(line)));
	document_save(GEBR_GEOXML_DOC(gebr.project), TRUE, FALSE);
	document_save(GEBR_GEOXML_DOC(line), TRUE, FALSE);

	project_line_select_iter(&iter);

	line_setup_wizard(gebr.line);

	//document_properties_setup_ui(GEBR_GEOXML_DOCUMENT(gebr.line), on_properties_response, TRUE);
}

gboolean line_delete(GtkTreeIter * iter, gboolean warn_user)
{
	GebrGeoXmlLine * line;
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->store), iter,
			   PL_XMLPOINTER, &line, -1);
	GtkTreeIter parent;
	gtk_tree_model_iter_parent(GTK_TREE_MODEL(gebr.ui_project_line->store), &parent, iter);
	GebrGeoXmlProject * project;
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->store), &parent,
			   PL_XMLPOINTER, &project, -1);

	/* removes its flows */
	GebrGeoXmlSequence *line_flow;
	for (gebr_geoxml_line_get_flow(line, &line_flow, 0); line_flow != NULL; gebr_geoxml_sequence_next(&line_flow)) {
		const gchar *flow_source;

		flow_source = gebr_geoxml_line_get_flow_source(GEBR_GEOXML_LINE_FLOW(line_flow));
		GString *path = document_get_path(flow_source);
		g_unlink(path->str);
		g_string_free(path, TRUE);

		/* log action */
		gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Deleting child Flow '%s'."), flow_source);
	}
	/* remove the line from its project */
	const gchar *line_filename = gebr_geoxml_document_get_filename(GEBR_GEOXML_DOC(line));
	if (gebr_geoxml_project_remove_line(project, line_filename)) {
		document_save(GEBR_GEOXML_DOC(project), TRUE, FALSE);
		document_delete(line_filename);
	}
	/* GUI */
	gebr_remove_help_edit_window(GEBR_GEOXML_DOCUMENT(line));
	gtk_tree_store_remove(GTK_TREE_STORE(gebr.ui_project_line->store), iter);

	/* inform the user */
	if (warn_user) {
		gebr_message(GEBR_LOG_INFO, TRUE, FALSE, _("Deleting Line '%s'."),
			     gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(line)));
		gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Deleting Line '%s' from project '%s'."),
			     gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(line)),
			     gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(project)));
	}

	return TRUE;
}

void line_set_paths_to_relative(GebrGeoXmlLine *line, gboolean relative)
{
	GString *path = g_string_new(NULL);
	GebrGeoXmlSequence *line_path;
	gebr_geoxml_line_get_path(line, &line_path, 0);
	for (; line_path != NULL; gebr_geoxml_sequence_next(&line_path)) {
		g_string_assign(path, gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(line_path)));
		gebr_path_set_to(path, relative);
		gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(line_path), path->str);
	}
	g_string_free(path, TRUE);
}

void line_set_paths_to_empty (GebrGeoXmlLine *line)
{
	GebrGeoXmlSequence *seq;
	gebr_geoxml_line_get_path (line, &seq, 0);
	while (seq) {
		gebr_geoxml_sequence_remove (seq);
		gebr_geoxml_line_get_path (line, &seq, 0);
	}

	GebrGeoXmlSequence *line_flow;
	gebr_geoxml_line_get_flow(line, &line_flow, 0);
	for (; line_flow != NULL; gebr_geoxml_sequence_next(&line_flow)) {
		const gchar *filename = gebr_geoxml_line_get_flow_source(GEBR_GEOXML_LINE_FLOW(line_flow));
		GebrGeoXmlFlow *flow;
	       	document_load((GebrGeoXmlDocument **)&flow, filename, TRUE);
		flow_set_paths_to_empty(flow);
	}
}

GtkTreeIter line_append_flow_iter(GebrGeoXmlFlow * flow, GebrGeoXmlLineFlow * line_flow)
{
	GtkTreeIter iter;

	/* add to the flow browser. */
	gtk_list_store_append(gebr.ui_flow_browse->store, &iter);
	gebr_geoxml_object_ref(flow);
	gebr_geoxml_object_ref(line_flow);
	gtk_list_store_set(gebr.ui_flow_browse->store, &iter,
			   FB_TITLE, gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(flow)),
			   FB_FILENAME, gebr_geoxml_document_get_filename(GEBR_GEOXML_DOC(flow)),
			   FB_LINE_FLOW_POINTER, line_flow,
			   FB_XMLPOINTER, flow,
			   -1);

	return iter;
}

void line_load_flows(void)
{
	GebrGeoXmlSequence *line_flow;
	GtkTreeIter iter;
	gboolean error = FALSE;

	flow_free();
	project_line_get_selected(&iter, DontWarnUnselection);

	/* iterate over its flows */
	gebr_geoxml_line_get_flow(gebr.line, &line_flow, 0);
	for (; line_flow; gebr_geoxml_sequence_next(&line_flow)) {
		GebrGeoXmlFlow *flow;

//		GebrGeoXmlSequence * next = line_flow;
//		gebr_geoxml_sequence_next(&next);

		const gchar *filename = gebr_geoxml_line_get_flow_source(GEBR_GEOXML_LINE_FLOW(line_flow));
		int ret = document_load_with_parent((GebrGeoXmlDocument**)(&flow), filename, &iter, TRUE);
		if (ret) {
//			line_flow = next;
			error = TRUE;
			continue;
		}

		line_append_flow_iter(flow, GEBR_GEOXML_LINE_FLOW(line_flow));

//		line_flow = next;
	}

	if (!error)
		gebr_message(GEBR_LOG_INFO, TRUE, FALSE, _("Flows loaded."));

	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter) == TRUE)
		flow_browse_select_iter(&iter);
}

void line_move_flow_top(void)
{
	GtkTreeIter iter;
	GebrGeoXmlSequence *line_flow;

	flow_browse_get_selected(&iter, FALSE);
	/* Update line XML */
	gebr_geoxml_line_get_flow(gebr.line, &line_flow,
				  gebr_gui_gtk_list_store_get_iter_index(gebr.ui_flow_browse->store, &iter));
	gebr_geoxml_sequence_move_after(line_flow, NULL);
	document_save(GEBR_GEOXML_DOC(gebr.line), TRUE, FALSE);
	/* GUI */
	gtk_list_store_move_after(GTK_LIST_STORE(gebr.ui_flow_browse->store), &iter, NULL);
}

void line_move_flow_bottom(void)
{
	GtkTreeIter iter;
	GebrGeoXmlSequence *line_flow;

	flow_browse_get_selected(&iter, FALSE);
	/* Update line XML */
	gebr_geoxml_line_get_flow(gebr.line, &line_flow,
				  gebr_gui_gtk_list_store_get_iter_index(gebr.ui_flow_browse->store, &iter));
	gebr_geoxml_sequence_move_before(line_flow, NULL);
	document_save(GEBR_GEOXML_DOC(gebr.line), TRUE, FALSE);
	/* GUI */
	gtk_list_store_move_before(GTK_LIST_STORE(gebr.ui_flow_browse->store), &iter, NULL);
}
