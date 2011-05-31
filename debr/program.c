/*   DeBR - GeBR Designer
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>

#include <glib/gi18n.h>
#include <libgebr/gui/gebr-gui-utils.h>
#include <libgebr/gui/gebr-gui-program-edit.h>

#include "program.h"
#include "debr.h"
#include "callbacks.h"
#include "help.h"

/*
 * Declarations
 */

#define RESPONSE_REFRESH	101

static const gchar * mpi_combo_to_xml [] = {
	"", "openmpi", NULL
};
static const gchar * mpi_xml_to_combo [] = {
	N_("None"), N_("OpenMPI"), NULL
};
static const gint mpi_xml_to_index(GebrGeoXmlProgram * program)
{
	for (int i = 0; mpi_combo_to_xml[i] != NULL; i++)
		if (!strcmp(mpi_combo_to_xml[i], gebr_geoxml_program_get_mpi(program)))
			return i;
	return 0;
}

struct program_preview_data {
	GebrGuiProgramEdit *program_edit;
	GebrGeoXmlProgram *program;
	GtkWidget *title_label;
	GtkWidget *hbox;
};

static void program_load_iter(GtkTreeIter * iter);
static void program_load_selected(void);
static GtkTreeIter program_append_to_ui(GebrGeoXmlProgram * program);
static void program_selected(void);
static GtkMenu *program_popup_menu(GtkWidget * tree_view);

static void program_stdin_changed(GtkToggleButton * togglebutton);
static void program_stdout_changed(GtkToggleButton * togglebutton);
static void program_stderr_changed(GtkToggleButton * togglebutton);
static gboolean program_title_changed(GtkEntry * entry);
static gboolean program_binary_changed(GtkEntry * entry);
static gboolean program_description_changed(GtkEntry * entry);
static gboolean program_version_changed(GtkEntry * entry);
static void program_mpi_changed(GtkComboBox * combo);
static gboolean program_url_changed(GtkEntry * entry);

static void program_preview_on_response(GtkWidget * dialog, gint arg1, struct program_preview_data *data);
static gboolean
program_preview_on_delete_event(GtkWidget * dialog, GdkEventAny * event, struct program_preview_data *data);

/*
 * Public functions.
 */

