/*   GeBR - An environment for seismic processing.
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

/* File: ui_flow_edition.c
 * Interface functions and callbacks for the "Flow Edition" page.
 */

#include <string.h>

#include <libgebr/intl.h>
#include <libgebr/gui/utils.h>

#include "ui_flow_edition.h"
#include "gebr.h"
#include "flow.h"
#include "document.h"
#include "menu.h"
#include "ui_flow.h"
#include "ui_parameters.h"
#include "ui_help.h"
#include "callbacks.h"

/*
 * Prototypes
 */

static gboolean
flow_edition_may_reorder(GtkTreeView * tree_view, GtkTreeIter * iter, GtkTreeIter * position,
			 GtkTreeViewDropPosition drop_position, struct ui_flow_edition *ui_flow_edition);
static gboolean
flow_edition_reorder(GtkTreeView * tree_view, GtkTreeIter * iter, GtkTreeIter * position,
		     GtkTreeViewDropPosition drop_position, struct ui_flow_edition *ui_flow_edition);

static void flow_edition_component_selected(void);
static gboolean flow_edition_get_selected_menu(GtkTreeIter * iter, gboolean warn_unselected);

static void flow_edition_menu_add(void);
static void flow_edition_menu_show_help(void);

static GtkMenu *flow_edition_component_popup_menu(GtkWidget * widget, struct ui_flow_edition *ui_flow_edition);
static GtkMenu *flow_edition_menu_popup_menu(GtkWidget * widget, struct ui_flow_edition *ui_flow_edition);
static void flow_edition_on_combobox_changed(GtkComboBox * combobox);

/*
 * Section: Public
 * Public functions
 */

/* Function: flow_edition_setup_ui
 * Assembly the flow edit ui_flow_edition->widget.
 *
 * Return:
 * The structure containing relevant data.
 */
struct ui_flow_edition *flow_edition_setup_ui(void)
{
	struct ui_flow_edition *ui_flow_edition;

	GtkWidget *hpanel;
	GtkWidget *frame;
	GtkWidget *alignment;
	GtkWidget *label;
	GtkWidget *left_vbox;
	GtkWidget *scrolled_window;
	GtkWidget *vbox;
	GtkWidget *combobox;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	/* alloc */
	ui_flow_edition = g_malloc(sizeof(struct ui_flow_edition));

	/* Create flow edit ui_flow_edition->widget */
	ui_flow_edition->widget = gtk_vbox_new(FALSE, 0);
	hpanel = gtk_hpaned_new();
	gtk_container_add(GTK_CONTAINER(ui_flow_edition->widget), hpanel);

	/*
	 * Left side: flow components
	 */
	left_vbox = gtk_vbox_new(FALSE, 5);
	gtk_paned_pack1(GTK_PANED(hpanel), left_vbox, FALSE, FALSE);

