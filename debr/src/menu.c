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
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <libgebr/intl.h>
#include <libgebr/date.h>
#include <libgebr/utils.h>
#include <libgebr/validate.h>
#include <libgebr/gui.h>

#include "menu.h"
#include "defines.h"
#include "debr.h"
#include "callbacks.h"
#include "help.h"
#include "program.h"
#include "interface.h"
#include "categoryedit.h"
#include "debr-help-edit-widget.h"

/*
 * Prototypes
 */

static GtkMenu *menu_popup_menu(GtkTreeView * tree_view);

static GtkMenu *menu_dialog_save_popup_menu(GtkTreeView * tree_view);

static void menu_title_changed(GtkEntry * entry);

static void menu_description_changed(GtkEntry * entry);

static void on_menu_help_edit_clicked(GtkWidget * edit_button);

static void menu_author_changed(GtkEntry * entry);

static void menu_email_changed(GtkEntry * entry);

static void menu_category_changed(void);

static void menu_category_renamed(GebrGuiValueSequenceEdit * sequence_edit,
				  const gchar * old_text, const gchar * new_text);

static void menu_category_removed(GebrGuiValueSequenceEdit * sequence_edit, const gchar * old_text);

static gboolean menu_on_query_tooltip(GtkTreeView * tree_view, GtkTooltip * tooltip,
				      GtkTreeIter * iter, GtkTreeViewColumn * column, gpointer user_data);

static gboolean menu_is_path_loaded(const gchar * path, GtkTreeIter * iter);

static gboolean menu_get_folder_iter_from_path(const gchar * path, GtkTreeIter * iter_);

static GList * menu_get_unsaved(GtkTreeIter * folder);

static void on_renderer_toggled(GtkCellRendererToggle * cell, gchar * path, GtkListStore * store);

static void on_menu_select_all_activate(GtkMenuItem *menuitem, GtkTreeView *tree_view);

static void on_menu_unselect_all_activate(GtkMenuItem *menuitem, GtkTreeView *tree_view);

static gboolean menu_save_iter_list(GList * unsaved);

static void menu_remove_with_validation(GtkTreeIter * iter);

static void debr_menu_sync_help_edit_window(GtkTreeIter * iter, gpointer object);

static void on_menu_help_show_clicked(GtkWidget * button);
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
	g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(debr.ui_menu.tree_view)), "changed",
			 G_CALLBACK(menu_selected), NULL);
	g_signal_connect(debr.ui_menu.tree_view, "row-activated", G_CALLBACK(on_menu_properties_activate), NULL);

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
	gtk_label_set_selectable(GTK_LABEL(debr.ui_menu.details.description_label), TRUE);
	g_object_set(G_OBJECT(debr.ui_menu.details.description_label), "wrap", TRUE, NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_menu.details.description_label), 0, 0);
	gtk_box_pack_start(GTK_BOX(details), debr.ui_menu.details.description_label, FALSE, TRUE, 0);

	debr.ui_menu.details.nprogs_label = gtk_label_new(NULL);
	gtk_label_set_selectable(GTK_LABEL(debr.ui_menu.details.nprogs_label), TRUE);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_menu.details.nprogs_label), 0, 0);
	gtk_box_pack_start(GTK_BOX(details), debr.ui_menu.details.nprogs_label, FALSE, TRUE, 10);

	table = gtk_table_new(6, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(details), table, FALSE, TRUE, 0);

	debr.ui_menu.details.created_label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_menu.details.created_label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), debr.ui_menu.details.created_label, 0, 1, 0, 1, (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 3, 3);

	debr.ui_menu.details.created_date_label = gtk_label_new(NULL);
	gtk_label_set_selectable(GTK_LABEL(debr.ui_menu.details.created_date_label), TRUE);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_menu.details.created_date_label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), debr.ui_menu.details.created_date_label, 1, 2, 0, 1,
			 (GtkAttachOptions)GTK_FILL, (GtkAttachOptions)GTK_FILL, 3, 3);

	debr.ui_menu.details.modified_label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_menu.details.modified_label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), debr.ui_menu.details.modified_label, 0, 1, 1, 2, (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 3, 3);

	debr.ui_menu.details.modified_date_label = gtk_label_new(NULL);
	gtk_label_set_selectable(GTK_LABEL(debr.ui_menu.details.modified_date_label), TRUE);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_menu.details.modified_date_label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), debr.ui_menu.details.modified_date_label, 1, 2, 1, 2,
			 (GtkAttachOptions)GTK_FILL, (GtkAttachOptions)GTK_FILL, 3,
			 3);

	debr.ui_menu.details.category_label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_menu.details.category_label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), debr.ui_menu.details.category_label, 0, 1, 3, 4, (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 3, 3);

	debr.ui_menu.details.categories_label = gtk_label_new(NULL);
	gtk_label_set_selectable(GTK_LABEL(debr.ui_menu.details.categories_label), TRUE);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_menu.details.categories_label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), debr.ui_menu.details.categories_label, 1, 2, 3, 4,
			 (GtkAttachOptions)GTK_FILL, (GtkAttachOptions)GTK_FILL, 3, 3);


	/* Help */
	GtkWidget * hbox;
	debr.ui_menu.details.hbox = hbox = gtk_hbox_new(TRUE, 0);

	debr.ui_menu.details.help_view = gtk_button_new_with_label(_("View Help"));
	debr.ui_menu.details.help_edit = gtk_button_new_with_label(_("Edit Help"));
	gtk_box_pack_start(GTK_BOX(hbox), debr.ui_menu.details.help_view, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), debr.ui_menu.details.help_edit, TRUE, TRUE, 0);
	g_signal_connect(GTK_OBJECT(debr.ui_menu.details.help_view), "clicked",
			 G_CALLBACK(on_menu_help_show_clicked), NULL);
	g_signal_connect(GTK_OBJECT(debr.ui_menu.details.help_edit), "clicked",
			 G_CALLBACK(on_menu_help_edit_clicked), NULL);
	gtk_box_pack_end(GTK_BOX(details), hbox, FALSE, TRUE, 0);

	debr.ui_menu.details.author_label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(debr.ui_menu.details.author_label), 0, 0);
	gtk_box_pack_end(GTK_BOX(details), debr.ui_menu.details.author_label, FALSE, TRUE, 0);

	gtk_action_set_sensitive(gtk_action_group_get_action(debr.action_group, "menu_save_all"), FALSE);
	debr.ui_menu.widget = hpanel;
	gtk_widget_show_all(debr.ui_menu.widget);
}

