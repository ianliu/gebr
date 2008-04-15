/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2008 GeBR core team (http://gebr.sourceforge.net)
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

/*
 * File: flow.c
 * Flow manipulation
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <gtk/gtk.h>

#include <geoxml.h>
#include <comm.h>
#include <gui/utils.h>
#include <misc/date.h>

#include "flow.h"
#include "gebr.h"
#include "menu.h"
#include "support.h"
#include "document.h"
#include "server.h"
#include "ui_flow_browse.h"

/* errors strings. */
extern gchar * no_line_selected_error;
gchar * no_flow_selected_error =	_("No flow selected");
gchar * no_program_selected_error =	_("No program selected");

/*
 * Section: Public
 * Public functions.
 */

/*
 * Function: flow_new
 * Create a new flow
 */
gboolean
flow_new(void)
{
	GtkTreeSelection *	selection;
	GtkTreeIter		iter;
	GtkTreePath *           path;

	gchar *			flow_title;
	gchar *			line_title;
	gchar *			line_filename;

	GeoXmlFlow *		flow;

	if (gebr.line == NULL) {
		gebr_message(LOG_ERROR, TRUE, FALSE, _("Select a line to which a flow will be added to"));
		return FALSE;
	}

	flow_title = _("New Flow");
	line_title = (gchar *)geoxml_document_get_title(GEOXML_DOCUMENT(gebr.line));
	line_filename = (gchar *)geoxml_document_get_filename(GEOXML_DOCUMENT(gebr.line));

	/* Create a new flow */
	flow = GEOXML_FLOW(document_new(GEOXML_DOCUMENT_TYPE_FLOW));
	geoxml_document_set_title(GEOXML_DOC(flow), flow_title);
	geoxml_document_set_author(GEOXML_DOC(flow), gebr.config.username->str);
	geoxml_document_set_email(GEOXML_DOC(flow), gebr.config.email->str);
	/* and add to current line */
	geoxml_line_add_flow(gebr.line, geoxml_document_get_filename(GEOXML_DOC(flow)));
	document_save(GEOXML_DOC(gebr.line));
	document_save(GEOXML_DOC(flow));
	geoxml_document_free(GEOXML_DOC(flow));
	/* and add to the GUI */
	gtk_list_store_append(gebr.ui_flow_browse->store, &iter);
	gtk_list_store_set(gebr.ui_flow_browse->store, &iter,
		FB_TITLE, flow_title,
		FB_FILENAME, geoxml_document_get_filename(GEOXML_DOC(flow)),
		-1);
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter);
	gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(gebr.ui_flow_browse->view), path,
				     NULL, FALSE, 0, 0);
	/* select it */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
	gtk_tree_selection_select_iter(selection, &iter);
	g_signal_emit_by_name(gebr.ui_flow_browse->view, "cursor-changed");

	/* feedback */
	gebr_message(LOG_INFO, TRUE, TRUE, _("New flow added to line '%s'"), line_title);

	return TRUE;
}

/* Function: flow_free
 * Frees the memory allocated to a flow
 *
 * Besides, update the detailed view of a flow in the interface.
 */
void
flow_free(void)
{
	if (gebr.flow != NULL) {
		gtk_list_store_clear(gebr.ui_flow_edition->fseq_store);
		geoxml_document_free(GEOXML_DOC(gebr.flow));
		gebr.flow = NULL;

		flow_browse_info_update();
	}
}

/*
 * Function: flow_delete
 * Delete the selected flow in flow browser
 *
 */