	combobox = gtk_combo_box_new_with_model(GTK_TREE_MODEL(gebr.ui_server_list->common.store));
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combobox), renderer, FALSE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(combobox), renderer, "pixbuf", SERVER_STATUS_ICON);
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combobox), renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(combobox), renderer, "text", SERVER_NAME);
	frame = gtk_frame_new(NULL);
	alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
	label = gtk_label_new_with_mnemonic(_("Server"));
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 4, 5, 5);
	ui_flow_edition->server_combobox = combobox;
	gtk_frame_set_label_widget(GTK_FRAME(frame), label);
	gtk_container_add(GTK_CONTAINER(frame), alignment);
	gtk_container_add(GTK_CONTAINER(alignment), combobox);
	gtk_box_pack_start(GTK_BOX(left_vbox), frame, FALSE, TRUE, 0);
	g_signal_connect(combobox, "changed", G_CALLBACK(flow_edition_on_combobox_changed), NULL);

	frame = gtk_frame_new(NULL);
	alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
	label = gtk_label_new_with_mnemonic(_("Queues"));
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 4, 5, 5);
	ui_flow_edition->queue_bin = GTK_BIN(alignment);
	gtk_frame_set_label_widget(GTK_FRAME(frame), label);
	gtk_container_add(GTK_CONTAINER(frame), alignment);
	gtk_box_pack_start(GTK_BOX(left_vbox), frame, FALSE, TRUE, 0);

	frame = gtk_frame_new(_("Flow sequence"));
	gtk_box_pack_start(GTK_BOX(left_vbox), frame, TRUE, TRUE, 0);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

	ui_flow_edition->fseq_store = gtk_list_store_new(FSEQ_N_COLUMN,
							 GDK_TYPE_PIXBUF,
							 G_TYPE_STRING, G_TYPE_POINTER);
	ui_flow_edition->fseq_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ui_flow_edition->fseq_store));
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_flow_edition->fseq_view)),
				    GTK_SELECTION_MULTIPLE);
	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(ui_flow_edition->fseq_view),
						  (GebrGuiGtkPopupCallback) flow_edition_component_popup_menu,
						  ui_flow_edition);
	gebr_gui_gtk_tree_view_set_reorder_callback(GTK_TREE_VIEW(ui_flow_edition->fseq_view),
						    (GebrGuiGtkTreeViewReorderCallback) flow_edition_reorder,
						    (GebrGuiGtkTreeViewReorderCallback) flow_edition_may_reorder,
						    ui_flow_edition);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ui_flow_edition->fseq_view), FALSE);

	renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_flow_edition->fseq_view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "pixbuf", FSEQ_STATUS_COLUMN);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_flow_edition->fseq_view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", FSEQ_TITLE_COLUMN);

	/* Double click on flow component open its parameter window */
	g_signal_connect(ui_flow_edition->fseq_view, "row-activated",
			 G_CALLBACK(flow_edition_component_activated), ui_flow_edition);
	g_signal_connect(GTK_OBJECT(ui_flow_edition->fseq_view), "cursor-changed",
			 G_CALLBACK(flow_edition_component_selected), ui_flow_edition);

	gtk_container_add(GTK_CONTAINER(scrolled_window), ui_flow_edition->fseq_view);
	gtk_widget_set_size_request(GTK_WIDGET(scrolled_window), 180, 30);

	/*
	 * Right side: Menu list
	 */
	frame = gtk_frame_new(_("Flow components"));
	gtk_paned_pack2(GTK_PANED(hpanel), frame, TRUE, TRUE);

	vbox = gtk_vbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(vbox), scrolled_window);

	ui_flow_edition->menu_store = gtk_tree_store_new(MENU_N_COLUMN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(ui_flow_edition->menu_store), MENU_TITLE_COLUMN, GTK_SORT_ASCENDING);
	ui_flow_edition->menu_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ui_flow_edition->menu_store));
	gtk_container_add(GTK_CONTAINER(scrolled_window), ui_flow_edition->menu_view);
	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(ui_flow_edition->menu_view),
						  (GebrGuiGtkPopupCallback) flow_edition_menu_popup_menu,
						  ui_flow_edition);
	g_signal_connect(GTK_OBJECT(ui_flow_edition->menu_view), "row-activated", G_CALLBACK(flow_edition_menu_add),
			 ui_flow_edition);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Flow"), renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_flow_edition->menu_view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "markup", MENU_TITLE_COLUMN);
	gtk_tree_view_column_set_sort_column_id(col, MENU_TITLE_COLUMN);
	gtk_tree_view_column_set_sort_indicator(col, TRUE);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Description"), renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_flow_edition->menu_view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", MENU_DESC_COLUMN);

	return ui_flow_edition;
}

/*
 * Function: flow_edition_load_components
 * Load current flow's (gebr.flow) programs
 */
void flow_edition_load_components(void)
{
	GebrGeoXmlSequence *first_program;

	gtk_list_store_clear(gebr.ui_flow_edition->fseq_store);
	if (!flow_browse_get_selected(NULL, FALSE))
		return;

	gtk_list_store_append(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->input_iter);
	gtk_list_store_append(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->output_iter);
	flow_edition_set_io();

	/* now into GUI */
	gebr_geoxml_flow_get_program(gebr.flow, &first_program, 0);
	flow_add_program_sequence_to_view(first_program, FALSE);
}

