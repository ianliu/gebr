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

/*
 * File: callbacks.c
 * General interface callbacks. See <interface.c>
 */

/*
 * Function: do_navigation_bar_update
 * Callback to update navigation bar content
 * according to menu, program and parameter selected
 */
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

/*
 * Function: on_new_activate
 * Select new target depending on the context
 */
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
		on_parameter_new_activate();
		break;
	default:
		break;
	}
}

/*
 * Function: on_copy_activate
 * Select copy target depending on the context
 */
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

/*
 * Function: on_paste_activate
 * Select paste target depending on the context
 */
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

/*
 * Function: on_quit_activate
 * Call <debr_quit>
 */
void on_quit_activate(void)
{
	debr_quit();
}

/*
 * Function: on_menu_new_activate
 * Call <menu_new>
 */
void on_menu_new_activate(void)
{
	menu_new(TRUE);
}

/*
 * Function: on_menu_open_activate
 * Create a open dialog and manage it to open a menu
 */
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

	g_free(path);
 out:	gtk_widget_destroy(chooser_dialog);
}

/*
 * Function: on_menu_save_activate
 * Save a menu. If it doesn't have a path assigned, call <on_menu_save_as_activate>
 */
void on_menu_save_activate(void)
{
	GtkTreeIter iter;

	if (!menu_get_selected(&iter, TRUE))
		return;
	if (!menu_save(&iter))
		on_menu_save_as_activate();
}

/*
 * Function: on_menu_save_as_activate
 * Open a save dialog; get the path and save the menu at it
 */
void on_menu_save_as_activate(void)
{
	GtkTreeIter iter;
	GtkTreeIter copy;
	GtkTreeIter parent;

	GtkWidget *chooser_dialog;
	GtkFileFilter *filefilter;

	GString *path;
	gchar *tmp;
	gchar *dirname;
	gchar *filename;
	gchar *current_path;

	if (!menu_get_selected(&iter, TRUE))
		return;

	/* run file chooser */
	chooser_dialog = gtk_file_chooser_dialog_new(_("Choose file"), GTK_WINDOW(debr.window),
						     GTK_FILE_CHOOSER_ACTION_SAVE,
						     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						     GTK_STOCK_SAVE, GTK_RESPONSE_YES, NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(chooser_dialog), TRUE);
	filefilter = gtk_file_filter_new();
	gtk_file_filter_set_name(filefilter, _("Menu files (*.mnu)"));
	gtk_file_filter_add_pattern(filefilter, "*.mnu");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_dialog), filefilter);
	tmp = NULL;
	gtk_tree_model_iter_parent(GTK_TREE_MODEL(debr.ui_menu.model), &parent, &iter);
	if (!gebr_gui_gtk_tree_iter_equal_to(&parent, &debr.ui_menu.iter_other))
		gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &parent, MENU_PATH, &tmp, -1);
	else if (debr.config.menu_dir && debr.config.menu_dir[0])
		tmp = debr.config.menu_dir[0];
	if (tmp)
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser_dialog), tmp);

	/* show file chooser */
	gtk_widget_show(chooser_dialog);
	if (gtk_dialog_run(GTK_DIALOG(chooser_dialog)) != GTK_RESPONSE_YES)
		goto out;

	/* Build path */
	tmp = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser_dialog));
	path = g_string_new(tmp);
	gebr_append_filename_extension(path, ".mnu");

	/* Get filename */
	filename = g_path_get_basename(path->str);

	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.model), &iter, MENU_PATH, &current_path, -1);

	// if the user saved on top of the same file
	if (strcmp(current_path, path->str) == 0)
		menu_save(&iter);
	// else, append another iterator in the correct location
	else {
		gchar *label;
		menu_path_get_parent(path->str, &parent);
		if (gebr_gui_gtk_tree_iter_equal_to(&parent, &debr.ui_menu.iter_other)) {
			dirname = g_path_get_dirname(path->str);
			label = g_markup_printf_escaped("%s <span color='#666666'><i>%s</i></span>", filename, dirname);
			g_free(dirname);
		} else
			label = g_markup_printf_escaped("%s", filename);

		gtk_tree_store_append(debr.ui_menu.model, &copy, &parent);
		gebr_gui_gtk_tree_model_iter_copy_values(GTK_TREE_MODEL(debr.ui_menu.model), &copy, &iter);
		gtk_tree_store_set(debr.ui_menu.model, &copy, MENU_FILENAME, label, MENU_PATH, path->str, -1);
		// if the item was a never saved file, remove it
		if (!strlen(current_path))
			gtk_tree_store_remove(debr.ui_menu.model, &iter);
		menu_save(&copy);
		menu_select_iter(&copy);

		g_free(label);
	}

	/* frees */
	g_string_free(path, TRUE);
	g_free(tmp);
	g_free(filename);
	g_free(current_path);

 out:	gtk_widget_destroy(chooser_dialog);
}

/*
 * Function: on_menu_save_all_activate
 * Call <menu_save_all>
 */
void on_menu_save_all_activate(void)
{
	menu_save_all();
}

/*
 * Function: on_menu_new_activate
 * Confirm action and if confirmed reload menu from file
 */
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
		menu_saved_status_set_from_iter(&iter, MENU_STATUS_SAVED);

		/* frees */
		g_free(path);
	}
	menu_selected();
}

