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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <gtk/gtk.h>

#include <glib/gi18n.h>
#include <libgebr/date.h>
#include <libgebr/utils.h>
#include <libgebr/gebr-tar.h>
#include <libgebr/geoxml/geoxml.h>
#include <libgebr/geoxml/flow.h>
#include <libgebr/comm/gebr-comm.h>
#include <libgebr/gui/gui.h>
#include <libgebr/gui/gebr-gui-utils.h>
#include <gebr-validator.h>

#include "flow.h"
#include "line.h"
#include "gebr.h"
#include "menu.h"
#include "document.h"
#include "callbacks.h"
#include "ui_flow.h"
#include "ui_document.h"
#include "ui_flow_browse.h"
#include "gebr-flow-edition.h"
#include "ui_project_line.h"

static void on_properties_response(gboolean accept)
{
	if (!accept)
		flow_delete(FALSE);
}

void flow_new (void)
{
	GtkTreeIter iter;

	const gchar *line_title;

	GebrGeoXmlFlow *flow;
	GebrGeoXmlLineFlow *line_flow;

	if (!project_line_get_selected(NULL, LineSelection))
		return;

	line_title = gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(gebr.line));

	/* Create a new flow */
	flow = GEBR_GEOXML_FLOW(document_new(GEBR_GEOXML_DOCUMENT_TYPE_FLOW));
	gebr_geoxml_document_set_title(GEBR_GEOXML_DOC(flow), _("New Flow"));
	gebr_geoxml_document_set_author(GEBR_GEOXML_DOC(flow), gebr.config.username->str);
	gebr_geoxml_document_set_email(GEBR_GEOXML_DOC(flow), gebr.config.email->str);

	line_flow = gebr_geoxml_line_append_flow(gebr.line, gebr_geoxml_document_get_filename(GEBR_GEOXML_DOC(flow)));
	iter = line_append_flow_iter(flow, line_flow);

	document_save (GEBR_GEOXML_DOC (gebr.line), TRUE, FALSE);
	document_save (GEBR_GEOXML_DOC (flow), TRUE, TRUE);

	flow_browse_select_iter(&iter);

	flow_edition_set_io();

	gebr_message(GEBR_LOG_INFO, TRUE, TRUE, _("New Flow added to Line '%s'."), line_title);
	document_properties_setup_ui(GEBR_GEOXML_DOCUMENT(gebr.flow), on_properties_response, TRUE);
}

void flow_free(void)
{
	gebr.flow = NULL;

	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store);
	gboolean valid = gtk_tree_model_get_iter_first(model, &iter);

	gtk_tree_view_set_model(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view), NULL);
	while (valid) {
		GebrGeoXmlProgram *program;
		gtk_tree_model_get(model, &iter, FSEQ_GEBR_GEOXML_POINTER, &program, -1);
		gebr_geoxml_object_unref(program);
		valid = gtk_list_store_remove(gebr.ui_flow_edition->fseq_store, &iter);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view), model);
	gtk_container_foreach(GTK_CONTAINER(gebr.ui_flow_browse->revisions_menu),
			      (GtkCallback) gtk_widget_destroy, NULL);
	flow_browse_info_update();
}

void flow_delete(gboolean confirm)
{
	gpointer document;
	GtkTreeIter iter;
	gboolean valid = FALSE;

	gchar *title;
	gchar *filename;

	GebrGeoXmlSequence *line_flow;

	if (!flow_browse_get_selected(NULL, TRUE))
		return;

	if (confirm && gebr_gui_confirm_action_dialog(_("Delete Flow"),
						      _("Are you sure you want to delete the selected Flow(s)?")) == FALSE)
		return;

	gebr_gui_gtk_tree_view_foreach_selected(&iter, gebr.ui_flow_browse->view) {
		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter,
				   FB_TITLE, &title,
				   FB_FILENAME, &filename,
				   FB_XMLPOINTER, &document,
				   -1);

		/* Some feedback */
		if (confirm) {
			gebr_message(GEBR_LOG_INFO, TRUE, FALSE, _("Deleting Flow '%s'."), title);
			gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Deleting  Flow '%s' from Line '%s'."),
				     title, gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(gebr.line)));
		}

		/* Seek and destroy */
		gebr_geoxml_line_get_flow(gebr.line, &line_flow, 0);
		for (; line_flow != NULL; gebr_geoxml_sequence_next(&line_flow)) {
			if (g_strcmp0(filename, gebr_geoxml_line_get_flow_source(GEBR_GEOXML_LINE_FLOW(line_flow))) == 0) {
				gebr_geoxml_sequence_remove(line_flow);
				document_save(GEBR_GEOXML_DOC(gebr.line), TRUE, FALSE);
				break;
			}
		}

		/* Free and delete flow from the disk */
		gebr_remove_help_edit_window(document);
		valid = gtk_list_store_remove(GTK_LIST_STORE(gebr.ui_flow_browse->store), &iter);
		flow_free();
		document_delete(filename);
		g_signal_emit_by_name(gebr.ui_flow_browse->view, "cursor-changed");

		g_free(title);
		g_free(filename);
	}
	if (valid)
		flow_browse_select_iter(&iter);
}