/*
 * Fuction: flow_edition_get_selected_component
 * Return TRUE if there is a selected component (program) and put it into _iter_
 * If _warn_unselected_ is TRUE then a error message is displayed if the FALSE is returned
 */
gboolean flow_edition_get_selected_component(GtkTreeIter * iter, gboolean warn_unselected)
{
	if (!gebr_gui_gtk_tree_view_get_selected(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view), iter)) {
		if (warn_unselected)
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("No flow component selected"));
		return FALSE;
	}

	return TRUE;
}

/*
 * Fuction: flow_edition_select_component_iter
 * Select _iter_ and scroll to it.
 */
void flow_edition_select_component_iter(GtkTreeIter * iter)
{
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view));
	gtk_tree_selection_unselect_all(selection);
	if (iter == NULL)
		return;
	gtk_tree_selection_select_iter(selection, iter);
	gebr_gui_gtk_tree_view_scroll_to_iter_cell(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view), iter);

	flow_edition_component_selected();
}

/*
 * Function: flow_edition_set_io
 * Set the XML IO into iterators
 */
void flow_edition_set_io(void)
{
	gchar *input;
	gchar *output;

	if ((input = (gchar *) gebr_geoxml_flow_server_io_get_input(gebr.flow_server)) == NULL)
		input = "";
	if ((output = (gchar *) gebr_geoxml_flow_server_io_get_output(gebr.flow_server)) == NULL)
		output = "";

	gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->input_iter,
			   FSEQ_STATUS_COLUMN, gebr.pixmaps.stock_go_back, FSEQ_TITLE_COLUMN, input, -1);
	gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->output_iter,
			   FSEQ_STATUS_COLUMN, gebr.pixmaps.stock_go_forward, FSEQ_TITLE_COLUMN, output, -1);
}

/*
 * Function: flow_edition_component_activated
 * Show the current selected flow components parameters
 */
void flow_edition_component_activated(void)
{
	GtkTreeIter iter;
	gchar *title;

	gebr_gui_gtk_tree_view_turn_to_single_selection(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view));
	if (!flow_edition_get_selected_component(&iter, FALSE))
		return;
	if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter)) {
		flow_io_simple_setup_ui(FALSE);
		return;
	}
	if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter)) {
		flow_io_simple_setup_ui(TRUE);
		return;
	}

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter, FSEQ_TITLE_COLUMN, &title, -1);
	gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Configuring flow component '%s'"), title);
	parameters_configure_setup_ui();

	g_free(title);
}

/*
 * Function: flow_edition_component_activated
 * Change the flow status when select the status from the "Flow Component" menu.
 */
void flow_edition_status_changed(void)
{
	GtkTreeIter iter;
	GtkRadioAction *radio_action;
	GdkPixbuf *pixbuf;
	const gchar *status;

	radio_action =
	    GTK_RADIO_ACTION(gtk_action_group_get_action(gebr.action_group, "flow_edition_status_configured"));
	switch (gtk_radio_action_get_current_value(radio_action)) {
	case 0:
		pixbuf = gebr.pixmaps.stock_apply;
		status = "configured";
		break;
	case 1:
		pixbuf = gebr.pixmaps.stock_cancel;
		status = "disabled";
		break;
	case 2:
		pixbuf = gebr.pixmaps.stock_warning;
		status = "unconfigured";
		break;
	default:
		return;
	}

	gebr_gui_gtk_tree_view_foreach_selected(&iter, gebr.ui_flow_edition->fseq_view) {
		GebrGeoXmlSequence *program;

		if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter) ||
		    gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter))
			continue;

		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter,
				   FSEQ_GEBR_GEOXML_POINTER, &program, -1);
		gebr_geoxml_program_set_status(GEBR_GEOXML_PROGRAM(program), status);
		gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &iter, FSEQ_STATUS_COLUMN, pixbuf, -1);
	}
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow));
}

/* Function: flow_edition_on_server_changed
 * Update flow edition interface with information of the
 * current selected server
 */