void
flow_delete(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		flow_iter;

	gchar *			title;
	gchar *			filename;

	GeoXmlSequence *	line_flow;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
	if (gtk_tree_selection_get_selected(selection, &model, &flow_iter) == FALSE) {
		gebr_message(LOG_ERROR, TRUE, FALSE, no_flow_selected_error);
		return;
	}

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &flow_iter,
		FB_TITLE, &title,
		FB_FILENAME, &filename,
		-1);

	if (confirm_action_dialog(_("Delete flow"), _("Are you sure you want to delete flow '%s'?"), title) == FALSE)
		goto out;

	/* Some feedback */
	gebr_message(LOG_INFO, TRUE, FALSE, _("Erasing flow '%s'"), title);
	gebr_message(LOG_INFO, FALSE, TRUE, _("Erasing flow '%s' from line '%s'"),
		title, geoxml_document_get_title(GEOXML_DOCUMENT(gebr.line)));

	/* Seek and destroy */
	geoxml_line_get_flow(gebr.line, &line_flow, 0);
	while (line_flow != NULL) {
		if (strcmp(filename, geoxml_line_get_flow_source(GEOXML_LINE_FLOW(line_flow))) == 0) {
			geoxml_sequence_remove(line_flow);
			document_save(GEOXML_DOC(gebr.line));
			break;
		}
		geoxml_sequence_next(&line_flow);
	}

	/* Free and delete flow from the disk */
	flow_free();
	document_delete(filename);

	/* Finally, from the GUI */
	gtk_list_store_remove(GTK_LIST_STORE (gebr.ui_flow_browse->store), &flow_iter);
	gtk_list_store_clear(gebr.ui_flow_edition->fseq_store);

out:	g_free(title);
	g_free(filename);
}

/*
 * Function: flow_save
 * Save the current flow
 */
void
flow_save(void)
{
	/* TODO: report errors on document_save */
	document_save(GEOXML_DOC(gebr.flow));
	flow_browse_info_update();
}

/*
 * Function: flow_import
 * Import flow from file to the current line
 */
void
flow_import(void)
{
	GtkTreeSelection *	selection;
	GtkTreeIter		iter;
	GtkTreePath *           path;

	GtkWidget *		chooser_dialog;
	GtkFileFilter *		filefilter;
	gchar *			dir;

	gchar *			flow_title;
	GString *		flow_filename;

	GeoXmlFlow *		imported_flow;

	if (gebr.line == NULL) {
		gebr_message(LOG_ERROR, TRUE, FALSE, _("Select a line to which a flow will be added to"));
		return;
	}

	/* assembly a file chooser dialog */
	chooser_dialog = gtk_file_chooser_dialog_new(_("Choose filename to save"),
		GTK_WINDOW(gebr.window),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_OPEN, GTK_RESPONSE_YES,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(chooser_dialog), TRUE);
	filefilter = gtk_file_filter_new();
	gtk_file_filter_set_name(filefilter, _("Flow files (*.flw)"));
	gtk_file_filter_add_pattern(filefilter, "*.flw");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_dialog), filefilter);

	/* show file chooser */
	gtk_widget_show(chooser_dialog);
	if (gtk_dialog_run(GTK_DIALOG(chooser_dialog)) != GTK_RESPONSE_YES)
		goto out;

	/* load flow */
	dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser_dialog));
	imported_flow = GEOXML_FLOW(document_load_path(dir));
	if (imported_flow == NULL)
		goto out2;

	/* initialization */
	flow_title = (gchar *)geoxml_document_get_title(GEOXML_DOC(imported_flow));
	flow_filename = document_assembly_filename("flw");

	/* feedback */
	gebr_message(LOG_INFO, TRUE, TRUE, _("Flow '%s' imported to line '%s' from file '%s'"),
		     flow_title, geoxml_document_get_title(GEOXML_DOC(gebr.line)), dir);

	/* change filename */
	geoxml_document_set_filename(GEOXML_DOC(imported_flow), flow_filename->str);
	document_save(GEOXML_DOC(imported_flow));
	geoxml_document_free(GEOXML_DOC(imported_flow));
	/* and add it to the line */
	geoxml_line_append_flow(gebr.line, flow_filename->str);
	document_save(GEOXML_DOC(gebr.line));
	/* and to the GUI */
	gtk_list_store_append(gebr.ui_flow_browse->store, &iter);
	gtk_list_store_set(gebr.ui_flow_browse->store, &iter,
		FB_TITLE, flow_title,
		FB_FILENAME, flow_filename->str,
		-1);

	/* select it */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
	gtk_tree_selection_select_iter (selection, &iter);
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter);
	gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(gebr.ui_flow_browse->view), path,
				     NULL, FALSE, 0, 0);
	g_signal_emit_by_name(gebr.ui_flow_browse->view, "cursor-changed");

	/* frees */
	g_string_free(flow_filename, TRUE);
	gtk_tree_path_free(path);