static gboolean flow_import_single (const gchar *path)
{
	gchar *new_title;
	const gchar *title;
	GtkTreeIter iter;
	GebrGeoXmlDocument *flow;
	GebrGeoXmlLineFlow *line_flow;

	if (document_load_path (&flow, path))
		return FALSE;

	title = gebr_geoxml_document_get_title (flow);

	gebr_message(GEBR_LOG_INFO, TRUE, TRUE, _("Flow '%s' imported into Line '%s' from the file '%s'."),
		     title, gebr_geoxml_document_get_title (GEBR_GEOXML_DOC (gebr.line)), path);

	document_import (flow, TRUE);
	line_flow = gebr_geoxml_line_append_flow (gebr.line, gebr_geoxml_document_get_filename (flow));
	document_save(GEBR_GEOXML_DOC(gebr.line), FALSE, FALSE);
	iter = line_append_flow_iter(GEBR_GEOXML_FLOW (flow), line_flow);
	gebr_validator_set_document(gebr.validator, (GebrGeoXmlDocument**) &flow, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, FALSE);
	gebr_geoxml_flow_revalidate(GEBR_GEOXML_FLOW(flow), gebr.validator);
	gebr_validator_set_document(gebr.validator, (GebrGeoXmlDocument**) &gebr.flow, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, FALSE);

	new_title = g_strdup_printf (_("%s (Imported)"), title);
	gtk_list_store_set(gebr.ui_flow_browse->store, &iter, FB_TITLE, new_title, -1);
	gebr_geoxml_document_set_title(flow, new_title);
	document_save(flow, FALSE, FALSE);
	g_free(new_title);

	return TRUE;
}

void flow_import(void)
{
	gchar *path;
	GtkWidget *chooser_dialog;
	GtkFileFilter *file_filter;

	if (!project_line_get_selected(NULL, LineSelection))
		return;

	chooser_dialog = gtk_file_chooser_dialog_new(_("Choose a Flow to import"),
						     GTK_WINDOW(gebr.window),
						     GTK_FILE_CHOOSER_ACTION_OPEN,
						     GTK_STOCK_OPEN, GTK_RESPONSE_YES,
						     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(chooser_dialog), TRUE);
	file_filter = gtk_file_filter_new();
	gtk_file_filter_set_name(file_filter, _("Flow file types (*.flw, *.flwz)"));
	gtk_file_filter_add_pattern(file_filter, "*.flw");
	gtk_file_filter_add_pattern(file_filter, "*.flwz");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_dialog), file_filter);

	gtk_widget_show(chooser_dialog);
	if (gtk_dialog_run(GTK_DIALOG(chooser_dialog)) != GTK_RESPONSE_YES)
		goto out;

	path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser_dialog));
	if (g_str_has_suffix (path, ".flwz")) {
		GebrTar *tar;
		tar = gebr_tar_new_from_file (path);
		if (!gebr_tar_extract (tar))
			gebr_message (GEBR_LOG_ERROR, TRUE, TRUE,
				      _("Could not import Flow from the file %s"), path);
		else
			gebr_tar_foreach (tar, (GebrTarFunc) flow_import_single, NULL);
		gebr_tar_free (tar);
	} else if (!flow_import_single (path))
		gebr_message (GEBR_LOG_ERROR, TRUE, TRUE,
			      _("Could not import Flow from the file %s"), path);

	g_free(path);
out:
	gtk_widget_destroy(chooser_dialog);
}

void flow_export(void)
{
	GList *rows;
	GString *tempdir;
	GString *title;
	GebrGeoXmlDocument *flow;
	GebrTar *tar;
	GtkFileFilter *file_filter;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkWidget *check_box_user;
	GtkWidget *chooser_dialog;
	gboolean active_user;
	gboolean have_flow = FALSE;
	gboolean error = FALSE;
	gchar *filename;
	gchar *flow_filename;
	gchar *tmp;
	gint len;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gebr.ui_flow_browse->view));
	rows = gtk_tree_selection_get_selected_rows (selection, &model);
	len = g_list_length (rows);

	if (!rows) {
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("No Flow selected"));
		return;
	}

	title = g_string_new(NULL);

	if (len > 1)
		g_string_printf (title, "Save selected Flows as...");
	else {
		GtkTreeIter iter;
		GtkTreePath *path = rows->data;

		gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_model_get (model, &iter, FB_FILENAME, &flow_filename, -1);

		//if (gebr_geoxml_document_load((GebrGeoXmlDocument**)&flow, flow_filename, TRUE, NULL) != GEBR_GEOXML_RETV_SUCCESS)
		if (document_load (&flow, flow_filename, FALSE))
			goto out;

		g_string_printf (title, _("Save '%s' as..."),
				 gebr_geoxml_document_get_title (flow));

		document_free (flow);
		//gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
	}

	GtkWidget *box;
	box = gtk_vbox_new(FALSE, 5);

	check_box_user = gtk_check_button_new_with_label(_("Make this Flow user-independent"));
	gtk_box_pack_start(GTK_BOX(box), check_box_user, TRUE, TRUE, 0);
	chooser_dialog = gebr_gui_save_dialog_new(title->str, GTK_WINDOW(gebr.window));
	gebr_gui_save_dialog_set_default_extension(GEBR_GUI_SAVE_DIALOG(chooser_dialog), ".flwz");

	file_filter = gtk_file_filter_new();
	gtk_file_filter_set_name(file_filter, _("Flow file types (*.flwz)"));
	gtk_file_filter_add_pattern(file_filter, "*.flwz");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_dialog), file_filter);
	gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(chooser_dialog), box);
	gtk_widget_show_all(box);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_box_user), TRUE);

	tmp = gebr_gui_save_dialog_run(GEBR_GUI_SAVE_DIALOG(chooser_dialog));
	if (!tmp)
		goto out;

	filename = g_path_get_basename (tmp);
	active_user = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_box_user));
	tempdir = gebr_temp_directory_create ();
	tar = gebr_tar_create (tmp);

	for (GList *i = rows; i; i = i->next) {
		gchar *filepath;
		GtkTreeIter iter;
		GtkTreePath *path = i->data;
		GebrGeoXmlDocument *doc;

		gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_model_get (model, &iter,
				    FB_FILENAME, &flow_filename,
				    FB_XMLPOINTER, &doc,
				    -1);

		flow = gebr_geoxml_document_clone (doc);
		if (!flow) {
			gebr_message (GEBR_LOG_ERROR, FALSE, TRUE, _("Could not clone Flow %s"), flow_filename);
			error = TRUE;
			continue;
		}

		flow_set_paths_to_relative(GEBR_GEOXML_FLOW(flow), active_user);
		filepath = g_build_path ("/", tempdir->str, flow_filename, NULL);

		if (!document_save_at (flow, filepath, FALSE, FALSE)) {
			gebr_message (GEBR_LOG_ERROR, FALSE, TRUE,
				      _("Could not save Flow %s at %s"),
				      flow_filename, tempdir->str);
			error = TRUE;
			g_free (filepath);
			continue;
		}

		gebr_tar_append (tar, flow_filename);
		document_free (flow);
		have_flow = TRUE;
		g_free (filepath);
	}

	if (have_flow && gebr_tar_compact (tar, tempdir->str)) {
		gchar *msg;
		if (error)
			msg = _("Exported selected Flow(s) into %s with error. See log file for more information.");
		else
			msg = _("Exported selected Flow(s) into %s.");
		gebr_message (GEBR_LOG_INFO, TRUE, TRUE, msg, tmp);
	} else {
		gebr_message (GEBR_LOG_ERROR, TRUE, TRUE,
			      _("Could not export Flow(s). See log file for more information."));
	}

	gebr_temp_directory_destroy (tempdir);

	g_free(tmp);
	g_free(filename);
	gebr_tar_free (tar);

