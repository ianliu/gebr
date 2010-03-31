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

//for WEXITSTATUS
#define _XOPEN_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <libgebr/intl.h>
#include <libgebr/date.h>
#include <libgebr/utils.h>
#include <libgebr/gui/utils.h>
#include <libgebr/gui/valuesequenceedit.h>
#include <libgebr/gui/gebr-gui-save-dialog.h>

#include "menu.h"
#include "debr.h"
#include "callbacks.h"
#include "help.h"
#include "program.h"
#include "interface.h"

/*
 * Prototypes
 */

static GtkMenu *menu_popup_menu(GtkTreeView * tree_view);

static void menu_title_changed(GtkEntry * entry);

static void menu_description_changed(GtkEntry * entry);

static void menu_help_view(void);

static void menu_help_edit(void);

static void menu_author_changed(GtkEntry * entry);

static void menu_email_changed(GtkEntry * entry);

static void menu_category_add(GebrGuiValueSequenceEdit * sequence_edit, GtkComboBox * combo_box);

static void menu_category_changed(void);

static void menu_category_renamed(GebrGuiValueSequenceEdit * sequence_edit,
				  const gchar * old_text, const gchar * new_text);

static void menu_category_removed(GebrGuiValueSequenceEdit * sequence_edit, const gchar * old_text);

static gboolean menu_on_query_tooltip(GtkTreeView * tree_view, GtkTooltip * tooltip,
				      GtkTreeIter * iter, GtkTreeViewColumn * column, gpointer user_data);

static gboolean menu_is_path_loaded(const gchar * path, GtkTreeIter * iter);

static gboolean menu_get_folder_iter_from_path(const gchar * path, GtkTreeIter * iter_);

/*
 * Public functions
 */

void menu_setup_ui(void)
{
	GtkWidget *hpanel;
	GtkWidget *scrolled_window;
	GtkWidget *frame;
	GtkWidget *details;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	GtkWidget *table;

	hpanel = gtk_hpaned_new();
	gtk_widget_show(hpanel);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_paned_pack1(GTK_PANED(hpanel), scrolled_window, FALSE, FALSE);
	gtk_widget_set_size_request(scrolled_window, 200, -1);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_IN);

	debr.ui_menu.model = gtk_tree_store_new(MENU_N_COLUMN,
						G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING,
					       	G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_STRING,
						G_TYPE_BOOLEAN, G_TYPE_POINTER);
	debr.ui_menu.tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(debr.ui_menu.model));
	gtk_widget_show(debr.ui_menu.tree_view);
	gtk_container_add(GTK_CONTAINER(scrolled_window), debr.ui_menu.tree_view);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(debr.ui_menu.tree_view)),
				    GTK_SELECTION_MULTIPLE);
	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(debr.ui_menu.tree_view),
						  (GebrGuiGtkPopupCallback) menu_popup_menu, NULL);
	gebr_gui_gtk_tree_view_change_cursor_on_row_collapsed(GTK_TREE_VIEW(debr.ui_menu.tree_view));
	gebr_gui_gtk_tree_view_set_tooltip_callback(GTK_TREE_VIEW(debr.ui_menu.tree_view), menu_on_query_tooltip, NULL);
	gebr_gui_gtk_tree_view_fancy_search(GTK_TREE_VIEW(debr.ui_menu.tree_view), MENU_FILENAME);
	//g_signal_connect(debr.ui_menu.tree_view, "cursor-changed", G_CALLBACK(menu_selected), NULL);
	g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(debr.ui_menu.tree_view)), "changed",
			 G_CALLBACK(menu_selected), NULL);
	g_signal_connect(debr.ui_menu.tree_view, "row-activated", G_CALLBACK(menu_dialog_setup_ui), NULL);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(debr.ui_menu.tree_view), FALSE);
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, _("Menu"));

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(col, renderer, FALSE);
	gtk_tree_view_column_add_attribute(col, renderer, "stock-id", MENU_IMAGE);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, FALSE);
	gtk_tree_view_column_add_attribute(col, renderer, "markup", MENU_FILENAME);

	gtk_tree_view_append_column(GTK_TREE_VIEW(debr.ui_menu.tree_view), col);

	/*
	 * Add special rows
	 */
	gtk_tree_store_append(debr.ui_menu.model, &debr.ui_menu.iter_other, NULL);
	gtk_tree_store_set(debr.ui_menu.model, &debr.ui_menu.iter_other,
			   MENU_IMAGE, GTK_STOCK_DIRECTORY, MENU_FILENAME, _("<i>Others</i>"), -1);

	/*
	 * Info Panel
	 */
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_paned_pack2(GTK_PANED(hpanel), scrolled_window, TRUE, FALSE);

	frame = gtk_frame_new(_("Details"));
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), frame);
	debr.ui_menu.details.vbox = details = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), details);

	debr.ui_menu.details.title_label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_menu.details.title_label), 0, 0);
	gtk_box_pack_start(GTK_BOX(details), debr.ui_menu.details.title_label, FALSE, TRUE, 0);

	debr.ui_menu.details.description_label = gtk_label_new(NULL);
	g_object_set(G_OBJECT(debr.ui_menu.details.description_label), "wrap", TRUE, NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_menu.details.description_label), 0, 0);
	gtk_box_pack_start(GTK_BOX(details), debr.ui_menu.details.description_label, FALSE, TRUE, 0);

	debr.ui_menu.details.nprogs_label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_menu.details.nprogs_label), 0, 0);
	gtk_box_pack_start(GTK_BOX(details), debr.ui_menu.details.nprogs_label, FALSE, TRUE, 10);

	table = gtk_table_new(6, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(details), table, FALSE, TRUE, 0);

	debr.ui_menu.details.created_label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_menu.details.created_label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), debr.ui_menu.details.created_label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 3, 3);

	debr.ui_menu.details.created_date_label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_menu.details.created_date_label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), debr.ui_menu.details.created_date_label, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 3,
			 3);

	debr.ui_menu.details.modified_label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_menu.details.modified_label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), debr.ui_menu.details.modified_label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 3, 3);

	debr.ui_menu.details.modified_date_label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_menu.details.modified_date_label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), debr.ui_menu.details.modified_date_label, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 3,
			 3);

	debr.ui_menu.details.category_label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_menu.details.category_label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), debr.ui_menu.details.category_label, 0, 1, 3, 4, GTK_FILL, GTK_FILL, 3, 3);

	debr.ui_menu.details.categories_label[0] = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_menu.details.categories_label[0]), 0, 0);
	gtk_table_attach(GTK_TABLE(table), debr.ui_menu.details.categories_label[0], 1, 2, 3, 4, GTK_FILL, GTK_FILL, 3,
			 3);

	debr.ui_menu.details.categories_label[1] = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_menu.details.categories_label[1]), 0, 0);
	gtk_table_attach(GTK_TABLE(table), debr.ui_menu.details.categories_label[1], 1, 2, 4, 5, GTK_FILL, GTK_FILL, 3,
			 3);

	debr.ui_menu.details.categories_label[2] = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_menu.details.categories_label[2]), 0, 0);
	gtk_table_attach(GTK_TABLE(table), debr.ui_menu.details.categories_label[2], 1, 2, 5, 6, GTK_FILL, GTK_FILL, 3,
			 3);

	debr.ui_menu.details.help_button = gtk_button_new_from_stock(GTK_STOCK_INFO);
	gtk_box_pack_end(GTK_BOX(details), debr.ui_menu.details.help_button, FALSE, TRUE, 0);
	g_signal_connect(GTK_OBJECT(debr.ui_menu.details.help_button), "clicked",
			 G_CALLBACK(menu_help_view), debr.menu);

	debr.ui_menu.details.author_label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_menu.details.author_label), 0, 0);
	gtk_box_pack_end(GTK_BOX(details), debr.ui_menu.details.author_label, FALSE, TRUE, 0);

	gtk_action_set_sensitive(gtk_action_group_get_action(debr.action_group, "menu_save_all"), FALSE);
	debr.ui_menu.widget = hpanel;
	gtk_widget_show_all(debr.ui_menu.widget);
}

