/*   DeBR - GeBR Designer
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>

#include <gui/utils.h>
#include <gui/valuesequenceedit.h>

#include "menu.h"
#include "support.h"
#include "debr.h"
#include "callbacks.h"
#include "help.h"
#include "program.h"

/*
 * Declarations
 */

enum {
	CATEGORY_NAME,
	CATEGORY_XMLPOINTER,
	CATEGORY_N_COLUMN
};

static void
menu_saved_status_set_from_iter(GtkTreeIter * iter, MenuStatus status);

static void
menu_details_update(void);

static void
menu_title_changed(GtkEntry * entry);
static void
menu_description_changed(GtkEntry * entry);
static void
menu_help_view(void);
static void
menu_help_edit(void);
static void
menu_author_changed(GtkEntry * entry);
static void
menu_email_changed(GtkEntry * entry);
static void
menu_category_add(ValueSequenceEdit * sequence_edit, GtkComboBox * combo_box);
static void
menu_category_changed(void);

/*
 * Section: Public
 */

/*
 * Function: menu_setup_ui
 * Setup menu view
 *
 */
void
menu_setup_ui(void)
{
	GtkWidget *		hpanel;
	GtkWidget *		scrolled_window;
	GtkWidget *		frame;
	GtkWidget *		details;
	GtkTreeViewColumn *	col;
	GtkCellRenderer *	renderer;
        GtkWidget *             table;

	hpanel = gtk_hpaned_new();
	gtk_widget_show(hpanel);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_paned_pack1(GTK_PANED(hpanel), scrolled_window, FALSE, FALSE);
	gtk_widget_set_size_request(scrolled_window, 200, -1);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_IN);	

	debr.ui_menu.list_store = gtk_list_store_new(MENU_N_COLUMN,
		GDK_TYPE_PIXBUF,
		G_TYPE_STRING,
		G_TYPE_POINTER,
		G_TYPE_STRING);
	debr.ui_menu.tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(debr.ui_menu.list_store));
	gtk_widget_show(debr.ui_menu.tree_view);
	gtk_container_add(GTK_CONTAINER(scrolled_window), debr.ui_menu.tree_view);
	g_signal_connect(debr.ui_menu.tree_view, "cursor-changed",
		(GCallback)menu_selected, NULL);
	g_signal_connect(debr.ui_menu.tree_view, "row-activated",
		GTK_SIGNAL_FUNC(menu_dialog_setup_ui), NULL);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(debr.ui_menu.tree_view), FALSE);
	renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width(col, 24);
	gtk_tree_view_column_add_attribute(col, renderer, "pixbuf", MENU_STATUS);
	gtk_tree_view_append_column(GTK_TREE_VIEW(debr.ui_menu.tree_view), col);
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_column_add_attribute(col, renderer, "text", MENU_FILENAME);
	gtk_tree_view_append_column(GTK_TREE_VIEW(debr.ui_menu.tree_view), col);

	/*
	 * Info Panel
	 */
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_paned_pack2(GTK_PANED(hpanel), scrolled_window, TRUE, FALSE);

	frame = gtk_frame_new(_("Details"));
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), frame);
	details = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), details);

	debr.ui_menu.details.title_label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_menu.details.title_label), 0, 0);
	gtk_box_pack_start(GTK_BOX(details), debr.ui_menu.details.title_label, FALSE, TRUE, 0);

	debr.ui_menu.details.description_label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_menu.details.description_label), 0, 0);
	gtk_box_pack_start(GTK_BOX(details), debr.ui_menu.details.description_label, FALSE, TRUE, 0);

	debr.ui_menu.details.nprogs_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(debr.ui_menu.details.nprogs_label), 0, 0);
	gtk_box_pack_start(GTK_BOX(details), debr.ui_menu.details.nprogs_label, FALSE, TRUE, 10);

	table = gtk_table_new(3, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(details), table, FALSE, TRUE, 0);

	debr.ui_menu.details.category_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(debr.ui_menu.details.category_label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), debr.ui_menu.details.category_label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 3, 3);

	debr.ui_menu.details.categories_label[0] = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(debr.ui_menu.details.categories_label[0]), 0, 0);
	gtk_table_attach(GTK_TABLE(table), debr.ui_menu.details.categories_label[0], 1, 2, 0, 1, GTK_FILL, GTK_FILL, 3, 3);

	debr.ui_menu.details.categories_label[1] = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(debr.ui_menu.details.categories_label[1]), 0, 0);
	gtk_table_attach(GTK_TABLE(table), debr.ui_menu.details.categories_label[1], 1, 2, 1, 2, GTK_FILL, GTK_FILL, 3, 3);

	debr.ui_menu.details.categories_label[2] = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(debr.ui_menu.details.categories_label[2]), 0, 0);
	gtk_table_attach(GTK_TABLE(table), debr.ui_menu.details.categories_label[2], 1, 2, 2, 3, GTK_FILL, GTK_FILL, 3, 3);

	debr.ui_menu.details.author_label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_menu.details.author_label), 0, 0);
	gtk_box_pack_end(GTK_BOX(details), debr.ui_menu.details.author_label, FALSE, TRUE, 0);

	debr.ui_menu.widget = hpanel;
	gtk_widget_show_all(debr.ui_menu.widget);
}

