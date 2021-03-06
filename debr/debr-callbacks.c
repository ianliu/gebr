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

#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "debr-gettext.h"

#include <glib/gi18n.h>
#include <libgebr/date.h>
#include <libgebr/utils.h>
#include <libgebr/gebr-tar.h>
#include <libgebr/gui/gui.h>
#include <libgebr/geoxml/gebr-geoxml-validate.h>

#include "debr-callbacks.h"
#include "debr.h"
#include "debr-preferences.h"
#include "debr-parameter.h"
#include "debr-menu.h"
#include "debr-program.h"

void on_notebook_switch_page(GtkNotebook *notebook, GtkNotebookPage *page, guint page_num)
{


	if (debr.last_notebook >= 0)
		gtk_window_remove_accel_group(GTK_WINDOW(debr.window), debr.accel_group_array[debr.last_notebook]);

	gtk_window_add_accel_group(GTK_WINDOW(debr.window), debr.accel_group_array[page_num]);
	debr.last_notebook = page_num;

	if (page_num == NOTEBOOK_PAGE_VALIDATE) {
		GtkTreeIter iter;
		gebr_gui_gtk_tree_model_foreach(iter, GTK_TREE_MODEL(debr.ui_validate.list_store)) {
			struct validate *validate;
			gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_validate.list_store), &iter, VALIDATE_POINTER, &validate, -1);

			gboolean need_update;
			gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &validate->menu_iter, MENU_VALIDATE_NEED_UPDATE, &need_update, -1);
			if (need_update) 
				validate_menu(&validate->menu_iter);
		}
	}
}

void do_navigation_bar_update(void)
{
	GString *markup;

	if (debr.menu == NULL) {
		gtk_label_set_markup(GTK_LABEL(debr.navigation_box_label), "");
		return;
	}

	markup = g_string_new(NULL);
	g_string_append(markup, g_markup_printf_escaped("<i>%s</i>",
							gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(debr.menu))));
	if (debr.program != NULL)
		g_string_append(markup, g_markup_printf_escaped(" :: <i>%s</i>",
								gebr_geoxml_program_get_title(debr.program)));
	if (debr.parameter != NULL)
		g_string_append(markup, g_markup_printf_escaped(" :: <i>%s</i>",
								gebr_geoxml_parameter_get_label(debr.parameter)));

	gtk_label_set_markup(GTK_LABEL(debr.navigation_box_label), markup->str);

	g_string_free(markup, TRUE);
}

void on_new_activate(void)
{
	switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(debr.notebook))) {
	case NOTEBOOK_PAGE_MENU:
		on_menu_new_activate();
		break;
	case NOTEBOOK_PAGE_PROGRAM:
		on_program_new_activate();
		break;
	case NOTEBOOK_PAGE_PARAMETER:
		gebr_gui_gtk_widget_drop_down_menu(GTK_WIDGET(debr.tool_item_new),
						   GTK_MENU(parameter_create_menu_with_types(FALSE)));
		break;
	default:
		break;
	}
}

void on_cut_activate(void)
{
	switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(debr.notebook))) {
	case NOTEBOOK_PAGE_PARAMETER:
		on_parameter_cut_activate();
		break;
	default:
		break;
	}
}

void on_copy_activate(void)
{
	switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(debr.notebook))) {
	case NOTEBOOK_PAGE_PROGRAM:
		on_program_copy_activate();
		break;
	case NOTEBOOK_PAGE_PARAMETER:
		on_parameter_copy_activate();
		break;
	default:
		break;
	}
}

void on_paste_activate(void)
{
	switch (gtk_notebook_get_current_page(GTK_NOTEBOOK(debr.notebook))) {
	case NOTEBOOK_PAGE_PROGRAM:
		on_program_paste_activate();
		break;
	case NOTEBOOK_PAGE_PARAMETER:
		on_parameter_paste_activate();
		break;
	default:
		break;
	}
}

void on_quit_activate(void)
{
	debr_quit();
}

void on_menu_new_activate(void)
{
	menu_new(TRUE);
}

static void
add_shortcut_folders_foreach(gpointer key, gpointer value, gpointer user_data)
{
	const gchar *folder = key;
	GtkFileChooser *chooser = user_data;
	gtk_file_chooser_add_shortcut_folder(chooser, folder, NULL);
}