out2:	g_free(dir);
out:	gtk_widget_destroy(chooser_dialog);
}

/*
 * Function: flow_export
 * Export current flow to a file
 */
void
flow_export(void)
{
	GtkWidget *		chooser_dialog;
	GtkFileFilter *		filefilter;
	gchar *			path;
	gchar *			filename;
	gchar *                 oldfilename;

	if (gebr.flow == NULL) {
		gebr_message(LOG_ERROR, TRUE, FALSE, no_flow_selected_error);
		return;
	}

	/* run file chooser */
	chooser_dialog = gtk_file_chooser_dialog_new(_("Choose filename to save"),
		GTK_WINDOW(gebr.window),
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_SAVE, GTK_RESPONSE_YES,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(chooser_dialog), TRUE);
	filefilter = gtk_file_filter_new();
	gtk_file_filter_set_name(filefilter, _("Flow files (*.flw)"));
	gtk_file_filter_add_pattern(filefilter, "*.flw");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_dialog), filefilter);

	/* show file chooser */
	gtk_widget_show(chooser_dialog);
	if (gtk_dialog_run(GTK_DIALOG(chooser_dialog)) != GTK_RESPONSE_YES)
		goto out;
	path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (chooser_dialog));
	filename = g_path_get_basename(path);

	/* export current flow to disk */
	oldfilename = (gchar*)geoxml_document_get_filename(GEOXML_DOC(gebr.flow));
	geoxml_document_set_filename(GEOXML_DOC(gebr.flow), filename);
	geoxml_document_save(GEOXML_DOC(gebr.flow), path);
	geoxml_document_set_filename(GEOXML_DOC(gebr.flow), oldfilename);

	gebr_message(LOG_INFO, TRUE, TRUE, _("Flow '%s' exported to %s"),
		(gchar*)geoxml_document_get_title(GEOXML_DOC(gebr.flow)), path);

	g_free(path);
	g_free(filename);

out:	gtk_widget_destroy(chooser_dialog);
}

/*
 * Function: flow_export_parameters_cleanup
 * Cleanup (if group recursevely) parameters value.
 * If _use_value_ is TRUE the value is made default
 */
static void
flow_export_parameters_cleanup(GeoXmlParameters * parameters, gboolean use_value)
{
	GeoXmlSequence *	parameter;

	parameter = geoxml_parameters_get_first_parameter(parameters);
	while (parameter != NULL) {
		if (geoxml_parameter_get_is_program_parameter(GEOXML_PARAMETER(parameter)) == TRUE) {
			if (use_value == TRUE)
				geoxml_program_parameter_set_default(GEOXML_PROGRAM_PARAMETER(parameter),
					geoxml_program_parameter_get_value(GEOXML_PROGRAM_PARAMETER(parameter)));
			geoxml_program_parameter_set_value(GEOXML_PROGRAM_PARAMETER(parameter), "");
		} else if (geoxml_parameter_get_type(GEOXML_PARAMETER(parameter)) == GEOXML_PARAMETERTYPE_GROUP)
			flow_export_parameters_cleanup(GEOXML_PARAMETERS(parameter), use_value);

		geoxml_sequence_next(&parameter);
	}
}

/*
 * Function: flow_export_as_menu
 * Export current flow converting it to a menu.
 */