/*
 * Function: menu_new
 * Create a new (unsaved) menu and add it to the tree view
 */
void
menu_new(void)
{
	static int		new_count = 0;
	GString *		new_menu_str;
	GtkTreeIter		iter;

	new_menu_str = g_string_new(NULL);
	g_string_printf(new_menu_str, "%s%d.mnu", _("untitled"), ++new_count);

	debr.menu = geoxml_flow_new();
	geoxml_document_set_author(GEOXML_DOC(debr.menu), debr.config.name->str);
	geoxml_document_set_email(GEOXML_DOC(debr.menu), debr.config.email->str);

	gtk_list_store_append(debr.ui_menu.list_store, &iter);
	gtk_list_store_set(debr.ui_menu.list_store, &iter,
		MENU_FILENAME, new_menu_str->str,
		MENU_XMLPOINTER, (gpointer)debr.menu,
		MENU_PATH, "",
		-1);

	/* select it */
	menu_select_iter(&iter);
	
	/* add a new program for the user to play with */
	program_new();
	/* new menu with no changes shouldn't be save */
	menu_saved_status_set(MENU_STATUS_SAVED);
	menu_dialog_setup_ui();

	g_string_free(new_menu_str, TRUE);
}

GeoXmlFlow *
menu_load(const gchar * path)
{
	GeoXmlDocument *	menu;
	int			ret;

	if ((ret = geoxml_document_load(&menu, path))) {
		debr_message(LOG_ERROR, _("Could not load menu at '%s': %s"), path,
			"unknown error"/*geoxml_error_string((enum GEOXML_RETV)ret)*/);

		return NULL;
	}

	return GEOXML_FLOW(menu);
}

/*
 * Function: menu_load_user_directory
 * Read each menu on user's menus directory.
 */
void
menu_load_user_directory(void)
{
	GtkTreeIter		iter;

	DIR *			dir;
	struct dirent *		file;
	GString *		path;

	if ((dir = opendir(debr.config.menu_dir->str)) == NULL) {
		debr_message(LOG_ERROR, _("Could not open menus' directory"));
		return;
	}

	while ((file = readdir(dir)) != NULL){
		if (fnmatch ("*.mnu", file->d_name, 1))
			continue;

		path = g_string_new(debr.config.menu_dir->str);
		g_string_append(path, "/");
		g_string_append(path, file->d_name);
		menu_open(path->str, FALSE);

		g_string_free(path, TRUE);
	}
	closedir(dir);

	/* select first menu */
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(debr.ui_menu.list_store), &iter) == TRUE)
		menu_select_iter(&iter);
}

/*
 * Function: menu_open
 * Load menu at _path_ and select it according to _select_
 */