void program_setup_ui(void)
{
	GtkWidget *hpanel;
	GtkWidget *scrolled_window;
	GtkWidget *details;
	GtkWidget *hbox;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	hpanel = gtk_hpaned_new();
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_paned_pack1(GTK_PANED(hpanel), scrolled_window, FALSE, FALSE);
	gtk_widget_set_size_request(scrolled_window, 200, -1);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_IN);

	debr.ui_program.list_store = gtk_list_store_new(PROGRAM_N_COLUMN,
							GDK_TYPE_PIXBUF,	// PROGRAM_STATUS
							G_TYPE_STRING,		// PROGRAM_TITLE
							G_TYPE_POINTER);	// PROGRAM_XMLPOINTER

	debr.ui_program.tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(debr.ui_program.list_store));
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(debr.ui_program.tree_view), TRUE);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(debr.ui_program.tree_view)),
				    GTK_SELECTION_MULTIPLE);
	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(debr.ui_program.tree_view),
						  (GebrGuiGtkPopupCallback) program_popup_menu, NULL);
	gebr_gui_gtk_tree_view_set_gebr_geoxml_sequence_moveable(GTK_TREE_VIEW(debr.ui_program.tree_view),
								 PROGRAM_XMLPOINTER,
								 (GebrGuiGtkTreeViewMoveSequenceCallback)
								 menu_status_set_unsaved, NULL);
	gtk_container_add(GTK_CONTAINER(scrolled_window), debr.ui_program.tree_view);
	g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(debr.ui_program.tree_view)), "changed",
			 G_CALLBACK(program_selected), NULL);
	g_signal_connect(debr.ui_program.tree_view, "row-activated", G_CALLBACK(on_program_properties_activate), NULL);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(debr.ui_program.tree_view), FALSE);
	renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width(col, 24);
	gtk_tree_view_column_add_attribute(col, renderer, "pixbuf", PROGRAM_STATUS);
	gtk_tree_view_append_column(GTK_TREE_VIEW(debr.ui_program.tree_view), col);
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_column_add_attribute(col, renderer, "text", PROGRAM_TITLE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(debr.ui_program.tree_view), col);

	/*
	 * Info panel
	 */
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_paned_pack2(GTK_PANED(hpanel), scrolled_window, TRUE, FALSE);

	debr.ui_program.details.frame = gtk_frame_new(NULL);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), debr.ui_program.details.frame);

	details = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(debr.ui_program.details.frame), details);

	debr.ui_program.details.title_label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_program.details.title_label), 0, 0);
	gtk_box_pack_start(GTK_BOX(details), debr.ui_program.details.title_label, FALSE, TRUE, 0);

	debr.ui_program.details.description_label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_program.details.description_label), 0, 0);
	gtk_box_pack_start(GTK_BOX(details), debr.ui_program.details.description_label, FALSE, TRUE, 0);

	debr.ui_program.details.nparams_label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_program.details.nparams_label), 0, 0);
	gtk_box_pack_start(GTK_BOX(details), debr.ui_program.details.nparams_label, FALSE, TRUE, 10);

	debr.ui_program.details.binary_label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_program.details.binary_label), 0, 0);
	gtk_box_pack_start(GTK_BOX(details), debr.ui_program.details.binary_label, FALSE, TRUE, 10);

	debr.ui_program.details.version_label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_program.details.version_label), 0, 0);
	gtk_box_pack_start(GTK_BOX(details), debr.ui_program.details.version_label, FALSE, TRUE, 0);

	debr.ui_program.details.mpi_label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_program.details.mpi_label), 0, 0);
	gtk_box_pack_start(GTK_BOX(details), debr.ui_program.details.mpi_label, FALSE, TRUE, 0);

	debr.ui_program.details.url_label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_program.details.url_label), 0, 0);
	gtk_box_pack_start(GTK_BOX(details), debr.ui_program.details.url_label, FALSE, TRUE, 10);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(details), hbox, FALSE, TRUE, 0);
	debr.ui_program.details.url_button = gtk_link_button_new("");
	gtk_box_pack_start(GTK_BOX(hbox), debr.ui_program.details.url_button, FALSE, TRUE, 0);

	/* Help */
	GtkWidget *hbox_aux;
	GtkWidget * alignment;
	GtkWidget * button_hbox;
	GtkWidget * edit_help_label;

	hbox_aux = gtk_hbox_new(FALSE, 0);
	alignment = gtk_alignment_new(0.5, 0.5, 0, 0);
	button_hbox = gtk_hbox_new(FALSE, 0);
	edit_help_label = gtk_label_new(_("Edit Help"));
	debr.ui_program.help_validate_image = validate_image_warning_new();
	debr.ui_program.details.help_view = gtk_button_new_with_label(_("View Help"));
	debr.ui_program.details.help_edit = gtk_button_new();

	gtk_box_pack_start(GTK_BOX(button_hbox), debr.ui_program.help_validate_image, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(button_hbox), edit_help_label, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(alignment), button_hbox);
	gtk_container_add(GTK_CONTAINER(debr.ui_program.details.help_edit), alignment);

	gtk_box_pack_end(GTK_BOX(details), hbox_aux, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_aux), debr.ui_program.details.help_view, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbox_aux), debr.ui_program.details.help_edit, TRUE, TRUE, 0);
	g_signal_connect(GTK_OBJECT(debr.ui_program.details.help_view), "clicked",
			 G_CALLBACK(program_help_show), NULL);
	g_signal_connect(GTK_OBJECT(debr.ui_program.details.help_edit), "clicked",
			 G_CALLBACK(program_help_edit), NULL);

	g_object_set(debr.ui_program.details.help_view, "sensitive", FALSE, NULL);
	g_object_set(debr.ui_program.details.help_edit, "sensitive", FALSE, NULL);

	debr.ui_program.widget = hpanel;
	gtk_widget_show_all(debr.ui_program.widget);

	program_details_update();
}

void program_load_menu(void)
{
	GebrGeoXmlSequence *program;
	GtkTreeIter iter;

	gtk_tree_store_clear(debr.ui_parameter.tree_store);
	gtk_list_store_clear(debr.ui_program.list_store);
	if (debr.menu == NULL) {
		debr.program = NULL;
		parameter_load_program();
		program_details_update();
		return;
	}

	gebr_geoxml_flow_get_program(debr.menu, &program, 0);
	for (; program != NULL; gebr_geoxml_sequence_next(&program))
		program_append_to_ui(GEBR_GEOXML_PROGRAM(program));

	debr.program = NULL;
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(debr.ui_program.list_store), &iter) == TRUE)
		program_select_iter(iter);
}

void get_program_count(GtkTreeIter * iter, gint * count)
{
	gchar * title;
	gint title_cont = 0;
	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_program.list_store), iter, PROGRAM_TITLE, &title, -1);

	if (sscanf(title, _("New program %d"), &title_cont) == 1)
	{
		if (title_cont >= *count && title_cont > 0)
			*count = title_cont + 1;
	}
}

void program_new()
{
	gint programs_count = 1;

	GtkTreeIter iter;
	GtkTreeIter iter_menu;

	gchar * program_title;
	GebrGeoXmlProgram *program;

	if (!menu_get_selected(&iter_menu, TRUE))
		return;
	gebr_gui_gtk_tree_model_foreach(iter_menu, GTK_TREE_MODEL(debr.ui_program.list_store))
	{
		get_program_count(&iter_menu, &programs_count);
	}
	menu_archive(); 
	program = gebr_geoxml_flow_append_program(debr.menu);
	/* default settings */
	program_title = g_strdup_printf(_("New program %d"), programs_count++);
	gebr_geoxml_program_set_title(program, program_title);
	gebr_geoxml_program_set_stdin(program, TRUE);
	gebr_geoxml_program_set_stdout(program, TRUE);
	gebr_geoxml_program_set_stderr(program, TRUE);

	iter = program_append_to_ui(program);
	gtk_list_store_set(debr.ui_program.list_store, &iter, PROGRAM_STATUS, debr.pixmaps.stock_no, -1);

	program_select_iter(iter);
	if (program_dialog_setup_ui(TRUE)) {
		menu_details_update();
		menu_saved_status_set(MENU_STATUS_UNSAVED);
	} else {
		program_remove(FALSE);
		menu_replace();
	}

	g_free(program_title);
}