void menu_new(gboolean edit)
{
	GebrGeoXmlFlow *menu = gebr_geoxml_flow_new();
	gebr_geoxml_document_set_author(GEBR_GEOXML_DOC(menu), debr.config.name->str);
	gebr_geoxml_document_set_email(GEBR_GEOXML_DOC(menu), debr.config.email->str);
	gebr_geoxml_document_set_date_created(GEBR_GEOXML_DOC(menu), gebr_iso_date());

	menu_new_from_menu(menu, edit);
}

void menu_new_from_menu(GebrGeoXmlFlow *menu, gboolean edit)
{
	static int new_count = 1;
	GtkTreeIter iter;
	GtkTreeIter target;
	gchar * new_menu_str;
	gchar * title_str;

	title_str = g_strdup_printf(_("Untitled menu %d"), new_count);
	new_menu_str = g_strdup_printf(_("untitled%d.mnu"), new_count);
	new_count++;


	debr.menu = menu;
	gebr_geoxml_document_set_title(GEBR_GEOXML_DOC(debr.menu), title_str);
	gebr_geoxml_document_set_author(GEBR_GEOXML_DOC(debr.menu), debr.config.name->str);
	gebr_geoxml_document_set_email(GEBR_GEOXML_DOC(debr.menu), debr.config.email->str);
	gebr_geoxml_document_set_date_created(GEBR_GEOXML_DOC(debr.menu), gebr_iso_date());
	gebr_geoxml_document_set_filename(GEBR_GEOXML_DOC(debr.menu), new_menu_str);

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
	gebr_geoxml_document_set_filename(GEBR_GEOXML_DOC(menu), new_menu_str);
	gtk_tree_store_set(debr.ui_menu.model, &iter,
			   MENU_STATUS, MENU_STATUS_UNSAVED, MENU_IMAGE, GTK_STOCK_NO,
			   MENU_FILENAME, new_menu_str, MENU_XMLPOINTER, (gpointer) debr.menu,
			   MENU_PATH, "", MENU_VALIDATE_POINTER, NULL, -1),
	menu_select_iter(&iter);

	menu_saved_status_set(MENU_STATUS_UNSAVED);
	if (edit) {
		if (!menu_dialog_setup_ui(TRUE))
			menu_close(&iter, FALSE);
	}

	g_free(new_menu_str);
	g_free(title_str);
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

	void foreach(gchar * key) {
		GString *path;
		gchar *dirname;
		gchar *dname;

		dname = g_path_get_basename(key);
		dirname = g_markup_printf_escaped("%s", dname);

		gtk_tree_store_append(debr.ui_menu.model, &iter, NULL);
		gtk_tree_store_set(debr.ui_menu.model, &iter,
				   MENU_IMAGE, GTK_STOCK_DIRECTORY,
				   MENU_FILENAME, dirname,
				   MENU_PATH, key,
				   -1);

		path = g_string_new(NULL);
		gebr_directory_foreach_file(filename, key) {
			if (fnmatch("*.mnu", filename, 1))
				continue;

			g_string_printf(path, "%s/%s", key, filename);
			menu_open_with_parent(path->str, &iter, FALSE);
		}

		g_string_free(path, TRUE);
		g_free(dirname);
		g_free(dname);
		i++;
	}

	g_hash_table_foreach(debr.config.opened_folders, (GHFunc)foreach, NULL);

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

	debr_menu_sync_help_edit_window(iter, menu);
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
		return menu_save_as(iter) ? MENU_MESSAGE_SUCCESS : MENU_MESSAGE_PERMISSION_DENIED;
	}

	filename = g_path_get_basename(path);
	if (gebr_geoxml_document_save(GEBR_GEOXML_DOC(menu), path) != GEBR_GEOXML_RETV_SUCCESS) {
		gebr_gui_message_dialog(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
					_("Could not save the menu"),
					_("You do not have necessary permissions to save "
					  "the menu. Please check the provided location is "
					  "correct and try again."));
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

gboolean menu_save_folder(GtkTreeIter * folder)
{
	gboolean ret;
	GList * unsaved;

	if (folder && menu_get_type(folder) != ITER_FOLDER)
		return FALSE;

	unsaved = menu_get_unsaved(folder);
	ret = menu_save_iter_list(unsaved);

	g_list_foreach(unsaved, (GFunc)gtk_tree_iter_free, NULL);
	g_list_free(unsaved);

	return ret;
}

gboolean menu_save_all(void)
{
	return menu_save_folder(NULL);
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
	const gchar * menu_filename;
	GebrGeoXmlFlow *menu;

	menu = menu_get_xml_pointer(iter);
	menu_filename = gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(menu));
	title = g_strdup_printf(_("Choose file for \"%s\""), menu_filename);
	dialog = gebr_gui_save_dialog_new(title, GTK_WINDOW(debr.window));
	gebr_gui_save_dialog_set_default_extension(GEBR_GUI_SAVE_DIALOG(dialog), ".mnu");
	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), menu_filename);
	g_free(title);

	void foreach(gchar * key) {
		gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(dialog), key, NULL);
	}
	g_hash_table_foreach(debr.config.opened_folders, (GHFunc)foreach, NULL);

	gtk_tree_model_iter_parent(GTK_TREE_MODEL(debr.ui_menu.model), &parent, iter);
	if (gebr_gui_gtk_tree_model_iter_equal_to(GTK_TREE_MODEL(debr.ui_menu.model),
						  &parent, &debr.ui_menu.iter_other)) {
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), getenv("HOME"));
	} else {
		gchar *menu_path;
		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &parent, MENU_PATH, &menu_path, -1);
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), menu_path);
		g_free(menu_path);
	}

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
	struct validate * validate;

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

	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &target,
			   MENU_PATH, &target_fname,
			   MENU_VALIDATE_POINTER, &validate,
			   -1);

	gtk_tree_store_set(debr.ui_menu.model, &target, MENU_VALIDATE_POINTER, validate, -1);

	menu_load_iter(filepath, &target, clone, TRUE);
	ret = (menu_save(&target) != MENU_MESSAGE_PERMISSION_DENIED);
	if (ret) {
		if (is_overwrite) {
			gebr_geoxml_document_free(remove);
			if (validate)
				validate_close_iter(&validate->iter);
		}

		/* If the menu was never saved, that means is 'currentpath' is empty,
		 * and we need to remove it from the interface. In case this menu was
		 * validated, we need to delete that too. */
		if (!strlen(currentpath))
			menu_remove_with_validation(iter);

		*iter = target;
	} else {

		/* We do not have permissions to save this menu, so we revert all operations.
		 * If we are not overwriting, just delete the new menu created. Otherwise, we
		 * need to load the old menu into target. */
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
	validate_menu(iter);
}