void on_menu_open_activate(void)
{
	GtkWidget *chooser_dialog;
	GtkFileFilter *filefilter;
	gchar *path;

	/* create file chooser */
	chooser_dialog = gtk_file_chooser_dialog_new(_("Open menu"), NULL,
						     GTK_FILE_CHOOSER_ACTION_OPEN,
						     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						     GTK_STOCK_OPEN, GTK_RESPONSE_YES, NULL);
	g_hash_table_foreach(debr.config.opened_folders, (GHFunc) add_shortcut_folders_foreach, chooser_dialog);

	filefilter = gtk_file_filter_new();
	gtk_file_filter_set_name(filefilter, _("Menu files (*.mnu)"));
	gtk_file_filter_add_pattern(filefilter, "*.mnu");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_dialog), filefilter);

	/* run file chooser */
	gtk_widget_show(chooser_dialog);
	if (gtk_dialog_run(GTK_DIALOG(chooser_dialog)) != GTK_RESPONSE_YES)
		goto out;

	/* open it */
	path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser_dialog));
	menu_open(path, TRUE);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(debr.notebook), NOTEBOOK_PAGE_MENU);
	debr_message(GEBR_LOG_INFO, _("Menu \"%s\" opened."), path);
	g_free(path);

 out:	gtk_widget_destroy(chooser_dialog);
}

void on_menu_save_activate(void)
{
	GtkTreeIter iter;

	if (!menu_get_selected(&iter, TRUE))
		return;
	menu_save(&iter);
}

void on_menu_save_as_activate(void)
{
	GtkTreeIter iter;
	if (!menu_get_selected(&iter, TRUE))
		return;
	if (menu_save_as(&iter))
		menu_select_iter(&iter);
}

void on_menu_save_all_activate(void)
{
	menu_save_all();
}

void on_menu_revert_activate(void)
{
	GtkTreeIter iter;

	const gchar *title = _("Revert changes");

	if (gebr_gui_confirm_action_dialog
		(title, title,
		 _("All unsaved changes will be lost. Are you sure you want to revert selected menu(s)?")) == FALSE)
		return;

	gebr_gui_gtk_tree_view_foreach_selected(&iter, debr.ui_menu.tree_view) {
		GebrGeoXmlFlow *menu, *old_menu;
		gchar *path;

		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &iter,
				   MENU_XMLPOINTER, &old_menu, MENU_PATH, &path, -1);

		/* is this a new menu? */
		if (!strlen(path)) {
			debr_message(GEBR_LOG_ERROR, _("Menu was not saved yet."));
			g_free(path);
			return;
		}

		menu = menu_load(path);
		if (menu == NULL)
			return;

		debr_remove_help_edit_window(old_menu, FALSE, TRUE);

		/* revert to the one in disk */
		gebr_geoxml_document_free(GEBR_GEOXML_DOC(old_menu));
		debr.program = NULL;
		gebr_geoxml_document_ref(GEBR_GEOXML_DOCUMENT(menu));
		gtk_tree_store_set(debr.ui_menu.model, &iter, MENU_XMLPOINTER, menu, -1);
		menu_status_set_from_iter(&iter, MENU_STATUS_SAVED);

		debr_message(GEBR_LOG_INFO, _("Menu reverted."));
		/* frees */
		g_free(path);
	}
	program_details_update();
	menu_selected();
}

void on_menu_delete_activate(void)
{
	GtkTreeIter iter;
	const gchar *title = _("Delete menu");

	if (gebr_gui_confirm_action_dialog(title, title, _("Are you sure you want to delete selected menu(s)?")) ==
		FALSE)
		return;

	gebr_gui_gtk_tree_view_foreach_selected(&iter, debr.ui_menu.tree_view) {
		GebrGeoXmlFlow *menu;
		gchar *path;

		/* if this is not a menu item, pass */
		if (!menu_get_selected(&iter, TRUE))
			continue;

		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &iter,
				   MENU_XMLPOINTER, &menu, MENU_PATH, &path, -1);

		if ((strlen(path)) && (g_unlink(path))) {
			GtkWidget *dialog;

			dialog = gtk_message_dialog_new(GTK_WINDOW(debr.window), (GtkDialogFlags)(GTK_DIALOG_MODAL),
							GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
							_("Could not delete menu '%s'."),
							gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(menu)));
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
		} else{
			menu_close(&iter, TRUE);
			gtk_notebook_set_current_page(GTK_NOTEBOOK(debr.notebook), NOTEBOOK_PAGE_MENU);
		}
		g_free(path);
	}

}