void program_preview(void)
{
	struct program_preview_data *data;
	GtkWidget *dialog;

	if (!program_get_selected(NULL, TRUE))
		return;

	data = g_new(struct program_preview_data, 1);
	data->program = debr.program;
	dialog = gtk_dialog_new_with_buttons(_("Parameter's dialog preview"),
					     GTK_WINDOW(NULL),
					     (GtkDialogFlags)(GTK_DIALOG_DESTROY_WITH_PARENT),
					     GTK_STOCK_REFRESH, RESPONSE_REFRESH,
					     GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);
	g_object_set(dialog, "type-hint", GDK_WINDOW_TYPE_HINT_NORMAL, NULL);
	g_signal_connect(dialog, "response", G_CALLBACK(program_preview_on_response), data);
	g_signal_connect(dialog, "delete-event", G_CALLBACK(program_preview_on_delete_event), data);

	data->program_edit =
		gebr_gui_program_edit_setup_ui(GEBR_GEOXML_PROGRAM(gebr_geoxml_object_copy(GEBR_GEOXML_OBJECT(debr.program))), NULL, TRUE, debr.validator);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), data->program_edit->widget, TRUE, TRUE, 0);
	gtk_widget_show(dialog);

	/* adjust window size to fit parameters horizontally */
	gdouble width;
	width = GTK_BIN(GTK_BIN(data->program_edit->scrolled_window)->child)->child->allocation.width + 30;
	if (width >= gdk_screen_get_width(gdk_screen_get_default()))
		gtk_window_maximize(GTK_WINDOW(dialog));
	else
		gtk_widget_set_size_request(dialog, width, 500);
}

void program_remove(gboolean confirm)
{
	GtkTreeIter iter;
	gboolean valid = FALSE;

	if (!program_get_selected(NULL, TRUE))
		return;
	if (confirm && 
	    gebr_gui_confirm_action_dialog(_("Delete program"),
					   _("Are you sure you want to delete selected(s) program(s)?")) == FALSE)
		return;

	gebr_gui_gtk_tree_view_foreach_selected(&iter, debr.ui_program.tree_view) {
		GebrGeoXmlSequence * program;
		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_program.list_store), &iter,
				   PROGRAM_XMLPOINTER, &program,
				   -1);
		debr_remove_help_edit_window(program, FALSE, FALSE);
		gebr_geoxml_sequence_remove(GEBR_GEOXML_SEQUENCE(program));
		valid = gtk_list_store_remove(debr.ui_program.list_store, &iter);
	}
	if (valid)
		program_select_iter(iter);
	else {
		debr.program = NULL;
		parameter_load_program();
		program_details_update();
		do_navigation_bar_update();
	}
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

void program_top(void)
{
	GtkTreeIter iter;

	program_get_selected(&iter, TRUE);

	gtk_list_store_move_after(debr.ui_program.list_store, &iter, NULL);
	gebr_geoxml_sequence_move_after(GEBR_GEOXML_SEQUENCE(debr.program), NULL);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

void program_bottom(void)
{
	GtkTreeIter iter;

	program_get_selected(&iter, TRUE);

	gtk_list_store_move_before(debr.ui_program.list_store, &iter, NULL);
	gebr_geoxml_sequence_move_before(GEBR_GEOXML_SEQUENCE(debr.program), NULL);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

void program_copy(void)
{
	GtkTreeIter iter;

	gebr_geoxml_clipboard_clear();
	gebr_gui_gtk_tree_view_foreach_selected(&iter, debr.ui_program.tree_view) {
		GebrGeoXmlObject *program;

		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_program.list_store), &iter, PROGRAM_XMLPOINTER, &program, -1);
		gebr_geoxml_clipboard_copy(GEBR_GEOXML_OBJECT(program));
	}
}