void flow_edition_on_server_changed(void)
{
	flow_edition_on_combobox_changed(GTK_COMBO_BOX(gebr.ui_flow_edition->server_combobox));
}

/*
 * Section: Private
 * Private functions
 */

/*
 * Fuction: flow_edition_get_selected_menu
 * Return TRUE if there is a selected menu and put it into _iter_
 * If _warn_unselected_ is TRUE then a error message is displayed if the FALSE is returned
 */
static gboolean flow_edition_get_selected_menu(GtkTreeIter * iter, gboolean warn_unselected)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_edition->menu_view));
	if (gtk_tree_selection_get_selected(selection, &model, iter) == FALSE) {
		if (warn_unselected)
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("No menu selected"));
		return FALSE;
	}
	if (!gtk_tree_store_iter_depth(gebr.ui_flow_edition->menu_store, iter)) {
		if (warn_unselected)
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Select a menu instead of a category"));
		return FALSE;
	}

	return TRUE;
}

/*
 * Fuction: flow_edition_may_reorder
 *
 *
 */
static gboolean
flow_edition_may_reorder(GtkTreeView * tree_view, GtkTreeIter * iter, GtkTreeIter * position,
			 GtkTreeViewDropPosition drop_position, struct ui_flow_edition *ui_flow_edition)
{
	if (gebr_gui_gtk_tree_iter_equal_to(iter, &ui_flow_edition->input_iter) ||
	    gebr_gui_gtk_tree_iter_equal_to(iter, &ui_flow_edition->output_iter))
		return FALSE;
	if (drop_position != GTK_TREE_VIEW_DROP_AFTER &&
	    gebr_gui_gtk_tree_iter_equal_to(position, &ui_flow_edition->input_iter))
		return FALSE;
	if (drop_position == GTK_TREE_VIEW_DROP_AFTER &&
	    gebr_gui_gtk_tree_iter_equal_to(position, &ui_flow_edition->output_iter))
		return FALSE;

	return TRUE;
}

/*
 * Fuction: flow_edition_reorder
 *
 * 
 */
static gboolean
flow_edition_reorder(GtkTreeView * tree_view, GtkTreeIter * iter, GtkTreeIter * position,
		     GtkTreeViewDropPosition drop_position, struct ui_flow_edition *ui_flow_edition)
{
	GebrGeoXmlSequence *program;
	GebrGeoXmlSequence *position_program;

	gtk_tree_model_get(gtk_tree_view_get_model(tree_view), iter, FSEQ_GEBR_GEOXML_POINTER, &program, -1);
	gtk_tree_model_get(gtk_tree_view_get_model(tree_view), position,
			   FSEQ_GEBR_GEOXML_POINTER, &position_program, -1);

	if (drop_position != GTK_TREE_VIEW_DROP_AFTER) {
		gebr_geoxml_sequence_move_before(program, position_program);
		gtk_list_store_move_before(gebr.ui_flow_edition->fseq_store, iter, position);
	} else {
		gebr_geoxml_sequence_move_after(program, position_program);
		gtk_list_store_move_after(gebr.ui_flow_edition->fseq_store, iter, position);
	}
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow));

	return FALSE;
}

/* Function: flow_edition_component_selected
 * When a flow component (a program in the flow) is selected
 * this funtions get the state of the program and set it on Flow Component Menu
 *
 * PS: this function is called when the signal "cursor-changed" is triggered
 * also by hand.
 */
static void flow_edition_component_selected(void)
{
	GtkTreeIter iter;
	const gchar *status;

	gebr.program = NULL;
	if (!flow_edition_get_selected_component(&iter, FALSE))
		return;
	if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter) ||
	    gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter))
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->fseq_store), &iter,
			   FSEQ_GEBR_GEOXML_POINTER, &gebr.program, -1);
	status = gebr_geoxml_program_get_status(gebr.program);

	if (!strcmp(status, "configured"))
		gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(gtk_action_group_get_action(gebr.action_group,
											   "flow_edition_status_configured")),
					     TRUE);
	else if (!strcmp(status, "disabled"))
		gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(gtk_action_group_get_action(gebr.action_group,
											   "flow_edition_status_disabled")),
					     TRUE);
	else if (!strcmp(status, "unconfigured"))
		gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(gtk_action_group_get_action(gebr.action_group,
											   "flow_edition_status_unconfigured")),
					     TRUE);

	if (strlen(gebr_geoxml_program_get_help(gebr.program)) == 0)	
		gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group, "flow_edition_help"), FALSE);
	else
		gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group, "flow_edition_help"), TRUE);

}