/*
 * Function: on_menu_new_activate
 * Confirm action and if confirm delete it from the disk and call <on_menu_close_activate>
 */
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
							_("Could not delete menu '%s'"),
							gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(menu)));
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
		} else
			menu_close(&iter);

		g_free(path);
	}

	if (menu_get_n_menus() == 0)
		menu_new(FALSE);
	// else
	//      gebr_gui_gtk_tree_view_select_sibling(GTK_TREE_VIEW(debr.ui_menu.tree_view));
}

/*
 * Function: on_menu_properties_activate
 * Call <menu_dialog_setup_ui>
 */
void on_menu_properties_activate(void)
{
	menu_dialog_setup_ui();
}

/*
 * Function: on_menu_validate_activate
 * Call <menu_validate>
 */
void on_menu_validate_activate(void)
{
	GtkTreeIter iter;

	gebr_gui_gtk_tree_view_foreach_selected(&iter, debr.ui_menu.tree_view)
	    menu_validate(&iter);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(debr.notebook), NOTEBOOK_PAGE_VALIDATE);
}

/*
 * Function: on_menu_install_activate
 * Call <menu_install>
 */
void on_menu_install_activate(void)
{
	menu_install();
}

/*
 * Function: on_menu_new_activate
 * Delete menu from the view.
 */
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

	// FIXME: Selecionar o próximo menu após fechar
	// if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(debr.ui_menu.model), NULL) == 0)
	//      menu_new(FALSE);
	// else
	//      gebr_gui_gtk_tree_view_select_sibling(GTK_TREE_VIEW(debr.ui_menu.tree_view));
}

/*
 * Function: on_program_new_activate
 * Call <program_new>
 */
void on_program_new_activate(void)
{
	program_new(TRUE);
}

/*
 * Function: on_program_new_activate
 * Call <program_remove>
 */
void on_program_delete_activate(void)
{
	program_remove();
}

/*
 * Function: on_program_new_activate
 * Call <program_remove>
 */
void on_program_properties_activate(void)
{
	program_dialog_setup_ui();
}

/*
 * Function: on_program_preview_activate
 * Call <program_preview>
 */
void on_program_preview_activate(void)
{
	program_preview();
}

/*
 * Function: on_program_top_activate
 * Call <program_top>
 */
void on_program_top_activate(void)
{
	program_top();
}

/*
 * Function: on_program_bottom_activate
 * Call <program_bottom>
 */
void on_program_bottom_activate(void)
{
	program_bottom();
}

/*
 * Function: on_program_copy_activate
 * Call <program_copy>
 */
void on_program_copy_activate(void)
{
	program_copy();
}

/*
 * Function: on_program_paste_activate
 * Call <program_paste>
 */
void on_program_paste_activate(void)
{
	program_paste();
}

/*
 * Function: on_parameter_new_activate
 * Call <parameter_new>
 */
void on_parameter_new_activate(void)
{
	parameter_new();
}

/*
 * Function: on_parameter_delete_activate
 * Call <parameter_remove>
 */
void on_parameter_delete_activate(void)
{
	parameter_remove(TRUE);
}

/*
 * Function: on_parameter_new_activate
 * Call <parameter_remove>
 */
void on_parameter_properties_activate(void)
{
	parameter_properties();
}

/*
 * Function: on_parameter_top_activate
 * Call <parameter_top>
 */
void on_parameter_top_activate(void)
{
	parameter_top();
}

/*
 * Function: on_parameter_bottom_activate
 * Call <parameter_bottom>
 */
void on_parameter_bottom_activate(void)
{
	parameter_bottom();
}

/*
 * Function: on_parameter_change_type_activate
 * Call <parameter_remove>
 */
void on_parameter_change_type_activate(void)
{
	parameter_change_type_setup_ui();
}

/*
 * Function: on_parameter_type_activate
 * Call <parameter_change_type>
 */
void on_parameter_type_activate(GtkRadioAction * first_action)
{
	parameter_change_type((enum GEBR_GEOXML_PARAMETER_TYPE)gtk_radio_action_get_current_value(first_action));
}

/*
 * Function: on_parameter_copy_activate
 * Call <parameter_copy>
 */
void on_parameter_copy_activate(void)
{
	parameter_copy();
}

/*
 * Function: on_parameter_paste_activate
 * Call <parameter_paste>
 */
void on_parameter_paste_activate(void)
{
	parameter_paste();
}

/*
 * Function: on_validate_close_activate
 * Call <validate_close>
 */
void on_validate_close_activate(void)
{
	validate_close();
}

/*
 * Function: on_validate_clear_activate
 * Call <validate_clear>
 */
void on_validate_clear_activate(void)
{
	validate_clear();
}

/*
 * Function: on_configure_preferences_activate
 * Call <preferences_dialog_setup_ui>
 */
void on_configure_preferences_activate(void)
{
	preferences_dialog_setup_ui();
}

/* Function: on_help_contents_activate
 *
 */
void on_help_contents_activate(void)
{
	gebr_gui_help_show(DEBR_USERDOC_HTML, debr.config.browser->str);
}

/*
 * Function: on_help_about_activate
 * Show debr.about.dialog
 */
void on_help_about_activate(void)
{
	gtk_widget_show(debr.about.dialog);
}