out:
	g_free(flow_filename);
	g_string_free(title, TRUE);
}

static void flow_copy_from_dicts_parse_parameters(GebrGeoXmlParameters * parameters);

/** 
 * \internal
 * Parse a parameter.
 */
static void flow_copy_from_dicts_parse_parameter(GebrGeoXmlParameter * parameter)
{
	GebrGeoXmlDocument *documents[] = {
		GEBR_GEOXML_DOCUMENT(gebr.project), GEBR_GEOXML_DOCUMENT(gebr.line),
		GEBR_GEOXML_DOCUMENT(gebr.flow), NULL
	};

	if (gebr_geoxml_parameter_get_type(parameter) == GEBR_GEOXML_PARAMETER_TYPE_GROUP) {
		GebrGeoXmlSequence *instance;

		gebr_geoxml_parameter_group_get_instance(GEBR_GEOXML_PARAMETER_GROUP(parameter), &instance, 0);
		for (; instance != NULL; gebr_geoxml_sequence_next(&instance))
			flow_copy_from_dicts_parse_parameters(GEBR_GEOXML_PARAMETERS(instance));

		return;
	}

	for (int i = 0; documents[i] != NULL; i++) {
		GebrGeoXmlProgramParameter *dict_parameter;

		dict_parameter =
			gebr_geoxml_program_parameter_find_dict_parameter(GEBR_GEOXML_PROGRAM_PARAMETER
									  (parameter), documents[i]);
		if (dict_parameter != NULL)
			gebr_geoxml_program_parameter_set_first_value(GEBR_GEOXML_PROGRAM_PARAMETER
								      (parameter), FALSE,
								      gebr_geoxml_program_parameter_get_first_value
								      (dict_parameter, FALSE));
	}
	gebr_geoxml_program_parameter_set_value_from_dict(GEBR_GEOXML_PROGRAM_PARAMETER(parameter),
							  NULL);
}

/** 
 * \internal
 * Parse a set of parameters.
 */
static void flow_copy_from_dicts_parse_parameters(GebrGeoXmlParameters * parameters)
{
	GebrGeoXmlSequence *parameter;

	parameter = GEBR_GEOXML_SEQUENCE(gebr_geoxml_parameters_get_first_parameter(parameters));
	for (; parameter != NULL; gebr_geoxml_sequence_next(&parameter))
		flow_copy_from_dicts_parse_parameter(GEBR_GEOXML_PARAMETER(parameter));
}

void flow_copy_from_dicts(GebrGeoXmlFlow * flow)
{
	GebrGeoXmlSequence *program;

	gebr_geoxml_flow_get_program(flow, &program, 0);
	for (; program != NULL; gebr_geoxml_sequence_next(&program))
		flow_copy_from_dicts_parse_parameters(gebr_geoxml_program_get_parameters(GEBR_GEOXML_PROGRAM(program)));
}

