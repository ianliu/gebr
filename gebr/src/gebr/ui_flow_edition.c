/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2008 GeBR core team (http://gebr.sourceforge.net)
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

#include <gui/utils.h>

#include "ui_flow_edition.h"
#include "gebr.h"
#include "support.h"
#include "flow.h"
#include "document.h"
#include "menu.h"
#include "ui_flow.h"
#include "ui_parameters.h"
#include "ui_help.h"
#include "callbacks.h"

extern gchar * no_flow_selected_error;
gchar * no_flow_comp_selected_error =	_("No flow component selected");
gchar * no_menu_selected_error =	_("No menu selected");
gchar * selected_menu_instead_error =	_("Select a menu instead of a category");

/*
 * Prototypes
 */

static gboolean
flow_edition_can_reorder(GtkTreeView * tree_view, GtkTreeIter * iter, GtkTreeIter * before,
	struct ui_flow_edition * ui_flow_edition);
static gboolean
flow_edition_reorder(GtkTreeView * tree_view, GtkTreeIter * iter, GtkTreeIter * before,
	struct ui_flow_edition * ui_flow_edition);

static void
flow_edition_component_selected(void);

static void
flow_edition_menu_add(void);

static void
flow_edition_menu_show_help(void);

static GtkMenu *
flow_edition_popup_menu(GtkWidget * widget, struct ui_flow_edition * ui_flow_edition);

static GtkMenu *
flow_edition_menu_popup_menu(GtkWidget * widget, struct ui_flow_edition * ui_flow_edition);


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
struct ui_flow_edition *
flow_edition_setup_ui(void)
{
	struct ui_flow_edition *	ui_flow_edition;

	GtkWidget *			frame;
	GtkWidget *			hpanel;
	GtkWidget *			scrolled_window;
	GtkWidget *			vbox;
	GtkTreeViewColumn *		col;
	GtkCellRenderer *		renderer;

	/* alloc */
	ui_flow_edition = g_malloc(sizeof(struct ui_flow_edition));

	/* Create flow edit ui_flow_edition->widget */
	ui_flow_edition->widget = gtk_vbox_new(FALSE, 0);
	hpanel = gtk_hpaned_new();
	gtk_container_add(GTK_CONTAINER(ui_flow_edition->widget), hpanel);

	/*
	 * Left side: flow components
	 */
	frame = gtk_frame_new(_("Flow sequence"));
	gtk_paned_pack1(GTK_PANED(hpanel), frame, FALSE, FALSE);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(frame), scrolled_window);

	ui_flow_edition->fseq_store = gtk_list_store_new(FSEQ_N_COLUMN,
		GDK_TYPE_PIXBUF,
		G_TYPE_STRING,
		G_TYPE_POINTER,
		G_TYPE_STRING,
		G_TYPE_ULONG);
	ui_flow_edition->fseq_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ui_flow_edition->fseq_store));
	gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(ui_flow_edition->fseq_view),
		(GtkPopupCallback)flow_edition_popup_menu, ui_flow_edition);
	gtk_tree_view_set_reorder_callback(GTK_TREE_VIEW(ui_flow_edition->fseq_view),
		(GtkTreeViewReorderCallback)flow_edition_reorder,
		(GtkTreeViewReorderCallback)flow_edition_can_reorder,
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
		GTK_SIGNAL_FUNC(flow_edition_component_activated), ui_flow_edition);
	g_signal_connect(GTK_OBJECT(ui_flow_edition->fseq_view), "cursor-changed",
		GTK_SIGNAL_FUNC(flow_edition_component_selected), ui_flow_edition);

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
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(vbox), scrolled_window);

	ui_flow_edition->menu_store = gtk_tree_store_new(MENU_N_COLUMN,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_STRING);
	ui_flow_edition->menu_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ui_flow_edition->menu_store));
	gtk_container_add(GTK_CONTAINER(scrolled_window), ui_flow_edition->menu_view);
	gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(ui_flow_edition->menu_view),
		(GtkPopupCallback)flow_edition_menu_popup_menu, ui_flow_edition);
	g_signal_connect(GTK_OBJECT(ui_flow_edition->menu_view), "row-activated",
		GTK_SIGNAL_FUNC(flow_edition_menu_add), ui_flow_edition);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("Flow", renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_flow_edition->menu_view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "markup", MENU_TITLE_COLUMN);
	gtk_tree_view_column_set_sort_column_id(col, MENU_TITLE_COLUMN);
	gtk_tree_view_column_set_sort_indicator(col, TRUE);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("Description", renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_flow_edition->menu_view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", MENU_DESC_COLUMN);

	return ui_flow_edition;
}

