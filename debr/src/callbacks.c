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

#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>

#include <gui/utils.h>
#include <misc/utils.h>

#include "callbacks.h"
#include "debr.h"
#include "support.h"
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
void
do_navigation_bar_update(void)
{
	GString *	markup;

	markup = g_string_new(NULL);
	g_string_append(markup, g_markup_printf_escaped("<i>%s</i>",
		geoxml_document_get_title(GEOXML_DOC(debr.menu))));
	if (debr.program != NULL)
		g_string_append(markup, g_markup_printf_escaped(" :: <i>%s</i>",
			geoxml_program_get_title(debr.program)));
	if (debr.parameter != NULL)
		g_string_append(markup, g_markup_printf_escaped(" :: <i>%s</i>",
			geoxml_parameter_get_label(debr.parameter)));

	gtk_label_set_markup(GTK_LABEL(debr.navigation_box_label), markup->str);

	g_string_free(markup, TRUE);
}

/*
 * Function: on_menu_new_activate
 * Call <menu_new>
 */
void
on_menu_new_activate(void)
{
	menu_new();
}

/*
 * Function: on_menu_open_activate
 * Create a open dialog and manage it to open a menu
 */
void
on_menu_open_activate(void)
{
	GtkWidget *		chooser_dialog;
	GtkFileFilter *		filefilter;
	gchar *			path;

	/* create file chooser */
	chooser_dialog = gtk_file_chooser_dialog_new(_("Open flow"), NULL,
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_YES,
		NULL);
	filefilter = gtk_file_filter_new();
	gtk_file_filter_set_name(filefilter, _("System flow files (*.mnu)"));
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
void
on_menu_save_activate(void)
{
	GtkTreeIter		iter;

	gchar *			path;

	/* get path of selection */
	menu_get_selected(&iter);
	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.list_store), &iter,
		MENU_PATH, &path,
		-1);

	/* is this a new menu? */
	if (strlen(path)) {
		menu_save(path);
		g_free(path);
		return;
	}

	g_free(path);
	on_menu_save_as_activate();
}

/*
 * Function: on_menu_save_as_activate
 * Open a save dialog; get the path and save the menu at it
 */
void
on_menu_save_as_activate(void)
{
	GtkTreeIter		iter;

	GtkWidget *		chooser_dialog;
	GtkFileFilter *		filefilter;

	GString *		path;
	gchar *			tmp;
	gchar *			filename;

	/* run file chooser */
	chooser_dialog = gtk_file_chooser_dialog_new(_("Choose file"), GTK_WINDOW(debr.window),
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE, GTK_RESPONSE_YES,
		NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(chooser_dialog), TRUE);
	filefilter = gtk_file_filter_new();
	gtk_file_filter_set_name(filefilter, _("System flow files (*.mnu)"));
	gtk_file_filter_add_pattern(filefilter, "*.mnu");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_dialog), filefilter);
	if (debr.config.menu_dir != NULL && strlen(debr.config.menu_dir->str) > 0)
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser_dialog), debr.config.menu_dir->str);

	/* show file chooser */
	gtk_widget_show(chooser_dialog);
	if (gtk_dialog_run(GTK_DIALOG(chooser_dialog)) != GTK_RESPONSE_YES)
		goto out;

	/* path and file */
	tmp = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser_dialog));
	path = g_string_new(tmp);
	append_filename_extension(path, ".mnu");
	filename = g_path_get_basename(path->str);

	/* get selection, change the view and save to disk */
	menu_get_selected(&iter);
	gtk_list_store_set(debr.ui_menu.list_store, &iter,
		MENU_FILENAME, filename,
		MENU_PATH, path,
		-1);
	geoxml_document_set_filename(GEOXML_DOC(debr.menu), filename);
	menu_save(path->str);

	/* frees */
	g_string_free(path, TRUE);
	g_free(tmp);
	g_free(filename);

out:	gtk_widget_destroy(chooser_dialog);
}

/*
 * Function: on_menu_save_all_activate
 * Call <menu_save_all>
 */
void
on_menu_save_all_activate(void)
{
	menu_save_all();
}

/*
 * Function: on_menu_new_activate
 * Confirm action and if confirmed reload menu from file
 */
void
on_menu_revert_activate(void)
{
	GtkTreeIter		iter;

	gchar *			path;
	GeoXmlFlow *		menu;

	if (confirm_action_dialog(_("Revert changes"), _("All unsaved changes will be lost. Are you sure you want to revert flow '%s'?"),
	geoxml_document_get_filename(GEOXML_DOC(debr.menu))) == FALSE)
		return;

	/* get path of selection */
	menu_get_selected(&iter);
	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.list_store), &iter,
		MENU_PATH, &path,
		-1);

	/* is this a new menu? */
	if (!strlen(path)) {
		debr_message(LOG_ERROR, _("Menu was not saved yet."));
		g_free(path);
		return;
	}

	menu = menu_load(path);
	if (menu == NULL)
		return;
	/* revert to the one in disk */
	geoxml_document_free(GEOXML_DOC(debr.menu));
	gtk_list_store_set(debr.ui_menu.list_store, &iter,
		MENU_XMLPOINTER, menu,
		-1);
	menu_selected();
	menu_saved_status_set(MENU_STATUS_SAVED);

	/* frees */
	g_free(path);
}

/*
 * Function: on_menu_new_activate
 * Confirm action and if confirm delete it from the disk and call <on_menu_close_activate>
 */
