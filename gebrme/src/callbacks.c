/*   GÍBR ME - GÍBR Menu Editor
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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

#include "callbacks.h"
#include "gebrme.h"
#include "support.h"
#include "menu.h"
#include "preferences.h"

void
on_new_activate(void)
{
	menu_new();
}

void
on_open_activate(void)
{
	GtkWidget *		chooser_dialog;
	GtkFileFilter *		filefilter;
	gchar *			path;

	/* create file chooser */
	chooser_dialog = gtk_file_chooser_dialog_new(	_("Open flow"), NULL,
							GTK_FILE_CHOOSER_ACTION_OPEN,
							GTK_STOCK_OPEN, GTK_RESPONSE_YES,
							GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
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
	path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser_dialog));
	menu_open(path);

	g_free(path);
out:	gtk_widget_destroy(chooser_dialog);
}

void
on_save_activate(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	gchar *			path;

	/* get path of selection */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (gebrme.menus_treeview));
	gtk_tree_selection_get_selected(selection, &model, &iter);
	gtk_tree_model_get(GTK_TREE_MODEL(gebrme.menus_liststore), &iter,
		MENU_PATH, &path,
		-1);

	/* is this a new menu? */
	if (strlen(path)) {
		menu_save(path);
		g_free(path);
		return;
	}

	g_free(path);
	on_save_as_activate();
}

void
on_save_as_activate(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	GtkWidget *		chooser_dialog;
	GtkFileFilter *		filefilter;

	gchar *			path;
	gchar *			filename;

	/* run file chooser */
	chooser_dialog = gtk_file_chooser_dialog_new(_("Choose file"), GTK_WINDOW(gebrme.window),
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_SAVE, GTK_RESPONSE_YES,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(chooser_dialog), TRUE);
	filefilter = gtk_file_filter_new();
	gtk_file_filter_set_name(filefilter, _("System flow files (*.mnu)"));
	gtk_file_filter_add_pattern(filefilter, "*.mnu");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_dialog), filefilter);

	if (gebrme.config.menu_dir != NULL && strlen(gebrme.config.menu_dir->str) > 0){
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser_dialog), gebrme.config.menu_dir->str);
	}

	/* show file chooser */
	gtk_widget_show(chooser_dialog);
	if (gtk_dialog_run(GTK_DIALOG(chooser_dialog)) != GTK_RESPONSE_YES)
		goto out;
	path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser_dialog));
	filename = g_path_get_basename(path);

	/* get selection, change the view and save to disk */
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gebrme.menus_treeview));
	gtk_tree_selection_get_selected (selection, &model, &iter);
	gtk_list_store_set (gebrme.menus_liststore, &iter,
			MENU_FILENAME, filename,
			MENU_PATH, path,
			-1);
	geoxml_document_set_filename(GEOXML_DOC(gebrme.current), filename);
	menu_save(path);
	g_free(path);
	g_free(filename);

out:	gtk_widget_destroy(chooser_dialog);
}

void
on_revert_activate(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	gchar *			path;
	GeoXmlFlow *		menu;

	/* get path of selection */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (gebrme.menus_treeview));
	gtk_tree_selection_get_selected(selection, &model, &iter);
	gtk_tree_model_get(GTK_TREE_MODEL(gebrme.menus_liststore), &iter,
		MENU_PATH, &path,
		-1);

	/* is this a new menu? */
	if (!strlen(path)) {
		gebrme_message(ERROR, _("Menu was not saved yet."));
		g_free(path);
		return;
	}

	menu = menu_load(path);
	if (menu == NULL)
		return;
	/* revert to the one in disk */
	geoxml_document_free(GEOXML_DOC(gebrme.current));
	gtk_list_store_set(gebrme.menus_liststore, &iter,
		MENU_XMLPOINTER, menu,
		-1);
	menu_selected();
	menu_saved_status_set(MENU_STATUS_SAVED);

	/* frees */
	g_free(path);
}

void
on_delete_activate(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	gchar *			path;

	/* get path of selection */
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gebrme.menus_treeview));
	gtk_tree_selection_get_selected (selection, &model, &iter);
	gtk_tree_model_get (GTK_TREE_MODEL(gebrme.menus_liststore), &iter,
			MENU_PATH, &path,
			-1);

	if (g_unlink(path)) {
		GtkWidget *	dialog;

		dialog = gtk_message_dialog_new(GTK_WINDOW(gebrme.window),
					GTK_DIALOG_MODAL,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_OK,
					_("Could not delete flow"));
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	} else
		on_close_activate();

	g_free(path);
}

void
on_close_activate(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	GdkPixbuf *		pixbuf;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebrme.menus_treeview));
	gtk_tree_selection_get_selected(selection, &model, &iter);

	gtk_tree_model_get(GTK_TREE_MODEL(gebrme.menus_liststore), &iter,
				MENU_STATUS, &pixbuf,
				-1);

	if (pixbuf == gebrme.pixmaps.stock_no) {
		GtkWidget *	dialog;
		gboolean	cancel;

		/* TODO: add cancel button */
		cancel = FALSE;
		dialog = gtk_message_dialog_new(GTK_WINDOW(gebrme.window),
					GTK_DIALOG_MODAL,
					GTK_MESSAGE_QUESTION,
					GTK_BUTTONS_YES_NO,
					_("This flow has unsaved changes. Do you want to save it?"));
		switch (gtk_dialog_run(GTK_DIALOG(dialog))) {
		case GTK_RESPONSE_YES: {
			gchar *	path;

			gtk_tree_model_get(GTK_TREE_MODEL(gebrme.menus_liststore), &iter,
				MENU_PATH, &path,
				-1);

			geoxml_document_save(GEOXML_DOC(gebrme.current), path);
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

	geoxml_document_free(GEOXML_DOC(gebrme.current));
	gtk_list_store_remove(gebrme.menus_liststore, &iter);

	if (gtk_tree_model_iter_n_children(model, NULL) == 0)
		menu_new();
	else {
		GtkTreeIter	first_iter;

		gtk_tree_model_iter_nth_child(model, &first_iter, NULL, 0);
		gtk_tree_selection_select_iter(selection, &first_iter);
		menu_selected();
	}
}

void
on_quit_activate(void)
{
	gebrme_quit();
}

void
on_cut_activate(void)
{

}

void
on_copy_activate(void)
{

}

void
on_paste_activate(void)
{

}

void
on_preferences_activate(void)
{
	create_preferences_window();
}

void
on_about_activate(void)
{
	gtk_widget_show(gebrme.about.dialog);
}