void program_paste(void)
{
	GebrGeoXmlSequence *pasted;
	GtkTreeIter pasted_iter;

	pasted = GEBR_GEOXML_SEQUENCE(gebr_geoxml_clipboard_paste(GEBR_GEOXML_OBJECT(debr.menu)));
	if (pasted == NULL) {
		debr_message(GEBR_LOG_ERROR, _("Could not paste program"));
		return;
	}

	do {
		gtk_list_store_append(debr.ui_program.list_store, &pasted_iter);
		gtk_list_store_set(debr.ui_program.list_store, &pasted_iter, PROGRAM_XMLPOINTER, pasted, -1);
		program_load_iter(&pasted_iter);
	} while (!gebr_geoxml_sequence_next(&pasted));

	program_select_iter(pasted_iter);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

gboolean program_dialog_setup_ui(gboolean new_program)
{
	GtkWidget *dialog;
	GtkWidget *table;

	GtkWidget *io_vbox;
	GtkWidget *io_depth_hbox;
	GtkWidget *io_label;
	GtkWidget *io_stdin_checkbutton;
	GtkWidget *io_stdout_checkbutton;
	GtkWidget *io_stderr_checkbutton;

	GtkWidget *title_label;
	GtkWidget *title_entry;
	GtkWidget *binary_label;
	GtkWidget *binary_entry;
	GtkWidget *description_label;
	GtkWidget *description_entry;
	GtkWidget *version_label;
	GtkWidget *version_entry;
	GtkWidget *mpi_label;
	GtkWidget *mpi_combo;
	GtkWidget *url_label;
	GtkWidget *url_entry;

	gint row = 0;
	gboolean ret = TRUE;

	gebr_gui_gtk_tree_view_turn_to_single_selection(GTK_TREE_VIEW(debr.ui_program.tree_view));
	if (program_get_selected(NULL, TRUE) == FALSE)
		return FALSE;

	dialog = gtk_dialog_new_with_buttons(_("Edit program"),
					     GTK_WINDOW(debr.window),
					     (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     GTK_STOCK_OK, GTK_RESPONSE_OK,NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
	gtk_widget_set_size_request(dialog, 400, 450);

	table = gtk_table_new(8, 2, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, TRUE, TRUE, 5);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);

	/*
	 * IO
	 */
	io_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(io_vbox);
	gtk_table_attach(GTK_TABLE(table), io_vbox, 0, 2, row, row+1,
			 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0), row++;

	io_label = gtk_label_new(_("This program:"));
	gtk_misc_set_alignment(GTK_MISC(io_label), 0, 0);
	gtk_widget_show(io_label);
	gtk_box_pack_start(GTK_BOX(io_vbox), io_label, FALSE, FALSE, 0);

	io_depth_hbox = gebr_gui_gtk_container_add_depth_hbox(io_vbox);
	io_vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(io_vbox);
	gtk_box_pack_start(GTK_BOX(io_depth_hbox), io_vbox, FALSE, TRUE, 0);

	io_stdin_checkbutton = gtk_check_button_new_with_label(_("reads from standard input"));
	gtk_widget_show(io_stdin_checkbutton);
	gtk_box_pack_start(GTK_BOX(io_vbox), io_stdin_checkbutton, FALSE, TRUE, 0);
	g_signal_connect(io_stdin_checkbutton, "clicked", G_CALLBACK(program_stdin_changed), debr.program);

	io_stdout_checkbutton = gtk_check_button_new_with_label(_("writes to standard output"));
	gtk_widget_show(io_stdout_checkbutton);
	gtk_box_pack_start(GTK_BOX(io_vbox), io_stdout_checkbutton, FALSE, TRUE, 0);
	g_signal_connect(io_stdout_checkbutton, "clicked", G_CALLBACK(program_stdout_changed), debr.program);

	io_stderr_checkbutton = gtk_check_button_new_with_label(_("appends to standard error"));
	gtk_widget_show(io_stderr_checkbutton);
	gtk_box_pack_start(GTK_BOX(io_vbox), io_stderr_checkbutton, FALSE, TRUE, 0);
	g_signal_connect(io_stderr_checkbutton, "clicked", G_CALLBACK(program_stderr_changed), debr.program);

	/*
	 * Title
	 */
	title_label = gtk_label_new(_("Title:"));
	gtk_widget_show(title_label);
	gtk_table_attach(GTK_TABLE(table), title_label, 0, 1, row, row+1,
			 (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(title_label), 0, 0.5);

	title_entry = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(title_entry), TRUE);
	gtk_widget_show(title_entry);
	gtk_table_attach(GTK_TABLE(table), title_entry, 1, 2, row, row+1,
			 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0), row++;
	g_signal_connect(title_entry, "changed", G_CALLBACK(program_title_changed), debr.program);

	/*
	 * Binary
	 */
	binary_label = gtk_label_new(_("Executable:"));
	gtk_widget_show(binary_label);
	gtk_table_attach(GTK_TABLE(table), binary_label, 0, 1, row, row+1,
			 (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(binary_label), 0, 0.5);

	binary_entry = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(binary_entry), TRUE);
	gtk_widget_show(binary_entry);
	gtk_table_attach(GTK_TABLE(table), binary_entry, 1, 2, row, row+1,
			 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0), row++;
	g_signal_connect(binary_entry, "changed", G_CALLBACK(program_binary_changed), debr.program);

	/*
	 * Version
	 */
	version_label = gtk_label_new(_("Version:"));
	gtk_widget_show(version_label);
	gtk_table_attach(GTK_TABLE(table), version_label, 0, 1, row, row+1,
			 (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(version_label), 0, 0.5);

	version_entry = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(version_entry), TRUE);
	gtk_widget_show(version_entry);
	gtk_table_attach(GTK_TABLE(table), version_entry, 1, 2, row, row+1,
			 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0), row++;
	g_signal_connect(version_entry, "changed", G_CALLBACK(program_version_changed), debr.program);

	/*
	 * mpi
	 */
	mpi_label = gtk_label_new(_("MPI:"));
	gtk_widget_show(mpi_label);
	gtk_table_attach(GTK_TABLE(table), mpi_label, 0, 1, row, row+1,
			 (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(mpi_label), 0, 0.5);

	mpi_combo = gtk_combo_box_new_text();
	for (int i = 0; mpi_xml_to_combo[i] != NULL; i++)
		gtk_combo_box_append_text(GTK_COMBO_BOX(mpi_combo), _(mpi_xml_to_combo[i]));
	gtk_widget_show(mpi_combo);
	gtk_table_attach(GTK_TABLE(table), mpi_combo, 1, 2, row, row+1,
			 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0), row++;
	g_signal_connect(mpi_combo, "changed", G_CALLBACK(program_mpi_changed), debr.program);

	/*
	 * Description
	 */
	description_label = gtk_label_new(_("Description:"));
	gtk_widget_show(description_label);
	gtk_table_attach(GTK_TABLE(table), description_label, 0, 1, row, row+1,
			 (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(description_label), 0, 0.5);

	description_entry = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(description_entry), TRUE);
	gtk_widget_show(description_entry);
	gtk_table_attach(GTK_TABLE(table), description_entry, 1, 2, row, row+1,
			 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0), row++;
	g_signal_connect(description_entry, "changed", G_CALLBACK(program_description_changed), debr.program);

	/*
	 * URL
	 */
	url_label = gtk_label_new(_("URL:"));
	gtk_widget_show(url_label);
	gtk_table_attach(GTK_TABLE(table), url_label, 0, 1, row, row+1,
			 (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(url_label), 0, 0.5);

	url_entry = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(url_entry), TRUE);
	gtk_widget_show(url_entry);
	gtk_table_attach(GTK_TABLE(table), url_entry, 1, 2, row, row+1,
			 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0), row++;
	g_signal_connect(url_entry, "changed", G_CALLBACK(program_url_changed), debr.program);

	/*
	 * Load program into UI
	 */
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(io_stdin_checkbutton),
				     gebr_geoxml_program_get_stdin(debr.program));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(io_stdout_checkbutton),
				     gebr_geoxml_program_get_stdout(debr.program));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(io_stderr_checkbutton),
				     gebr_geoxml_program_get_stderr(debr.program));
	gtk_entry_set_text(GTK_ENTRY(title_entry), gebr_geoxml_program_get_title(debr.program));
	gtk_entry_set_text(GTK_ENTRY(binary_entry), gebr_geoxml_program_get_binary(debr.program));
	gtk_entry_set_text(GTK_ENTRY(description_entry), gebr_geoxml_program_get_description(debr.program));
	gtk_entry_set_text(GTK_ENTRY(version_entry), gebr_geoxml_program_get_version(debr.program));
	gtk_combo_box_set_active(GTK_COMBO_BOX(mpi_combo), mpi_xml_to_index(debr.program));
	gtk_entry_set_text(GTK_ENTRY(version_entry), gebr_geoxml_program_get_version(debr.program));
	gtk_entry_set_text(GTK_ENTRY(url_entry), gebr_geoxml_program_get_url(debr.program));

	/* validate */
	GebrValidateCase *validate_case = gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_PROGRAM_TITLE);
	g_signal_connect(title_entry, "focus-out-event", G_CALLBACK(on_entry_focus_out), validate_case);
	if (!new_program)
		on_entry_focus_out(GTK_ENTRY(title_entry), NULL, validate_case);
	validate_case = gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_PROGRAM_BINARY);
	g_signal_connect(binary_entry, "focus-out-event", G_CALLBACK(on_entry_focus_out), validate_case);
	if (!new_program)
		on_entry_focus_out(GTK_ENTRY(binary_entry), NULL, validate_case);
	validate_case = gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_PROGRAM_VERSION);
	g_signal_connect(version_entry, "focus-out-event", G_CALLBACK(on_entry_focus_out), validate_case);
	if (!new_program)
		on_entry_focus_out(GTK_ENTRY(version_entry), NULL, validate_case);
	validate_case = gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_PROGRAM_DESCRIPTION);
	g_signal_connect(description_entry, "focus-out-event", G_CALLBACK(on_entry_focus_out), validate_case);
	if (!new_program)
		on_entry_focus_out(GTK_ENTRY(description_entry), NULL, validate_case);
	validate_case = gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_PROGRAM_URL);
	g_signal_connect(url_entry, "focus-out-event", G_CALLBACK(on_entry_focus_out), validate_case);
	if (!new_program)
		on_entry_focus_out(GTK_ENTRY(url_entry), NULL, validate_case);

	gtk_container_set_focus_child(GTK_CONTAINER(table), title_entry);

	gtk_widget_show(dialog);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK) {
		ret = FALSE;
		goto out;
	}

	debr_help_update (GEBR_GEOXML_OBJECT (debr.program));
	menu_saved_status_set(MENU_STATUS_UNSAVED);