void
on_menu_delete_activate(void)
{
	GtkTreeIter		iter;

	gchar *			path;

	if (confirm_action_dialog(_("Delete flow"), _("Are you sure you delete flow '%s'?"),
	geoxml_document_get_filename(GEOXML_DOC(debr.menu))) == FALSE)
		return;

	/* get path of selection */
	menu_get_selected(&iter);
	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.list_store), &iter,
		MENU_PATH, &path,
		-1);
	
	if ((strlen(path)) && (g_unlink(path))) {
		GtkWidget *	dialog;

		dialog = gtk_message_dialog_new(GTK_WINDOW(debr.window),
			GTK_DIALOG_MODAL,
			GTK_MESSAGE_ERROR,
			GTK_BUTTONS_OK,
			_("Could not delete flow"));
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	} else
		on_menu_close_activate();

	g_free(path);
}

/*
 * Function: on_menu_new_activate
 * Call <menu_remove>
 */
void
on_menu_properties_activate(void)
{
	menu_dialog_setup_ui();
}

/*
 * Function: on_menu_new_activate
 * Delete menu from the view.
 */
void
on_menu_close_activate(void)
{
	GtkTreeIter		iter;

	GdkPixbuf *		pixbuf;

	menu_get_selected(&iter);
	gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.list_store), &iter,
		MENU_STATUS, &pixbuf,
		-1);

	if (pixbuf == debr.pixmaps.stock_no) {
		GtkWidget *	dialog;
		gboolean	cancel;

		/* TODO: add cancel button */
		cancel = FALSE;
		dialog = gtk_message_dialog_new(GTK_WINDOW(debr.window),
					GTK_DIALOG_MODAL,
					GTK_MESSAGE_QUESTION,
					GTK_BUTTONS_YES_NO,
					_("This flow has unsaved changes. Do you want to save it?"));
		switch (gtk_dialog_run(GTK_DIALOG(dialog))) {
		case GTK_RESPONSE_YES: {
			gchar *	path;

			gtk_tree_model_get(GTK_TREE_MODEL(debr.ui_menu.list_store), &iter,
				MENU_PATH, &path,
				-1);

			geoxml_document_save(GEOXML_DOC(debr.menu), path);
			break;
		} case GTK_RESPONSE_NO:
			break;
		case GTK_RESPONSE_CANCEL:
			cancel = TRUE;
			return;
		}

		gtk_widget_destroy(dialog);
		if (cancel == TRUE)
			return;
	}

	geoxml_document_free(GEOXML_DOC(debr.menu));
	if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(debr.ui_menu.list_store), NULL) == 0)
		menu_new();
	else
		gtk_tree_view_select_sibling(GTK_TREE_VIEW(debr.ui_menu.tree_view));

	gtk_list_store_remove(debr.ui_menu.list_store, &iter);
}

/*
 * Function: on_program_new_activate
 * Call <program_new>
 */
void
on_program_new_activate(void)
{
	program_new(TRUE);
}

/*
 * Function: on_program_new_activate
 * Call <program_remove>
 */
void
on_program_delete_activate(void)
{
	program_remove();
}

/*
 * Function: on_program_new_activate
 * Call <program_remove>
 */
void
on_program_properties_activate(void)
{
	program_dialog_setup_ui();
}

/*
 * Function: on_program_top_activate
 * Call <program_top>
 */
void
on_program_top_activate(void)
{
	program_top();
}

/*
 * Function: on_program_bottom_activate
 * Call <program_bottom>
 */
void
on_program_bottom_activate(void)
{
	program_bottom();
}

/*
 * Function: on_parameter_new_activate
 * Call <parameter_new>
 */
void
on_parameter_new_activate(void)
{
	parameter_new();
}

/*
 * Function: on_parameter_delete_activate
 * Call <parameter_remove>
 */
void
on_parameter_delete_activate(void)
{
	parameter_remove();
}

/*
 * Function: on_parameter_duplicate_activate
 * Call <parameter_duplicate>
 */
void
on_parameter_duplicate_activate(void)
{
	parameter_duplicate();
}

/*
 * Function: on_parameter_new_activate
 * Call <parameter_remove>
 */
void
on_parameter_properties_activate(void)
{
	parameter_dialog_setup_ui();
}

/*
 * Function: on_parameter_top_activate
 * Call <parameter_top>
 */
void
on_parameter_top_activate(void)
{
	parameter_top();
}

/*
 * Function: on_parameter_bottom_activate
 * Call <parameter_bottom>
 */
void
on_parameter_bottom_activate(void)
{
	parameter_bottom();
}

/*
 * Function: on_parameter_change_type_activate
 * Call <parameter_remove>
 */
void
on_parameter_change_type_activate(void)
{
	parameter_change_type_setup_ui();
}

/*
 * Function: on_parameter_type_activate
 * Call <parameter_change_type>
 */
void
on_parameter_type_activate(GtkRadioAction * first_action)
{
	parameter_change_type((enum GEOXML_PARAMETERTYPE)gtk_radio_action_get_current_value(first_action));
}

/*
 * Function: on_configure_preferences_activate
 * Call <preferences_dialog_setup_ui>
 */
void
on_configure_preferences_activate(void)
{
	preferences_dialog_setup_ui();
}

/*
 * Function: on_help_about_activate
 * Show debr.about.dialog
 */
void
on_help_about_activate(void)
{
	gtk_widget_show(debr.about.dialog);
}