void menu_new(gboolean edit)
{
	static int new_count = 0;
	GString *new_menu_str;
	GtkTreeIter iter;
	GtkTreeIter target;

	new_menu_str = g_string_new(NULL);
	g_string_printf(new_menu_str, "%s%d.mnu", _("untitled"), ++new_count);

	debr.menu = gebr_geoxml_flow_new();
	gebr_geoxml_document_set_author(GEBR_GEOXML_DOC(debr.menu), debr.config.name->str);
	gebr_geoxml_document_set_email(GEBR_GEOXML_DOC(debr.menu), debr.config.email->str);
	gebr_geoxml_document_set_date_created(GEBR_GEOXML_DOC(debr.menu), gebr_iso_date());

	switch (menu_get_selected_type(&iter, FALSE)) {
	case ITER_FILE:
		gtk_tree_model_iter_parent(GTK_TREE_MODEL(debr.ui_menu.model), &target, &iter);
		break;
	case ITER_FOLDER:
		target = iter;
		break;
	default:
		target = debr.ui_menu.iter_other;
		break;
	}

	gtk_tree_store_append(debr.ui_menu.model, &iter, &target);
	gtk_tree_store_set(debr.ui_menu.model, &iter,
			   MENU_STATUS, MENU_STATUS_UNSAVED, MENU_IMAGE, GTK_STOCK_NO,
			   MENU_FILENAME, new_menu_str->str, MENU_XMLPOINTER, (gpointer) debr.menu,
			   MENU_PATH, "", MENU_VALIDATE_POINTER, NULL, -1),
	menu_select_iter(&iter);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
	if (edit) {
		if (!menu_dialog_setup_ui())
			menu_close(&iter);
	}

	g_string_free(new_menu_str, TRUE);
}

GebrGeoXmlFlow *menu_load(const gchar * path)
{
	GebrGeoXmlDocument *menu;
	int ret;
	GebrGeoXmlSequence *category;

	if ((ret = gebr_geoxml_document_load(&menu, path, TRUE, NULL))) {
		debr_message(GEBR_LOG_ERROR, _("Could not load menu at '%s': %s."), path,
			     gebr_geoxml_error_string((enum GEBR_GEOXML_RETV)ret));
		return NULL;
	}

	gebr_geoxml_flow_get_category(GEBR_GEOXML_FLOW(menu), &category, 0);
	for (; category != NULL; gebr_geoxml_sequence_next(&category))
		debr_has_category(gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(category)), TRUE);

	return GEBR_GEOXML_FLOW(menu);
}

void menu_load_user_directory(void)
{
	GtkTreeIter iter;
	gchar *filename;

	gint i = 0;

	if (!debr.config.menu_dir)
		return;

	while (debr.config.menu_dir[i]) {
		GString *path;
		gchar *dirname;
		gchar *dname;

		dname = g_path_get_basename(debr.config.menu_dir[i]);
		dirname = g_markup_printf_escaped("%s", dname);

		gtk_tree_store_append(debr.ui_menu.model, &iter, NULL);
		gtk_tree_store_set(debr.ui_menu.model, &iter,
				   MENU_IMAGE, GTK_STOCK_DIRECTORY, MENU_FILENAME, dirname,
				   MENU_PATH, debr.config.menu_dir[i], -1);

		path = g_string_new(NULL);
		gebr_directory_foreach_file(filename, debr.config.menu_dir[i]) {
			if (fnmatch("*.mnu", filename, 1))
				continue;

			g_string_printf(path, "%s/%s", debr.config.menu_dir[i], filename);
			menu_open_with_parent(path->str, &iter, FALSE);
		}

		g_string_free(path, TRUE);
		g_free(dirname);
		g_free(dname);
		i++;
	}

	/* select first menu */
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(debr.ui_menu.model), &iter) == TRUE)
		menu_select_iter(&iter);
}

void menu_load_iter(const gchar * path, GtkTreeIter * iter, GebrGeoXmlFlow * menu, gboolean select)
{
	const gchar *date;
	gchar *tmp;
	gchar *label;
	gchar *filename;
	GtkTreeIter parent;

	filename = g_path_get_basename(path);
	date = gebr_geoxml_document_get_date_modified(GEBR_GEOXML_DOCUMENT(menu));
	tmp = (strlen(date))
	    ? g_strdup_printf("%ld", gebr_iso_date_to_g_time_val(date).tv_sec)
	    : g_strdup_printf("%ld", gebr_iso_date_to_g_time_val("2007-01-01T00:00:00.000000Z").tv_sec);

	gtk_tree_model_iter_parent(GTK_TREE_MODEL(debr.ui_menu.model), &parent, iter);
	if (gebr_gui_gtk_tree_model_iter_equal_to(GTK_TREE_MODEL(debr.ui_menu.model),
						  &debr.ui_menu.iter_other, &parent))
	{
		gchar *dirname;

		dirname = g_path_get_dirname(path);
		label = g_markup_printf_escaped("%s <span color='#666666'><i>%s</i></span>", filename, dirname);
		g_free(dirname);
	} else
		label = g_markup_printf_escaped("%s", filename);

	gtk_tree_store_set(debr.ui_menu.model, iter,
			   MENU_FILENAME, label, MENU_MODIFIED_DATE, tmp,
			   MENU_XMLPOINTER, menu, MENU_PATH, path,
			   MENU_VALIDATE_POINTER, NULL,
			   -1);
	/* select it and load its contents into UI */
	if (select == TRUE) {
		menu_select_iter(iter);
		menu_selected();
	}

	g_free(filename);
	g_free(label);
	g_free(tmp);
}

void menu_open_with_parent(const gchar * path, GtkTreeIter * parent, gboolean select)
{
	GtkTreeIter child;
	gboolean valid;

	GebrGeoXmlFlow *menu;

	valid = gtk_tree_model_iter_children(GTK_TREE_MODEL(debr.ui_menu.model), &child, parent);
	while (valid) {
		gchar *ipath;

		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &child, MENU_PATH, &ipath, -1);

		if (!strcmp(ipath, path)) {
			menu_select_iter(&child);
			g_free(ipath);
			return;
		}
		g_free(ipath);

		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(debr.ui_menu.model), &child);
	}

	menu = menu_load(path);
	if (menu == NULL)
		return;

	gtk_tree_store_append(debr.ui_menu.model, &child, parent);
	menu_status_set_from_iter(&child, MENU_STATUS_SAVED);
	menu_load_iter(path, &child, menu, select);
}

void menu_open(const gchar * path, gboolean select)
{
	GtkTreeIter parent;

	menu_path_get_parent(path, &parent);
	menu_open_with_parent(path, &parent, select);
}

MenuMessage menu_save(GtkTreeIter * iter)
{
	GtkTreeIter selected_iter;
	GebrGeoXmlFlow *menu;
	gchar *path;
	gchar *filename;
	gchar *tmp;

	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), iter, MENU_XMLPOINTER, &menu, MENU_PATH, &path, -1);
	/* is this a new menu? */
	if (!strlen(path)) {
		g_free(path);
		return MENU_MESSAGE_FIRST_TIME_SAVE;
	}

	filename = g_path_get_basename(path);
	if (gebr_geoxml_document_save(GEBR_GEOXML_DOC(menu), path) != GEBR_GEOXML_RETV_SUCCESS) {
		gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
					_("Could not save the menu"),
					_("You do not have the permissions necessary to save "
					  "the menu. Please check that you typed the location "
					  "correctly and try again."));
		debr_message(GEBR_LOG_ERROR, _("Permission denied."));
		return MENU_MESSAGE_PERMISSION_DENIED;	
	} else {
		gebr_geoxml_document_set_filename(GEBR_GEOXML_DOC(menu), filename);
		gebr_geoxml_document_set_date_modified(GEBR_GEOXML_DOC(menu), gebr_iso_date());
		gebr_geoxml_document_save(GEBR_GEOXML_DOC(menu), path);
	}

	tmp = g_strdup_printf("%ld",
			      gebr_iso_date_to_g_time_val(gebr_geoxml_document_get_date_modified
							  (GEBR_GEOXML_DOCUMENT(menu))).tv_sec);
	gtk_tree_store_set(debr.ui_menu.model, iter, MENU_MODIFIED_DATE, tmp, -1);
	menu_get_selected(&selected_iter, FALSE);
	if (gebr_gui_gtk_tree_iter_equal_to(iter, &selected_iter))
		menu_details_update();

	menu_status_set_from_iter(iter, MENU_STATUS_SAVED);

	debr_message(GEBR_LOG_INFO, _("Menu \"%s\" saved."), path);

	g_free(path);
	g_free(filename);
	g_free(tmp);

	return MENU_MESSAGE_SUCCESS;
}

