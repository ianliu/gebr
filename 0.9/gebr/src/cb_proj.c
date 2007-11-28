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

/* File: cb_proj.c
 * Callbacks for the projects manipulation
 */
#include "cb_proj.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <fnmatch.h>

#include "gebr.h"
#include "callbacks.h"
#include "cb_line.h"

#define PROJDIR "/projects"
#define PROJIDX "/projects.idx"

/* Function: project_load
 * Load a project from its filename, handling errors
 */
GeoXmlProject *
project_load (gchar * path)
{
	GeoXmlDocument * doc;
	int ret;

	/* TODO: handle errors in a different way, maybe using statusbar */
	if ((ret = geoxml_document_load (&doc, path)))
		switch (ret) {
		case GEOXML_RETV_DTD_SPECIFIED:
			printf("dtd specified\n");
			break;
		case GEOXML_RETV_INVALID_DOCUMENT:
			printf("invalid document\n");
			break;
		case GEOXML_RETV_CANT_ACCESS_FILE:
			printf("can't access file\n");
			break;
		case GEOXML_RETV_CANT_ACCESS_DTD:
			printf("can't access dtd\n");
			break;
		default:
			printf("unspecified error\n");
			break;
		}

	return GEOXML_PROJECT(doc);
}

/* Function: projects_refresh
 * Reload the projets from the data directory
 */
void
projects_refresh (void)
{
	struct dirent *	file;
	DIR *		dir;

	if (access(W.pref.data_value->str, F_OK | R_OK | W_OK)) {
		log_message(INTERFACE, "Unable to access data directory", TRUE);
		return;
	}

	if (W.proj_line_selection_path != NULL) {
		gtk_tree_path_free(W.proj_line_selection_path);
		W.proj_line_selection_path = NULL;
	}
	if ((dir = opendir (W.pref.data_value->str)) == NULL)
		return;

	/* Remove any previous menus from the list */
	gtk_tree_store_clear (W.proj_line_store);

	while ((file = readdir (dir)) != NULL) {
		if (fnmatch ("*.prj", file->d_name, 1))
			continue;

		GeoXmlProject * prj;
		GString *projectfn;

		data_fname(file->d_name, &projectfn);
		prj = project_load(projectfn->str);
		g_string_free(projectfn, TRUE);

		if (prj == NULL)
			goto out;

		/* Gtk stuff */
		GtkTreeIter iproj, iline;

		gtk_tree_store_append (W.proj_line_store, &iproj, NULL);
		gtk_tree_store_set (W.proj_line_store, &iproj,
				PL_NAME, geoxml_document_get_title(GEOXML_DOC(prj)),
				PL_FILENAME, geoxml_document_get_filename(GEOXML_DOC(prj)),
				-1);

		GeoXmlProjectLine * project_line;
		geoxml_project_get_line(prj, &project_line, 0);
		while (project_line != NULL) {
			GeoXmlLine * line;
			/* full path to the project and line */
			GString *lne_path;
			data_fname(geoxml_project_get_line_source(project_line), &lne_path);
			line = line_load(lne_path->str);
			g_string_free(lne_path, TRUE);
			if (line == NULL) {
				geoxml_project_remove_line(prj, project_line);
				geoxml_document_save (GEOXML_DOC(prj), geoxml_document_get_filename(GEOXML_DOC(prj)));
				continue;
			}

			gtk_tree_store_append (W.proj_line_store, &iline, &iproj);
			gtk_tree_store_set (W.proj_line_store, &iline,
					PL_NAME, geoxml_document_get_title(GEOXML_DOC(line)),
					PL_FILENAME, geoxml_project_get_line_source(project_line),
					-1);

			geoxml_document_free(GEOXML_DOC(line));
			geoxml_project_next_line(&project_line);
		}

		/* reset part of flow GUI */
		gtk_list_store_clear(W.flow_store);
		gtk_list_store_clear(W.fseq_store);
		geoxml_document_free (GEOXML_DOC (flow));
		flow = NULL;
		flow_info_update ();

		geoxml_document_free (GEOXML_DOC(prj));
	}

out:
	closedir (dir);
}

/*
 * Function: project_new
 * Create a new project
 *
 */