void on_menu_create_from_flow_activate(void)
{
	gboolean use_value;
	GtkWidget *dialog;
	GtkFileFilter *file_filter;

	/* 
	 * Open flow.
	 */
	dialog = gtk_file_chooser_dialog_new(_("Choose flow to open"), GTK_WINDOW(debr.window), GTK_FILE_CHOOSER_ACTION_OPEN,
					     GTK_STOCK_OPEN, GTK_RESPONSE_YES,
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	file_filter = gtk_file_filter_new();
	gtk_file_filter_set_name(file_filter, _("Flow files (*.flw, *.flwz, *.flwx)"));
	gtk_file_filter_add_pattern(file_filter, "*.flw");
	gtk_file_filter_add_pattern(file_filter, "*.flwz");
	gtk_file_filter_add_pattern(file_filter, "*.flwx");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), file_filter);
	gtk_widget_show(dialog);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_YES)
		goto out;

	const gchar *title = _("Default values");
	use_value = gebr_gui_confirm_action_dialog(title, title,
						   _("Do you want to use your parameter's values as default values?"));

	gchar *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

	if (g_str_has_suffix (path, ".flwz") || g_str_has_suffix (path, ".flwx")) {
		GebrTar *tar;
		tar = gebr_tar_new_from_file (path);
		if (!gebr_tar_extract (tar))
			debr_message (GEBR_LOG_ERROR, _("Could not create menu from flow file %s"), path);
		else
			gebr_tar_foreach (tar, (GebrTarFunc) menu_create_from_flow, GINT_TO_POINTER (use_value));
		gebr_tar_free (tar);
	} else if (!menu_create_from_flow (path, use_value))
			debr_message (GEBR_LOG_ERROR, _("Could not create menu from flow file %s"), path);

	g_free (path);

out:
	gtk_widget_destroy(dialog);
}

void on_menu_add_folder_activate(void)
{
	GtkWidget *file_chooser;
	gchar *fname;

	file_chooser = gtk_file_chooser_dialog_new(_("Choose a folder"), GTK_WINDOW(debr.window),
						   GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
						   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						   GTK_STOCK_ADD, GTK_RESPONSE_OK,
						   NULL);
	if (gtk_dialog_run(GTK_DIALOG(file_chooser)) != GTK_RESPONSE_OK)
		goto err;
	
	fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));

	menu_open_folder(fname);

	g_free(fname);
err:
	gtk_widget_destroy(file_chooser);
}

void on_menu_remove_folder_activate(void)
{
	GtkTreeIter iter;
	if (menu_get_selected_type(&iter, FALSE) == ITER_FOLDER && menu_cleanup_folder(&iter))
		menu_close_folder(&iter);
}

gboolean on_menu_properties_activate(void)
{
	return menu_dialog_setup_ui(FALSE);
}

void on_menu_validate_activate(void)
{
	GtkTreeIter iter;

	gebr_gui_gtk_tree_view_foreach_selected(&iter, debr.ui_menu.tree_view)
		validate_menu(&iter);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(debr.notebook), NOTEBOOK_PAGE_VALIDATE);
}

void on_menu_install_activate(void)
{
	menu_install();
}

void on_menu_close_activate(void)
{
	GList *list = NULL;
	GList *saved_list = NULL;

	GtkTreeIter iter;
	gebr_gui_gtk_tree_view_foreach_selected(&iter, debr.ui_menu.tree_view) {
		MenuStatus status;
		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &iter, MENU_STATUS, &status, -1);
		if (status == MENU_STATUS_UNSAVED)
			list = g_list_prepend(list, gtk_tree_iter_copy(&iter));
		else
			saved_list = g_list_prepend(saved_list, gtk_tree_iter_copy(&iter));
	}
	list = g_list_reverse(list);

	if (menu_cleanup_iter_list(list)) {
		for (GList * i = saved_list; i != NULL; i = i->next)
			menu_close((GtkTreeIter*)i->data, FALSE);
	}

	g_list_foreach(list, (GFunc)gtk_tree_iter_free, NULL);
	g_list_free(list);
	g_list_foreach(saved_list, (GFunc)gtk_tree_iter_free, NULL);
	g_list_free(saved_list);
}

void on_program_new_activate(void)
{
	program_new();
}