void
menu_open(const gchar * path, gboolean select)
{
	GtkTreeIter		iter;
	gboolean		valid;

	gchar *			filename;
	GeoXmlFlow *		menu;

	/* check if it is already open */
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(debr.ui_menu.list_store), &iter);
	while (valid) {
		gchar *	ipath;

		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.list_store), &iter,
			MENU_PATH, &ipath,
			-1);

		if (!strcmp(ipath, path)) {
			menu_select_iter(&iter);

			g_free(ipath);
			return;
		}
		g_free(ipath);
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(debr.ui_menu.list_store), &iter);
	}

	menu = menu_load(path);
	if (menu == NULL)
		return;

	/* add to the view */
	filename = g_path_get_basename(path);
	gtk_list_store_append(debr.ui_menu.list_store, &iter);
	gtk_list_store_set(debr.ui_menu.list_store, &iter,
		MENU_FILENAME, filename,
		MENU_PATH, path,
		MENU_XMLPOINTER, menu,
		-1);

	/* select it and load its contents into UI */
	if (select == TRUE) {
		menu_select_iter(&iter);
		menu_load_selected();
	}

	g_free(filename);
}

/*
 * Function: menu_save
 * Save current menu with a given _path_
 */
void
menu_save(const gchar * path)
{
	GeoXmlSequence *	program;
	gulong			index;
	gchar *			filename;

	/* Write menu tag for each program
	 * TODO: make it on the fly?
	 */
	index = 0;
	filename = (gchar*)geoxml_document_get_filename(GEOXML_DOC(debr.menu));
	geoxml_flow_get_program(debr.menu, &program, index);
	while (program != NULL) {
		geoxml_program_set_menu(GEOXML_PROGRAM(program), filename, index++);
		geoxml_sequence_next(&program);
	}

	geoxml_document_save(GEOXML_DOC(debr.menu), path);
	menu_saved_status_set(MENU_STATUS_SAVED);
}

void
menu_save_all(void)
{
	GtkTreeIter	iter;
	gboolean	has_next;

	if (!debr.unsaved_count)
		return;

	has_next = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(debr.ui_menu.list_store), &iter);
	while (has_next) {
		GdkPixbuf *	pixbuf;

		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.list_store), &iter,
			MENU_STATUS, &pixbuf,
			-1);
		if (pixbuf == debr.pixmaps.stock_no) {
			GeoXmlFlow *	menu;
			gchar *		path;

			gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.list_store), &iter,
				MENU_XMLPOINTER, &menu,
				MENU_PATH, &path,
				-1);
			geoxml_document_save(GEOXML_DOC(menu), path);
			menu_saved_status_set_from_iter(&iter, MENU_STATUS_SAVED);

			g_free(path);
		}

		has_next = gtk_tree_model_iter_next(GTK_TREE_MODEL(debr.ui_menu.list_store), &iter);
	}
}

/*
 * Function: menu_selected
 * Propagate the UI for a selected menu on the view
 */
void
menu_selected(void)
{
	GtkTreeIter		iter;

	GdkPixbuf *		icon;

	menu_get_selected(&iter);
	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.list_store), &iter,
		MENU_STATUS, &icon,
		MENU_XMLPOINTER, &debr.menu,
		-1);

	/* fire it! */
	program_load_menu();

	/* recover status set to unsaved by other functions */
	menu_saved_status_set(
		(icon == debr.pixmaps.stock_no) ? MENU_STATUS_UNSAVED : MENU_STATUS_SAVED);

	/* update details view */
	menu_details_update();
	do_navigation_bar_update();
}

/*
 * Function: menu_clenup
 * Asks for save unsaved menus. If yes, free memory allocated for menus
 */
gboolean
menu_cleanup(void)
{
	GtkWidget *	dialog;
	gboolean	ret;

	if (!debr.unsaved_count)
		return TRUE;

	dialog = gtk_message_dialog_new(GTK_WINDOW(debr.window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_QUESTION,
		GTK_BUTTONS_YES_NO,
		_("There are flows unsaved. Do you want to save them?"));
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	switch (gtk_dialog_run(GTK_DIALOG(dialog))) {
	case GTK_RESPONSE_YES: {
		menu_save_all();
		ret = TRUE;
		break;
	}
	case GTK_RESPONSE_NO:
		ret = TRUE;
		break;
	case GTK_RESPONSE_CANCEL:
		ret = FALSE;
		break;
	default:
		ret = FALSE;
	}

	gtk_widget_destroy(dialog);
	return ret;
}

/*
 * Function: menu_saved_status_set
 * Change the status of the menu (saved or unsaved)
 */
void
menu_saved_status_set(MenuStatus status)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (debr.ui_menu.tree_view));
	gtk_tree_selection_get_selected(selection, &model, &iter);

	menu_saved_status_set_from_iter(&iter, status);
}

