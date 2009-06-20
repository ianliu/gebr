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

/*
 * File: flow.c
 * Flow manipulation
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <gtk/gtk.h>

#include <libgebr/intl.h>
#include <libgebr/geoxml.h>
#include <libgebr/comm.h>
#include <libgebr/gui/utils.h>
#include <libgebr/date.h>
#include <libgebr/utils.h>

#include "flow.h"
#include "line.h"
#include "gebr.h"
#include "menu.h"
#include "document.h"
#include "server.h"
#include "callbacks.h"
#include "ui_flow.h"
#include "ui_flow_browse.h"

/* errors strings. */
extern gchar * no_line_selected_error;
gchar * no_flow_selected_error =	N_("No flow selected");
gchar * no_program_selected_error =	N_("No program selected");

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

	gchar *			line_title;
	gchar *			line_filename;

	GeoXmlFlow *		flow;

	if (gebr.line == NULL) {
		gebr_message(LOG_ERROR, TRUE, FALSE, _("Select a line to which a flow will be added to"));
		return FALSE;
	}
	line_title = (gchar *)geoxml_document_get_title(GEOXML_DOCUMENT(gebr.line));
	line_filename = (gchar *)geoxml_document_get_filename(GEOXML_DOCUMENT(gebr.line));

	/* Create a new flow */
	flow = GEOXML_FLOW(document_new(GEOXML_DOCUMENT_TYPE_FLOW));
	geoxml_document_set_title(GEOXML_DOC(flow), _("New Flow"));
	geoxml_document_set_author(GEOXML_DOC(flow), gebr.config.username->str);
	geoxml_document_set_email(GEOXML_DOC(flow), gebr.config.email->str);
	/* and add to the GUI */
	gtk_list_store_append(gebr.ui_flow_browse->store, &iter);
	gtk_list_store_set(gebr.ui_flow_browse->store, &iter,
		FB_TITLE, geoxml_document_get_title(GEOXML_DOC(flow)),
		FB_FILENAME, geoxml_document_get_filename(GEOXML_DOC(flow)),
		-1);

	/* and add to current line */
	geoxml_line_append_flow(gebr.line, geoxml_document_get_filename(GEOXML_DOC(flow)));
	line_save();
	document_save(GEOXML_DOC(flow));
	geoxml_document_free(GEOXML_DOC(flow));

	/* select it */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
	gtk_tree_selection_select_iter(selection, &iter);
	g_signal_emit_by_name(gebr.ui_flow_browse->view, "cursor-changed");
	libgebr_gtk_tree_view_scroll_to_iter_cell(GTK_TREE_VIEW(gebr.ui_flow_browse->view), &iter);

	/* feedback */
	gebr_message(LOG_INFO, TRUE, TRUE, _("New flow added to line '%s'"), line_title);
	on_flow_properties_activate();

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
		geoxml_document_free(GEOXML_DOC(gebr.flow));
		gebr.flow = NULL;
	}
	gtk_list_store_clear(gebr.ui_flow_edition->fseq_store);
	gtk_container_foreach(GTK_CONTAINER(gebr.ui_flow_browse->revisions_menu),
		(GtkCallback)gtk_widget_destroy, NULL);
	flow_browse_info_update();
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
	for (; line_flow != NULL; geoxml_sequence_next(&line_flow)) {
		if (strcmp(filename, geoxml_line_get_flow_source(GEOXML_LINE_FLOW(line_flow))) == 0) {
			geoxml_sequence_remove(line_flow);
			line_save();
			break;
		}
	}

	/* Free and delete flow from the disk */
	flow_free();
	document_delete(filename);

	/* Finally, from the GUI */
	gtk_tree_view_select_sibling(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
	gtk_list_store_remove(GTK_LIST_STORE(gebr.ui_flow_browse->store), &flow_iter);

out:	g_free(title);
	g_free(filename);
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

	GtkWidget *		chooser_dialog;
	GtkFileFilter *		file_filter;
	gchar *			dir;

	gchar *			flow_title;
	GString *		flow_filename;

	GeoXmlFlow *		imported_flow;

	if (gebr.line == NULL) {
		gebr_message(LOG_ERROR, TRUE, FALSE, _("Select a line to which a flow will be added to"));
		return;
	}

	/* assembly a file chooser dialog */
	chooser_dialog = gtk_file_chooser_dialog_new(_("Choose filename to open"),
		GTK_WINDOW(gebr.window),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_OPEN, GTK_RESPONSE_YES,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(chooser_dialog), TRUE);
	file_filter = gtk_file_filter_new();
	gtk_file_filter_set_name(file_filter, _("Flow files (*.flw)"));
	gtk_file_filter_add_pattern(file_filter, "*.flw");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_dialog), file_filter);

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
	line_save();
	/* and to the GUI */
	gtk_list_store_append(gebr.ui_flow_browse->store, &iter);
	gtk_list_store_set(gebr.ui_flow_browse->store, &iter,
		FB_TITLE, flow_title,
		FB_FILENAME, flow_filename->str,
		-1);

	/* select it */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
	gtk_tree_selection_select_iter(selection, &iter);
	g_signal_emit_by_name(gebr.ui_flow_browse->view, "cursor-changed");
	libgebr_gtk_tree_view_scroll_to_iter_cell(GTK_TREE_VIEW(gebr.ui_flow_browse->view), &iter);

	/* frees */
	g_string_free(flow_filename, TRUE);
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
	GtkFileFilter *		file_filter;

	GString *		path;
	gchar *			tmp;

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
	file_filter = gtk_file_filter_new();
	gtk_file_filter_set_name(file_filter, _("Flow files (*.flw)"));
	gtk_file_filter_add_pattern(file_filter, "*.flw");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_dialog), file_filter);

	/* show file chooser */
	gtk_widget_show(chooser_dialog);
	if (gtk_dialog_run(GTK_DIALOG(chooser_dialog)) != GTK_RESPONSE_YES)
		goto out;
	tmp = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser_dialog));
	path = g_string_new(tmp);
	append_filename_extension(path, ".flw");
	filename = g_path_get_basename(path->str);

	/* export current flow to disk */
	oldfilename = (gchar*)geoxml_document_get_filename(GEOXML_DOC(gebr.flow));
	geoxml_document_set_filename(GEOXML_DOC(gebr.flow), filename);
	geoxml_document_save(GEOXML_DOC(gebr.flow), path->str);
	geoxml_document_set_filename(GEOXML_DOC(gebr.flow), oldfilename);

	gebr_message(LOG_INFO, TRUE, TRUE, _("Flow '%s' exported to %s"),
		(gchar*)geoxml_document_get_title(GEOXML_DOC(gebr.flow)), path->str);

	/* frees */
	g_string_free(path, TRUE);
	g_free(tmp);
	g_free(filename);

