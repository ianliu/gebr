/*   GÍBR - An environment for seismic processing.
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

/*
 * File: menus.c
 */
#include "menus.h"

#include <geoxml.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>

#include "gebr.h"
#include "callbacks.h"
#include "ui_pages.h"
#include "ui_help.h"
#include "parse.h"
#include "cb_flow.h"

const gchar * no_menu_selected_error = "No menu selected";
const gchar * selected_menu_instead_error = "Select a menu instead of a category";

/*
 * Function: menus_populate
 * Read index and add menus from it to the view
 */
int
menus_populate (void)
{
   FILE *		menuindex_fp;
   gchar		fname[STRMAX];
   gchar		line[STRMAX];
   GtkTreeIter	category_iter;
   GtkTreeIter *	parent_iter;

   strcpy (fname, getenv ("HOME"));
   strcat (fname, "/.gebr/menus.idx");

   if ( (menuindex_fp = fopen(fname, "r")) == NULL ) {
      if (! menus_create_index ())
	 return EXIT_FAILURE;
      else
	 menuindex_fp = fopen(fname, "r");
   }

   /* Remove any previous menus from the list */
   gtk_tree_store_clear (W.menu_store);
   parent_iter = NULL;

   while (read_line(line, STRMAX, menuindex_fp)){
      gchar *	parts[5];
      GString * dummy;
      GString *	titlebf;

      desmembra(line, 4, parts);
      titlebf = g_string_new(NULL);
      g_string_printf(titlebf, "<b>%s</b>", parts[0]);

      if (menus_fname(parts[3], &dummy) == EXIT_SUCCESS) {
	 GtkTreeIter iter;

	 if (parts[0] == NULL || !strlen(parts[0]))
	    parent_iter = NULL;
	 else {
	    gchar * category;

	    if (parent_iter != NULL) {
	       gtk_tree_model_get ( GTK_TREE_MODEL(W.menu_store), parent_iter,
				    MENU_TITLE_COLUMN, &category,
				    -1);


	       /* different category? */
	       if (g_ascii_strcasecmp(category, titlebf->str)) {
		  gtk_tree_store_append (W.menu_store, &category_iter, NULL);


		  gtk_tree_store_set (W.menu_store, &category_iter,
				      MENU_TITLE_COLUMN, titlebf->str,
				      -1);

		  parent_iter = &category_iter;
	       }

	       g_free(category);
	    } else {
	       gtk_tree_store_append (W.menu_store, &category_iter, NULL);

	       gtk_tree_store_set (W.menu_store, &category_iter,
				   MENU_TITLE_COLUMN, titlebf->str,
				   -1);
	       parent_iter = &category_iter;
	    }
	 }

	 gtk_tree_store_append (W.menu_store, &iter, parent_iter);
	 gtk_tree_store_set (W.menu_store, &iter,
			     MENU_TITLE_COLUMN, parts[1],
			     MENU_DESC_COLUMN, parts[2],
			     MENU_FILE_NAME_COLUMN, parts[3],
			     -1);
      }
      g_string_free(titlebf, TRUE);
   }

   fclose (menuindex_fp);
   return EXIT_SUCCESS;
}

/*
 * Function: scan_dir
 * Scans user's and system menus' directories for menus
 */
void
scan_dir (const char *path, FILE *fp)
{
	DIR *		dir;
	struct dirent *	file;

	if ((dir = opendir(path)) == NULL)
		return;

	while ((file = readdir (dir)) != NULL){
		if (fnmatch ("*.mnu", file->d_name, 1))
			continue;

		GeoXmlDocument *	doc;
		GeoXmlCategory *	category;
		gchar *			category_str;
		int			ret;
		gchar			filename[STRMAX];

		sprintf(filename, "%s/%s", path, file->d_name);

		if ((ret = geoxml_document_load (&doc, filename))) {
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

		geoxml_flow_get_category(GEOXML_FLOW(doc), &category, 0);
		while ( category != NULL ){
		        category_str = (gchar *)geoxml_category_get_name(category);

			fprintf(fp, "%s|%s|%s|%s\n",
				category_str,
				geoxml_document_get_title(doc),
				geoxml_document_get_description(doc),
				geoxml_document_get_filename(doc) );

			geoxml_category_next(&category);
		}
		geoxml_document_free (doc);
	}
	closedir (dir);
}

/*
 * Function: menus_create_index
 * Create menus from found using scan_dir
 */
int
menus_create_index(void)
{

   gchar	fname[STRMAX];
   FILE *	menuindex;

   strcpy (fname, getenv ("HOME"));
   strcat (fname, "/.gebr/menus.idx");

   if ((menuindex = fopen(fname, "w")) == NULL ) {
      log_message(INTERFACE, "Unable to access user's menus directory", TRUE);
      return EXIT_FAILURE;
   }

   scan_dir(GEBRMENUSYS,   menuindex);
   scan_dir(W.pref.usermenus_value->str, menuindex);

   fclose(menuindex);

   /* Sort index */

   {
      char command[STRMAX];
      sprintf(command, "sort %s >/tmp/gebrmenus.tmp; mv /tmp/gebrmenus.tmp %s",
	      fname, fname);
      system(command);
   }

   return EXIT_SUCCESS;
}

/*
 * Function: menus_fname
 * Look for a given menu and fill in its path
 */
int
menus_fname   (const gchar  *menu,
	       GString     **fname  )
{   
   
   if (menu == NULL)
      return EXIT_FAILURE;   
   
   *fname = g_string_new(GEBRMENUSYS);
   g_string_append(*fname, "/");
   g_string_append(*fname, menu);

   if (access ((*fname)->str, F_OK) == 0)
      return EXIT_SUCCESS;

   g_string_free(*fname, TRUE);

   /* user's menus directory */
   *fname = g_string_new(W.pref.usermenus_value->str);
   g_string_append(*fname, "/");
   g_string_append(*fname, menu);

   if (access ((*fname)->str, F_OK) == 0)
      return EXIT_SUCCESS;

   g_string_free(*fname, TRUE);
   return EXIT_FAILURE;
}

/*
 * Function: menus_show_help
 * Show's menus help
 */
void
menu_show_help (void)
{
	GtkTreeIter		iter;
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;

	gchar *		        menu_fn;
	GString *	        menu_path;
	GeoXmlFlow *		menu;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (W.menu_view));
	if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
		log_message(INTERFACE, no_menu_selected_error, TRUE);
		return;
	}
	if (!gtk_tree_store_iter_depth(W.menu_store, &iter)) {
		log_message(INTERFACE, selected_menu_instead_error, TRUE);
		return;
	}

	gtk_tree_model_get ( GTK_TREE_MODEL (W.menu_store), &iter,
			MENU_FILE_NAME_COLUMN, &menu_fn,
			-1);

	if (menus_fname(menu_fn, &menu_path) != EXIT_SUCCESS)
		goto out;
	menu = flow_load_path (menu_path->str);
	if (menu == NULL)
		goto out;
	else
	   g_string_free(menu_path, TRUE);

	show_help( (gchar*)geoxml_document_get_help(GEOXML_DOC(menu)), "Menu help", (gchar*)geoxml_document_get_filename(GEOXML_DOC(menu)));

out:
	g_free(menu_fn);
}