gboolean menu_save_all(void)
{
	gboolean ret = TRUE;

	if (!menu_count_unsaved())
		return FALSE;

	GList *unsaved = NULL;

	GtkTreeIter iter;
	GtkTreeIter child;
	gboolean valid;

	/* We save a reference to all unsaved rows, so we can save them later */
	gebr_gui_gtk_tree_model_foreach(iter, GTK_TREE_MODEL(debr.ui_menu.model)) {
		valid = gtk_tree_model_iter_children(GTK_TREE_MODEL(debr.ui_menu.model), &child, &iter);
		while (valid) {
			MenuStatus status;
			gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &child, MENU_STATUS, &status, -1);
			if (status == MENU_STATUS_UNSAVED) {
				GtkTreePath *path;
				path = gtk_tree_model_get_path(GTK_TREE_MODEL(debr.ui_menu.model), &child);
				unsaved = g_list_prepend(unsaved,
							 gtk_tree_row_reference_new(GTK_TREE_MODEL(debr.ui_menu.model), path));
				gtk_tree_path_free(path);
			}
			valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(debr.ui_menu.model), &child);
		}
	}

	GList *entry = unsaved;
	while (entry) {
		GtkTreePath *path;
		GtkTreeRowReference *row;
		row = (GtkTreeRowReference*)(entry->data);
		path = gtk_tree_row_reference_get_path(row);
		gtk_tree_model_get_iter(GTK_TREE_MODEL(debr.ui_menu.model), &iter, path);
		MenuMessage result = MENU_MESSAGE_SUCCESS;

		result = menu_save(&iter);
		if (result == MENU_MESSAGE_FIRST_TIME_SAVE) {
			if (!menu_save_as(&iter))
				ret = FALSE;
			goto out;
		} else if (result == MENU_MESSAGE_PERMISSION_DENIED) {
			gtk_tree_path_free(path);
			gtk_tree_row_reference_free(row);
			ret = FALSE;
			goto out;
		}

		gtk_tree_path_free(path);
		gtk_tree_row_reference_free(row);
		entry = entry->next;
	}

	menu_details_update();
	debr_message(GEBR_LOG_INFO, _("All menus were saved."));
out:
	g_list_free(unsaved);
	return ret;
}

gboolean menu_save_as(GtkTreeIter * iter)
{
	GtkTreeIter parent;
	GtkWidget *dialog;
	gboolean ret;

	/*
	 * Setup file chooser
	 */
	gchar *title;
	gchar *fname;
	GebrGeoXmlFlow *menu;

	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), iter,
			   MENU_XMLPOINTER, &menu,
			   MENU_FILENAME, &fname,
			   -1);
	title = g_strdup_printf(_("Choose file for \"%s\""),
				gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(menu)));
	dialog = gebr_gui_save_dialog_new(title, GTK_WINDOW(debr.window));
	gebr_gui_save_dialog_set_default_extension(GEBR_GUI_SAVE_DIALOG(dialog), ".mnu");
	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), fname);
	gtk_tree_model_iter_parent(GTK_TREE_MODEL(debr.ui_menu.model), &parent, iter);
	g_free(title);
	g_free(fname);

	if (gebr_gui_gtk_tree_model_iter_equal_to(GTK_TREE_MODEL(debr.ui_menu.model),
						  &parent, &debr.ui_menu.iter_other)) {
		if (debr.config.menu_dir[0])
			gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), debr.config.menu_dir[0]);
	} else {
		gchar *menu_path;
		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &parent, MENU_PATH, &menu_path, -1);
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), menu_path);
		g_free(menu_path);
	}

	for (gsize i = 0; debr.config.menu_dir[i]; i++)
		gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(dialog), debr.config.menu_dir[i], NULL);

	GtkFileFilter *filefilter;
	filefilter = gtk_file_filter_new();
	gtk_file_filter_set_name(filefilter, _("Menu files (*.mnu)"));
	gtk_file_filter_add_pattern(filefilter, "*.mnu");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filefilter);

	/*
	 * Get file path
	 */

	gchar *filepath;
	gchar *currentpath;

	filepath = gebr_gui_save_dialog_run(GEBR_GUI_SAVE_DIALOG(dialog));

	if (!filepath)
		return FALSE;

	menu_path_get_parent(filepath, &parent);
	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), iter,
			   MENU_PATH, &currentpath,
			   -1);

	/*
	 * Saving on top of the same file. Do a normal save and exit
	 */
	if (strcmp(currentpath, filepath) == 0) {
		ret = (menu_save(iter) != MENU_MESSAGE_PERMISSION_DENIED);
		goto out;
	}

	/*
	 * Now, check if we are overwriting a menu
	 */

	GebrGeoXmlDocument *remove;
	GebrGeoXmlFlow *clone;
	GtkTreeIter child;
	GtkTreeIter target;
	gboolean is_overwrite;
	gchar * target_fname;

	clone = GEBR_GEOXML_FLOW(gebr_geoxml_document_clone(GEBR_GEOXML_DOCUMENT(menu)));
	is_overwrite = menu_is_path_loaded(filepath, &child);

	if (is_overwrite) {
		/* If we found a menu, we must load the new Xml in the same iterator. */
		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &child, MENU_XMLPOINTER, &remove, -1);
		target = child;
	} else {
		/* Otherwise, we must create a new menu. */
		gtk_tree_store_append(debr.ui_menu.model, &target, &parent);
	}

	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &target, MENU_PATH, &target_fname, -1);
	menu_load_iter(filepath, &target, clone, FALSE);
	ret = (menu_save(&target) != MENU_MESSAGE_PERMISSION_DENIED);
	if (ret) {
		if (is_overwrite)
			gebr_geoxml_document_free(remove);

		if (!strlen(currentpath))
			gtk_tree_store_remove(debr.ui_menu.model, iter);

		*iter = target;
	} else {
		if (!is_overwrite)
			gtk_tree_store_remove(debr.ui_menu.model, &target);
		else
			menu_load_iter(target_fname, &target, GEBR_GEOXML_FLOW(remove), FALSE);
		gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(clone));
	}

out:
	g_free(filepath);
	g_free(currentpath);

	return ret;
}

void menu_validate(GtkTreeIter * iter)
{
	GebrGeoXmlFlow *menu;

	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), iter, MENU_XMLPOINTER, &menu, -1);
	validate_menu(iter, menu);
}