void
flow_export_as_menu(void)
{
	GtkWidget *		dialog;
	GtkFileFilter *		filefilter;

	gchar *			path;
	gchar *			filename;
	GString *               menufn;
	GString *               menupath;


	GeoXmlFlow *		flow;
	GeoXmlSequence *	program;
	gboolean		use_value;
	gint			i;

	if (gebr.flow == NULL) {
		gebr_message(LOG_ERROR, TRUE, FALSE, no_flow_selected_error);
		return;
	}

	/* run file chooser */
	dialog = gtk_file_chooser_dialog_new(_("Choose filename to save"),
		GTK_WINDOW(gebr.window),
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_SAVE, GTK_RESPONSE_YES,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), gebr.config.usermenus->str);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
	filefilter = gtk_file_filter_new();
	gtk_file_filter_set_name(filefilter, _("Menu files (*.mnu)"));
	gtk_file_filter_add_pattern(filefilter, "*.mnu");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filefilter);

	/* show file chooser */
	gtk_widget_show(dialog);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_YES)
		goto out;
	path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (dialog));
	filename = g_path_get_basename(path);

	/* first clone it */
	flow = GEOXML_FLOW(geoxml_document_clone(GEOXML_DOC(gebr.flow)));

	gtk_widget_destroy(dialog);
	dialog = gtk_message_dialog_new(GTK_WINDOW(gebr.window),
		GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
		GTK_MESSAGE_QUESTION,
		GTK_BUTTONS_YES_NO,
		"Do you want to use your parameters' values as default values?");
	gtk_window_set_title(GTK_WINDOW(dialog), _("Default values"));

	geoxml_flow_get_program(flow, &program, 0);
	use_value = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES ? TRUE : FALSE;
	i = 0;
	while (program != NULL) {
		GString *	menu_help;

		menu_help = menu_get_help_from_program_ref(GEOXML_PROGRAM(program));
		geoxml_program_set_help(GEOXML_PROGRAM(program), menu_help->str);

		flow_export_parameters_cleanup(
			geoxml_program_get_parameters(GEOXML_PROGRAM(program)),
			use_value);
		geoxml_program_set_menu(GEOXML_PROGRAM(program), filename, i++);
		geoxml_program_set_status(GEOXML_PROGRAM(program), "unconfigured");

		geoxml_sequence_next(&program);
		g_string_free(menu_help, TRUE);
	}

	geoxml_flow_io_set_input(flow, "");
	geoxml_flow_io_set_output(flow, "");
	geoxml_flow_io_set_error(flow, "");

	geoxml_flow_set_date_last_run(flow, "");
	geoxml_document_set_date_created(GEOXML_DOC(flow), iso_date());
	geoxml_document_set_date_modified(GEOXML_DOC(flow), iso_date());

	geoxml_document_set_help(GEOXML_DOC(flow), "");
	menufn = g_string_new(filename);
	menupath = g_string_new(path);
	if (!g_str_has_suffix(filename,".mnu")){
		g_string_append(menufn, ".mnu");
		g_string_append(menupath, ".mnu");
	}
	geoxml_document_set_filename(GEOXML_DOC(flow), menufn->str);
	geoxml_document_save(GEOXML_DOC(flow), menupath->str);
	geoxml_document_free(GEOXML_DOC(flow));

	gebr_message(LOG_INFO, TRUE, TRUE, _("Flow '%s' exported as menu to %s"),
		     (gchar*)geoxml_document_get_title(GEOXML_DOC(gebr.flow)), path);

	g_free(path);
	g_free(filename);
	g_string_free(menufn, TRUE);
	g_string_free(menupath, TRUE);

out:	gtk_widget_destroy(dialog);
}

/*
 * Function: flow_run
 * Runs a flow
 */
