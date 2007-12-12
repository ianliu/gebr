/*   GêBR ME - GêBR Menu Editor
 *   Copyright (C) 2007 GêBR core team (http://gebr.sourceforge.net)
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

#include "menu.h"
#include "support.h"
#include "gebrme.h"
#include "program.h"

void
menu_new(void)
{
	static int		new_count = 0;
	GString *		new_menu_str;
	GtkTreeIter		iter;
	GtkTreeSelection *	selection;

	new_menu_str = g_string_new(NULL);
	g_string_printf(new_menu_str, "%s%d.mnu", _("untitled"), ++new_count);

	gebrme.current = geoxml_flow_new();
	geoxml_document_set_author(GEOXML_DOC(gebrme.current), gebrme.config.name->str);
	geoxml_document_set_email(GEOXML_DOC(gebrme.current), gebrme.config.email->str);

	gtk_list_store_append(gebrme.menus_liststore, &iter);
	gtk_list_store_set(gebrme.menus_liststore, &iter,
			MENU_FILENAME, new_menu_str->str,
			MENU_XMLPOINTER, (gpointer)gebrme.current,
			MENU_PATH, "",
			-1);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebrme.menus_treeview));
	gtk_tree_selection_select_iter(selection, &iter);
	menu_selected();

	program_add();
	menu_saved_status_set(MENU_STATUS_UNSAVED);

	g_string_free(new_menu_str, TRUE);
}

void
menu_open(const gchar * path)
{
	gchar *			filename;
	GtkTreeIter		iter;
	GtkTreeSelection *	selection;
	GeoXmlDocument *	menu;
	int			ret;
	gboolean		valid;

	/* check if it is already open */
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebrme.menus_liststore), &iter);
	while (valid) {
		gchar *	ipath;

		gtk_tree_model_get (GTK_TREE_MODEL(gebrme.menus_liststore), &iter,
				MENU_PATH, &ipath,
				-1);

		if (!g_ascii_strcasecmp(ipath, path)) {
			selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebrme.menus_treeview));
			gtk_tree_selection_select_iter(selection, &iter);
			menu_selected();

			g_free(ipath);
			return;
		}
		g_free(ipath);
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(gebrme.menus_liststore), &iter);
	}

	/* try to load it */
	if ((ret = geoxml_document_load (&menu, path))) {
		/* TODO: */
		switch (ret) {
		case GEOXML_RETV_DTD_SPECIFIED:

		break;
		case GEOXML_RETV_INVALID_DOCUMENT:

		break;
		case GEOXML_RETV_CANT_ACCESS_FILE:

		break;
		case GEOXML_RETV_CANT_ACCESS_DTD:

		break;
		default:

		break;
		}
		return;
	}

	/* add to the view */
	filename = g_path_get_basename(path);
	gebrme.current = GEOXML_FLOW(menu);
	gtk_list_store_append (gebrme.menus_liststore, &iter);
	gtk_list_store_set (gebrme.menus_liststore, &iter,
			    MENU_FILENAME, filename,
			    MENU_XMLPOINTER, gebrme.current,
			    MENU_PATH, path,
			    -1);

	/* select it and load its contents into UI */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebrme.menus_treeview));
	gtk_tree_selection_select_iter(selection, &iter);
	menu_selected();

	g_free(filename);
}

void
menu_load_user_directory(void)
{
	DIR *		dir;
	struct dirent *	file;
	GString *	path;

	if ((dir = opendir(gebrme.config.menu_dir->str)) == NULL) {
		gebrme_message(ERROR, TRUE, TRUE, _("Could not open menus' directory"));
		return;
	}

	while ((file = readdir (dir)) != NULL){
		if (fnmatch ("*.mnu", file->d_name, 1))
			continue;

		path = g_string_new(gebrme.config.menu_dir->str);
		g_string_append(path, "/");
		g_string_append(path, file->d_name);
		menu_open(path->str);
		menu_saved_status_set(MENU_STATUS_SAVED);

		g_string_free(path, TRUE);
	}
	closedir(dir);
}

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
	filename = (gchar*)geoxml_document_get_filename(GEOXML_DOC(gebrme.current));
	geoxml_flow_get_program(gebrme.current, &program, index);
	while (program != NULL) {
		geoxml_program_set_menu(GEOXML_PROGRAM(program), filename, index++);
		geoxml_sequence_next(&program);
	}

	geoxml_document_save(GEOXML_DOC(gebrme.current), path);
	menu_saved_status_set(MENU_STATUS_SAVED);
}