void menu_install(void)
{
	gboolean overwriteall;
	GtkTreeIter iter;

	overwriteall = FALSE;

	gebr_gui_gtk_tree_view_foreach_selected(&iter, debr.ui_menu.tree_view) {
		GtkWidget *dialog;

		gchar *menu_filename;
		gchar *menu_path;
		GString *destination;
		GString *command;
		MenuStatus status;
		gboolean do_save = FALSE;

		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &iter,
				   MENU_FILENAME, &menu_filename,
				   MENU_STATUS, &status,
				   MENU_PATH, &menu_path,
				   -1);

		destination = g_string_new(NULL);
		command = g_string_new(NULL);
		g_string_printf(destination, "%s/.gebr/menus/%s", getenv("HOME"), menu_filename);
		g_string_printf(command, "cp %s %s", menu_path, destination->str);

		if (status == MENU_STATUS_UNSAVED) {
			dialog = gtk_message_dialog_new(GTK_WINDOW(debr.window),
							GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
							GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
							_("Menu %s is unsaved. This means that "
							  "you are installing an older state of it. "
							  "Would you like to save if first?"), menu_filename);
			if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES)
				menu_save(&iter);
			gtk_widget_destroy(dialog);
		}

		if (!overwriteall && g_file_test(destination->str, G_FILE_TEST_EXISTS)) {
			gint response;
			dialog = gtk_message_dialog_new(GTK_WINDOW(debr.window),
							GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
							GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
							_("Menu '%s' already exists. Do you want to overwrite it?"),
							menu_filename);
			gtk_dialog_add_button(GTK_DIALOG(dialog), _("Don't overwrite"), GTK_RESPONSE_NO);
			gtk_dialog_add_button(GTK_DIALOG(dialog), _("Overwrite"), GTK_RESPONSE_YES);
			gtk_dialog_add_button(GTK_DIALOG(dialog), _("Overwrite all"), GTK_RESPONSE_OK);

			response = gtk_dialog_run(GTK_DIALOG(dialog));
			if (response == GTK_RESPONSE_YES || response == GTK_RESPONSE_OK) {
				do_save = TRUE;
				overwriteall = (response == GTK_RESPONSE_OK);
			}
			gtk_widget_destroy(dialog);
		} else
			do_save = TRUE;

		if (do_save && system(command->str) != 0)
			gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, NULL,
						_("Failed to install menu %s"),
						menu_filename);
		g_free(menu_filename);
		g_free(menu_path);
		g_string_free(destination, TRUE);
		g_string_free(command, TRUE);
	}
}

void menu_close(GtkTreeIter * iter)
{
	GebrGeoXmlFlow *menu;
	gchar *path;
	struct validate *validate;

	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), iter,
			   MENU_XMLPOINTER, &menu, MENU_PATH, &path,
			   MENU_VALIDATE_POINTER, &validate, -1);

	if (validate)
		validate_close_iter(&validate->iter);

	gebr_geoxml_document_free(GEBR_GEOXML_DOC(menu));
	if (gtk_tree_store_remove(debr.ui_menu.model, iter))
		menu_select_iter(iter);
	else
		menu_selected();

	debr_message(GEBR_LOG_INFO, _("Menu \"%s\" closed."), path);
	g_free(path);
}

void menu_selected(void)
{
	GtkTreeIter iter;
	IterType type;
	MenuStatus current_status;

	type = menu_get_selected_type(&iter, FALSE);
	if (type == ITER_FILE) {
		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &iter, MENU_XMLPOINTER, &debr.menu, MENU_STATUS, &current_status, -1);
	} else
		debr.menu = NULL;

	program_load_menu();
	do_navigation_bar_update();

	if (type == ITER_FILE) {
		menu_details_update();
			
		gchar *names[] = {
			"menu_properties",
			"menu_validate",
			"menu_install",
			"menu_close",
			"menu_save_as",
			"menu_delete",
			NULL
		};
		debr_set_actions_sensitive(names, TRUE);
			
		menu_status_set_from_iter(&iter, current_status);

	} else if (type == ITER_FOLDER) {
		menu_folder_details_update(&iter);

		gchar *names[] = {
			"menu_properties",
			"menu_validate",
			"menu_install",
			"menu_close",
			"menu_save",
			"menu_save_as",
			"menu_revert",
			"menu_delete",
			NULL
		};
		debr_set_actions_sensitive(names, FALSE);

	} else if (type == ITER_NONE) {
		menu_details_update();
		
		gchar *names[] = {
			"menu_properties",
			"menu_validate",
			"menu_install",
			"menu_close",
			"menu_save",
			"menu_save_as",
			"menu_revert",
			"menu_delete",
			NULL
		};
		debr_set_actions_sensitive(names, FALSE);
	}
}

gboolean menu_cleanup(void)
{
	GtkWidget *dialog;
	GtkWidget *button;
	gboolean ret;
	gboolean still_running = TRUE;

	if (!menu_count_unsaved())
		return TRUE;

	dialog = gtk_message_dialog_new(GTK_WINDOW(debr.window),
					GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION,
					GTK_BUTTONS_NONE, _("There are menus unsaved. Do you want to save them?"));
	button = gtk_dialog_add_button(GTK_DIALOG(dialog), _("Don't save"), GTK_RESPONSE_NO);
	g_object_set(G_OBJECT(button), "image", gtk_image_new_from_stock(GTK_STOCK_NO, GTK_ICON_SIZE_BUTTON), NULL);
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_SAVE, GTK_RESPONSE_YES);
	while(still_running) {
		switch (gtk_dialog_run(GTK_DIALOG(dialog))) {
		case GTK_RESPONSE_YES:
			if (!menu_save_all()) {
				ret = FALSE;
				still_running = TRUE;
			} else {
				ret = TRUE;
				still_running = FALSE;
			}
			break;
		case GTK_RESPONSE_NO:
			ret = TRUE;
			still_running = FALSE;
			break;
		case GTK_RESPONSE_CANCEL:
		default:
			still_running = FALSE;
			ret = FALSE;
		}
	}
	gtk_widget_destroy(dialog);
	return ret;
}

void menu_saved_status_set(MenuStatus status)
{
	GtkTreeIter iter;

	if (menu_get_selected(&iter, FALSE))
		menu_status_set_from_iter(&iter, status);
}

void menu_status_set_from_iter(GtkTreeIter * iter, MenuStatus status)
{
	gboolean unsaved;

	unsaved = status == MENU_STATUS_UNSAVED;
	gtk_tree_store_set(debr.ui_menu.model, iter,
			   MENU_STATUS, status,
			   MENU_IMAGE, unsaved? GTK_STOCK_NO : GTK_STOCK_FILE,
			   -1);
	if (unsaved)
		gtk_tree_store_set(debr.ui_menu.model, iter,
				   MENU_VALIDATE_NEED_UPDATE, TRUE,
				   -1);

	gtk_action_set_sensitive(gtk_action_group_get_action(debr.action_group, "menu_save"), unsaved);
	gtk_action_set_sensitive(gtk_action_group_get_action(debr.action_group, "menu_revert"), unsaved);

	if (menu_count_unsaved() > 0)
		gtk_action_set_sensitive(gtk_action_group_get_action(debr.action_group, "menu_save_all"), TRUE);
	else
		gtk_action_set_sensitive(gtk_action_group_get_action(debr.action_group, "menu_save_all"), FALSE);
}

