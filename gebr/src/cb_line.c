/*   GêBR - An environment for seismic processing.
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

/* File: cb_line.c
 * Callbacks for the lines manipulation
 */
#include "cb_line.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "gebr.h"
#include "callbacks.h"
#include "cb_flow.h"
#include "cb_proj.h"

const char *no_line_selected_error = "No line selected";
const char *no_selection_error     = "Nothing selected";

/* Function: line_load
 * Load a line from its path, handling errors
 */
GeoXmlLine *
line_load (gchar * path)
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

	return GEOXML_LINE(doc);
}

/*
 * Function: line_new
 * Create a new line
 *
 */
void
line_new     (GtkMenuItem *menuitem,
	      gpointer     user_data)
{
	GtkTreeIter		iter;
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	static const char *	no_project_selected_error = "Select a project to which a line will be added to";
	GtkTreePath *		iline_path;
	GtkTreeIter		iline;
	GtkTreePath *		path;
	gchar *			prj_filename;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (W.proj_line_view));
	if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
		log_message(INTERFACE, no_selection_error, TRUE);
		return;
	}

	gtk_tree_model_get (model, &iter,
				PL_FILENAME, &prj_filename,
				-1);
	path = gtk_tree_model_get_path (model, &iter);

	if (gtk_tree_path_get_depth(path) > 1) {
		log_message(INTERFACE, no_project_selected_error, TRUE);
		gtk_tree_path_free(path);
		g_free(prj_filename);
		return;
	}
	else {
		time_t t;
		struct tm *lt;
		char lne_filename[20];
		static const char * title = "New Line";

		time (&t);
		lt = localtime (&t);

		strftime (lne_filename, STRMAX, "%Y_%m", lt);
		strcat (lne_filename, "_XXXXXX");
		mktemp (lne_filename);
		strcat (lne_filename, ".lne");

		/* gtk stuff */
		gtk_tree_store_append (W.proj_line_store, &iline, &iter);
		gtk_tree_store_set (W.proj_line_store, &iline,
				PL_NAME, title,
				PL_FILENAME, lne_filename,
				-1);

		iline_path = gtk_tree_model_get_path (GTK_TREE_MODEL (W.proj_line_store),
						&iline);
		gtk_tree_view_expand_to_path (GTK_TREE_VIEW (W.proj_line_view), iline_path);

		/* full path to the project and line */
		GString *lne_path;
		GString *prj_path;

		data_fname(prj_filename, &prj_path);
		data_fname(lne_filename, &lne_path);

		/* libgeoxml stuff */
		GeoXmlProject *prj;
		GeoXmlLine    *lne;

		prj = project_load (prj_path->str);
		if (prj == NULL) {
		   printf("FIXME: %s:%d", __FILE__, __LINE__);
		   g_string_free(lne_path, TRUE);
		   g_string_free(prj_path, TRUE);
		   goto out;
		}
		geoxml_project_add_line (prj, lne_filename);
		geoxml_document_save (GEOXML_DOC(prj), prj_path->str);
		geoxml_document_free (GEOXML_DOC(prj));

		lne = geoxml_line_new ();
		geoxml_document_set_filename (GEOXML_DOC(lne), lne_filename);
		geoxml_document_set_title (GEOXML_DOC(lne), title);
		geoxml_document_set_author (GEOXML_DOC(lne), W.pref.username_value->str);
		geoxml_document_set_email (GEOXML_DOC(lne), W.pref.email_value->str);

		geoxml_document_save (GEOXML_DOC(lne), lne_path->str);
		geoxml_document_free (GEOXML_DOC(lne));

		g_string_free(lne_path, TRUE);
		g_string_free(prj_path, TRUE);

	}

out:
	gtk_tree_path_free(path);
	g_free(prj_filename);
	g_free(iline_path);
}

/*
 * Function: line_delete
 * Delete the selected line
 *
 * TODO:
 * * ask the user about erasing all flows associated to this line.
 */