out:	gtk_widget_destroy(chooser_dialog);
}

/*
 * Function: flow_export_parameters_cleanup
 * Cleanup (if group recursively) parameters value.
 * If _use_value_as_default_ is TRUE the value is made default
 */
static void
flow_export_parameters_cleanup(GeoXmlParameters * parameters, gboolean use_value_as_default)
{
	GeoXmlSequence *	parameter;

	parameter = geoxml_parameters_get_first_parameter(parameters);
	for (; parameter != NULL; geoxml_sequence_next(&parameter)) {
		if (geoxml_parameter_get_is_program_parameter(GEOXML_PARAMETER(parameter)) == TRUE) {
			GeoXmlSequence *	value;

			if (use_value_as_default == TRUE) {
				GeoXmlSequence *	default_value;

				geoxml_program_parameter_get_value(GEOXML_PROGRAM_PARAMETER(parameter),
					FALSE, &value, 0);
				geoxml_program_parameter_get_value(GEOXML_PROGRAM_PARAMETER(parameter),
					TRUE, &default_value, 0);
				for (; value != NULL; geoxml_sequence_next(&value),
				geoxml_sequence_next(&default_value)) {
					if (default_value == NULL)
						default_value = GEOXML_SEQUENCE(geoxml_program_parameter_append_value(
							GEOXML_PROGRAM_PARAMETER(parameter), TRUE));
					geoxml_value_sequence_set(GEOXML_VALUE_SEQUENCE(default_value),
						geoxml_value_sequence_get(GEOXML_VALUE_SEQUENCE(value)));
				}

				/* remove extras default values */
				while (default_value != NULL) {
					GeoXmlSequence *	tmp;

					tmp = default_value;
					geoxml_sequence_next(&tmp);
					geoxml_sequence_remove(default_value);
					default_value = tmp;
				}
			}

			geoxml_program_parameter_get_value(GEOXML_PROGRAM_PARAMETER(parameter),
				FALSE, &value, 0);
			for (; value != NULL; geoxml_sequence_next(&value))
				geoxml_value_sequence_set(GEOXML_VALUE_SEQUENCE(value), "");
		} else { /* a group, time for recursion! */
			GeoXmlSequence *	instance;

			geoxml_parameter_group_get_instance(GEOXML_PARAMETER_GROUP(parameter), &instance, 0);
			for (; instance != NULL; geoxml_sequence_next(&instance))
				flow_export_parameters_cleanup(GEOXML_PARAMETERS(instance), use_value_as_default);
		}
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
	GtkFileFilter *		file_filter;

	GString *		path;
	gchar *			filename;

	GeoXmlFlow *		flow;
	GeoXmlSequence *	program;
	gboolean		use_value;
	gint			i;
	gchar *			tmp;

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
	file_filter = gtk_file_filter_new();
	gtk_file_filter_set_name(file_filter, _("Menu files (*.mnu)"));
	gtk_file_filter_add_pattern(file_filter, "*.mnu");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), file_filter);

	/* show file chooser */
	gtk_widget_show(dialog);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_YES)
		goto out;
	tmp = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (dialog));
	path = g_string_new(tmp);
	append_filename_extension(path, ".mnu");
	filename = g_path_get_basename(path->str);

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
	for (; program != NULL; geoxml_sequence_next(&program)) {
		GString *	menu_help;

		menu_help = menu_get_help_from_program_ref(GEOXML_PROGRAM(program));
		geoxml_program_set_help(GEOXML_PROGRAM(program), menu_help->str);

		flow_export_parameters_cleanup(
			geoxml_program_get_parameters(GEOXML_PROGRAM(program)),
			use_value);
		geoxml_program_set_menu(GEOXML_PROGRAM(program), filename, i++);
		geoxml_program_set_status(GEOXML_PROGRAM(program), "unconfigured");

		g_string_free(menu_help, TRUE);
	}

	geoxml_flow_io_set_input(flow, "");
	geoxml_flow_io_set_output(flow, "");
	geoxml_flow_io_set_error(flow, "");

	geoxml_flow_set_date_last_run(flow, "");
	geoxml_document_set_date_created(GEOXML_DOC(flow), iso_date());
	geoxml_document_set_date_modified(GEOXML_DOC(flow), iso_date());

	geoxml_document_set_help(GEOXML_DOC(flow), "");
	geoxml_document_set_filename(GEOXML_DOC(flow), filename);
	geoxml_document_save(GEOXML_DOC(flow), path->str);
	geoxml_document_free(GEOXML_DOC(flow));

	gebr_message(LOG_INFO, TRUE, TRUE, _("Flow '%s' exported as menu to %s"),
		     (gchar*)geoxml_document_get_title(GEOXML_DOC(gebr.flow)), path);

	/* frees */
	g_string_free(path, TRUE);
	g_free(filename);
	g_free(tmp);

