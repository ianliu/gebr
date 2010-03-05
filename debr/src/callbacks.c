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

#include <libgebr/intl.h>
#include <libgebr/gui/utils.h>
#include <libgebr/gui/help.h>
#include <libgebr/utils.h>

#include "callbacks.h"
#include "defines.h"
#include "debr.h"
#include "preferences.h"
#include "parameter.h"

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
	for (int i = 0; debr.config.menu_dir[i]; i++)
		gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(chooser_dialog), debr.config.menu_dir[i], NULL);
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

	if (!menu_get_selected(&iter, TRUE)){
		debr_message(GEBR_LOG_INFO, _("No menu selected."));
		return;
	}
	if (!menu_save(&iter))
		on_menu_save_as_activate();
}

void on_menu_save_as_activate(void)
{
	GtkTreeIter iter;
	GtkTreeIter copy;
	GtkTreeIter child;
	GtkTreeIter parent;

	GtkWidget *chooser_dialog;
	GtkFileFilter *filefilter;

	gchar *dirname;
	gchar *filename;
	gchar *current_path;
	gchar *dirpath;
	GString *path;
	GString *title;
	gboolean valid;
	gboolean is_overwrite = FALSE;

	if (!menu_get_selected(&iter, TRUE)) {
		debr_message(GEBR_LOG_INFO, _("Menu not selected."));
		return;
	}

	title = g_string_new(NULL);
	g_string_printf(title, _("Choose file for \"%s\""), gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(debr.menu)));

	/* run file chooser */
	chooser_dialog = gtk_file_chooser_dialog_new(title->str, GTK_WINDOW(debr.window),
							 GTK_FILE_CHOOSER_ACTION_SAVE,
							 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
							 GTK_STOCK_SAVE, GTK_RESPONSE_YES, NULL);
	if (debr.config.menu_dir[0])
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser_dialog), debr.config.menu_dir[0]);
	for (int i = 0; debr.config.menu_dir[i]; i++)
		gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(chooser_dialog), debr.config.menu_dir[i], NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(chooser_dialog), TRUE);
	filefilter = gtk_file_filter_new();
	gtk_file_filter_set_name(filefilter, _("Menu files (*.mnu)"));
	gtk_file_filter_add_pattern(filefilter, "*.mnu");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_dialog), filefilter);

	gtk_tree_model_iter_parent(GTK_TREE_MODEL(debr.ui_menu.model), &parent, &iter);
	if (!gebr_gui_gtk_tree_iter_equal_to(&parent, &debr.ui_menu.iter_other)) {
		gchar *menu_path;
		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &parent, MENU_PATH, &menu_path, -1);
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser_dialog), menu_path);
		g_free(menu_path);
	} else if (debr.config.menu_dir && debr.config.menu_dir[0])
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser_dialog), debr.config.menu_dir[0]);

	gtk_widget_show(chooser_dialog);
	gboolean rerun = FALSE;
	path = g_string_new(NULL);
	do {
		if (gtk_dialog_run(GTK_DIALOG(chooser_dialog)) != GTK_RESPONSE_YES)
			goto out;

		gchar *tmp;
		tmp = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser_dialog));
		g_string_assign(path, tmp);
		gebr_append_filename_extension(path, ".mnu");
		if (strcmp(tmp, path->str))
			rerun = TRUE;	
		else
			rerun = FALSE;
		g_free(tmp);

		if (rerun && g_file_test(path->str, G_FILE_TEST_IS_REGULAR))
			if (gebr_gui_confirm_action_dialog(_(""), _("A file named \"%s\" already exists.\nDo you want to replace it?"), g_path_get_basename(path->str)))
				break;
	} while(rerun);
	filename = g_path_get_basename(path->str);

	/*Verify if another file with same name already exist*/

	menu_path_get_parent(path->str, &parent);
	valid = gtk_tree_model_iter_children(GTK_TREE_MODEL(debr.ui_menu.model), &child, &parent);
	while (valid) {
		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &child, MENU_PATH, &dirpath, -1);
		if (!dirpath)
			continue;

		if (strcmp(dirpath, path->str) == 0) {
			is_overwrite = TRUE;
			break;
		}
		g_free(dirpath);
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(debr.ui_menu.model), &child);
	}

	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &iter, MENU_PATH, &current_path, -1);

	// if the user saved on top of the same file...
	if (strcmp(current_path, path->str) == 0)
		menu_save(&iter);
	//...or saved over an other file...
	else if (is_overwrite) {
		GebrGeoXmlFlow * menu;
		filename = g_path_get_basename(dirpath);

		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &child, MENU_XMLPOINTER, &menu, -1);
		gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(menu));

		menu = GEBR_GEOXML_FLOW(gebr_geoxml_document_clone(GEBR_GEOXML_DOCUMENT(debr.menu)));
		menu_load_iter(path->str, &child, menu, TRUE);
		menu_save(&child);
		if (!strlen(current_path))
			gtk_tree_store_remove(debr.ui_menu.model, &iter);
	}
	//...or creating a new file
	else {
		gchar *label;
		GebrGeoXmlFlow * menu;

		menu_path_get_parent(path->str, &parent);
		if (gebr_gui_gtk_tree_iter_equal_to(&parent, &debr.ui_menu.iter_other)) {
			dirname = g_path_get_dirname(path->str);
			label = g_markup_printf_escaped("%s <span color='#666666'><i>%s</i></span>", filename, dirname);
			g_free(dirname);
		} else
			label = g_markup_printf_escaped("%s", filename);

		menu = GEBR_GEOXML_FLOW(gebr_geoxml_document_clone(GEBR_GEOXML_DOCUMENT(debr.menu)));

		gtk_tree_store_append(debr.ui_menu.model, &copy, &parent);
		gebr_gui_gtk_tree_model_iter_copy_values(GTK_TREE_MODEL(debr.ui_menu.model), &copy, &iter);
		gtk_tree_store_set(debr.ui_menu.model, &copy,
				   MENU_FILENAME, label,
				   MENU_PATH, path->str,
				   MENU_XMLPOINTER, menu,
				   -1);
		// if the item was a never saved file, remove it
		if (!strlen(current_path))
			gtk_tree_store_remove(debr.ui_menu.model, &iter);

		menu_select_iter(&copy);
		menu_save(&copy);

		g_free(label);
	}

	/* frees */
	g_string_free(path, TRUE);
	g_free(filename);
	g_free(current_path);

 out:	gtk_widget_destroy(chooser_dialog);
	g_string_free(title, TRUE);
}