typedef void (*set_path_func)(GString *path);
static void flow_set_paths_to(GebrGeoXmlFlow * flow, set_path_func func, gboolean set_programs_unconfigured)
{
	GString *path = g_string_new(NULL);

	gboolean flow_paths_foreach_parameter(GebrGeoXmlParameter * parameter)
	{
		gboolean cleaned = FALSE;
		if (gebr_geoxml_parameter_get_type(parameter) == GEBR_GEOXML_PARAMETER_TYPE_FILE) {
			GebrGeoXmlSequence *value;

			gebr_geoxml_program_parameter_get_value(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), FALSE, &value, 0);
			for (; value != NULL; gebr_geoxml_sequence_next(&value)) {
				GString *path;

				path = g_string_new(gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(value)));
				if (path->len == 0)
					continue;
				else
					cleaned = TRUE;

				func(path);
				gebr_geoxml_value_sequence_set(GEBR_GEOXML_VALUE_SEQUENCE(value), path->str);
				g_string_free(path, TRUE);
			}
			if (set_programs_unconfigured && cleaned) {
				GebrGeoXmlProgram *program = gebr_geoxml_parameter_get_program(parameter);
				gebr_geoxml_program_set_status (program, GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED);
				gebr_geoxml_program_set_error_id(program, FALSE, GEBR_IEXPR_ERROR_PATH);
				gebr_geoxml_object_unref(program);
				return FALSE;
			}
		}
		return TRUE;
	}

	/* flow's IO */
	g_string_assign(path, gebr_geoxml_flow_io_get_input(flow));
	func(path);
	gebr_geoxml_flow_io_set_input(flow, path->str);
	g_string_assign(path, gebr_geoxml_flow_io_get_output(flow));
	func(path);
	gebr_geoxml_flow_io_set_output(flow, path->str);
	g_string_assign(path, gebr_geoxml_flow_io_get_error(flow));
	func(path);
	gebr_geoxml_flow_io_set_error(flow, path->str);

	/* all parameters */
	gebr_geoxml_flow_foreach_parameter(flow, (GebrGeoXmlCallback)flow_paths_foreach_parameter, NULL);

	/* call recursively for each revision */
	GebrGeoXmlSequence *revision;
	gebr_geoxml_flow_get_revision(flow, &revision, 0);
	for (; revision != NULL; gebr_geoxml_sequence_next(&revision)) {
		gchar *xml;
		gebr_geoxml_flow_get_revision_data(GEBR_GEOXML_REVISION(revision), &xml, NULL, NULL);
		GebrGeoXmlFlow *rev;
		if (gebr_geoxml_document_load_buffer((GebrGeoXmlDocument **)&rev, xml) == GEBR_GEOXML_RETV_SUCCESS) {
			flow_set_paths_to(rev, func, set_programs_unconfigured);
			g_free(xml);
			gebr_geoxml_document_to_string(GEBR_GEOXML_DOCUMENT(rev), &xml);
			gebr_geoxml_flow_set_revision_data(GEBR_GEOXML_REVISION(revision), xml, NULL, NULL);
			document_free(GEBR_GEOXML_DOCUMENT(rev));
		}
		g_free(xml);
	}

	g_string_free(path, TRUE);
}
void flow_set_paths_to_relative(GebrGeoXmlFlow * flow, gboolean relative)
{
	void func(GString *path)
	{
		gebr_path_set_to(path, relative);
	}
	flow_set_paths_to(flow, func, FALSE);
}
void flow_set_paths_to_empty(GebrGeoXmlFlow * flow)
{
	void func(GString *path)
	{
		g_string_assign(path, "");
	}
	flow_set_paths_to(flow, func, TRUE);
}

gboolean flow_revision_save(void)
{
	GtkWidget *dialog;
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *entry;
	GtkWidget *align;

	GtkTreeIter iter;
	gboolean ret = FALSE;

	gchar *flow_filename;

	GebrGeoXmlDocument *flow;
	if (!flow_browse_get_selected(&iter, TRUE))
		return FALSE;

	dialog = gtk_dialog_new_with_buttons(_("Save selected Flows?"),
					     GTK_WINDOW(gebr.window),
					     (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

	align = gtk_alignment_new(0, 0, 1, 1);
	gtk_alignment_set_padding(GTK_ALIGNMENT(align), 10, 10, 10, 10);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), align, TRUE, TRUE, 0);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(align), vbox);

	label = gtk_label_new(_("Write a description about this version of the selected Flows:"));
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);
	entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(vbox), entry, TRUE, TRUE, 0);
	gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);

	gtk_widget_show_all(dialog);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {

		GebrGeoXmlRevision *revision;

		gebr_gui_gtk_tree_view_foreach_selected(&iter, gebr.ui_flow_browse->view) {

			gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter, FB_FILENAME, &flow_filename, -1);
			if (document_load((GebrGeoXmlDocument**)(&flow), flow_filename, TRUE))
			{
				g_free(flow_filename);
				return FALSE;
			}

			revision = gebr_geoxml_flow_append_revision(GEBR_GEOXML_FLOW(flow), 
								    gtk_entry_get_text(GTK_ENTRY(entry)));
			document_save(flow, TRUE, FALSE);
			flow_browse_load_revision(revision, TRUE);
			flow_browse_info_update();
			ret = TRUE;

			//document_free(flow);
			g_free (flow_filename);
		}
	}

	gtk_widget_destroy(dialog);

	return ret;
}