/*
 * Function: flow_edition_load_components
 * Load flow at _filename_ and title _title_
 */
void
flow_edition_load_components(const gchar * filename, const gchar * title)
{
	GeoXmlSequence *	first_program;
	gchar *			input_file;
	gchar *			output_file;

	/* free previous flow */
	flow_free();

	/* load it */
	gebr.flow = GEOXML_FLOW(document_load(filename));
	if (gebr.flow == NULL) {
		gebr_message(LOG_ERROR, TRUE, FALSE, _("Unable to load flow '%s'"), title);
		gebr_message(LOG_ERROR, FALSE, TRUE, _("Unable to load flow '%s' from file '%s'"), title, filename);
		return;
	}

	/* input iter */
	input_file = strlen(geoxml_flow_io_get_input(gebr.flow))
		? g_path_get_basename(geoxml_flow_io_get_input(gebr.flow)) : strdup("");
	gtk_list_store_append(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->input_iter);
	gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->input_iter,
		FSEQ_TITLE_COLUMN, input_file,
		FSEQ_STATUS_COLUMN, gebr.pixmaps.stock_go_back,
		-1);
	/* output iter */
	output_file = strlen(geoxml_flow_io_get_output(gebr.flow))
		? g_path_get_basename(geoxml_flow_io_get_output(gebr.flow)) : strdup("");
	gtk_list_store_append(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->output_iter);
	gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &gebr.ui_flow_edition->output_iter,
		FSEQ_TITLE_COLUMN, output_file,
		FSEQ_STATUS_COLUMN, gebr.pixmaps.stock_go_forward,
		-1);

	/* now into GUI */
	geoxml_flow_get_program(gebr.flow, &first_program, 0);
	flow_add_program_sequence_to_view(first_program);

	g_free(input_file);
	g_free(output_file);
}

/*
 * Function: flow_edition_component_activated
 * Show the current selected flow components parameters
 */
void
flow_edition_component_activated(void)
{
	GtkTreeIter		iter;
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;

	gchar *			title;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE) {
		gebr_message(LOG_ERROR, TRUE, FALSE, no_flow_comp_selected_error);
		return;
	}

	if (gtk_tree_model_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter) ||
	gtk_tree_model_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter)) {
		on_flow_io_activate();
		return;
	}

	gtk_tree_model_get(model, &iter,
		FSEQ_TITLE_COLUMN, &title,
		-1);
	gebr_message(LOG_ERROR, TRUE, FALSE, _("Configuring flow component '%s'"), title);
	parameters_configure_setup_ui();

	g_free(title);
}

/*
 * Function: flow_edition_component_activated
 * Change the flow status when select the status from the "Flow Component" menu.
 */
void
flow_edition_set_status(GtkRadioAction * action)
{
	GtkTreeIter		iter;
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;

	GeoXmlSequence *	program;

	GdkPixbuf *		pixbuf;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE) {
		gebr_message(LOG_ERROR, TRUE, FALSE, no_flow_comp_selected_error);
		return;
	}

	if (gtk_tree_model_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter) ||
	gtk_tree_model_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter))
		return;

	if (action == gebr.actions.flow_edition.configured)
		pixbuf = gebr.pixmaps.stock_apply;
	else if (action == gebr.actions.flow_edition.disabled)
		pixbuf = gebr.pixmaps.stock_cancel;
	else if (action == gebr.actions.flow_edition.unconfigured)
		pixbuf = gebr.pixmaps.stock_warning;
	else
		return;
	gtk_list_store_set(gebr.ui_flow_edition->fseq_store, &iter,
		FSEQ_STATUS_COLUMN, pixbuf,
		-1);

	gtk_tree_model_get(model, &iter, FSEQ_GEOXML_POINTER, &program, -1);
	geoxml_program_set_status(GEOXML_PROGRAM(program), gtk_action_get_name(GTK_ACTION(action)));

	flow_save();
}