out:	gtk_widget_destroy(dialog);
}

/*
 * Function: flow_run
 * Runs a flow
 */
void
flow_run(void)
{
	GeoXmlFlow *		flow;
	GeoXmlSequence *	i;
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

	flow = GEOXML_FLOW(geoxml_document_clone(GEOXML_DOCUMENT(gebr.flow)));
	/* Strip flow: remove helps and revisions */
	geoxml_document_set_help(GEOXML_DOCUMENT(flow), "");
	geoxml_flow_get_program(flow, &i, 0);
	for (; i != NULL; geoxml_sequence_next(&i))
		geoxml_program_set_help(GEOXML_PROGRAM(i), "");
	geoxml_flow_get_revision(flow, &i, 0);
	while (i != NULL) {
		GeoXmlSequence *	tmp;

		tmp = i;
		geoxml_sequence_next(&tmp);
		geoxml_sequence_remove(i);
		i = tmp;
	}
	/* RUN */
	comm_server_run_flow(server->comm, flow);

	/* TODO: check save */
	/* Save manualy to preserve run date */
	geoxml_flow_set_date_last_run(gebr.flow, iso_date());
	path = document_get_path(geoxml_document_get_filename(GEOXML_DOC(gebr.flow)));
	geoxml_document_save(GEOXML_DOC(gebr.flow), path->str);
	flow_browse_info_update();

	gebr_message(LOG_INFO, TRUE, FALSE, _("Asking server to run flow '%s'"),
		geoxml_document_get_title(GEOXML_DOC(flow)));
	if (comm_server_is_local(server->comm) == FALSE) {
		gebr_message(LOG_INFO, FALSE, TRUE, _("Asking server '%s' to run flow '%s'"),
			server->comm->address->str,
			geoxml_document_get_title(GEOXML_DOC(flow)));
	} else {
		gebr_message(LOG_INFO, FALSE, TRUE, _("Asking local server to run flow '%s'"),
			geoxml_document_get_title(GEOXML_DOC(flow)));
	}

	/* frees */
	g_string_free(path, TRUE);
	geoxml_document_free(GEOXML_DOCUMENT(flow));
}