void on_menu_save_all_activate(void)
{
	menu_save_all();
}

void on_menu_revert_activate(void)
{
	GtkTreeIter iter;

	if (gebr_gui_confirm_action_dialog
		(_("Revert changes"),
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
		/* revert to the one in disk */
		gebr_geoxml_document_free(GEBR_GEOXML_DOC(old_menu));
		gtk_tree_store_set(debr.ui_menu.model, &iter, MENU_XMLPOINTER, menu, -1);
		menu_status_set_from_iter(&iter, MENU_STATUS_SAVED);

		debr_message(GEBR_LOG_INFO, _("Menu reverted."));
		/* frees */
		g_free(path);
	}
	menu_selected();
}

void on_menu_delete_activate(void)
{
	GtkTreeIter iter;

	if (gebr_gui_confirm_action_dialog(_("Delete menu"), _("Are you sure you want to delete selected menu(s)?")) ==
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

			dialog = gtk_message_dialog_new(GTK_WINDOW(debr.window),
							GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
							_("Could not delete menu '%s'."),
							gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(menu)));
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
		} else{
			menu_close(&iter);
			gtk_notebook_set_current_page(GTK_NOTEBOOK(debr.notebook), NOTEBOOK_PAGE_MENU);
		}

		g_free(path);
	}

}

void on_menu_properties_activate(void)
{
	menu_dialog_setup_ui();
}

void on_menu_validate_activate(void)
{
	GtkTreeIter iter;

	gebr_gui_gtk_tree_view_foreach_selected(&iter, debr.ui_menu.tree_view)
		menu_validate(&iter);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(debr.notebook), NOTEBOOK_PAGE_VALIDATE);
}