/*
 * Section: Private
 * Private functions
 */

/*
 * Fuction: flow_edition_can_reorder
 *
 *
 */
static gboolean
flow_edition_can_reorder(GtkTreeView * tree_view, GtkTreeIter * iter, GtkTreeIter * before,
	struct ui_flow_edition * ui_flow_edition)
{
	if (gtk_tree_model_iter_equal_to(iter, &gebr.ui_flow_edition->input_iter) ||
	gtk_tree_model_iter_equal_to(iter, &gebr.ui_flow_edition->output_iter) ||
	gtk_tree_model_iter_equal_to(before, &gebr.ui_flow_edition->input_iter))
		return FALSE;

	return TRUE;
}

/*
 * Fuction: flow_edition_reorder
 *
 * 
 */
static gboolean
flow_edition_reorder(GtkTreeView * tree_view, GtkTreeIter * iter, GtkTreeIter * before,
	struct ui_flow_edition * ui_flow_edition)
{
	return TRUE;
}


/* Function: flow_edition_component_selected
 * When a flow component (a program in the flow) is selected
 * this funtions get the state of the program and set it on Flow Component Menu
 *
 * PS: this function is called when the signal "cursor-changed" is triggered
 * also by hand.
 */
static void
flow_edition_component_selected(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	GeoXmlSequence *	program;
	const gchar *		status;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE) {
		gebr_message(LOG_ERROR, TRUE, FALSE, no_flow_comp_selected_error);
		return;
	}

	if (gtk_tree_model_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter) ||
	gtk_tree_model_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter))
		return;

	gtk_tree_model_get(model, &iter, FSEQ_GEOXML_POINTER, &program, -1);
	status = geoxml_program_get_status(GEOXML_PROGRAM(program));

#if GTK_CHECK_VERSION(2,10,0)
	gtk_radio_action_set_current_value(gebr.actions.flow_edition.configured,
		(!strcmp(status, "configured"))<<0 |
		(!strcmp(status, "disabled"))<<1 |
		(!strcmp(status, "unconfigured"))<<2);
#else
	if (!strcmp(status, "configured"))
		gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(gebr.actions.flow_edition.configured), TRUE);
	else if (!strcmp(status, "disabled"))
		gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(gebr.actions.flow_edition.disabled), TRUE);
	else if (!strcmp(status, "unconfigured"))
		gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(gebr.actions.flow_edition.unconfigured), TRUE);
#endif
}

/*
 * Function: flow_edition_menu_add
 * Add selected menu to flow sequence
 *
 */
static void
flow_edition_menu_add(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	gchar *			name;
	gchar *			filename;

	GeoXmlFlow *		menu;
	GeoXmlSequence *	program;
	gint			menu_programs_index;
	GeoXmlSequence *	menu_programs;

	if (gebr.flow == NULL) {
		gebr_message(LOG_ERROR, TRUE, FALSE, no_flow_selected_error);
		return;
	}
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_edition->menu_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE) {
		gebr_message(LOG_ERROR, TRUE, FALSE, no_menu_selected_error);
		return;
	}
	if (!gtk_tree_store_iter_depth(gebr.ui_flow_edition->menu_store, &iter)) {
		gebr_message(LOG_ERROR, TRUE, FALSE, selected_menu_instead_error);
		return;
	}

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->menu_store), &iter,
		MENU_TITLE_COLUMN, &name,
		MENU_FILE_NAME_COLUMN, &filename,
		-1);

	menu = menu_load(filename);
	if (menu == NULL)
		goto out;

	/* set parameters' values of menus' programs to default
	 * note that menu changes aren't saved to disk
	 */
	geoxml_flow_get_program(menu, &program, 0);
	while (program != NULL) {
		parameters_reset_to_default(geoxml_program_get_parameters(GEOXML_PROGRAM(program)));
		geoxml_sequence_next(&program);
	}

	menu_programs_index = geoxml_flow_get_programs_number(gebr.flow);
	/* add it to the file */
	geoxml_flow_add_flow(gebr.flow, menu);
	geoxml_document_free(GEOXML_DOC(menu));
	flow_save();

	/* and to the GUI */
	geoxml_flow_get_program(gebr.flow, &menu_programs, menu_programs_index);
	flow_add_program_sequence_to_view(menu_programs);