void flow_program_remove(void)
{
	GtkTreeIter iter;
	gboolean valid = FALSE;
	GebrGeoXmlProgram *program;

	gebr_gui_gtk_tree_view_foreach_selected(&iter, gebr.ui_flow_edition->fseq_view) {
		if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter) ||
		    gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter) ||
		    gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->error_iter)){
			flow_edition_set_io();
		} else {
			gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter,
					   FSEQ_GEBR_GEOXML_POINTER, &program, -1);
			gebr_geoxml_sequence_remove(GEBR_GEOXML_SEQUENCE(program));

			// Remove `iter' variable from dictionary if the Loop is configured
			if (gebr_geoxml_program_get_control (program) == GEBR_GEOXML_PROGRAM_CONTROL_FOR
			    && gebr_geoxml_program_get_status(program) != GEBR_GEOXML_PROGRAM_STATUS_DISABLED) {
				GebrGeoXmlSequence *param;
				GError *err = NULL;
				GList *affected;
				param = gebr_geoxml_document_get_dict_parameter(GEBR_GEOXML_DOCUMENT(gebr.flow));
				gebr_validator_remove(gebr.validator, GEBR_GEOXML_PARAMETER(param), &affected, &err);
				flow_edition_revalidate_programs();
			}
			valid = gtk_list_store_remove(GTK_LIST_STORE(gebr.ui_flow_edition->fseq_store), &iter);
		}

	}
	flow_program_check_sensitiveness();
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);

	if (valid)
		flow_edition_select_component_iter(&iter);
}

void flow_program_move_top(void)
{
	GtkTreeIter iter;
	GebrGeoXmlProgramControl control;

	control = gebr_geoxml_program_get_control (gebr.program);
	if (control != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY)
		return;

	flow_edition_get_selected_component(&iter, FALSE);
	gebr_geoxml_sequence_move_after(GEBR_GEOXML_SEQUENCE(gebr.program), NULL);
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);
	gtk_list_store_move_after(GTK_LIST_STORE(gebr.ui_flow_edition->fseq_store),
				  &iter, &gebr.ui_flow_edition->input_iter);
	flow_program_check_sensitiveness();
}

void flow_program_move_bottom(void)
{
	GtkTreeIter iter;
	GebrGeoXmlProgramControl control;

	control = gebr_geoxml_program_get_control (gebr.program);
	if (control != GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY)
		return;

	flow_edition_get_selected_component(&iter, FALSE);
	gebr_geoxml_sequence_move_before(GEBR_GEOXML_SEQUENCE(gebr.program), NULL);
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);
	gtk_list_store_move_before(GTK_LIST_STORE(gebr.ui_flow_edition->fseq_store),
				   &iter, &gebr.ui_flow_edition->output_iter);
	flow_program_check_sensitiveness();
}

void flow_copy(void)
{
	GtkTreeIter iter;

	if (gebr.flow_clipboard != NULL) {
		g_list_foreach(gebr.flow_clipboard, (GFunc) g_free, NULL);
		g_list_free(gebr.flow_clipboard);
		gebr.flow_clipboard = NULL;
	}

	gebr_gui_gtk_tree_view_foreach_selected(&iter, gebr.ui_flow_browse->view) {
		gchar *filename;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter, FB_FILENAME, &filename, -1);
		gebr.flow_clipboard = g_list_prepend(gebr.flow_clipboard, filename);
	}
	gebr.flow_clipboard = g_list_reverse(gebr.flow_clipboard);
}

void flow_paste(void)
{
	GList *i;

	if (!project_line_get_selected(NULL, LineSelection))
		return;

	for (i = g_list_first(gebr.flow_clipboard); i != NULL; i = g_list_next(i)) {
		GebrGeoXmlDocument *flow;

		if (document_load(&flow, (gchar *)i->data, FALSE)) {
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Could not paste Flow. It was probably deleted."));
			continue;
		}

		document_import(flow, TRUE);
		line_append_flow_iter(GEBR_GEOXML_FLOW(flow),
				      GEBR_GEOXML_LINE_FLOW(gebr_geoxml_line_append_flow(gebr.line,
											 gebr_geoxml_document_get_filename(flow))));
		document_save(GEBR_GEOXML_DOCUMENT(gebr.line), TRUE, FALSE);
		gebr_validator_set_document(gebr.validator, (GebrGeoXmlDocument**) &flow, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, FALSE);
		gebr_geoxml_flow_revalidate(GEBR_GEOXML_FLOW(flow), gebr.validator);
	}
	gebr_validator_set_document(gebr.validator, (GebrGeoXmlDocument**) &gebr.flow, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, FALSE);
}

void flow_program_copy(void)
{
	GtkTreeIter iter;

	gebr_geoxml_clipboard_clear();
	gebr_gui_gtk_tree_view_foreach_selected(&iter, gebr.ui_flow_edition->fseq_view) {
		GebrGeoXmlObject *program;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter,
				   FSEQ_GEBR_GEOXML_POINTER, &program, -1);
		gebr_geoxml_clipboard_copy(GEBR_GEOXML_OBJECT(program));
	}
}

void flow_program_paste(void)
{
	GebrGeoXmlSequence *pasted;

	pasted = GEBR_GEOXML_SEQUENCE(gebr_geoxml_clipboard_paste(GEBR_GEOXML_OBJECT(gebr.flow)));
	if (pasted == NULL) {
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Could not paste program."));
		return;
	}

	flow_add_program_sequence_to_view(GEBR_GEOXML_SEQUENCE(pasted), TRUE, FALSE);
	flow_program_check_sensitiveness();
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);
	flow_edition_revalidate_programs();
}