void menu_install(void)
{
	gboolean overwriteall;
	GtkTreeIter iter;

	overwriteall = FALSE;

	gebr_gui_gtk_tree_view_foreach_selected(&iter, debr.ui_menu.tree_view) {
		GtkWidget *dialog;

		gchar *menu_path;
		GString *destination;
		GString *command;
		GebrGeoXmlFlow *menu;
		MenuStatus status;
		gboolean do_save = FALSE;

		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &iter,
				   MENU_STATUS, &status, MENU_PATH, &menu_path, MENU_XMLPOINTER, &menu, -1);

		const gchar *menu_filename = gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(menu));
		destination = g_string_new(NULL);
		command = g_string_new(NULL);
		g_string_printf(destination, "%s/.gebr/gebr/menus/%s", getenv("HOME"), menu_filename);
		g_string_printf(command, "cp %s %s", menu_path, destination->str);

		if (status == MENU_STATUS_UNSAVED) {
			dialog = gtk_message_dialog_new(GTK_WINDOW(debr.window),
							(GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
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
							(GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
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
		g_free(menu_path);
		g_string_free(destination, TRUE);
		g_string_free(command, TRUE);
	}
}

void menu_close(GtkTreeIter * iter, gboolean warn_user)
{
	GebrGeoXmlFlow *menu;
	gchar *path;
	struct validate *validate;

	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), iter,
			   MENU_XMLPOINTER, &menu, MENU_PATH, &path,
			   MENU_VALIDATE_POINTER, &validate, -1);

	if (validate)
		validate_close_iter(&validate->iter);

	debr_remove_help_edit_window(menu);
	gebr_geoxml_document_free(GEBR_GEOXML_DOC(menu));
	if (gtk_tree_store_remove(debr.ui_menu.model, iter))
		menu_select_iter(iter);
	else
		menu_selected();

	if (warn_user)
	{  
		if (strlen(path)){
			debr_message(GEBR_LOG_INFO, _("Menu \"%s\" closed."), path);
		} else {
			debr_message(GEBR_LOG_INFO, _("Menu closed."));
		}
	}

	g_free(path);
}