void
menu_selected(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		menu_iter;
	GtkTreeIter		category_iter;

	GeoXmlSequence *	category;
	GeoXmlSequence *	program;
	GdkPixbuf *             icon;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (gebrme.menus_treeview));
	gtk_tree_selection_get_selected(selection, &model, &menu_iter);
	gtk_tree_model_get(GTK_TREE_MODEL(gebrme.menus_liststore), &menu_iter,
			    MENU_STATUS, &icon,
			    MENU_XMLPOINTER, &gebrme.current,
			    -1);

	/* load menu into widgets */
	gtk_entry_set_text(GTK_ENTRY(gebrme.title_entry),
		geoxml_document_get_title(GEOXML_DOC(gebrme.current)));
	gtk_entry_set_text(GTK_ENTRY(gebrme.description_entry),
		geoxml_document_get_description(GEOXML_DOC(gebrme.current)));
	gtk_entry_set_text(GTK_ENTRY(gebrme.author_entry),
		geoxml_document_get_author(GEOXML_DOC(gebrme.current)));
	gtk_entry_set_text(GTK_ENTRY(gebrme.email_entry),
		geoxml_document_get_email(GEOXML_DOC(gebrme.current)));

	/* categories */
	gtk_list_store_clear(gebrme.categories_liststore);
	geoxml_flow_get_category(gebrme.current, &category, 0);
	while (category != NULL) {
		gtk_list_store_append(gebrme.categories_liststore, &category_iter);
		gtk_list_store_set(gebrme.categories_liststore, &category_iter,
			CATEGORY_NAME, geoxml_category_get_name(GEOXML_CATEGORY(category)),
			CATEGORY_XMLPOINTER, (gpointer)category,
			-1);

		geoxml_sequence_next(&category);
	}

	/* clear program list */
	gtk_container_foreach(GTK_CONTAINER(gebrme.programs_vbox), (GtkCallback)gtk_widget_destroy, NULL);
	/* programs */
	geoxml_flow_get_program(gebrme.current, &program, 0);
	while (program != NULL) {
		program_create_ui(GEOXML_PROGRAM(program), FALSE);
		geoxml_sequence_next(&program);
	}

	/* recover status */
	gtk_list_store_set(GTK_LIST_STORE(gebrme.menus_liststore), &menu_iter,
			   MENU_STATUS, icon,
			   -1);
}

gboolean
menu_cleanup(void)
{
	GtkWidget *	dialog;

	GtkTreeIter	iter;

	GSList *	unsaved;
	gboolean	has_next;
	gboolean	has_unsaved;
	gboolean	ret;

	ret = TRUE;
	has_unsaved = FALSE;
	unsaved = g_slist_alloc();
	has_next = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gebrme.menus_liststore), &iter);
	while (has_next) {
		GdkPixbuf *	pixbuf;

		gtk_tree_model_get(GTK_TREE_MODEL(gebrme.menus_liststore), &iter,
				MENU_STATUS, &pixbuf,
				-1);
		if (pixbuf == gebrme.pixmaps.stock_no) {
			GtkTreeIter *	iter_pointer;

			iter_pointer = g_malloc(sizeof(GtkTreeIter));
			*iter_pointer = iter;
			/* FIXME: why doesn't work with prepend?? */
			unsaved = g_slist_append(unsaved, iter_pointer);
			has_unsaved = TRUE;
		}

		has_next = gtk_tree_model_iter_next(GTK_TREE_MODEL(gebrme.menus_liststore), &iter);
	}

	if (has_unsaved == FALSE)
		goto out;

	/* TODO: add cancel button */
	dialog = gtk_message_dialog_new(GTK_WINDOW(gebrme.window),
					GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_QUESTION,
					GTK_BUTTONS_YES_NO,
					_("There are flows unsaved. Do you want to save them?"));
	switch (gtk_dialog_run(GTK_DIALOG(dialog))) {
	case GTK_RESPONSE_YES: {
		GSList *	link;

		link = g_slist_last(unsaved);
		while (link != NULL) {
			GeoXmlFlow *	menu;
			gchar *		path;

			gtk_tree_model_get(GTK_TREE_MODEL(gebrme.menus_liststore), (GtkTreeIter*)link->data,
				MENU_XMLPOINTER, &menu,
				MENU_PATH, &path,
				-1);
			geoxml_document_save(GEOXML_DOC(menu), path);

			g_free(link->data);
			g_free(path);
			link = g_slist_next(link);
		}

		ret = TRUE;
		break;
	}
	case GTK_RESPONSE_NO:
		ret = TRUE;
		break;
	case GTK_RESPONSE_CANCEL:
		ret = FALSE;
		break;
	}

	gtk_widget_destroy(dialog);
out:	g_slist_free(unsaved);
	return ret;
}

void
menu_saved_status_set(MenuStatus status)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (gebrme.menus_treeview));
	gtk_tree_selection_get_selected(selection, &model, &iter);

	switch (status) {
	case MENU_STATUS_SAVED:
		gtk_list_store_set(GTK_LIST_STORE(gebrme.menus_liststore), &iter,
			MENU_STATUS, NULL,
			-1);
		break;
	case MENU_STATUS_UNSAVED:
		gtk_list_store_set(GTK_LIST_STORE(gebrme.menus_liststore), &iter,
			MENU_STATUS, gebrme.pixmaps.stock_no,
			-1);
		break;
	}
}