static void append_parameter_row(GebrGeoXmlParameter * parameter, GString * dump, gboolean in_group)
{
	gint i, n_instances;
	GebrGeoXmlSequence * param;
	GebrGeoXmlSequence * instance;
	GebrGeoXmlParameters * parameters;

	if (gebr_geoxml_parameter_get_is_program_parameter(parameter)) {
		GString * str_value;
		GString * default_value;
		GebrGeoXmlProgramParameter * program;
		gint radio_value = GEBR_PARAM_TABLE_NO_TABLE;

		program = GEBR_GEOXML_PROGRAM_PARAMETER(parameter);
		str_value = gebr_geoxml_program_parameter_get_string_value(program, FALSE);
                default_value = gebr_geoxml_program_parameter_get_string_value(program, TRUE);

		switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook))) {
		case NOTEBOOK_PAGE_PROJECT_LINE:
			radio_value = gebr.config.detailed_line_parameter_table;
			break;
		case NOTEBOOK_PAGE_FLOW_BROWSE:
			radio_value = gebr.config.detailed_flow_parameter_table;
			break;
		default:
			g_warn_if_reached ();
			break;
		}

		if (((radio_value == GEBR_PARAM_TABLE_ONLY_CHANGED) && (g_strcmp0(str_value->str, default_value->str) != 0)) ||
		    ((radio_value == GEBR_PARAM_TABLE_ONLY_FILLED) && (str_value->len > 0)) ||
		    ((radio_value == GEBR_PARAM_TABLE_ALL)))
		{
			/* Translating enum values to labels */
			GebrGeoXmlSequence *enum_option = NULL;

			gebr_geoxml_program_parameter_get_enum_option(GEBR_GEOXML_PROGRAM_PARAMETER(parameter), &enum_option, 0);

			for (; enum_option; gebr_geoxml_sequence_next(&enum_option))
			{
				gchar *enum_value = gebr_geoxml_enum_option_get_value(GEBR_GEOXML_ENUM_OPTION(enum_option));
				if (g_strcmp0(str_value->str, enum_value) == 0)
				{
					gchar *label = gebr_geoxml_enum_option_get_label(GEBR_GEOXML_ENUM_OPTION(enum_option));
					g_string_printf(str_value, "%s", label);
					g_free(enum_value);
					g_free(label);
					gebr_geoxml_object_unref(enum_option);
					break;
				}
				g_free(enum_value);
			}
			gchar *label = gebr_geoxml_parameter_get_label(parameter);
			g_string_append_printf(dump, "<tr>\n  <td class=\"%slabel\">%s</td>\n  <td class=\"value\">%s</td>\n</tr>\n",
					       (in_group?"group-":""), label, str_value->str);
			g_free(label);
		}
		g_string_free(str_value, TRUE);
		g_string_free(default_value, TRUE);
	} else {
		GString * previous_table = g_string_new(dump->str);
		gchar *label = gebr_geoxml_parameter_get_label(parameter);
		g_string_append_printf(dump, "<tr class='parameter-group'>\n  <td colspan='2'>%s</td>\n</tr>\n", label);
		g_free(label);
		gebr_geoxml_parameter_group_get_instance(GEBR_GEOXML_PARAMETER_GROUP(parameter), &instance, 0);
		n_instances = gebr_geoxml_parameter_group_get_instances_number(GEBR_GEOXML_PARAMETER_GROUP(parameter));

		i = 1;
		GString * group_table = g_string_new(dump->str);

		while (instance) {
			GString * instance_table = g_string_new(dump->str);
			if (n_instances > 1)
				g_string_append_printf(dump, "<tr class='group-instance'>\n  <td colspan='2'>%s %d</td>\n</tr>\n",
						       _("Instance"), i++);
			parameters = GEBR_GEOXML_PARAMETERS(instance);
			gebr_geoxml_parameters_get_parameter(parameters, &param, 0);

			GString * inner_table = g_string_new(dump->str);

			while (param) {
				append_parameter_row(GEBR_GEOXML_PARAMETER(param), dump, TRUE);
				gebr_geoxml_sequence_next(&param);
			}
			/*If there are no parameters returned by the dialog choice...*/
			if(g_string_equal(dump, inner_table))
			/*...return the table to it previous content*/
				g_string_assign(dump, instance_table->str);

			g_string_free(inner_table, TRUE);

			gebr_geoxml_sequence_next(&instance);
			g_string_free(instance_table, TRUE);
		}
		/* If there are no instance inside the group...*/
		if(g_string_equal(dump, group_table))
		/*...return the table to it outermost previous content*/
			g_string_assign(dump, previous_table->str);

		g_string_free(previous_table, TRUE);
		g_string_free(group_table, TRUE);
	}
}