void
line_delete     (GtkMenuItem *menuitem,
		 gpointer     user_data)
{
   GtkTreeIter        iter;
   GtkTreeSelection  *selection;
   GtkTreeModel      *model;

   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (W.proj_line_view));

   if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
      GtkTreePath *path;
      gchar       *name, *lne_filename;

      gtk_tree_model_get (model, &iter,
			  PL_NAME, &name,
			  PL_FILENAME, &lne_filename,
			  -1);
      path = gtk_tree_model_get_path (model, &iter);

      if (gtk_tree_path_get_depth(path) < 2)
	 log_message(INTERFACE, no_line_selected_error, TRUE);
      else {
	    GtkTreeIter  piter;
	    GString     *message;
	    GString     *lne_path;
	    GString     *prj_path;
	    gchar	*prj_filename;

	    gtk_tree_model_iter_parent(model, &piter, &iter);
	    gtk_tree_model_get (model, &piter,
				PL_FILENAME, &prj_filename,
				-1);

	    /* make the user happy */
	    message = g_string_new("Erasing line '");
	    g_string_append(message, name);
	    g_string_append(message, "'");

	    log_message(INTERFACE, message->str, TRUE);
	    g_string_free(message, TRUE);

	    /* Assembly paths */
	    data_fname(prj_filename, &prj_path);
	    data_fname(lne_filename, &lne_path);

	    /* Remove the line from its project */
	    GeoXmlProject * prj;

	    prj = project_load(prj_path->str);
	    if (prj == NULL) {
	       printf("FIXME: %s:%d", __FILE__, __LINE__);
	       goto out;
	    }

	    GeoXmlProjectLine * project_line;
	    geoxml_project_get_line(prj, &project_line, 0);
	    while (project_line != NULL) {
		if (g_ascii_strcasecmp(lne_filename, geoxml_project_get_line_source(project_line)) == 0) {
			geoxml_project_remove_line(prj, project_line);
			geoxml_document_save(GEOXML_DOC(prj), prj_path->str);
			break;
		}

		geoxml_project_next_line(&project_line);
	    }
	    geoxml_document_free(GEOXML_DOC(prj));

	    /* Removes its flows */
	    /* TODO: ask for user's confirmation */
	    GeoXmlLine * line;

	    line = line_load(lne_path->str);
	    if (line == NULL) {
	      printf("FIXME: %s:%d", __FILE__, __LINE__);
	      g_string_free(prj_path, TRUE);
	      g_string_free(lne_path, TRUE);
	      goto out;
	    }

	    GeoXmlLineFlow * line_flow;
	    geoxml_line_get_flow(line, &line_flow, 0);
	    while (line_flow != NULL) {
		GString *flw_path;

		data_fname(geoxml_line_get_flow_source(line_flow), &flw_path);
		unlink (flw_path->str);
		g_string_free(flw_path, TRUE);

		geoxml_line_next_flow(&line_flow);
	    }
	    geoxml_document_free(GEOXML_DOC(line));

	    /* finally, remove it from the disk and from the tree*/
	    gtk_tree_store_remove (GTK_TREE_STORE (W.proj_line_store), &iter);
	    unlink(lne_path->str);

	    /* Clear the flow list */
            gtk_list_store_clear (W.flow_store);
	    gtk_list_store_clear (W.fseq_store);
	    geoxml_document_free(GEOXML_DOC(flow));
	    flow = NULL;
	    flow_info_update ();
	    g_string_free(prj_path, TRUE);
	    g_string_free(lne_path, TRUE);
      }

out:
      gtk_tree_path_free(path);
      g_free(name);
      g_free(lne_filename);

   } else
	   log_message(INTERFACE, no_selection_error, TRUE);
}

/*
 * Function: line_load_flows
 * Load flows associated to the selected line.
 *
 */
void
line_load_flows (void)
{
	GtkTreeIter        iter;
	GtkTreePath       *path;
	GtkTreeSelection  *selection;
	GtkTreeModel      *model;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (W.proj_line_view));
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		GtkTreePath *	selection_path;
		GString *       lne_path;
		gchar *		name;
		gchar *		lne_filename;

		selection_path = gtk_tree_model_get_path(model, &iter);
		if (W.proj_line_selection_path != NULL && !gtk_tree_path_compare(selection_path, W.proj_line_selection_path)) {
			/* uhm, the same line/project is selected, don't need to do nothing */
			gtk_tree_path_free(selection_path);
			return;
		}
		gtk_tree_path_free(W.proj_line_selection_path);
		W.proj_line_selection_path = selection_path;

		/* reset part of GUI */
		gtk_list_store_clear(W.flow_store);
		gtk_list_store_clear(W.fseq_store);
		geoxml_document_free (GEOXML_DOC (flow));
		flow = NULL;
		flow_info_update ();

		/* check if the item is a project */
		path = gtk_tree_model_get_path (model, &iter);
		if (gtk_tree_path_get_depth(path) < 2)
			return;

		gtk_tree_model_get (model, &iter,
					PL_NAME, &name,
					PL_FILENAME, &lne_filename,
					-1);

		/* assembly paths */
		data_fname(lne_filename, &lne_path);

		/* iterate over its flows */
		/* TODO: ask for user's confirmation */
		GeoXmlLine * line;

		line = line_load(lne_path->str);
		g_string_free(lne_path, TRUE);

		if (line == NULL) {
			log_message(INTERFACE, "Unable to load line", TRUE);
			goto out;
		}

		GeoXmlLineFlow * line_flow;
		geoxml_line_get_flow(line, &line_flow, 0);
		while (line_flow != NULL) {
			GtkTreeIter  fiter;
			GeoXmlFlow  *flow;
			GString     *flw_path;
			const gchar *flw_filename = geoxml_line_get_flow_source(line_flow);

			data_fname(flw_filename, &flw_path);
			flow = flow_load_path (flw_path->str);
			g_string_free(flw_path, TRUE);
			if (flow == NULL) {
				geoxml_line_next_flow(&line_flow);
				continue;
			}

			/* FIXME: empty title leads to seg. fault! */
			/* add to the flow browser. */
			gtk_list_store_append (W.flow_store, &fiter);
			gtk_list_store_set (W.flow_store, &fiter,
					FB_NAME, geoxml_document_get_title(GEOXML_DOC(flow)),
					FB_FILENAME, flw_filename,
					-1);

			geoxml_document_free(GEOXML_DOC(flow));
			geoxml_line_next_flow(&line_flow);
		}

		log_message(INTERFACE, "Flows loaded", TRUE);

out:
		gtk_tree_path_free(path);
		g_free(name);
		g_free(lne_filename);
	} else {
		gtk_list_store_clear(W.fseq_store);
		geoxml_document_free (GEOXML_DOC (flow));
		flow = NULL;
		log_message(INTERFACE, no_selection_error, TRUE);
	}
}