out:
	program_load_selected();
	gtk_widget_destroy(dialog);
	return ret;

}

gboolean program_get_selected(GtkTreeIter * iter, gboolean warn_user)
{
	if (gebr_gui_gtk_tree_view_get_selected(GTK_TREE_VIEW(debr.ui_program.tree_view), iter) == FALSE) {
		if (warn_user)
			debr_message(GEBR_LOG_ERROR, _("No program is selected"));
		return FALSE;
	}

	return TRUE;
}

void program_select_iter(GtkTreeIter iter)
{
	gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(debr.ui_program.tree_view), &iter);
	program_selected();
}

/**
 * program_details_update:
 * Load details of selected program to the details view.
 */
void program_details_update(void)
{
	gchar *markup;
	gsize parameters_count;
	GString *text;
	gboolean is_program_selected;

	is_program_selected = debr.program != NULL;

	g_object_set(debr.ui_program.details.url_button, "visible", is_program_selected, NULL);
	g_object_set(debr.ui_program.details.help_edit, "visible", is_program_selected, NULL);
	g_object_set(debr.ui_program.details.help_view, "visible", is_program_selected, NULL);

	if (is_program_selected) {
		const gchar * help;
		gboolean help_exists;
		help = gebr_geoxml_program_get_help(GEBR_GEOXML_PROGRAM(debr.program));
		help_exists = strlen(help) > 0 ? TRUE : FALSE;
		g_object_set(debr.ui_program.details.help_view, "sensitive", help_exists, NULL);
		g_object_set(debr.ui_program.details.help_edit, "sensitive", TRUE, NULL);
		gtk_action_set_sensitive(gtk_action_group_get_action(debr.action_group_program, "program_help_view"), help_exists);
		validate_image_set_check_help(debr.ui_program.help_validate_image, help);
	}

	gtk_widget_set_sensitive(GTK_WIDGET(debr.tool_item_new), is_program_selected);
	if (!is_program_selected) {
		/* Comentary for translators: Tooltip shown in 'New' button in Parameters page, when no programs are
		 * selected */
		gtk_tool_item_set_tooltip_text((debr.tool_item_new), _("There are no programs selected"));
		gtk_frame_set_label(GTK_FRAME(debr.ui_program.details.frame), NULL);
		gtk_label_set_markup(GTK_LABEL(debr.ui_program.details.title_label), "");
		gtk_label_set_markup(GTK_LABEL(debr.ui_program.details.description_label), "");
		gtk_label_set_markup(GTK_LABEL(debr.ui_program.details.nparams_label), "");
		gtk_label_set_markup(GTK_LABEL(debr.ui_program.details.binary_label), "");
		gtk_label_set_markup(GTK_LABEL(debr.ui_program.details.version_label), "");
		gtk_label_set_markup(GTK_LABEL(debr.ui_program.details.mpi_label), "");
		gtk_label_set_markup(GTK_LABEL(debr.ui_program.details.url_label), "");
		return;
	}

	gtk_tool_item_set_tooltip_text(debr.tool_item_new, _("Create a new parameter"));
	gtk_frame_set_label(GTK_FRAME(debr.ui_program.details.frame), _("Details"));

	markup = g_markup_printf_escaped("<b>%s</b>", gebr_geoxml_program_get_title(debr.program));
	gtk_label_set_markup(GTK_LABEL(debr.ui_program.details.title_label), markup);
	g_free(markup);
	markup = g_markup_printf_escaped("<i>%s</i>", gebr_geoxml_program_get_description(debr.program));
	gtk_label_set_markup(GTK_LABEL(debr.ui_program.details.description_label), markup);
	g_free(markup);

	parameters_count = gebr_geoxml_program_count_parameters(debr.program);
	text = g_string_new(NULL);
	switch (parameters_count) {
	case 0:
		g_string_printf(text, _("This program has no parameters"));
		break;
	case 1:
		g_string_printf(text, _("This program has 1 parameter"));
		break;
	default:
		g_string_printf(text, _("This program has %" G_GSIZE_FORMAT " parameters"),
				parameters_count);
	}
	gtk_label_set_text(GTK_LABEL(debr.ui_program.details.nparams_label), text->str);
	g_string_free(text, TRUE);

	markup = g_markup_printf_escaped("<b>%s</b>: %s", _("Binary"), gebr_geoxml_program_get_binary(debr.program));
	gtk_label_set_markup(GTK_LABEL(debr.ui_program.details.binary_label), markup);
	g_free(markup);

	markup = g_markup_printf_escaped("<b>%s</b>: %s", _("Version"), gebr_geoxml_program_get_version(debr.program));
	gtk_label_set_markup(GTK_LABEL(debr.ui_program.details.version_label), markup);
	g_free(markup);

	markup = g_markup_printf_escaped("<b>%s</b>: %s", _("MPI"), mpi_xml_to_combo[mpi_xml_to_index(debr.program)]);
	gtk_label_set_markup(GTK_LABEL(debr.ui_program.details.mpi_label), markup);
	g_free(markup);

	markup = g_markup_printf_escaped("<b>%s</b>:", _("URL"));
	gtk_label_set_markup(GTK_LABEL(debr.ui_program.details.url_label), markup);
	g_free(markup);

	const gchar *uri;
	uri = gebr_geoxml_program_get_url(debr.program);
	if (strlen(uri) > 0) {
		GString *full_uri;
		full_uri = g_string_new(NULL);
		if (!g_str_has_prefix(uri, "http://"))
			g_string_printf(full_uri, "http://%s", uri);
		else
			g_string_assign(full_uri, uri);

		gtk_button_set_label(GTK_BUTTON(debr.ui_program.details.url_button), full_uri->str);
		gtk_link_button_set_uri(GTK_LINK_BUTTON(debr.ui_program.details.url_button), full_uri->str);
		g_string_free(full_uri, TRUE);
	} else {
		gtk_link_button_set_uri(GTK_LINK_BUTTON(debr.ui_program.details.url_button), "");
		gtk_button_set_label(GTK_BUTTON(debr.ui_program.details.url_button), _("URL not set"));
	}

	g_object_set(G_OBJECT(debr.ui_program.details.url_button), "sensitive", strlen(uri) > 0, NULL);
}