static gchar *
gebr_program_generate_parameter_value_table (GebrGeoXmlProgram *program)
{
	GString * table;
	GebrGeoXmlParameters *parameters;
	GebrGeoXmlSequence *sequence;

	table = g_string_new ("");
	parameters = gebr_geoxml_program_get_parameters (program);
	gebr_geoxml_parameters_get_parameter (parameters, &sequence, 0);
	gebr_geoxml_object_unref(parameters);

	gchar *title = gebr_geoxml_program_get_title (program);
	gchar *translated = g_strdup_printf (_("Parameters for the Program \"%s\""), title);
	g_free(title);
	
	if (sequence == NULL) {
		g_string_append_printf(table,
				       "<table class=\"gebr-parameter-table\" summary=\"Parameter table\">\n"
				       "<caption>%s</caption>\n"
				       "<tbody>\n"
				       "<tr><td>this program has no parameters.</td></tr>\n",
				       translated);
	} else {
		g_string_append_printf (table,
					"<table class=\"gebr-parameter-table\" summary=\"Parameter table\">\n"
					"<caption>%s</caption>\n"
					"<thead>\n<tr>\n"
					"  <td>%s</td><td>%s</td>\n"
					"</tr>\n</thead>\n"
					"<tbody>\n",
					translated, _("Parameter"), _("Value"));

		GString * initial_table = g_string_new(table->str);
		while (sequence) {
			append_parameter_row(GEBR_GEOXML_PARAMETER(sequence), table, FALSE);
			gebr_geoxml_sequence_next (&sequence);
		}

		if (g_string_equal(initial_table, table)) {
			switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(gebr.notebook))) {
			case NOTEBOOK_PAGE_PROJECT_LINE:
				if (gebr.config.detailed_line_parameter_table == GEBR_PARAM_TABLE_ONLY_CHANGED)
					g_string_printf(table,
							"<table class=\"gebr-parameter-table\" summary=\"Parameter table\">\n"
							"<caption>%s</caption>\n"
							"<tbody>\n"
							"<tr><td>This program has only default parameters</td></tr>\n",
							translated);

				else if (gebr.config.detailed_line_parameter_table == GEBR_PARAM_TABLE_ONLY_FILLED)
					g_string_printf(table,
							"<table class=\"gebr-parameter-table\" summary=\"Parameter table\">\n"
							"<caption>%s</caption>\n"
							"<tbody>\n"
							"<tr><td>This program has only empty parameters</td></tr>\n",
							translated);

				break;
			case NOTEBOOK_PAGE_FLOW_BROWSE:
				if (gebr.config.detailed_flow_parameter_table == GEBR_PARAM_TABLE_ONLY_CHANGED)
					g_string_printf(table,
							"<table class=\"gebr-parameter-table\" summary=\"Parameter table\">\n"
							"<caption>%s</caption>\n"
							"<tbody>\n"
							"<tr><td>This program has only default parameters</td></tr>\n",
							translated);

				else if (gebr.config.detailed_flow_parameter_table == GEBR_PARAM_TABLE_ONLY_FILLED)
					g_string_printf(table,
							"<table class=\"gebr-parameter-table\" summary=\"Parameter table\">\n"
							"<caption>%s</caption>\n"
							"<tbody>\n"
							"<tr><td>This program has only empty parameters</td></tr>\n",
							translated);
				break;
			default:
				g_warn_if_reached ();
				break;
			}

			g_string_free(initial_table, TRUE);
		} 
	}

	g_free (translated);

	g_string_append_printf (table, "</tbody>\n</table>\n");
	return g_string_free (table, FALSE);
}

gchar *gebr_generate_variables_value_table(GebrGeoXmlDocument *doc, gboolean header, gboolean close)
{
	GString *dump;
	gchar *name;
	gchar *value;
	gchar *comment;
	gchar *eval;
	GebrGeoXmlParameters *params;
	GebrGeoXmlSequence *sequence;
	GebrGeoXmlParameterType type;
	GebrGeoXmlDocumentType scope;
	GError *error = NULL;
	gboolean have_vars = FALSE;

	dump = g_string_new(NULL);

	scope = gebr_geoxml_document_get_type(doc);

	params = gebr_geoxml_document_get_dict_parameters(doc);
	gebr_geoxml_parameters_get_parameter(params, &sequence, 0);
	gebr_geoxml_object_unref(params);

	if (header)
		g_string_append_printf (dump,
		                        "\n<div class=\"gebr-flow-dump\">\n"
		                        "<table class=\"gebr-variables-table\" summary=\"Variable table\">\n"
		                        "<caption>%s</caption>\n"
		                        "<thead>\n<tr>\n"
		                        "  <td>%s</td><td>%s</td><td>%s</td><td>%s</td>\n"
		                        "</tr>\n</thead>\n"
		                        "<tbody>\n",
		                        _("Variables"), _("Keyword"), _("Value"),_("Result"), _("Comment"));

	g_string_append_printf(dump, "<tr class=\"variables-group\">\n  <td colspan='4'>%s</td>\n</tr>\n",
	                       scope == GEBR_GEOXML_DOCUMENT_TYPE_FLOW? _("Flow") :
	                       scope == GEBR_GEOXML_DOCUMENT_TYPE_LINE? _("Line") : _("Project"));

	while (sequence) {
		have_vars = TRUE;
		name = gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(sequence));
		value = gebr_geoxml_program_parameter_get_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(sequence), FALSE);
		type = gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(sequence));
		comment = gebr_geoxml_parameter_get_label(GEBR_GEOXML_PARAMETER(sequence));

		gebr_validator_evaluate_param(gebr.validator, GEBR_GEOXML_PARAMETER(sequence), &eval, &error);

		g_string_append_printf(dump, "<tr >\n  <td class=\"%2$sgroup-label\">%1$s</td>\n  <td class=\"%2$svalue\">%3$s</td>\n"
		                       "<td class=\"%2$svalue\">%4$s</td>\n<td class=\"%2$svalue\">%5$s</td>\n</tr>\n",
		                       name, (error? "error-" : (type == GEBR_GEOXML_PARAMETER_TYPE_STRING? "string-" : "numeric-")), value,
		                       (error? error->message : eval), comment);

		g_free(name);
		g_free(value);
		g_free(comment);

		if (error)
			g_clear_error(&error);
		else
			g_free(eval);

		gebr_geoxml_sequence_next(&sequence);
	}
	if (error)
		g_error_free(error);

	if(!have_vars)
		g_string_append_printf(dump, "<tr >\n  <td class=\"group-label\" colspan='4'>%s</td>\n</tr>\n",_("There are no variables in this scope"));

	if (close)
		g_string_append (dump, "</tbody>\n</table>\n");

	return g_string_free(dump, FALSE);

}