void menu_status_set_unsaved(void)
{
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

gboolean menu_dialog_setup_ui(void)
{
	GtkWidget *dialog;
	GtkWidget *table;

	GtkWidget *title_label;
	GtkWidget *title_entry;
	GtkWidget *description_label;
	GtkWidget *description_entry;
	GtkWidget *menuhelp_label;
	GtkWidget *menuhelp_hbox;
	GtkWidget *menuhelp_edit_button;
	GtkWidget *author_label;
	GtkWidget *author_entry;
	GtkWidget *email_label;
	GtkWidget *email_entry;
	GtkWidget *categories_label;
	GtkWidget *categories_combo;
	GtkWidget *categories_sequence_edit;
	GtkWidget *widget;

	IterType type;
	GtkTreeIter iter;
	gboolean ret = TRUE;

	menu_archive();

	gebr_gui_gtk_tree_view_turn_to_single_selection(GTK_TREE_VIEW(debr.ui_menu.tree_view));
	type = menu_get_selected_type(&iter, FALSE);
	if (type == ITER_FOLDER) {
		GtkTreePath *path;
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(debr.ui_menu.model), &iter);
		if (gtk_tree_view_row_expanded(GTK_TREE_VIEW(debr.ui_menu.tree_view), path))
			gtk_tree_view_collapse_row(GTK_TREE_VIEW(debr.ui_menu.tree_view), path);
		else
			gtk_tree_view_expand_row(GTK_TREE_VIEW(debr.ui_menu.tree_view), path, FALSE);
		gtk_tree_path_free(path);
		return FALSE;
	}
	if (type != ITER_FILE)
		return FALSE;

	dialog = gtk_dialog_new_with_buttons(_("Edit menu"),
					     GTK_WINDOW(debr.window),
					     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_widget_set_size_request(dialog, 400, 350);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

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
			 (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(title_label), 0, 0.5);

	title_entry = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(title_entry), TRUE);
	gtk_widget_show(title_entry);
	gtk_table_attach(GTK_TABLE(table), title_entry, 1, 2, 0, 1,
			 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0);

	/*
	 * Description
	 */
	description_label = gtk_label_new(_("Description:"));
	gtk_widget_show(description_label);
	gtk_table_attach(GTK_TABLE(table), description_label, 0, 1, 1, 2,
			 (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(description_label), 0, 0.5);

	description_entry = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(description_entry), TRUE);
	gtk_widget_show(description_entry);
	gtk_table_attach(GTK_TABLE(table), description_entry, 1, 2, 1, 2,
			 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0);

	/*
	 * Help
	 */
	menuhelp_label = gtk_label_new(_("Help"));
	gtk_widget_show(menuhelp_label);
	gtk_table_attach(GTK_TABLE(table), menuhelp_label, 0, 1, 2, 3,
			 (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(menuhelp_label), 0, 0.5);

	menuhelp_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(menuhelp_hbox);
	gtk_table_attach(GTK_TABLE(table), menuhelp_hbox, 1, 2, 2, 3,
			 (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);

	menuhelp_edit_button = gtk_button_new_from_stock(GTK_STOCK_EDIT);
	gtk_widget_show(menuhelp_edit_button);
	gtk_box_pack_start(GTK_BOX(menuhelp_hbox), menuhelp_edit_button, FALSE, FALSE, 0);
	g_object_set(G_OBJECT(menuhelp_edit_button), "relief", GTK_RELIEF_NONE, NULL);

	/*
	 * Author
	 */
	author_label = gtk_label_new(_("Author:"));
	gtk_widget_show(author_label);
	gtk_table_attach(GTK_TABLE(table), author_label, 0, 1, 3, 4,
			 (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(author_label), 0, 0.5);

	author_entry = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(author_entry), TRUE);
	gtk_widget_show(author_entry);
	gtk_table_attach(GTK_TABLE(table), author_entry, 1, 2, 3, 4,
			 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0);

	/*
	 * Email
	 */
	email_label = gtk_label_new(_("Email:"));
	gtk_widget_show(email_label);
	gtk_table_attach(GTK_TABLE(table), email_label, 0, 1, 4, 5,
			 (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(email_label), 0, 0.5);

	email_entry = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY(email_entry), TRUE);
	gtk_widget_show(email_entry);
	gtk_table_attach(GTK_TABLE(table), email_entry, 1, 2, 4, 5,
			 (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), (GtkAttachOptions) (0), 0, 0);

	/*
	 * Categories
	 */
	categories_label = gtk_label_new(_("Categories:"));
	gtk_widget_show(categories_label);
	widget = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(widget), categories_label, FALSE, FALSE, 0);
	gtk_table_attach(GTK_TABLE(table), widget, 0, 1, 5, 6,
			 (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(categories_label), 0, 0.5);

	categories_combo = gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(debr.categories_model), CATEGORY_NAME);
	gtk_widget_show(categories_combo);

	categories_sequence_edit = gebr_gui_value_sequence_edit_new(categories_combo);
	gtk_widget_show(categories_sequence_edit);
	gtk_table_attach(GTK_TABLE(table), categories_sequence_edit, 1, 2, 5, 6,
			 (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_widget_show(categories_sequence_edit);

	/*
	 * Load menu into widgets
	 */
	gtk_entry_set_text(GTK_ENTRY(title_entry), gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(debr.menu)));
	gtk_entry_set_text(GTK_ENTRY(description_entry),
			   gebr_geoxml_document_get_description(GEBR_GEOXML_DOC(debr.menu)));
	gtk_entry_set_text(GTK_ENTRY(author_entry), gebr_geoxml_document_get_author(GEBR_GEOXML_DOC(debr.menu)));
	gtk_entry_set_text(GTK_ENTRY(email_entry), gebr_geoxml_document_get_email(GEBR_GEOXML_DOC(debr.menu)));

	g_signal_connect(title_entry, "changed", G_CALLBACK(menu_title_changed), NULL);
	g_signal_connect(description_entry, "changed", G_CALLBACK(menu_description_changed), NULL);
	g_signal_connect(GTK_OBJECT(categories_sequence_edit), "add-request",
			 G_CALLBACK(menu_category_add), categories_combo);
	g_signal_connect(GTK_OBJECT(categories_sequence_edit), "changed", G_CALLBACK(menu_category_changed), NULL);
	g_signal_connect(GTK_OBJECT(categories_sequence_edit), "renamed", G_CALLBACK(menu_category_renamed), NULL);
	g_signal_connect(GTK_OBJECT(categories_sequence_edit), "removed", G_CALLBACK(menu_category_removed), NULL);
	g_signal_connect(email_entry, "changed", G_CALLBACK(menu_email_changed), NULL);
	g_signal_connect(menuhelp_edit_button, "clicked", G_CALLBACK(menu_help_edit), NULL);
	g_signal_connect(author_entry, "changed", G_CALLBACK(menu_author_changed), NULL);

	/* categories */
	GebrGeoXmlSequence *category;
	gebr_geoxml_flow_get_category(debr.menu, &category, 0);
	gebr_gui_value_sequence_edit_load(GEBR_GUI_VALUE_SEQUENCE_EDIT(categories_sequence_edit), category,
					  (ValueSequenceSetFunction) gebr_geoxml_value_sequence_set,
					  (ValueSequenceGetFunction) gebr_geoxml_value_sequence_get, NULL);

	gtk_widget_show(dialog);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK){
		menu_replace();
		ret = FALSE;
		goto out;
	}

	menu_selected();

out:
	gtk_widget_destroy(dialog);
	return ret;
}

gboolean menu_get_selected(GtkTreeIter * iter, gboolean warn_unselected_menu)
{
	return menu_get_selected_type(iter, warn_unselected_menu) == ITER_FILE;
}

IterType menu_get_type(GtkTreeIter * iter)
{
	switch (gtk_tree_store_iter_depth(debr.ui_menu.model, iter)) {
	case 0:
		return ITER_FOLDER;
	case 1:
		return ITER_FILE;
	default:
		return ITER_NONE;
	}
}

IterType menu_get_selected_type(GtkTreeIter * _iter, gboolean warn_unselected_menu)
{
	IterType type;
	GtkTreeIter iter;

	if (gebr_gui_gtk_tree_view_get_selected(GTK_TREE_VIEW(debr.ui_menu.tree_view), &iter))
		type = menu_get_type(&iter);
	else
		type = ITER_NONE;

	if (_iter != NULL)
		*_iter = iter;
	if (type != ITER_FILE && warn_unselected_menu)
		debr_message(GEBR_LOG_ERROR, _("Please select a menu."));

	return type;
}

void menu_select_iter(GtkTreeIter * iter)
{
	gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(debr.ui_menu.tree_view), iter);
	menu_selected();
}