/*
 * Private functions.
 */

/**
 * \internal
 * Load stuff into \p iter.
 */
static void program_load_iter(GtkTreeIter * iter)
{
	GebrGeoXmlProgram *program;

	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_program.list_store), iter, PROGRAM_XMLPOINTER, &program, -1);

	gtk_list_store_set(debr.ui_program.list_store, iter, PROGRAM_TITLE, gebr_geoxml_program_get_title(program), -1);
	program_details_update();
	do_navigation_bar_update();
}

/**
 * \internal
 * Reload selected program contents.
 */
static void program_load_selected(void)
{
	GtkTreeIter iter;

	if (!program_get_selected(&iter, FALSE))
		return;
	program_load_iter(&iter);

	/* program is now considered as reviewed by the user */
	gtk_list_store_set(debr.ui_program.list_store, &iter, PROGRAM_STATUS, NULL, -1);
}

/**
 * \internal
 * Appends \p program into user interface.
 */
static GtkTreeIter program_append_to_ui(GebrGeoXmlProgram * program)
{
	GtkTreeIter iter;
	const gchar * help;

	help = gebr_geoxml_program_get_help(program);

	gtk_list_store_append(debr.ui_program.list_store, &iter);
	gtk_list_store_set(debr.ui_program.list_store, &iter,
			   PROGRAM_XMLPOINTER, program,
			   -1);
	program_load_iter(&iter);

	return iter;
}