gchar * gebr_flow_generate_parameter_value_table(GebrGeoXmlFlow *flow)
{ // OK
	GString * dump;
	gchar * input;
	gchar * output;
	gchar * error;
	GebrGeoXmlSequence * sequence;

	input = gebr_geoxml_flow_io_get_input (flow);
	output = gebr_geoxml_flow_io_get_output (flow);
	error = gebr_geoxml_flow_io_get_error (flow);
        
	dump = g_string_new(NULL);

	g_string_append_printf(dump,
			       "<div class=\"gebr-flow-dump\">\n"
			       "<table class=\"gebr-io-table\" summary=\"I/O table\">\n"
                               "<caption>%s</caption>\n"
			       "<tr><td>%s</td><td>%s</td></tr>\n"
			       "<tr><td>%s</td><td>%s</td></tr>\n"
			       "<tr><td>%s</td><td>%s</td></tr>\n"
			       "</table>\n",
                               _("I/O table"),
			       _("Input"), strlen (input) > 0? input : _("(none)"),
			       _("Output"), strlen (output) > 0? output : _("(none)"),
			       _("Error"), strlen (error) > 0? error : _("(none)"));

	g_free(input);
	g_free(output);
	g_free(error);

	gebr_geoxml_flow_get_program(flow, &sequence, 0);
	while (sequence) {
		GebrGeoXmlProgram * prog;
		GebrGeoXmlProgramStatus status;

		prog = GEBR_GEOXML_PROGRAM (sequence);
		status = gebr_geoxml_program_get_status (prog);

		if (status == GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED) {
			gchar * table;
			table = gebr_program_generate_parameter_value_table(prog);
			g_string_append (dump, table);
			g_free (table);
		}
		gebr_geoxml_sequence_next(&sequence);
	}

	g_string_append (dump, "</div>\n</div>\n");
	return g_string_free(dump, FALSE);
}

gchar * gebr_flow_generate_header(GebrGeoXmlFlow * flow, gboolean include_date)
{
	gchar *date;
	GString *dump;
	GebrGeoXmlSequence * program;
	gchar *title;
	gchar *description;
	gchar *author;
	gchar *email;

	dump = g_string_new(NULL);
	title = gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(flow));
	description = gebr_geoxml_document_get_description(GEBR_GEOXML_DOC(flow));
	g_string_printf(dump,
			"<h1>%s</h1>\n"
			"<h2>%s</h2>\n",
			title,
			description);
	g_free(title);
	g_free(description);

	if (include_date)
		date = g_strdup_printf (", <span class=\"gebr-date\">%s</span>",
					gebr_localized_date(gebr_iso_date()));
	else
		date = NULL;

	author = gebr_geoxml_document_get_author(GEBR_GEOXML_DOC(flow));
	email = gebr_geoxml_document_get_email(GEBR_GEOXML_DOC(flow));
	g_string_append_printf (dump,
				"<p class=\"credits\">%s <span class=\"gebr-author\">%s</span>"
				" <span class=\"gebr-email\">%s</span>%s</p>\n",
				_("By"),
				author,
				email,
				date ? date : "");
	g_free(date);
	g_free(email);
	g_free(author);

	g_string_append_printf (dump, "<p>%s</p>\n", _("Flow composed of the program(s):"));
	g_string_append_printf (dump, "<ul>\n");
	gebr_geoxml_flow_get_program (flow, &program, 0);
	while (program) {
		GebrGeoXmlProgram * prog;
		GebrGeoXmlProgramStatus status;
		gchar *title;

		prog = GEBR_GEOXML_PROGRAM (program);
		status = gebr_geoxml_program_get_status (prog);
		title = gebr_geoxml_program_get_title (prog);

		if (status == GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED)
			g_string_append_printf (dump, "  <li>%s</li>\n",
						title);

		g_free(title);
		gebr_geoxml_sequence_next (&program);
	}
	g_string_append_printf(dump, "</ul>\n");

	return g_string_free(dump, FALSE);
}

gchar * gebr_flow_get_detailed_report (GebrGeoXmlFlow * flow, gboolean include_table, gboolean include_date)
{ // OK
	gchar * table;
	gchar * inner;
	gchar * report;
	gchar * header;
	gchar * flow_dict;
	gchar * help;

	help = gebr_geoxml_document_get_help (GEBR_GEOXML_DOCUMENT (flow));
	inner = gebr_document_report_get_inner_body (help);
	g_free(help);
	
	if (inner == NULL)
		inner = g_strdup("");

	table = include_table ? gebr_flow_generate_parameter_value_table (flow) : "";
	flow_dict = include_table ? gebr_generate_variables_value_table(GEBR_GEOXML_DOCUMENT(flow), TRUE, TRUE) : "";
	header = gebr_flow_generate_header (flow, include_date);
	report = g_strdup_printf ("<div class='gebr-geoxml-flow'>%s%s%s%s</div>\n", header, inner, flow_dict, table);

	if (include_table) {
		g_free (table);
		g_free(flow_dict);
	}
	g_free (inner);
	g_free (header);

	return report;
}