/*
 * Function: flow_revision_save
 * Make a revision from current flow.
 * Opens a dialog asking the user for a comment of it.
 */
gboolean
flow_revision_save(void)
{
	GtkWidget *		dialog;
	GtkBox *		vbox;
	GtkWidget *		label;
	GtkWidget *		entry;
	gboolean		ret;

	if (gebr.flow == NULL)
		return FALSE;

	dialog = gtk_dialog_new_with_buttons(_("Save flow state"),
		GTK_WINDOW(gebr.window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		NULL);
	vbox = GTK_BOX(GTK_DIALOG(dialog)->vbox);

	label = gtk_label_new(_("Make a comment for this state:"));
	gtk_box_pack_start(vbox, label, FALSE, TRUE, 0);
	entry = gtk_entry_new();
	gtk_box_pack_start(vbox, entry, FALSE, TRUE, 0);

	gtk_widget_show_all(dialog);
	switch (gtk_dialog_run(GTK_DIALOG(dialog))) {
	case GTK_RESPONSE_OK: {
		GeoXmlRevision *	revision;

		revision = geoxml_flow_append_revision(gebr.flow, gtk_entry_get_text(GTK_ENTRY(entry)));
		document_save(GEOXML_DOCUMENT(gebr.flow));
		flow_browse_load_revision(revision, TRUE);
		ret = TRUE;

		break;
	} default:
		ret = FALSE;
		break;
	}

	gtk_widget_destroy(dialog);

	return ret;
}

/*
 * Function: flow_program_duplicate
 * Remove selected program from flow process
 */
void
flow_program_duplicate(void)
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

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter,
		FSEQ_GEOXML_POINTER, &program,
		-1);
	program = geoxml_sequence_append_clone(program);
	document_save(GEOXML_DOCUMENT(gebr.flow));
	flow_add_program_sequence_to_view(program);
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
	if (gtk_tree_model_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter) ||
	gtk_tree_model_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter))
		return;

	/* SEEK AND DESTROY */
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter,
		FSEQ_GEOXML_POINTER, &program,
		-1);
	geoxml_sequence_remove(program);
	document_save(GEOXML_DOCUMENT(gebr.flow));
	/* from GUI... */
	gtk_tree_view_select_sibling(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view));
	gtk_list_store_remove(GTK_LIST_STORE(gebr.ui_flow_edition->fseq_store), &iter);
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
	gtk_tree_selection_get_selected(selection, &model, &iter);

	/* Update flow */
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter,
		FSEQ_GEOXML_POINTER, &program,
		-1);
	geoxml_sequence_move_after(program, NULL);
	document_save(GEOXML_DOCUMENT(gebr.flow));
	/* Update GUI */
	gtk_list_store_move_after(GTK_LIST_STORE(gebr.ui_flow_edition->fseq_store),
		&iter, &gebr.ui_flow_edition->input_iter);
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
	gtk_tree_selection_get_selected(selection, &model, &iter);

	/* Update flow */
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter,
		FSEQ_GEOXML_POINTER, &program,
		-1);
	geoxml_sequence_move_before(program, NULL);
	document_save(GEOXML_DOCUMENT(gebr.flow));
	/* Update GUI */
	gtk_list_store_move_before(GTK_LIST_STORE(gebr.ui_flow_edition->fseq_store),
		&iter, &gebr.ui_flow_edition->output_iter);
}