/**
 * program_selected:
 * Load user selected program.
 */
static void program_selected(void)
{
	GtkTreeIter iter;

	if (program_get_selected(&iter, FALSE) == FALSE) {
		debr.program = NULL;
		return;
	}
	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_program.list_store), &iter, PROGRAM_XMLPOINTER, &debr.program, -1);

	parameter_load_program();
	program_details_update();
	do_navigation_bar_update();
}

/**
 * \internal
 * Agregate action to the popup menu and shows it.
 */
static GtkMenu *program_popup_menu(GtkWidget * tree_view)
{
	GtkWidget *menu;

	GtkTreeIter iter;

	menu = gtk_menu_new();

	if (program_get_selected(&iter, FALSE) == FALSE) {
		gtk_container_add(GTK_CONTAINER(menu),
				  gtk_action_create_menu_item(gtk_action_group_get_action
							      (debr.action_group_program, "program_new")));
		goto out;
	}

	if (gebr_gui_gtk_list_store_can_move_up(debr.ui_program.list_store, &iter) == TRUE)
		gtk_container_add(GTK_CONTAINER(menu),
				  gtk_action_create_menu_item(gtk_action_group_get_action
							      (debr.action_group_program, "program_top")));
	if (gebr_gui_gtk_list_store_can_move_down(debr.ui_program.list_store, &iter) == TRUE)
		gtk_container_add(GTK_CONTAINER(menu),
				  gtk_action_create_menu_item(gtk_action_group_get_action
							      (debr.action_group_program, "program_bottom")));
	if (gebr_gui_gtk_list_store_can_move_up(debr.ui_program.list_store, &iter) == TRUE
	    || gebr_gui_gtk_list_store_can_move_down(debr.ui_program.list_store, &iter) == TRUE)
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(debr.action_group_program, "program_new")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (debr.action_group_program, "program_delete")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (debr.action_group_program, "program_properties")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(debr.action_group_program, "program_copy")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(debr.action_group_program, "program_paste")));

	/* view help */
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (debr.action_group_program, "program_help_view")));

	/* edit help */
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (debr.action_group_program, "program_help_edit")));
 out:	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}