out:	g_free(name);
	g_free(filename);
}

/*
 * Function: menus_show_help
 * Show's menus help
 */
static void
flow_edition_menu_show_help(void)
{
	GtkTreeIter		iter;
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;

	gchar *		        menu_filename;
	GeoXmlFlow *		menu;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gebr.ui_flow_edition->menu_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE) {
		gebr_message(LOG_ERROR, TRUE, FALSE, no_menu_selected_error);
		return;
	}
	if (!gtk_tree_store_iter_depth(gebr.ui_flow_edition->menu_store, &iter)) {
		gebr_message(LOG_ERROR, TRUE, FALSE, selected_menu_instead_error);
		return;
	}

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_edition->menu_store), &iter,
			MENU_FILE_NAME_COLUMN, &menu_filename,
			-1);

	menu = menu_load(menu_filename);
	if (menu == NULL)
		goto out;
	help_show(geoxml_document_get_help(GEOXML_DOC(menu)), _("Menu help"));

out:	g_free(menu_filename);
}

static GtkMenu *
flow_edition_popup_menu(GtkWidget * widget, struct ui_flow_edition * ui_flow_edition)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	GtkWidget *		menu;
	GtkWidget *		menu_item;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_edition->fseq_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE)
		return NULL;

	menu = gtk_menu_new();

	if (gtk_tree_model_iter_equal_to(&iter, &gebr.ui_flow_edition->input_iter) ||
	gtk_tree_model_iter_equal_to(&iter, &gebr.ui_flow_edition->output_iter)) {
		gtk_container_add(GTK_CONTAINER(menu),
			gtk_action_create_menu_item(gebr.actions.flow_edition.properties));
		goto out;
	}

	/* Move top */
	if (gtk_list_store_can_move_up(ui_flow_edition->fseq_store, &iter) == TRUE) {
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_GOTO_TOP, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect(menu_item, "activate",
			(GCallback)flow_program_move_top, NULL);
	}
	/* Move bottom */
	if (gtk_list_store_can_move_down(ui_flow_edition->fseq_store, &iter) == TRUE) {
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_GOTO_BOTTOM, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect(menu_item, "activate",
			(GCallback)flow_program_move_bottom, NULL);
	}

	/* separator */
	if (gtk_list_store_can_move_up(ui_flow_edition->fseq_store, &iter) == TRUE ||
	gtk_list_store_can_move_down(ui_flow_edition->fseq_store, &iter) == TRUE) {
		menu_item = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	}
	/* properties */
	gtk_container_add(GTK_CONTAINER(menu),
		gtk_action_create_menu_item(gebr.actions.flow_edition.properties));
	/* status */
	gtk_menu_shell_append(GTK_MENU_SHELL(menu),
		gtk_action_create_menu_item(GTK_ACTION(gebr.actions.flow_edition.configured)));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu),
		gtk_action_create_menu_item(GTK_ACTION(gebr.actions.flow_edition.disabled)));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu),
		gtk_action_create_menu_item(GTK_ACTION(gebr.actions.flow_edition.unconfigured)));

	/* separator */
	menu_item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	gtk_container_add(GTK_CONTAINER(menu),
		gtk_action_create_menu_item(gebr.actions.flow_edition.delete));
	gtk_container_add(GTK_CONTAINER(menu),
		gtk_action_create_menu_item(gebr.actions.flow_edition.help));

out:	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}

static GtkMenu *
flow_edition_menu_popup_menu(GtkWidget * widget, struct ui_flow_edition * ui_flow_edition)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	GtkWidget *		menu;
	GtkWidget *		menu_item;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_edition->menu_view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE)
		return NULL;

	if (!gtk_tree_store_iter_depth(gebr.ui_flow_edition->menu_store, &iter)) {
		return NULL;
	}

	menu = gtk_menu_new();

	/* add */
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(GTK_OBJECT(menu_item), "activate",
		GTK_SIGNAL_FUNC(flow_edition_menu_add), NULL);
	/* help */
	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_HELP, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(GTK_OBJECT(menu_item), "activate",
		GTK_SIGNAL_FUNC(flow_edition_menu_show_help), NULL);

	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}