void
flow_run(void)
{
	GString *		path;

	struct server *		server;

	/* check for a flow selected */
	if (gebr.flow == NULL) {
		gebr_message(LOG_ERROR, TRUE, FALSE, no_flow_selected_error);
		return;
	}
	server = server_select_setup_ui();
	if (server == NULL)
		return;

	gebr_message(LOG_INFO, TRUE, FALSE, _("Asking server to run flow '%s'"),
		geoxml_document_get_title(GEOXML_DOC(gebr.flow)));
	gebr_message(LOG_INFO, FALSE, TRUE, _("Asking server '%s' to run flow '%s'"),
		server->comm->address->str,
		geoxml_document_get_title(GEOXML_DOC(gebr.flow)));

	comm_server_run_flow(server->comm, gebr.flow);

	/* get today's date */
	geoxml_flow_set_date_last_run(gebr.flow, iso_date());

	/* TODO: check save */
	/* Saving manualy to preserve modified date */
	path = document_get_path(geoxml_document_get_filename(GEOXML_DOC(gebr.flow)));
	geoxml_document_save(GEOXML_DOC(gebr.flow), path->str);
	g_string_free(path, TRUE);

	/* Update details */
	flow_browse_info_update();
}

/*
 * Function: flow_program_remove
 * Remove selected program from flow process
 */
void
flow_program_remove(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	GeoXmlSequence *	program;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE) {
		gebr_message(LOG_ERROR, TRUE, FALSE, no_program_selected_error);
		return;
	}

	/* SEEK AND DESTROY */
	geoxml_flow_get_program(gebr.flow, &program,
		gtk_list_store_get_iter_index(gebr.ui_flow_edition->fseq_store, &iter));
	geoxml_sequence_remove(program);
	flow_save();
	/* from GUI... */
	gtk_list_store_remove(GTK_LIST_STORE (gebr.ui_flow_edition->fseq_store), &iter);
}

/*
 * Function: flow_program_move_up
 * Move selected program up in the processing flow
 */
void
flow_program_move_up(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	GeoXmlSequence *	program;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE) {
		gebr_message(LOG_ERROR, TRUE, FALSE, no_program_selected_error);
		return;
	}
	if (gtk_list_store_can_move_up(gebr.ui_flow_edition->fseq_store, &iter) == FALSE)
		return;

	/* Update flow */
	geoxml_flow_get_program(gebr.flow, &program,
		gtk_list_store_get_iter_index(gebr.ui_flow_edition->fseq_store, &iter));
	geoxml_sequence_move_up(program);
	flow_save();
	/* Update GUI */
	gtk_list_store_move_up(gebr.ui_flow_edition->fseq_store, &iter);
}

/*
 * Function: flow_program_move_down
 * Move selected program down in the processing flow
 */
void
flow_program_move_down(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	GeoXmlSequence *	program;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE) {
		gebr_message(LOG_ERROR, TRUE, FALSE, no_program_selected_error);
		return;
	}
	if (gtk_list_store_can_move_down(gebr.ui_flow_edition->fseq_store, &iter) == FALSE)
		return;

	/* Update flow */
	geoxml_flow_get_program(gebr.flow, &program,
		gtk_list_store_get_iter_index(gebr.ui_flow_edition->fseq_store, &iter));
	geoxml_sequence_move_down(program);
	flow_save();
	/* Update GUI */
	gtk_list_store_move_down(gebr.ui_flow_edition->fseq_store, &iter);
}

/*
 * Function: flow_program_move_top
 * Move selected program to top in the processing flow
 */
void
flow_program_move_top(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	GeoXmlSequence *	program;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE) {
		gebr_message(LOG_ERROR, TRUE, FALSE, no_program_selected_error);
		return;
	}
	if (gtk_list_store_can_move_up(gebr.ui_flow_edition->fseq_store, &iter) == FALSE)
		return;

	/* Update flow */
	geoxml_flow_get_program(gebr.flow, &program,
		gtk_list_store_get_iter_index(gebr.ui_flow_edition->fseq_store, &iter));
	geoxml_sequence_move_after(program, NULL);
	flow_save();
	/* Update GUI */
	gtk_list_store_move_after(GTK_LIST_STORE(gebr.ui_flow_edition->fseq_store), &iter, NULL);
}

/*
 * Function: flow_program_move_bottom
 * Move selected program to bottom in the processing flow
 */