void on_menu_install_activate(void)
{
	menu_install();
}

void on_menu_close_activate(void)
{
	GtkTreeIter iter;

	gebr_gui_gtk_tree_view_foreach_selected(&iter, debr.ui_menu.tree_view) {
		GebrGeoXmlFlow *menu;
		GtkWidget *button;
		MenuStatus status;

		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &iter,
				   MENU_XMLPOINTER, &menu, MENU_STATUS, &status, -1);

		if (status == MENU_STATUS_UNSAVED) {
			GtkWidget *dialog;
			gboolean cancel;

			cancel = FALSE;

			/* FIXME: gebr_geoxml_document_get_title returns empty string for never saved menus.
			   However, dialog should display temporary filename set.                        */
			dialog = gtk_message_dialog_new(GTK_WINDOW(debr.window),
							GTK_DIALOG_MODAL,
							GTK_MESSAGE_QUESTION,
							GTK_BUTTONS_NONE,
							_("'%s' menu has unsaved changes. Do you want to save it?"),
							(strlen
							 (gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(menu)))
							 ? gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(menu))
							 : _("Untitled")));
			button = gtk_dialog_add_button(GTK_DIALOG(dialog), _("Don't save"), GTK_RESPONSE_NO);
			g_object_set(G_OBJECT(button),
					 "image", gtk_image_new_from_stock(GTK_STOCK_NO, GTK_ICON_SIZE_BUTTON), NULL);
			gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
			gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_SAVE, GTK_RESPONSE_YES);
			switch (gtk_dialog_run(GTK_DIALOG(dialog))) {
			case GTK_RESPONSE_YES:{
					gchar *path;

					gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &iter,
							   MENU_PATH, &path, -1);
					gebr_geoxml_document_save(GEBR_GEOXML_DOC(debr.menu), path);
					g_free(path);
					break;
				}
			case GTK_RESPONSE_NO:
				break;
			default:	/* cancel or dialog destroy */
				cancel = TRUE;
				break;
			}

			gtk_widget_destroy(dialog);
			if (cancel == TRUE)
				return;
		}

		menu_close(&iter);
	}
}

void on_program_new_activate(void)
{
	program_new(TRUE);
}

void on_program_delete_activate(void)
{
	program_remove(TRUE);
}

gboolean on_program_properties_activate(void)
{
	return program_dialog_setup_ui();
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

void on_parameter_properties_activate(void)
{
	parameter_properties();
}

void on_parameter_top_activate(void)
{
	parameter_top();
}

void on_parameter_bottom_activate(void)
{
	parameter_bottom();
}

void on_parameter_change_type_activate(void)
{
	parameter_change_type_setup_ui();
}

void on_parameter_type_activate(GtkRadioAction * first_action)
{
	gint type;
	type = gtk_radio_action_get_current_value(first_action);
	parameter_change_type((enum GEBR_GEOXML_PARAMETER_TYPE)type);
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
	gebr_gui_help_show(DEBR_USERDOC_HTML, _("User's Manual"));
}

void on_help_about_activate(void)
{
	gtk_widget_show(debr.about.dialog);
}

gboolean on_parameter_tool_item_new_press(GtkWidget * tool_button)
{
	gint x, y;
	gint xb, yb, hb;
	GtkWidget * menu;

	menu = parameter_create_menu_with_types(FALSE);
	gdk_window_get_origin(tool_button->window, &x, &y);
	xb = tool_button->allocation.x;
	yb = tool_button->allocation.y;
	hb = tool_button->allocation.height;

	void popup_position(GtkMenu * menu, gint * xp, gint * yp, gboolean * push_in, gpointer user_data) {
		*xp = x + xb;
		*yp = y + yb + hb;
		*push_in = TRUE;
	}

	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, popup_position, NULL, 0, gtk_get_current_event_time());

	return FALSE;
}

void on_drop_down_menu_requested(GtkWidget * button, gpointer data)
{
	parameter_create_menu_with_types(GPOINTER_TO_INT(data));
}