/*
 * Function: menu_saved_status_set_unsaved
 * Connected to signal of components which change the menu
 */
void
menu_saved_status_set_unsaved(void)
{
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}


/*
 * Function: menu_dialog_setup_ui
 * Create a dialog to edit information about menu, like title, description, categories, etc
 */
void
menu_dialog_setup_ui(void)
{
	GtkWidget *		dialog;
	GtkWidget *		table;

	GtkWidget *		title_label;
	GtkWidget *		title_entry;
	GtkWidget *		description_label;
	GtkWidget *		description_entry;
	GtkWidget *		menuhelp_label;
	GtkWidget *		menuhelp_hbox;
	GtkWidget *		menuhelp_view_button;
	GtkWidget *		menuhelp_edit_button;
	GtkWidget *		author_label;
	GtkWidget *		author_entry;
	GtkWidget *		email_label;
	GtkWidget *		email_entry;
	GtkWidget *		categories_label;
	GtkWidget *		categories_combo;
	GtkWidget *		categories_sequence_edit;

	GtkWidget *		widget;

	dialog = gtk_dialog_new_with_buttons(_("Edit menu"),
		GTK_WINDOW(debr.window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
		NULL);
	gtk_widget_set_size_request(dialog, 400, 350);

	table = gtk_table_new(6, 2, FALSE);
	gtk_widget_show(table);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, TRUE, TRUE, 5);
	gtk_table_set_row_spacings(GTK_TABLE(table), 5);
	gtk_table_set_col_spacings(GTK_TABLE(table), 5);

	/*
	 * Title
	 */
	title_label = gtk_label_new(_("Title:"));
	gtk_widget_show(title_label);
	gtk_table_attach(GTK_TABLE(table), title_label, 0, 1, 0, 1,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(title_label), 0, 0.5);

	title_entry = gtk_entry_new();
	gtk_widget_show(title_entry);
	gtk_table_attach(GTK_TABLE(table), title_entry, 1, 2, 0, 1,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	g_signal_connect(title_entry, "changed",
		(GCallback)menu_title_changed, NULL);

	/*
	 * Description
	 */
	description_label = gtk_label_new(_("Description:"));
	gtk_widget_show(description_label);
	gtk_table_attach(GTK_TABLE(table), description_label, 0, 1, 1, 2,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(description_label), 0, 0.5);

	description_entry = gtk_entry_new();
	gtk_widget_show(description_entry);
	gtk_table_attach(GTK_TABLE(table), description_entry, 1, 2, 1, 2,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	g_signal_connect(description_entry, "changed",
		(GCallback)menu_description_changed, NULL);

	/*
	 * Help
	 */
	menuhelp_label = gtk_label_new(_("Help"));
	gtk_widget_show(menuhelp_label);
	gtk_table_attach(GTK_TABLE(table), menuhelp_label, 0, 1, 2, 3,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(menuhelp_label), 0, 0.5);

	menuhelp_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(menuhelp_hbox);
	gtk_table_attach(GTK_TABLE(table), menuhelp_hbox, 1, 2, 2, 3,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	menuhelp_view_button = gtk_button_new_from_stock(GTK_STOCK_OPEN);
	gtk_widget_show(menuhelp_view_button);
	gtk_box_pack_start(GTK_BOX(menuhelp_hbox), menuhelp_view_button, FALSE, FALSE, 0);
	g_signal_connect(menuhelp_view_button, "clicked",
		GTK_SIGNAL_FUNC(menu_help_view), NULL);
	g_object_set(G_OBJECT(menuhelp_view_button), "relief", GTK_RELIEF_NONE, NULL);

	menuhelp_edit_button = gtk_button_new_from_stock(GTK_STOCK_EDIT);
	gtk_widget_show(menuhelp_edit_button);
	gtk_box_pack_start(GTK_BOX(menuhelp_hbox), menuhelp_edit_button, FALSE, FALSE, 5);
	g_signal_connect(menuhelp_edit_button, "clicked",
		GTK_SIGNAL_FUNC(menu_help_edit), NULL);
	g_object_set(G_OBJECT(menuhelp_edit_button), "relief", GTK_RELIEF_NONE, NULL);

	/*
	 * Author
	 */
	author_label = gtk_label_new(_("Author:"));
	gtk_widget_show(author_label);
	gtk_table_attach(GTK_TABLE(table), author_label, 0, 1, 3, 4,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(author_label), 0, 0.5);

	author_entry = gtk_entry_new();
	gtk_widget_show(author_entry);
	gtk_table_attach(GTK_TABLE(table), author_entry, 1, 2, 3, 4,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	g_signal_connect(author_entry, "changed",
		(GCallback)menu_author_changed, NULL);

	/*
	 * Email
	 */
	email_label = gtk_label_new(_("Email:"));
	gtk_widget_show(email_label);
	gtk_table_attach(GTK_TABLE(table), email_label, 0, 1, 4, 5,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(email_label), 0, 0.5);

	email_entry = gtk_entry_new();
	gtk_widget_show(email_entry);
	gtk_table_attach(GTK_TABLE(table), email_entry, 1, 2, 4, 5,
		(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	g_signal_connect(email_entry, "changed",
		(GCallback)menu_email_changed, NULL);

	/*
	 * Categories
	 */
	categories_label = gtk_label_new(_("Categories:"));
	gtk_widget_show(categories_label);
	widget = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(widget), categories_label, FALSE, FALSE, 0);
	gtk_table_attach(GTK_TABLE(table), widget, 0, 1, 5, 6,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(GTK_FILL), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(categories_label), 0, 0.5);

	categories_combo = gtk_combo_box_entry_new_text();
	gtk_widget_show(categories_combo);
	gtk_combo_box_append_text(GTK_COMBO_BOX(categories_combo), "Data Compression");
	gtk_combo_box_append_text(GTK_COMBO_BOX(categories_combo), "Editing, Sorting and Manipulation");
	gtk_combo_box_append_text(GTK_COMBO_BOX(categories_combo), "Filtering, Transforms and Attributes");
	gtk_combo_box_append_text(GTK_COMBO_BOX(categories_combo), "Gain, NMO, Stack and Standard Processes");
	gtk_combo_box_append_text(GTK_COMBO_BOX(categories_combo), "Graphics");
	gtk_combo_box_append_text(GTK_COMBO_BOX(categories_combo), "Import/Export");
	gtk_combo_box_append_text(GTK_COMBO_BOX(categories_combo), "Inversion");
	gtk_combo_box_append_text(GTK_COMBO_BOX(categories_combo), "Migration and Dip Moveout");
	gtk_combo_box_append_text(GTK_COMBO_BOX(categories_combo), "Multiple Supression");
	gtk_combo_box_append_text(GTK_COMBO_BOX(categories_combo), "Seismic Unix");
	gtk_combo_box_append_text(GTK_COMBO_BOX(categories_combo), "Simulation and Model Building");
	gtk_combo_box_append_text(GTK_COMBO_BOX(categories_combo), "Utilities");
	/* TODO: GtkComboBoxEntry doesn't have activate signal */
// 	g_signal_connect(GTK_OBJECT(categories_combo), "activate",
// 		GTK_SIGNAL_FUNC(category_add), NULL);

	categories_sequence_edit = value_sequence_edit_new(categories_combo);
	gtk_widget_show(categories_sequence_edit);
	g_signal_connect(GTK_OBJECT(categories_sequence_edit), "add-request",
		GTK_SIGNAL_FUNC(menu_category_add), categories_combo);
	g_signal_connect(GTK_OBJECT(categories_sequence_edit), "changed",
		GTK_SIGNAL_FUNC(menu_category_changed), NULL);
	gtk_table_attach(GTK_TABLE(table), categories_sequence_edit, 1, 2, 5, 6,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(GTK_FILL), 0, 0);
	gtk_widget_show(categories_sequence_edit);

	/*
	 * Load menu into widgets
	 */
	gtk_entry_set_text(GTK_ENTRY(title_entry),
		geoxml_document_get_title(GEOXML_DOC(debr.menu)));
	gtk_entry_set_text(GTK_ENTRY(description_entry),
		geoxml_document_get_description(GEOXML_DOC(debr.menu)));
	gtk_entry_set_text(GTK_ENTRY(author_entry),
		geoxml_document_get_author(GEOXML_DOC(debr.menu)));
	gtk_entry_set_text(GTK_ENTRY(email_entry),
		geoxml_document_get_email(GEOXML_DOC(debr.menu)));
	/* categories */
	GeoXmlSequence * category;
	geoxml_flow_get_category(debr.menu, &category, 0);
	g_object_set(G_OBJECT(categories_sequence_edit), "value-sequence", category, NULL);
	value_sequence_edit_load(VALUE_SEQUENCE_EDIT(categories_sequence_edit));

	gtk_widget_show(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	menu_load_selected();
}

/*
 * Function: menu_get_selected
 * Return true if there is a menu selected and write it to _iter_
 */
gboolean
menu_get_selected(GtkTreeIter * iter)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(debr.ui_menu.tree_view));
	return gtk_tree_selection_get_selected(selection, &model, iter);
}

/*
 * Function: menu_load_selected
 * Reload selected menu contents to the interface
 */
void
menu_load_selected(void)
{
	GtkTreeIter	iter;

	menu_get_selected(&iter);
	menu_details_update();
}

/*
 * Function: menu_select_iter
 * Select _iter_ for menu's tree view
 */
void
menu_select_iter(GtkTreeIter * iter)
{
	GtkTreeSelection *	selection;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(debr.ui_menu.tree_view));
	gtk_tree_selection_select_iter(selection, iter);
	menu_selected();
}

/*
 * Section: Private
 */

/*
 * Function: menu_saved_status_set_from_iter
 * Change the status of the menu (saved or unsaved)
 */
static void
menu_saved_status_set_from_iter(GtkTreeIter * iter, MenuStatus status)
{
	GdkPixbuf *		pixbuf;
	gboolean		enable;

	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.list_store), iter,
		MENU_STATUS, &pixbuf,
		-1);
	switch (status) {
	case MENU_STATUS_SAVED:
		gtk_list_store_set(GTK_LIST_STORE(debr.ui_menu.list_store), iter,
			MENU_STATUS, NULL,
			-1);
		enable = FALSE;
		if (pixbuf == debr.pixmaps.stock_no)
			--debr.unsaved_count;
		break;
	case MENU_STATUS_UNSAVED:
		gtk_list_store_set(GTK_LIST_STORE(debr.ui_menu.list_store), iter,
			MENU_STATUS, debr.pixmaps.stock_no,
			-1);
		enable = TRUE;
		if (pixbuf == NULL)
			++debr.unsaved_count;
		break;
	default:
		enable = FALSE;
	}

	gtk_action_set_sensitive(gtk_action_group_get_action(debr.action_group, "menu_save"), enable);
	gtk_action_set_sensitive(gtk_action_group_get_action(debr.action_group, "menu_revert"), enable);
}

/*
 * Function: menu_details_update
 * Load details of selected menu to the details view
 */
static void
menu_details_update(void)
{
	gchar *		markup;
	GString *       text;
        glong           icmax;

	markup = g_markup_printf_escaped("<b>%s</b>", geoxml_document_get_title(GEOXML_DOC(debr.menu)));
	gtk_label_set_markup(GTK_LABEL(debr.ui_menu.details.title_label), markup);
	g_free(markup);

	markup = g_markup_printf_escaped("<i>%s</i>", geoxml_document_get_description(GEOXML_DOC(debr.menu)));
	gtk_label_set_markup(GTK_LABEL(debr.ui_menu.details.description_label), markup);
	g_free(markup);

        text = g_string_new(NULL);
        switch (geoxml_flow_get_programs_number(GEOXML_FLOW(debr.menu))){
                case 0:
                        g_string_printf(text, _("This menu has no programs"));
                        break;
                case 1:
                        g_string_printf(text, _("This menu has 1 program"));
                        break;
                default:
                        g_string_printf(text, _("This menu has %li programs"),
                                        geoxml_flow_get_programs_number(GEOXML_FLOW(debr.menu)));
                }
        gtk_label_set_text(GTK_LABEL(debr.ui_menu.details.nprogs_label), text->str);
	g_string_free(text, TRUE);

        markup = g_markup_printf_escaped("<b>%s</b>", _("Categories: "));
        gtk_label_set_markup(GTK_LABEL(debr.ui_menu.details.category_label), markup);
	g_free(markup);

        gtk_label_set_text(GTK_LABEL(debr.ui_menu.details.categories_label[0]),"");
        gtk_label_set_text(GTK_LABEL(debr.ui_menu.details.categories_label[1]),"");
        gtk_label_set_text(GTK_LABEL(debr.ui_menu.details.categories_label[2]),"");


        icmax = MIN(geoxml_flow_get_categories_number(GEOXML_FLOW(debr.menu)), 2);
        for (long int ic=0; ic<icmax; ic++){
		GeoXmlSequence *  	category;

                geoxml_flow_get_category(GEOXML_FLOW(debr.menu), &category, ic);

                text = g_string_new(NULL);
                g_string_printf(text, "%s", geoxml_value_sequence_get(GEOXML_VALUE_SEQUENCE(category)));
                gtk_label_set_text(GTK_LABEL(debr.ui_menu.details.categories_label[ic]),text->str);
                g_string_free(text, TRUE);                
        }
        if (icmax == 0){
                markup = g_markup_printf_escaped("<i>%s</i>", _("None"));
                gtk_label_set_markup(GTK_LABEL(debr.ui_menu.details.categories_label[0]), markup);
                g_free(markup);
        }
        
        if (geoxml_flow_get_categories_number(GEOXML_FLOW(debr.menu)) > 2)
                gtk_label_set_text(GTK_LABEL(debr.ui_menu.details.categories_label[2]),"...");

	text = g_string_new(NULL);
	g_string_printf(text, "%s <%s>",
		geoxml_document_get_author(GEOXML_DOC(debr.menu)),
		geoxml_document_get_email(GEOXML_DOC(debr.menu)));
	gtk_label_set_text(GTK_LABEL(debr.ui_menu.details.author_label), text->str);
	g_string_free(text, TRUE);
}

/*
 * Function: menu_title_changed
 * Keep XML in sync with widget
 */
static void
menu_title_changed(GtkEntry * entry)
{
	geoxml_document_set_title(GEOXML_DOC(debr.menu), gtk_entry_get_text(entry));
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/*
 * Function: menu_description_changed
 * Keep XML in sync with widget
 */
static void
menu_description_changed(GtkEntry * entry)
{
	geoxml_document_set_description(GEOXML_DOC(debr.menu), gtk_entry_get_text(entry));
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/*
 * Function: menu_help_view
 * Call <help_show> with menu's help
 */
static void
menu_help_view(void)
{
	help_show(geoxml_document_get_help(GEOXML_DOC(debr.menu)));
}

/*
 * Function: menu_help_edit
 * Call <help_edit> with menu's help. After help was edited in
 * a external browser, save it back to XML
 */
static void
menu_help_edit(void)
{
	GString *	help;

	help = help_edit(geoxml_document_get_help(GEOXML_DOC(debr.menu)), NULL);
	if (help == NULL)
		return;

	geoxml_document_set_help(GEOXML_DOC(debr.menu), help->str);
	menu_saved_status_set(MENU_STATUS_UNSAVED);

	g_string_free(help, TRUE);
}

/*
 * Function: menu_author_changed
 * Keep XML in sync with widget
 */
static void
menu_author_changed(GtkEntry * entry)
{
	geoxml_document_set_author(GEOXML_DOC(debr.menu), gtk_entry_get_text(entry));
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/*
 * Function: menu_email_changed
 * Keep XML in sync with widget
 */
static void
menu_email_changed(GtkEntry * entry)
{
	geoxml_document_set_email(GEOXML_DOC(debr.menu), gtk_entry_get_text(entry));
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/*
 * Function: menu_category_add
 * Add a category
 */
static void
menu_category_add(ValueSequenceEdit * sequence_edit, GtkComboBox * combo_box)
{
	gchar *	name;

	name = gtk_combo_box_get_active_text(combo_box);
	if (!strlen(name))
		name = _("New category");
	value_sequence_edit_add(VALUE_SEQUENCE_EDIT(sequence_edit),
		GEOXML_VALUE_SEQUENCE(geoxml_flow_append_category(debr.menu, name)));

	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/*
 * Function: menu_category_changed
 * Just wrap signal to notify an unsaved status
 */
static void
menu_category_changed(void)
{
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}