void
project_new     (GtkMenuItem *menuitem,
		 gpointer     user_data)
{
   GeoXmlProject *prj;
   GtkTreeIter iter;
   time_t t;
   struct tm *lt;
   char filename[20];
   static const char * title = "New project";

   time (&t);
   lt = localtime (&t);

   strftime (filename, STRMAX, "%Y_%m", lt);
   strcat (filename, "_XXXXXX");
   mktemp (filename);
   strcat (filename, ".prj");

   prj = geoxml_project_new();
   geoxml_document_set_filename(GEOXML_DOC(prj), filename);
   geoxml_document_set_title(GEOXML_DOC(prj), title);
   geoxml_document_set_author (GEOXML_DOC(prj), W.pref.username_value->str);
   geoxml_document_set_email (GEOXML_DOC(prj), W.pref.email_value->str);

   gtk_tree_store_append (W.proj_line_store, &iter, NULL);
   gtk_tree_store_set (W.proj_line_store, &iter,
		       PL_NAME, title,
		       PL_FILENAME, filename,
		       -1);

   /* assembly the file path */
   GString *str;

   data_fname(filename, &str);
   geoxml_document_save(GEOXML_DOC(prj), str->str);
   geoxml_document_free(GEOXML_DOC(prj));
   g_string_free(str, TRUE);
}

/*
 * Function: project_delete
 * Delete the selected project
 *
 * TODO:
 * * If a project is not empty, the user user should be
 *   warned. Besides, it should be asked about erasing
 *   all project's lines.
 * * Project's line files should be deleted as well.
 */
void
project_delete     (GtkMenuItem *menuitem,
		    gpointer     user_data)
{
   GtkTreeIter        iter;
   GtkTreeSelection  *selection;
   GtkTreeModel      *model;
   static const char *no_project_selected_error = "No project selected";

   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (W.proj_line_view));

   if (gtk_tree_selection_get_selected (selection, &model, &iter)){
      GtkTreePath *path;
      gchar       *name, *filename;

      gtk_tree_model_get (model, &iter,
			  PL_NAME, &name,
     			  PL_FILENAME, &filename,
			  -1);
      path = gtk_tree_model_get_path (model, &iter);

      if (gtk_tree_path_get_depth(path) > 1)
	 log_message(INTERFACE, no_project_selected_error, TRUE);
      else {
	 char message[STRMAX];
	 int nlines;

	/* TODO: If this project is not empty,
	    prompt the user to take about erasing its lines */

	 /* Delete each line of the project */
	 if ((nlines = gtk_tree_model_iter_n_children (model, &iter)) > 0){
	    sprintf (message, "Project '%s' still has %i lines", name, nlines);
	    log_message(INTERFACE, message, TRUE);
	 }
	 else {
	    sprintf (message, "Erasing project '%s'", name);
	    log_message(INTERFACE, message, TRUE);

	    /* Remove the project from the store (and its children) */
	    gtk_tree_store_remove (GTK_TREE_STORE (W.proj_line_store), &iter);

	    /* finally, remove it from the disk */
	    GString *str;

	    data_fname(filename, &str);
	    unlink(str->str);
	    g_string_free(str, TRUE);

	 }
      }

      gtk_tree_path_free(path);
   }
   else
      log_message(INTERFACE, no_project_selected_error, TRUE);
}

/*
 * Function: proj_line_rename
 * Rename a projet or a line upon double click
 */
void
proj_line_rename  (GtkCellRendererText *cell,
		   gchar               *path_string,
		   gchar               *new_text,
		   gpointer             user_data)
{

   GtkTreeIter iter;

   if (gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (W.proj_line_store),
					    &iter, path_string)) {

      gchar          *filename;
      GString        *path;
      GeoXmlDocument *doc;

      gtk_tree_model_get ( GTK_TREE_MODEL (W.proj_line_store), &iter,
			   PL_FILENAME, &filename,
			   -1);

      data_fname(filename, &path);
      if (gtk_tree_store_iter_depth(W.proj_line_store, &iter) > 0) {
         doc = GEOXML_DOC(line_load(path->str));
         if (doc == NULL) {
	    log_message(INTERFACE, "Unable to access this line", TRUE);
	    goto out;
         }
      } else {
         doc = GEOXML_DOC(project_load(path->str));
         if (doc == NULL) {
	    log_message(INTERFACE, "Unable to access this project", TRUE);
	    goto out;
         }
      }

      /* change it on the xml. */
      geoxml_document_set_title(doc, new_text);
      geoxml_document_save(doc, path->str);
      geoxml_document_free(doc);

      /* stores changes */
      gtk_tree_store_set (W.proj_line_store, &iter, PL_NAME, new_text, -1);
out:
      g_free(filename);
      g_string_free(path, TRUE);
   }
}