/* Function: flow_edition_menu_add
 * Add selected menu to flow sequence
 */
static void flow_edition_menu_add(void)
{
	GtkTreeIter iter;
	gchar *name;
	gchar *filename;
	GebrGeoXmlFlow *menu;
	GebrGeoXmlSequence *program;
	GebrGeoXmlSequence *menu_programs;
	gint menu_programs_index;

	if (!flow_browse_get_selected(NULL, TRUE))
		return;
	if (!flow_edition_get_selected_menu(&iter, TRUE))
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->menu_store), &iter,
			   MENU_TITLE_COLUMN, &name, MENU_FILEPATH_COLUMN, &filename, -1);
	menu = menu_load(filename);
	if (menu == NULL)
		goto out;

	/* set parameters' values of menus' programs to default
	 * note that menu changes aren't saved to disk
	 */
	gebr_geoxml_flow_get_program(menu, &program, 0);
	for (; program != NULL; gebr_geoxml_sequence_next(&program))
		parameters_reset_to_default(gebr_geoxml_program_get_parameters(GEBR_GEOXML_PROGRAM(program)));

	menu_programs_index = gebr_geoxml_flow_get_programs_number(gebr.flow);
	/* add it to the file */
	gebr_geoxml_flow_add_flow(gebr.flow, menu);
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow));

	/* and to the GUI */
	gebr_geoxml_flow_get_program(gebr.flow, &menu_programs, menu_programs_index);
	flow_add_program_sequence_to_view(menu_programs, TRUE);

	gebr_geoxml_document_free(GEBR_GEOXML_DOC(menu));
 out:	g_free(name);
	g_free(filename);
}

/* Function: menus_show_help
 * Show's menus help
 */
static void flow_edition_menu_show_help(void)
{
	GtkTreeIter iter;
	gchar *menu_filename;
	GebrGeoXmlFlow *menu;

	if (!flow_edition_get_selected_menu(&iter, TRUE))
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->menu_store), &iter,
			   MENU_FILEPATH_COLUMN, &menu_filename, -1);

	menu = menu_load(menu_filename);
	if (menu == NULL)
		goto out;
	help_show(gebr_geoxml_document_get_help(GEBR_GEOXML_DOC(menu)), _("Menu help"));

 out:	g_free(menu_filename);
}

/* Function: flow_edition_component_popup_menu
 * Show popup menu for flow component
 */