void menu_selected(void)
{
	GtkTreeIter iter;
	IterType type;
	MenuStatus current_status;

	type = menu_get_selected_type(&iter, FALSE);
	if (type == ITER_FILE) {
		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &iter,
				   MENU_XMLPOINTER, &debr.menu,
				   MENU_STATUS, &current_status,
				   -1);
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
			"menu_add_folder",
			NULL
		};
		debr_set_actions_sensitive(names, TRUE);

		gchar *names_off[] = {
			"menu_remove_folder",
			NULL
		};
		debr_set_actions_sensitive(names_off, FALSE);
			
		menu_status_set_from_iter(&iter, current_status);

		struct validate *validate;
		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &iter, MENU_VALIDATE_POINTER, &validate, -1);
		if (validate != NULL) {
			validate_set_selected(&validate->iter);
			menu_validate(&iter);
		}
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

		gchar *names_on[] = {
			"menu_add_folder",
			NULL
		};
		debr_set_actions_sensitive(names_on, TRUE);

		GtkAction * action;
		action = gtk_action_group_get_action(debr.action_group, "menu_remove_folder");
		if (gebr_gui_gtk_tree_model_iter_equal_to(GTK_TREE_MODEL(debr.ui_menu.model), &iter, &debr.ui_menu.iter_other))
			gtk_action_set_sensitive(action, FALSE);
		else
			gtk_action_set_sensitive(action, TRUE);


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
			"menu_remove_folder",
			NULL
		};
		debr_set_actions_sensitive(names, FALSE);

		gchar *names_on[] = {
			"menu_add_folder",
			NULL
		};
		debr_set_actions_sensitive(names_on, TRUE);
	}
}