void menu_details_update(void)
{
	gchar *markup;
	GString *text;
	glong icmax;

	if (debr.menu == NULL) {
		gtk_container_foreach(GTK_CONTAINER(debr.ui_menu.details.vbox), (GtkCallback) gtk_widget_hide, NULL);
		return;
	} else
		gtk_container_foreach(GTK_CONTAINER(debr.ui_menu.details.vbox), (GtkCallback) gtk_widget_show, NULL);

	gtk_widget_set_sensitive(debr.ui_menu.details.help_button, TRUE);

	markup = g_markup_printf_escaped("<b>%s</b>", gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(debr.menu)));
	gtk_label_set_markup(GTK_LABEL(debr.ui_menu.details.title_label), markup);
	g_free(markup);

	markup = g_markup_printf_escaped("<i>%s</i>", gebr_geoxml_document_get_description(GEBR_GEOXML_DOC(debr.menu)));
	gtk_label_set_markup(GTK_LABEL(debr.ui_menu.details.description_label), markup);
	g_free(markup);

	text = g_string_new(NULL);
	switch (gebr_geoxml_flow_get_programs_number(GEBR_GEOXML_FLOW(debr.menu))) {
	case 0:
		g_string_printf(text, _("This menu has no programs"));
		break;
	case 1:
		g_string_printf(text, _("This menu has 1 program"));
		break;
	default:
		g_string_printf(text, _("This menu has %li programs"),
				gebr_geoxml_flow_get_programs_number(GEBR_GEOXML_FLOW(debr.menu)));
	}
	gtk_label_set_text(GTK_LABEL(debr.ui_menu.details.nprogs_label), text->str);
	g_string_free(text, TRUE);

	markup = g_markup_printf_escaped("<b>%s</b>", _("Created: "));
	gtk_label_set_markup(GTK_LABEL(debr.ui_menu.details.created_label), markup);
	g_free(markup);

	markup = g_markup_printf_escaped("<b>%s</b>", _("Modified: "));
	gtk_label_set_markup(GTK_LABEL(debr.ui_menu.details.modified_label), markup);
	g_free(markup);

	gtk_label_set_text(GTK_LABEL(debr.ui_menu.details.created_date_label),
			   gebr_localized_date(gebr_geoxml_document_get_date_created(GEBR_GEOXML_DOCUMENT(debr.menu))));
	gtk_label_set_text(GTK_LABEL(debr.ui_menu.details.modified_date_label),
			   gebr_localized_date(gebr_geoxml_document_get_date_modified
					       (GEBR_GEOXML_DOCUMENT(debr.menu))));

	markup = g_markup_printf_escaped("<b>%s</b>", _("Categories: "));
	gtk_label_set_markup(GTK_LABEL(debr.ui_menu.details.category_label), markup);
	g_free(markup);

	gtk_label_set_text(GTK_LABEL(debr.ui_menu.details.categories_label[0]), NULL);
	gtk_label_set_text(GTK_LABEL(debr.ui_menu.details.categories_label[1]), NULL);
	gtk_label_set_text(GTK_LABEL(debr.ui_menu.details.categories_label[2]), NULL);

	icmax = MIN(gebr_geoxml_flow_get_categories_number(GEBR_GEOXML_FLOW(debr.menu)), 2);
	for (long int ic = 0; ic < icmax; ic++) {
		GebrGeoXmlSequence *category;

		gebr_geoxml_flow_get_category(GEBR_GEOXML_FLOW(debr.menu), &category, ic);

		text = g_string_new(NULL);
		g_string_printf(text, "%s", gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(category)));
		gtk_label_set_text(GTK_LABEL(debr.ui_menu.details.categories_label[ic]), text->str);
		g_string_free(text, TRUE);
	}
	if (icmax == 0) {
		markup = g_markup_printf_escaped("<i>%s</i>", _("None"));
		gtk_label_set_markup(GTK_LABEL(debr.ui_menu.details.categories_label[0]), markup);
		g_free(markup);
	}

	if (gebr_geoxml_flow_get_categories_number(GEBR_GEOXML_FLOW(debr.menu)) > 2)
		gtk_label_set_text(GTK_LABEL(debr.ui_menu.details.categories_label[2]), "...");

	text = g_string_new(NULL);
	g_string_printf(text, "%s <%s>",
			gebr_geoxml_document_get_author(GEBR_GEOXML_DOC(debr.menu)),
			gebr_geoxml_document_get_email(GEBR_GEOXML_DOC(debr.menu)));
	gtk_label_set_text(GTK_LABEL(debr.ui_menu.details.author_label), text->str);
	g_string_free(text, TRUE);

	g_object_set(G_OBJECT(debr.ui_menu.details.help_button),
		     "sensitive", strlen(gebr_geoxml_document_get_help(GEBR_GEOXML_DOC(debr.menu))) > 1 ? TRUE : FALSE,
		     NULL);
}

void menu_folder_details_update(GtkTreeIter * iter)
{
	gchar *folder_name;
	gchar *folder_path;
	gchar *markup;
	gint n_menus;
	GString *text;

	if (menu_get_selected_type(iter, FALSE) != ITER_FOLDER)
		return;
	else
		gtk_container_foreach(GTK_CONTAINER(debr.ui_menu.details.vbox), (GtkCallback) gtk_widget_show, NULL);

	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), iter, MENU_PATH, &folder_path, -1);

	gtk_widget_set_sensitive(debr.ui_menu.details.help_button, FALSE);
	if (gebr_gui_gtk_tree_iter_equal_to(iter, &debr.ui_menu.iter_other)) {
		gtk_label_set_markup(GTK_LABEL(debr.ui_menu.details.title_label), _("<b>Others</b>"));
		gtk_label_set_markup(GTK_LABEL(debr.ui_menu.details.description_label),
				     _("<i>This folder lists the menus which are "
				       "not in any configured folder through " "users' preferences.</i>"));
	} else {
		folder_name = g_path_get_basename(folder_path);
		markup = g_markup_printf_escaped("<b>%s</b>", folder_name);
		gtk_label_set_markup(GTK_LABEL(debr.ui_menu.details.title_label), markup);
		g_free(markup);

		markup = g_markup_printf_escaped("<i>%s</i>", folder_path);
		gtk_label_set_markup(GTK_LABEL(debr.ui_menu.details.description_label), markup);
		g_free(markup);
		g_free(folder_name);
	}

	text = g_string_new(NULL);
	n_menus = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(debr.ui_menu.model), iter);
	switch (n_menus) {
	case 0:
		g_string_printf(text, _("This folder has no menu"));
		break;
	case 1:
		g_string_printf(text, _("This folder has 1 menu"));
		break;
	default:
		g_string_printf(text, _("This folder has %d menus"), n_menus);
		break;
	}
	gtk_label_set_markup(GTK_LABEL(debr.ui_menu.details.nprogs_label), text->str);
	g_string_free(text, TRUE);

	gtk_label_set_text(GTK_LABEL(debr.ui_menu.details.created_label), "");
	gtk_label_set_text(GTK_LABEL(debr.ui_menu.details.created_date_label), "");
	gtk_label_set_text(GTK_LABEL(debr.ui_menu.details.modified_label), "");
	gtk_label_set_text(GTK_LABEL(debr.ui_menu.details.modified_date_label), "");
	gtk_label_set_text(GTK_LABEL(debr.ui_menu.details.category_label), "");
	gtk_label_set_text(GTK_LABEL(debr.ui_menu.details.categories_label[0]), "");
	gtk_label_set_text(GTK_LABEL(debr.ui_menu.details.categories_label[1]), "");
	gtk_label_set_text(GTK_LABEL(debr.ui_menu.details.categories_label[2]), "");

	g_object_set(G_OBJECT(debr.ui_menu.details.help_button), "sensitive", FALSE, NULL);

	g_free(folder_path);
}

void menu_reset()
{
	GtkTreeIter iter;
	gebr_gui_gtk_tree_model_foreach(iter, GTK_TREE_MODEL(debr.ui_menu.model)) {
		if (gebr_gui_gtk_tree_iter_equal_to(&iter, &debr.ui_menu.iter_other))
			continue;
		menu_close_folder(&iter);
	}
	menu_load_user_directory();
}

gint menu_get_n_menus()
{
	GtkTreeIter iter;
	gint n;

	n = 0;
	gebr_gui_gtk_tree_model_foreach(iter, GTK_TREE_MODEL(debr.ui_menu.model))
	    n += gtk_tree_model_iter_n_children(GTK_TREE_MODEL(debr.ui_menu.model), &iter);
	return n;
}

void menu_path_get_parent(const gchar * path, GtkTreeIter * parent)
{
	gchar *dirname;

	dirname = g_path_get_dirname(path);
	if (!menu_get_folder_iter_from_path(dirname, parent))
		*parent = debr.ui_menu.iter_other;
	g_free(dirname);
}