void on_program_delete_activate(void)
{
	program_remove(TRUE);
}

gboolean on_program_properties_activate(void)
{
	menu_archive();
	if (!program_dialog_setup_ui(FALSE)) {
		menu_replace();
		return FALSE;
	}
	return TRUE;
}

void on_program_preview_activate(void)
{
	program_preview();
}

void on_program_top_activate(void)
{
	program_top();
}

void on_program_bottom_activate(void)
{
	program_bottom();
}

void on_program_copy_activate(void)
{
	program_copy();
}

void on_program_paste_activate(void)
{
	program_paste();
}

void on_parameter_delete_activate(void)
{
	parameter_remove(TRUE);
}

gboolean on_parameter_properties_activate(void)
{
	return parameter_properties(FALSE);
}

void on_parameter_top_activate(void)
{
	parameter_top();
}

void on_parameter_bottom_activate(void)
{
	parameter_bottom();
}

void on_parameter_type_activate(GtkRadioAction * first_action)
{
	gint type;
	type = gtk_radio_action_get_current_value(first_action);
	parameter_change_type((GebrGeoXmlParameterType)type);
}

void on_parameter_copy_activate(void)
{
	parameter_copy();
}

void on_parameter_cut_activate(void)
{
	parameter_cut();
}

void on_parameter_paste_activate(void)
{
	parameter_paste();
}

void on_validate_close_activate(void)
{
	validate_close();
}

void on_validate_clear_activate(void)
{
	validate_clear();
}

void on_configure_preferences_activate(void)
{
	preferences_dialog_setup_ui();
}

void on_help_contents_activate(void)
{
}

void on_help_about_activate(void)
{
	gtk_widget_show(debr.about.dialog);
}

void on_drop_down_menu_requested(GtkWidget * button, gpointer data)
{
	parameter_create_menu_with_types(GPOINTER_TO_INT(data));
}

static void on_icon_release(GtkEntry * entry, GtkEntryIconPosition pos, GdkEvent * event, GebrValidateCase * validate_case)
{
	gulong *id;
	id = g_object_get_data(G_OBJECT(entry), "icon-release-id");
	g_signal_handler_disconnect(entry, *id);
	g_object_set_data(G_OBJECT(entry), "icon-release-id", NULL);
	g_free(id);

	gchar *fix = gebr_validate_case_fix(validate_case, gtk_entry_get_text(entry));
	gtk_entry_set_text(entry, fix);
	g_free(fix);

	/* maybe we can fix it again... */
	on_entry_focus_out(entry, NULL, validate_case);
}

gboolean on_entry_focus_out(GtkEntry * entry, GdkEventFocus * event, GebrValidateCase * data)
{
	gchar * fixes;
	gboolean can_fix;

	if (!gebr_validate_case_check_value(data, gtk_entry_get_text(entry), NULL)) {
		gtk_entry_set_icon_from_stock(entry, GTK_ENTRY_ICON_SECONDARY, NULL);
		return FALSE;
	}

	GString *tooltip = g_string_new("");
	fixes = gebr_validate_case_automatic_fixes_msg(data, gtk_entry_get_text(entry), &can_fix);

	g_string_printf(tooltip, "%s\n\n%s", gebr_validate_case_get_message (data), fixes);
	gtk_entry_set_icon_from_stock(entry, GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_DIALOG_WARNING);
	if (can_fix) {
		gulong *id;
		if ((id = g_object_get_data(G_OBJECT(entry), "icon-release-id")) == NULL)
			id = g_new(gulong, 1);
		else
			g_signal_handler_disconnect(entry, *id);
		*id = g_signal_connect(entry, "icon-release", G_CALLBACK(on_icon_release), data);
		g_object_set_data(G_OBJECT(entry), "icon-release-id", id);

		gtk_entry_set_icon_tooltip_markup(entry, GTK_ENTRY_ICON_SECONDARY, tooltip->str);
	} else
		gtk_entry_set_icon_tooltip_markup(entry, GTK_ENTRY_ICON_SECONDARY, tooltip->str);

	g_free(fixes);
	g_string_free(tooltip, TRUE);

	return FALSE;
}

void on_menu_help_edit_clicked(void){
	menu_help_edit_clicked();
}

void on_menu_help_show_clicked(void){
	menu_help_show_clicked();
}

void on_program_help_show(void){
	program_help_show();
}

void on_program_help_edit(void){
	program_help_edit();
}