static GtkMenu *flow_edition_component_popup_menu(GtkWidget * widget, struct ui_flow_edition *ui_flow_edition)
{
	GtkTreeIter iter;
	GtkWidget *menu;
	GtkWidget *menu_item;

	if (!flow_edition_get_selected_component(&iter, FALSE))
		return NULL;

	menu = gtk_menu_new();

	if (gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter) ||
	    gebr_gui_gtk_tree_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter)) {
		gtk_container_add(GTK_CONTAINER(menu),
				  gtk_action_create_menu_item(gtk_action_group_get_action
							      (gebr.action_group, "flow_edition_properties")));
		goto out;
	}

	/* Move top */
	if (gebr_gui_gtk_list_store_can_move_up(ui_flow_edition->fseq_store, &iter) == TRUE) {
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_GOTO_TOP, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect(menu_item, "activate", G_CALLBACK(flow_program_move_top), NULL);
	}
	/* Move bottom */
	if (gebr_gui_gtk_list_store_can_move_down(ui_flow_edition->fseq_store, &iter) == TRUE) {
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_GOTO_BOTTOM, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect(menu_item, "activate", G_CALLBACK(flow_program_move_bottom), NULL);
	}
	/* separator */
	if (gebr_gui_gtk_list_store_can_move_up(ui_flow_edition->fseq_store, &iter) == TRUE ||
	    gebr_gui_gtk_list_store_can_move_down(ui_flow_edition->fseq_store, &iter) == TRUE)
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

	/* status */
	gtk_menu_shell_append(GTK_MENU_SHELL(menu),
			      gtk_action_create_menu_item(GTK_ACTION
							  (gtk_action_group_get_action
							   (gebr.action_group, "flow_edition_status_configured"))));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu),
			      gtk_action_create_menu_item(GTK_ACTION
							  (gtk_action_group_get_action
							   (gebr.action_group, "flow_edition_status_disabled"))));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu),
			      gtk_action_create_menu_item(GTK_ACTION
							  (gtk_action_group_get_action
							   (gebr.action_group, "flow_edition_status_unconfigured"))));

	/* separator */
	menu_item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group, "flow_edition_copy")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group, "flow_edition_paste")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group, "flow_edition_delete")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group, "flow_edition_properties")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group, "flow_edition_help")));

 out:	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}

/* Function: flow_edition_component_popup_menu
 * Show popup menu for menu list
 */
static GtkMenu *flow_edition_menu_popup_menu(GtkWidget * widget, struct ui_flow_edition *ui_flow_edition)
{
	GtkTreeIter iter;
	GtkWidget *menu;
	GtkWidget *menu_item;

	menu = gtk_menu_new();
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group, "flow_edition_refresh")));

	if (!flow_edition_get_selected_menu(&iter, FALSE))
		goto out;

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
	/* add */
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(GTK_OBJECT(menu_item), "activate", G_CALLBACK(flow_edition_menu_add), NULL);
	/* help */
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_HELP, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(GTK_OBJECT(menu_item), "activate", G_CALLBACK(flow_edition_menu_show_help), NULL);

 out:	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}

/*
 *
 */
static void flow_edition_on_combobox_changed(GtkComboBox * combobox)
{
	struct server *server;
	GtkTreeIter iter;
	GtkTreeIter iter_queue;
	GtkWidget *queue_combobox;
	GtkCellRenderer *renderer;

	gebr.flow_server = NULL;
	if (!flow_browse_get_selected(NULL, TRUE))
		return;
	if (!gtk_combo_box_get_active_iter(combobox, &iter))
		return;

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_server_list->common.store), &iter, SERVER_POINTER, &server, -1);

	if (server->type == GEBR_COMM_SERVER_TYPE_REGULAR) {
		queue_combobox = gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(server->queues_model), 0);
		g_signal_connect();
	} else {
		queue_combobox = gtk_combo_box_new_with_model(GTK_TREE_MODEL(server->queues_model));
	}
	gebr.ui_flow_edition->queue_combobox = queue_combobox;
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(queue_combobox), renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(queue_combobox), renderer, "text", 0);
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(server->queues_model), &iter_queue))
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(queue_combobox), &iter_queue);

	if (gtk_bin_get_child(gebr.ui_flow_edition->queue_bin))
		gtk_widget_destroy(gtk_bin_get_child(gebr.ui_flow_edition->queue_bin));
	gtk_container_add(GTK_CONTAINER(gebr.ui_flow_edition->queue_bin), queue_combobox);
	gtk_widget_show(queue_combobox);

	/* select the first server entry, which is the last edited one */
	gebr.flow_server = gebr_geoxml_flow_servers_query(gebr.flow, server->comm->address->str, NULL, NULL, NULL);
	/* Add server if it doesn't yet exist on flow */
	if (gebr.flow_server == NULL) {
		gebr.flow_server = gebr_geoxml_flow_append_server(gebr.flow);
		flow_io_set_server(&iter, "", "", "");
	}
	gebr_geoxml_sequence_move_after(GEBR_GEOXML_SEQUENCE(gebr.flow_server), NULL);
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow));

	flow_edition_set_io();
	flow_browse_info_update();
}