glong menu_count_unsaved()
{
	GtkTreeIter iter;
	gboolean valid;
	glong count;

	count = 0;

	gebr_gui_gtk_tree_model_foreach(iter, GTK_TREE_MODEL(debr.ui_menu.model)) {
		GtkTreeIter child;
		MenuStatus status;

		valid = gtk_tree_model_iter_children(GTK_TREE_MODEL(debr.ui_menu.model), &child, &iter);
		while (valid) {
			gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &child, MENU_STATUS, &status, -1);
			if (status == MENU_STATUS_UNSAVED)
				count++;
			valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(debr.ui_menu.model), &child);
		}
	}
	return count;
}

void menu_replace(void){

	GtkTreeIter iter;
	GtkTreePath * program_path, * parameter_path;

	program_path = parameter_path = NULL;
	
	if (program_get_selected(&iter, FALSE))
		program_path = gtk_tree_model_get_path(GTK_TREE_MODEL(debr.ui_program.list_store), &iter);

	if (parameter_get_selected(&iter, FALSE))
		parameter_path = gtk_tree_model_get_path(GTK_TREE_MODEL(debr.ui_parameter.tree_store), &iter);

	if (menu_get_selected(&iter, FALSE)){
		gtk_tree_store_set(debr.ui_menu.model, &iter, MENU_XMLPOINTER, debr.menu_recovery.clone, -1);
		gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(debr.menu));
		menu_selected();
		menu_saved_status_set(debr.menu_recovery.status);
	}

	if ((program_path != NULL) && gtk_tree_model_get_iter(GTK_TREE_MODEL(debr.ui_program.list_store), &iter, program_path)){
		program_select_iter(iter);
		gtk_tree_path_free(program_path);
	}

	if ((parameter_path != NULL) && gtk_tree_model_get_iter(GTK_TREE_MODEL(debr.ui_parameter.tree_store), &iter, parameter_path)){
		gtk_tree_view_expand_to_path(GTK_TREE_VIEW(debr.ui_parameter.tree_view), parameter_path);
		parameter_select_iter(iter);
		gtk_tree_path_free(parameter_path);
	}

}

void menu_archive(void) {

	GtkTreeIter iter;

	debr.menu_recovery.clone = GEBR_GEOXML_FLOW(gebr_geoxml_document_clone(GEBR_GEOXML_DOC(debr.menu)));

	if (menu_get_selected(&iter, FALSE))
		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &iter, MENU_STATUS, &debr.menu_recovery.status, -1);
}

void menu_select_program_and_paramater(const gchar *program_path_string, const gchar *parameter_path_string)
{
	GtkTreeIter iter;

	if (!gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(debr.ui_program.list_store), &iter, program_path_string))
		return;
	program_select_iter(iter);

	if (parameter_path_string == NULL)
		return;
	if (!gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(debr.ui_parameter.tree_store), &iter, parameter_path_string))
		return;
	parameter_select_iter(iter);
}

gboolean menu_open_folder(const gchar * path)
{
	GtkTreeIter iter;
	GString *menu_path;
	gchar *dirname;
	gchar *dname;
	gchar *filename;

	if (g_hash_table_lookup_extended(debr.config.opened_folders, path, NULL, NULL))
		return TRUE;

	dname = g_path_get_basename(path);
	dirname = g_markup_printf_escaped("%s", dname);

	gtk_tree_store_append(debr.ui_menu.model, &iter, NULL);
	gtk_tree_store_set(debr.ui_menu.model, &iter,
			   MENU_IMAGE, GTK_STOCK_DIRECTORY,
			   MENU_FILENAME, dirname,
			   MENU_PATH, path,
			   -1);

	menu_path = g_string_new(NULL);
	gebr_directory_foreach_file(filename, path) {
		if (fnmatch("*.mnu", filename, 1))
			continue;

		g_string_printf(menu_path, "%s/%s", path, filename);
		menu_open_with_parent(menu_path->str, &iter, FALSE);
	}

	g_string_free(menu_path, TRUE);
	g_free(dirname);
	g_free(dname);

	return TRUE;
}

void menu_close_folder(GtkTreeIter * iter)
{
	gboolean valid;
	GtkTreeIter parent;
	GtkTreeIter child;
	GList *rows = NULL;

	if (menu_get_type(iter) != ITER_FOLDER)
		return;

	parent = *iter;
	valid = gtk_tree_model_iter_children(GTK_TREE_MODEL(debr.ui_menu.model), &child, &parent);
	while (valid) {
		GtkTreePath *tree_path;
		tree_path = gtk_tree_model_get_path(GTK_TREE_MODEL(debr.ui_menu.model), &child);
		rows = g_list_prepend(rows, gtk_tree_row_reference_new(GTK_TREE_MODEL(debr.ui_menu.model), tree_path));
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(debr.ui_menu.model), &child);
		gtk_tree_path_free(tree_path);
	}

	GList *rows_iter = rows;
	while (rows_iter) {
		GtkTreePath *tree_path;
		tree_path = gtk_tree_row_reference_get_path((GtkTreeRowReference*)(rows_iter->data));
		gtk_tree_model_get_iter(GTK_TREE_MODEL(debr.ui_menu.model), &child, tree_path);
		menu_close(&child);
		rows_iter = rows_iter->next;
		gtk_tree_path_free(tree_path);
		gtk_tree_row_reference_free((GtkTreeRowReference*)(rows_iter->data));
	}

	gchar *path;
	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), iter, MENU_PATH, &path, -1);
	g_hash_table_remove(debr.config.opened_folders, path);
	gtk_tree_store_remove(debr.ui_menu.model, &parent);
	g_free(path);

	g_list_free(rows);
}

void menu_close_folder_from_path(const gchar * path)
{
	GtkTreeIter iter;
	if (menu_get_folder_iter_from_path(path, &iter))
		menu_close_folder(&iter);
}


/**
 * \internal
 * Sets the sort column for menu tree view to \p column.
 */
static void debr_menu_sort_by_column_id(gint column)
{
	gint id;
	GtkSortType order;
	gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE(debr.ui_menu.model), &id, &order);
	order = (id == column)
	    ? (order == GTK_SORT_ASCENDING) ? GTK_SORT_DESCENDING : GTK_SORT_ASCENDING : GTK_SORT_ASCENDING;
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(debr.ui_menu.model), column, order);
	debr.config.menu_sort_column = column;
	debr.config.menu_sort_ascending = (order == GTK_SORT_ASCENDING) ? TRUE : FALSE;
	debr_config_save();
}

/**
 * \internal
 * Sort the list of menus alphabetically.
 */
static void menu_sort_by_name(GtkMenuItem * menu_item)
{
	debr_menu_sort_by_column_id(MENU_FILENAME);
}

/**
 * \internal
 * Sort the list of menus by the newest modified date.
 */
static void menu_sort_by_modified_date(GtkMenuItem * menu_item)
{
	debr_menu_sort_by_column_id(MENU_MODIFIED_DATE);
}

/**
 * \internal
 * Agregate action to the popup menu and shows it.
 */