gboolean menu_cleanup_folder(GtkTreeIter * folder)
{
	GError *error = NULL;
	GList *unsaved_list;
	GtkBuilder *builder;
	GtkCellRenderer *renderer;
	GtkListStore *store;
	GtkTreeViewColumn *column;
	GtkWidget *dialog;
	GtkWidget *treeview;
	gboolean ret;
	gboolean still_running = TRUE;

	if (folder && menu_get_type(folder) != ITER_FOLDER)
		return FALSE;

	if (!menu_count_unsaved(folder))
		return TRUE;

	builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, DEBR_GLADE_DIR "/menu-dialog-save-ui.glade", &error);
	if (error) {
		g_warning("%s", error->message);
		g_error_free(error);
		error = NULL;
	}

	dialog = GTK_WIDGET(gtk_builder_get_object(builder, "dialog1"));
	treeview = GTK_WIDGET(gtk_builder_get_object(builder, "treeview1"));
	store = GTK_LIST_STORE(gtk_builder_get_object(builder, "liststore1"));

	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(debr.window));

	renderer = gtk_cell_renderer_toggle_new();
	g_signal_connect(renderer, "toggled", G_CALLBACK(on_renderer_toggled), store);
	column = gtk_tree_view_column_new_with_attributes("", renderer, "active", 0, NULL);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("", renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	
	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(treeview),
						  (GebrGuiGtkPopupCallback) menu_dialog_save_popup_menu, NULL);

	gtk_widget_show_all(dialog);

	while (still_running) {
		unsaved_list = menu_get_unsaved(folder);
		unsaved_list = g_list_reverse(unsaved_list);
		gtk_list_store_clear(store);
		for (GList * i = unsaved_list; i != NULL; i = i->next) {
			GebrGeoXmlFlow *menu;
			GtkTreeIter iter;

			gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), (GtkTreeIter*)i->data,
					   MENU_XMLPOINTER, &menu, -1);
			gtk_list_store_append(store, &iter);
			gtk_list_store_set(store, &iter,
					   0, TRUE,
					   1, gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(menu)),
					   2, i->data,
					   -1);
		}

		switch (gtk_dialog_run(GTK_DIALOG(dialog))) {
		case 1: { // Save response
			gboolean valid;
			GList * chosen_list = NULL;
			GtkTreeIter iter;

			valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
			while (valid) {
				gboolean chosen;
				GtkTreeIter *menu_iter;
				gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
						   0, &chosen,
						   2, &menu_iter,
						   -1);
				if (chosen)
					chosen_list = g_list_prepend(chosen_list, menu_iter);
				valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
			}

			/* If chosen_list = NULL, the user unchecked all menus.
			 * This means that there is nothing to save and the previous operation
			 * should go on. That's why ret = TRUE.
			 */
			if (chosen_list == NULL) {
				ret = TRUE;
				still_running = FALSE;
			} else {
				if (!menu_save_iter_list(chosen_list)) {
					ret = FALSE;
					still_running = TRUE;
				} else {
					ret = TRUE;
					still_running = FALSE;
				}
				g_list_free(chosen_list);
			}
			break;
		}
		case 3: // Close without saving response
			ret = TRUE;
			still_running = FALSE;
			break;
		case 2: // Cancel response
		default:
			still_running = FALSE;
			ret = FALSE;
		} 
		g_list_foreach(unsaved_list, (GFunc)gtk_tree_iter_free, NULL);
		g_list_free(unsaved_list);
	}

	gtk_list_store_clear(store);
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

	if (menu_count_unsaved(NULL) > 0)
		gtk_action_set_sensitive(gtk_action_group_get_action(debr.action_group, "menu_save_all"), TRUE);
	else
		gtk_action_set_sensitive(gtk_action_group_get_action(debr.action_group, "menu_save_all"), FALSE);
}