/**
 * \internal
 * Syncronize stdin from UI and XML.
 */
static void program_stdin_changed(GtkToggleButton * togglebutton)
{
	gebr_geoxml_program_set_stdin(debr.program, gtk_toggle_button_get_active(togglebutton));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/**
 * \internal
 * Syncronize stdout from UI and XML.
 */
static void program_stdout_changed(GtkToggleButton * togglebutton)
{
	gebr_geoxml_program_set_stdout(debr.program, gtk_toggle_button_get_active(togglebutton));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/**
 * \internal
 * Syncronize stderr from UI and XML.
 */
static void program_stderr_changed(GtkToggleButton * togglebutton)
{
	gebr_geoxml_program_set_stderr(debr.program, gtk_toggle_button_get_active(togglebutton));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/**
 * \internal
 * Syncronize programs title from UI and XML.
 */
static gboolean program_title_changed(GtkEntry * entry)
{
	gebr_geoxml_program_set_title(debr.program, gtk_entry_get_text(GTK_ENTRY(entry)));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
	return FALSE;
}

/**
 * \internal
 * Syncronize programs binary from UI and XML.
 */
static gboolean program_binary_changed(GtkEntry * entry)
{
	gebr_geoxml_program_set_binary(debr.program, gtk_entry_get_text(GTK_ENTRY(entry)));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
	return FALSE;
}

/**
 * \internal
 * Syncronize programs description from UI and XML.
 */
static gboolean program_description_changed(GtkEntry * entry)
{
	gebr_geoxml_program_set_description(debr.program, gtk_entry_get_text(GTK_ENTRY(entry)));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
	return FALSE;
}

/**
 * \internal
 * Syncronize programs version from UI and XML.
 */
static gboolean program_version_changed(GtkEntry * entry)
{
	gebr_geoxml_program_set_version(debr.program, gtk_entry_get_text(GTK_ENTRY(entry)));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
	return FALSE;
}

/**
 * \internal
 * Syncronize program's parallelization from UI and XML.
 */
static void program_mpi_changed(GtkComboBox * combo)
{
	gebr_geoxml_program_set_mpi(debr.program, mpi_combo_to_xml[gtk_combo_box_get_active(combo)]);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/**
 * \internal
 * Syncronize programs url from UI and XML.
 */
static gboolean program_url_changed(GtkEntry * entry)
{
	gebr_geoxml_program_set_url(debr.program, gtk_entry_get_text(GTK_ENTRY(entry)));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
	return FALSE;
}

/**
 * Calls \ref debr_help_show with menu's help.
 */
void program_help_show(void)
{
	debr_help_show(GEBR_GEOXML_OBJECT(debr.program), TRUE, _("Program Help"));
}

/**
 * Edits the programs help.
 */
void program_help_edit(void)
{
	debr_help_edit(GEBR_GEOXML_OBJECT(debr.program));
}

gchar * debr_program_get_backup_help_from_pointer (gpointer program)
{
	gint index;
	gchar * help;
	const gchar * fname;
	GebrGeoXmlDocument * menu;
	GebrGeoXmlDocument * document;
	GebrGeoXmlSequence * sequence;
	GtkTreeIter iter;


	help = NULL;
	index = gebr_geoxml_sequence_get_index (GEBR_GEOXML_SEQUENCE (program));
	document = gebr_geoxml_object_get_owner_document (GEBR_GEOXML_OBJECT (program));
	if (!debr_menu_get_iter_from_xmlpointer (document, &iter))
		return NULL;

	gtk_tree_model_get (GTK_TREE_MODEL (debr.ui_menu.model), &iter, MENU_PATH, &fname, -1);

	gebr_geoxml_document_load (&menu, fname, TRUE, NULL);
	gebr_geoxml_flow_get_program (GEBR_GEOXML_FLOW (menu), &sequence, index);

	if (sequence) {
		help = g_strdup (gebr_geoxml_program_get_help (GEBR_GEOXML_PROGRAM (sequence)));
	}

	gebr_geoxml_document_free (menu);

	return help;
}

/**
 * \internal
 */
static void program_preview_on_response(GtkWidget * dialog, gint response, struct program_preview_data *data)
{
	switch (response) {
	case RESPONSE_REFRESH:
		gebr_gui_program_edit_reload(data->program_edit,
					     GEBR_GEOXML_PROGRAM(gebr_geoxml_object_copy
								 (GEBR_GEOXML_OBJECT(data->program))));
		break;
	case GTK_RESPONSE_CLOSE:
	default:
		gebr_gui_program_edit_destroy(data->program_edit);
		gtk_widget_destroy(dialog);
		g_free(data);
		break;
	}
}

/**
 * \internal
 */
static gboolean
program_preview_on_delete_event(GtkWidget * dialog, GdkEventAny * event, struct program_preview_data *data)
{
	program_preview_on_response(dialog, GTK_RESPONSE_CLOSE, data);
	return FALSE;
}