void
flow_program_move_bottom(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	GeoXmlSequence *	program;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE) {
		gebr_message(LOG_ERROR, TRUE, FALSE, no_program_selected_error);
		return;
	}
	if (gtk_list_store_can_move_down(gebr.ui_flow_edition->fseq_store, &iter) == FALSE)
		return;

	/* Update flow */
	geoxml_flow_get_program(gebr.flow, &program,
		gtk_list_store_get_iter_index(gebr.ui_flow_edition->fseq_store, &iter));
	geoxml_sequence_move_before(program, NULL);
	flow_save();
	/* Update GUI */
	gtk_list_store_move_before(GTK_LIST_STORE(gebr.ui_flow_edition->fseq_store), &iter, NULL);
}

/*
 * Function: flow_move_up
 * Move flow up
 */
void
flow_move_up(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	GeoXmlSequence *	line_flow;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE) {
		gebr_message(LOG_ERROR, TRUE, FALSE, no_program_selected_error);
		return;
	}
	if (gtk_list_store_can_move_up(gebr.ui_flow_browse->store, &iter) == FALSE)
		return;

	/* Update line XML */
	geoxml_line_get_flow(gebr.line, &line_flow,
		gtk_list_store_get_iter_index(gebr.ui_flow_browse->store, &iter));
	geoxml_sequence_move_up(line_flow);
	document_save(GEOXML_DOC(gebr.line));
	/* Update GUI */
	gtk_list_store_move_up(gebr.ui_flow_browse->store, &iter);
}

/*
 * Function: flow_move_down
 * Move flow down
 */
void
flow_move_down(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	GeoXmlSequence *	line_flow;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE) {
		gebr_message(LOG_ERROR, TRUE, FALSE, no_program_selected_error);
		return;
	}
	if (gtk_list_store_can_move_down(gebr.ui_flow_browse->store, &iter) == FALSE)
		return;

	/* Update line XML */
	geoxml_line_get_flow(gebr.line, &line_flow,
		gtk_list_store_get_iter_index(gebr.ui_flow_browse->store, &iter));
	geoxml_sequence_move_down(line_flow);
	document_save(GEOXML_DOC(gebr.line));
	/* Update GUI */
	gtk_list_store_move_down(gebr.ui_flow_browse->store, &iter);
}

/*
 * Function: flow_move_top
 * Move flow top
 */
void
flow_move_top(void)
{
	GtkTreeIter		iter;
	GtkTreeModel *		model;
	GtkTreeSelection *	selection;

	GeoXmlSequence *	line_flow;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE) {
		gebr_message(LOG_ERROR, TRUE, FALSE, no_flow_selected_error);
		return;
	}

	/* Update line XML */
	geoxml_line_get_flow(gebr.line, &line_flow,
		gtk_list_store_get_iter_index(gebr.ui_flow_browse->store, &iter));
	geoxml_sequence_move_after(line_flow, NULL);
	document_save(GEOXML_DOC(gebr.line));
	/* GUI */
	gtk_list_store_move_after(GTK_LIST_STORE(gebr.ui_flow_browse->store), &iter, NULL);
}

/*
 * Function: flow_move_bottom
 * Move flow bottom
 */
void
flow_move_bottom(void)
{
	GtkTreeIter		iter;
	GtkTreeModel *		model;
	GtkTreeSelection *	selection;

	GeoXmlSequence *	line_flow;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE) {
		gebr_message(LOG_ERROR, TRUE, FALSE, no_flow_selected_error);
		return;
	}

	/* Update line XML */
	geoxml_line_get_flow(gebr.line, &line_flow,
		gtk_list_store_get_iter_index(gebr.ui_flow_browse->store, &iter));
	geoxml_sequence_move_before(line_flow, NULL);
	document_save(GEOXML_DOC(gebr.line));
	/* GUI */
	gtk_list_store_move_before(GTK_LIST_STORE(gebr.ui_flow_browse->store), &iter, NULL);
}