void menu_status_set_unsaved(void)
{
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

gboolean menu_dialog_setup_ui(gboolean new_menu)
{
	GtkWidget *dialog;
	GtkWidget *table;

	GtkWidget *title_label;
	GtkWidget *title_entry;
	GtkWidget *description_label;
	GtkWidget *description_entry;
	GtkWidget *author_label;
	GtkWidget *author_entry;
	GtkWidget *email_label;
	GtkWidget *email_entry;
	GtkWidget *categories_label;
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
					     (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
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

	categories_sequence_edit = category_edit_new(debr.menu, new_menu);
	gtk_widget_show(categories_sequence_edit);
	gtk_table_attach(GTK_TABLE(table), categories_sequence_edit, 1, 2, 5, 6,
			 (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 0, 0);
	gtk_widget_show(categories_sequence_edit);

	/*
	 * Load menu into widgets
	 */
	gtk_entry_set_text(GTK_ENTRY(title_entry), gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(debr.menu)));
	gtk_entry_set_text(GTK_ENTRY(description_entry), gebr_geoxml_document_get_description(GEBR_GEOXML_DOC(debr.menu)));
	gtk_entry_set_text(GTK_ENTRY(author_entry), gebr_geoxml_document_get_author(GEBR_GEOXML_DOC(debr.menu)));
	gtk_entry_set_text(GTK_ENTRY(email_entry), gebr_geoxml_document_get_email(GEBR_GEOXML_DOC(debr.menu))); 

	/* Apply validation cases to the fields */

	GebrValidateCase *validate_case = gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_TITLE);
	if (!new_menu)
		on_entry_focus_out(GTK_ENTRY(title_entry), NULL, validate_case);
	g_signal_connect(title_entry, "focus-out-event", G_CALLBACK(on_entry_focus_out), validate_case);

	validate_case = gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_DESCRIPTION);
	g_signal_connect(description_entry, "focus-out-event", G_CALLBACK(on_entry_focus_out), validate_case);
	if (!new_menu)
		on_entry_focus_out(GTK_ENTRY(description_entry), NULL, validate_case);

	validate_case = gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_AUTHOR);
	g_signal_connect(author_entry, "focus-out-event", G_CALLBACK(on_entry_focus_out), validate_case);
	if (!new_menu)
		on_entry_focus_out(GTK_ENTRY(author_entry), NULL, validate_case);

	validate_case = gebr_validate_get_validate_case(GEBR_VALIDATE_CASE_EMAIL);
	g_signal_connect(email_entry, "focus-out-event", G_CALLBACK(on_entry_focus_out), validate_case);
	if (!new_menu)
		on_entry_focus_out(GTK_ENTRY(email_entry), NULL, validate_case);

	/* signals */
	g_signal_connect(title_entry, "changed", G_CALLBACK(menu_title_changed), NULL);
	g_signal_connect(description_entry, "changed", G_CALLBACK(menu_description_changed), NULL);
	g_signal_connect(categories_sequence_edit, "changed", G_CALLBACK(menu_category_changed), NULL);
	g_signal_connect(categories_sequence_edit, "renamed", G_CALLBACK(menu_category_renamed), NULL);
	g_signal_connect(categories_sequence_edit, "removed", G_CALLBACK(menu_category_removed), NULL);
	g_signal_connect(email_entry, "changed", G_CALLBACK(menu_email_changed), NULL);
	g_signal_connect(author_entry, "changed", G_CALLBACK(menu_author_changed), NULL);

	gtk_widget_show(dialog);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK) {
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

	if (debr.menu == NULL) {
		gtk_container_foreach(GTK_CONTAINER(debr.ui_menu.details.vbox), (GtkCallback) gtk_widget_hide, NULL);
		return;
	} else{
		gtk_container_foreach(GTK_CONTAINER(debr.ui_menu.details.vbox), (GtkCallback) gtk_widget_show, NULL);
		gtk_container_foreach(GTK_CONTAINER(debr.ui_menu.details.hbox), (GtkCallback) gtk_widget_show, NULL);

		g_object_set(debr.ui_menu.details.help_view,
			     "sensitive", strlen(gebr_geoxml_document_get_help(GEBR_GEOXML_DOCUMENT(debr.menu)))
			     ? TRUE : FALSE, NULL);
	}
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

	gtk_label_set_text(GTK_LABEL(debr.ui_menu.details.categories_label), NULL);

	GebrGeoXmlSequence * category;

	text = g_string_new("");
	gebr_geoxml_flow_get_category(GEBR_GEOXML_FLOW(debr.menu), &category, 0);
	while (category) {
		const gchar * value;
		value = gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(category));
		g_string_append_printf(text, "%s\n", value);
		gebr_geoxml_sequence_next(&category);
	}
	if (text->len == 0) {
		/* There is no category, put "None" */
		markup = g_markup_printf_escaped("<i>%s</i>", _("None"));
		gtk_label_set_markup(GTK_LABEL(debr.ui_menu.details.categories_label), markup);
		g_free(markup);
	} else
		gtk_label_set_text(GTK_LABEL(debr.ui_menu.details.categories_label), text->str);
	g_string_free(text, TRUE);

	text = g_string_new(NULL);
	g_string_printf(text, "%s <%s>",
			gebr_geoxml_document_get_author(GEBR_GEOXML_DOC(debr.menu)),
			gebr_geoxml_document_get_email(GEBR_GEOXML_DOC(debr.menu)));
	gtk_label_set_text(GTK_LABEL(debr.ui_menu.details.author_label), text->str);
	g_string_free(text, TRUE);
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

	gtk_container_foreach(GTK_CONTAINER(debr.ui_menu.details.vbox), (GtkCallback) gtk_widget_show, NULL);
	if (debr.menu == NULL){
		gtk_widget_hide(debr.ui_menu.details.help_view);
		gtk_widget_hide(debr.ui_menu.details.help_edit);
	}
	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), iter, MENU_PATH, &folder_path, -1);

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
	gtk_label_set_text(GTK_LABEL(debr.ui_menu.details.categories_label), "");

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