static GtkMenu *menu_popup_menu(GtkTreeView * tree_view)
{
	GtkWidget *menu;
	GtkWidget *menu_item;
	GtkWidget *sub_menu;
	GtkWidget *image;
	GtkSortType order;
	gint column_id;

	menu = gtk_menu_new();

	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(debr.action_group, "menu_new")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(debr.action_group, "menu_close")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (debr.action_group, "menu_properties")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(debr.action_group, "menu_validate")));

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
	if (gtk_action_get_sensitive(gtk_action_group_get_action(debr.action_group, "menu_save")))
		gtk_container_add(GTK_CONTAINER(menu),
				  gtk_action_create_menu_item(gtk_action_group_get_action
							      (debr.action_group, "menu_save")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(debr.action_group, "menu_save_as")));
	if (gtk_action_get_sensitive(gtk_action_group_get_action(debr.action_group, "menu_revert")))
		gtk_container_add(GTK_CONTAINER(menu),
				  gtk_action_create_menu_item(gtk_action_group_get_action
							      (debr.action_group, "menu_revert")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(debr.action_group, "menu_delete")));

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
	menu_item = gtk_menu_item_new_with_label(_("Collapse all"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_collapse_all), tree_view);
	menu_item = gtk_menu_item_new_with_label(_("Expand all"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_expand_all), tree_view);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
	menu_item = gtk_menu_item_new_with_label(_("Sort by"));
	gtk_container_add(GTK_CONTAINER(menu), menu_item);

	// Get informations to create menu
	gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE(debr.ui_menu.model), &column_id, &order);
	if (order == GTK_SORT_ASCENDING)
		image = gtk_image_new_from_stock(GTK_STOCK_SORT_ASCENDING, GTK_ICON_SIZE_MENU);
	else
		image = gtk_image_new_from_stock(GTK_STOCK_SORT_DESCENDING, GTK_ICON_SIZE_MENU);

	sub_menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), sub_menu);

	menu_item = gtk_image_menu_item_new_with_label(_("Name"));
	if (column_id == MENU_FILENAME)
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), image);
	g_signal_connect(menu_item, "activate", G_CALLBACK(menu_sort_by_name), NULL);
	gtk_container_add(GTK_CONTAINER(sub_menu), menu_item);

	menu_item = gtk_image_menu_item_new_with_label(_("Modified date"));
	if (column_id == MENU_MODIFIED_DATE)
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item), image);
	g_signal_connect(menu_item, "activate", G_CALLBACK(menu_sort_by_modified_date), NULL);
	gtk_container_add(GTK_CONTAINER(sub_menu), menu_item);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(debr.action_group, "menu_install")));

	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}

/**
 * \internal
 * Keep XML in sync with widget.
 */
static void menu_title_changed(GtkEntry * entry)
{
	gebr_geoxml_document_set_title(GEBR_GEOXML_DOC(debr.menu), gtk_entry_get_text(entry));
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/**
 * \internal
 * Keep XML in sync with widget.
 */
static void menu_description_changed(GtkEntry * entry)
{
	gebr_geoxml_document_set_description(GEBR_GEOXML_DOC(debr.menu), gtk_entry_get_text(entry));
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/**
 * \internal
 * Calls \ref help_show with menu's help.
 */
static void menu_help_view(void)
{
	gchar *help;

	help = (gchar *) gebr_geoxml_document_get_help(GEBR_GEOXML_DOC(debr.menu));

	if (strlen(help) > 1)
		help_show(help, gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(debr.menu)));
	else
		help_show(" ", gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(debr.menu)));
}

/**
 * \internal
 * Calls \ref debr_help_edit with menu's help. After help was edited in a external browser, save it back to XML.
 */
static void menu_help_edit(void)
{
	debr_help_edit(gebr_geoxml_document_get_help(GEBR_GEOXML_DOC(debr.menu)), NULL);
}

/**
 * \internal
 * Keep XML in sync with widget.
 */
static void menu_author_changed(GtkEntry * entry)
{
	gebr_geoxml_document_set_author(GEBR_GEOXML_DOC(debr.menu), gtk_entry_get_text(entry));
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/**
 * \internal
 * Keep XML in sync with widget.
 */
static void menu_email_changed(GtkEntry * entry)
{
	gebr_geoxml_document_set_email(GEBR_GEOXML_DOC(debr.menu), gtk_entry_get_text(entry));
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/**
 * \internal
 * Add a category.
 */
static void menu_category_add(GebrGuiValueSequenceEdit * sequence_edit, GtkComboBox * combo_box)
{
	gchar *name;

	name = gtk_combo_box_get_active_text(combo_box);
	if (!strlen(name))
		name = g_strdup(_("New category"));
	else
		debr_has_category(name, TRUE);
	gebr_gui_value_sequence_edit_add(GEBR_GUI_VALUE_SEQUENCE_EDIT(sequence_edit),
					 GEBR_GEOXML_SEQUENCE(gebr_geoxml_flow_append_category(debr.menu, name)));

	menu_saved_status_set(MENU_STATUS_UNSAVED);

	g_free(name);
}

/**
 * \internal
 * Just wrap signal to notify an unsaved status.
 */
static void menu_category_changed(void)
{
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

/**
 * \internal
 * Update category list upon rename.
 */
static void
menu_category_renamed(GebrGuiValueSequenceEdit * sequence_edit, const gchar * old_text, const gchar * new_text)
{
	menu_category_removed(sequence_edit, old_text);
	debr_has_category(new_text, TRUE);
}

/**
 * \internal
 * Update category list upon removal.
 */
static void menu_category_removed(GebrGuiValueSequenceEdit * sequence_edit, const gchar * old_text)
{
	GtkTreeIter iter;

	gebr_gui_gtk_tree_model_foreach(iter, GTK_TREE_MODEL(debr.categories_model)) {
		gchar *i;
		gint ref_count;

		gtk_tree_model_get(GTK_TREE_MODEL(debr.categories_model), &iter,
				   CATEGORY_NAME, &i, CATEGORY_REF_COUNT, &ref_count, -1);
		if (strcmp(i, old_text) == 0) {
			ref_count--;
			if (ref_count == 0)
				gtk_list_store_remove(debr.categories_model, &iter);
			else
				gtk_list_store_set(debr.categories_model, &iter, CATEGORY_REF_COUNT, ref_count, -1);
			g_free(i);
			break;
		}

		g_free(i);
	}
}

/**
 * \internal
 * Sets the tooltip for the menu entries as its path on the file system.
 */
static gboolean
menu_on_query_tooltip(GtkTreeView * tree_view, GtkTooltip * tooltip, GtkTreeIter * iter, GtkTreeViewColumn * column,
		      gpointer user_data)
{
	gchar *path;

	if (menu_get_type(iter) == ITER_FOLDER) {
		if (gebr_gui_gtk_tree_iter_equal_to(iter, &debr.ui_menu.iter_other))
			return FALSE;	/* It is the "Others" folder. */

		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), iter, MENU_PATH, &path, -1);
		gtk_tooltip_set_text(tooltip, path);
		g_free(path);
		return TRUE;
	}
	return FALSE;
}

/**
 * \internal
 * Tells if \p path is loaded in DeBR interface.
 * If \p path is loaded and \p iter is not NULL, set \p iter to point to the respective menu whose file path is \p path.
 * \return TRUE if \p path is loaded, FALSE otherwise.
 */
static gboolean menu_is_path_loaded(const gchar * path, GtkTreeIter * iter)
{
	gchar *fname;
	gboolean valid;
	GtkTreeIter child;
	GtkTreeIter parent;

	fname = g_path_get_basename(path);
	menu_path_get_parent(path, &parent);
	valid = gtk_tree_model_iter_children(GTK_TREE_MODEL(debr.ui_menu.model), &child, &parent);

	while (valid) {
		gchar *path_;
		gchar *fname_;

		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &child,
				   MENU_PATH, &path_,
				   -1);
		fname_ = g_path_get_basename(path_);

		if (strcmp(fname_, fname) == 0) {
			g_free(fname_);
			g_free(path_);
			break;
		}
		g_free(fname_);
		g_free(path_);
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(debr.ui_menu.model), &child);
	}

	if (valid && iter)
		*iter = child;
	return valid;
}

static gboolean menu_get_folder_iter_from_path(const gchar * path, GtkTreeIter * iter_)
{
	GtkTreeIter iter;

	gebr_gui_gtk_tree_model_foreach(iter, GTK_TREE_MODEL(debr.ui_menu.model)) {
		gchar *dirpath;

		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &iter, MENU_PATH, &dirpath, -1);
		if (!dirpath)
			continue;

		if (strcmp(dirpath, path) == 0) {
			*iter_ = iter;
			g_free(dirpath);
			return TRUE;
		}
		g_free(dirpath);
	}
	return FALSE;
}