glong menu_count_unsaved(GtkTreeIter * folder)
{
	glong count;

	count = 0;

	void process(GtkTreeIter * iter) {
		gboolean valid;
		GtkTreeIter child;
		MenuStatus status;

		valid = gtk_tree_model_iter_children(GTK_TREE_MODEL(debr.ui_menu.model), &child, iter);
		while (valid) {
			gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &child, MENU_STATUS, &status, -1);
			if (status == MENU_STATUS_UNSAVED)
				count++;
			valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(debr.ui_menu.model), &child);
		}
	}

	if (folder)
		process(folder);
	else {
		GtkTreeIter iter;
		gebr_gui_gtk_tree_model_foreach(iter, GTK_TREE_MODEL(debr.ui_menu.model))
			process(&iter);
	}

	return count;
}

void menu_replace(void) {

	GtkTreeIter iter;
	GtkTreePath * program_path, * parameter_path;

	program_path = parameter_path = NULL;
	
	if (program_get_selected(&iter, FALSE))
		program_path = gtk_tree_model_get_path(GTK_TREE_MODEL(debr.ui_program.list_store), &iter);

	if (parameter_get_selected(&iter, FALSE))
		parameter_path = gtk_tree_model_get_path(GTK_TREE_MODEL(debr.ui_parameter.tree_store), &iter);

	if (menu_get_selected(&iter, FALSE)) {
		debr_menu_sync_help_edit_window(&iter, debr.menu_recovery.clone);
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

	g_hash_table_insert(debr.config.opened_folders, g_strdup(path), NULL);

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
		rows = g_list_prepend(rows, gtk_tree_iter_copy(&child));
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(debr.ui_menu.model), &child);
	}

	GList *rows_iter = rows;
	while (rows_iter) {
		GtkTreeIter *child = (GtkTreeIter*)rows_iter->data;
		menu_close(child, FALSE);
		rows_iter = rows_iter->next;
		gtk_tree_iter_free(child);
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

GebrGeoXmlFlow * menu_get_xml_pointer(GtkTreeIter * iter)
{
	GtkTreeModel * model;
	GebrGeoXmlFlow * menu;
	model = GTK_TREE_MODEL(debr.ui_menu.model);
	gtk_tree_model_get(model, iter, MENU_XMLPOINTER, &menu, -1);
	return menu;
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
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(debr.action_group, "menu_add_folder")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(debr.action_group, "menu_remove_folder")));

	/*
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(debr.action_group, "menu_select_all")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(debr.action_group, "menu_unselect_all")));
	*/

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
 * Agregate action to the popup menu of the menus' save dialog and show it.
 */
static GtkMenu *menu_dialog_save_popup_menu(GtkTreeView * tree_view)
{
	GtkWidget *popup_menu;
	GtkWidget *select_all_menu_item;
	GtkWidget *unselect_all_menu_item;

	popup_menu = gtk_menu_new();

	select_all_menu_item = gtk_action_create_menu_item(gtk_action_group_get_action(debr.action_group, "menu_select_all"));
	g_signal_connect(select_all_menu_item, "activate", G_CALLBACK(on_menu_select_all_activate), tree_view);
	gtk_container_add(GTK_CONTAINER(popup_menu), select_all_menu_item);
	
	unselect_all_menu_item = gtk_action_create_menu_item(gtk_action_group_get_action(debr.action_group, "menu_unselect_all"));
	g_signal_connect(unselect_all_menu_item, "activate", G_CALLBACK(on_menu_unselect_all_activate), tree_view);
	gtk_container_add(GTK_CONTAINER(popup_menu), unselect_all_menu_item);

	gtk_widget_show_all(popup_menu);

	return GTK_MENU(popup_menu);
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
 * Calls \ref debr_help_show with menu's help.
 */
static void on_menu_help_show_clicked(GtkWidget * button)
{
	debr_help_show(GEBR_GEOXML_OBJECT(debr.menu), TRUE, _("Menu Help"));
}

/**
 * \internal
 * Calls \ref debr_help_edit with menu's help.
 * After help was edited in a external browser, save it back to XML.
 */
static void on_menu_help_edit_clicked(GtkWidget * button)
{
	debr_help_edit(GEBR_GEOXML_OBJECT(debr.menu));
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

/**
 * \internal
 */
static GList * menu_get_unsaved(GtkTreeIter * folder)
{
	g_return_val_if_fail(folder == NULL || menu_get_type(folder) == ITER_FOLDER, NULL);

	GList * list = NULL;

	void process(GtkTreeIter * iter) {
		gboolean valid;
		GtkTreeIter child;
		valid = gtk_tree_model_iter_children(GTK_TREE_MODEL(debr.ui_menu.model), &child, iter);
		while (valid) {
			MenuStatus status;
			gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &child, MENU_STATUS, &status, -1);
			if (status == MENU_STATUS_UNSAVED)
				list = g_list_prepend(list, gtk_tree_iter_copy(&child));
			valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(debr.ui_menu.model), &child);
		}
	}

	if (folder)
		process(folder);
	else {
		GtkTreeIter iter;
		gebr_gui_gtk_tree_model_foreach(iter, GTK_TREE_MODEL(debr.ui_menu.model))
			process(&iter);
	}

	return list;
}

static void on_renderer_toggled(GtkCellRendererToggle * cell, gchar * path, GtkListStore * store)
{
	gboolean chosen;
	GtkTreeIter iter;

	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iter, path);
	gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &chosen, -1);
	gtk_list_store_set(store, &iter, 0, !chosen, -1);
}

static void on_menu_select_all_activate(GtkMenuItem *menu_item, GtkTreeView *tree_view)
{
	GtkListStore *store;
	gboolean valid;
	GtkTreeIter iter;

	store = GTK_LIST_STORE(gtk_tree_view_get_model(tree_view));
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
	while (valid) {
		gtk_list_store_set(store, &iter, 0, TRUE, -1);
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
	}
}

static void on_menu_unselect_all_activate(GtkMenuItem *menu_item, GtkTreeView *tree_view)
{
	GtkListStore *store;
	gboolean valid;
	GtkTreeIter iter;

	store = GTK_LIST_STORE(gtk_tree_view_get_model(tree_view));
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
	while (valid) {
		gtk_list_store_set(store, &iter, 0, FALSE, -1);
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
	}
}

static gboolean menu_save_iter_list(GList * unsaved)
{
	GList * entry;
	gboolean ret;
	entry = unsaved;
	ret = TRUE;

	if (!entry)
		return FALSE;

	while (entry) {
		GtkTreeIter * iter;
		MenuMessage result;

		iter = (GtkTreeIter*)entry->data;
		result = menu_save(iter);
		if (result == MENU_MESSAGE_PERMISSION_DENIED) {
			ret = FALSE;
			break;
		}
		entry = entry->next;
	}

	if (ret) {
		menu_details_update();
		debr_message(GEBR_LOG_INFO, _("All menus were saved."));
	}

	return ret;

}

/*
 * menu_remove_with_validation:
 * Removes the validation entry corresponding to this menu @iter.
 * This function is needed to update the validate tab when a menu
 * is deleted.
 */
static void menu_remove_with_validation(GtkTreeIter * iter)
{
	GtkTreeModel * model;
	struct validate * validate;

	model = GTK_TREE_MODEL(debr.ui_menu.model);
	gtk_tree_model_get(model, iter, MENU_VALIDATE_POINTER, &validate, -1);

	if (validate != NULL)
		validate_close_iter(&validate->iter);

	gtk_tree_store_remove(debr.ui_menu.model, iter);
}

/*
 * debr_menu_sync_help_edit_window:
 * @iter: the iterator that will be modified.
 * @object: the new XML pointer for @iter.
 *
 * If @iter is having its XML changed to @object, this function must be called to synchronize the pointers of the help
 * edit windows.
 */
static void debr_menu_sync_help_edit_window(GtkTreeIter * iter, gpointer object)
{
	GebrGeoXmlFlow * old_menu;
	GebrGuiHelpEditWindow * help_edit_window;

	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), iter, MENU_XMLPOINTER, &old_menu, -1);

	/* Searches for 'old_menu' in the help_edit_windows hash table. If there is a window for this menu, we must
	 * update its geoxml-object. Further more, we must close all its programs window.
	 */
	help_edit_window = g_hash_table_lookup(debr.help_edit_windows, old_menu);

	if (help_edit_window != NULL) {
		GebrGeoXmlSequence * program;
		DebrHelpEditWidget * help_edit_widget;

		g_object_get(help_edit_window, "help-edit-widget", &help_edit_widget, NULL);
		g_object_set(help_edit_widget, "geoxml-object", object, NULL);

		/* Time to destroy the programs.
		 */
		gebr_geoxml_flow_get_program(old_menu, &program, 0);
		while (program) {
			help_edit_window = g_hash_table_lookup(debr.help_edit_windows, program);
			if (help_edit_window)
				gtk_widget_destroy(GTK_WIDGET(help_edit_window));
			gebr_geoxml_sequence_next(&program);
		}
	}
}

