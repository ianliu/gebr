/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see
 *   <http://www.gnu.org/licenses/>.
 */

#include <time.h>
#include <unistd.h>
#include <stdlib.h>

#include <glib/gstdio.h>

#include <libgebr/intl.h>
#include <libgebr/utils.h>
#include <libgebr/date.h>
#include <libgebr/gui/gebr-gui-save-dialog.h>

#include "document.h"
#include "gebr.h"
#include "project.h"
#include "line.h"
#include "flow.h"
#include "menu.h"

GebrGeoXmlDocument *document_new(enum GEBR_GEOXML_DOCUMENT_TYPE type)
{
	gchar *extension;
	GString *filename;

	GebrGeoXmlDocument *document;
	GebrGeoXmlDocument *(*new_func) ();

	switch (type) {
	case GEBR_GEOXML_DOCUMENT_TYPE_FLOW:
		extension = "flw";
		new_func = (typeof(new_func)) gebr_geoxml_flow_new;
		break;
	case GEBR_GEOXML_DOCUMENT_TYPE_LINE:
		extension = "lne";
		new_func = (typeof(new_func)) gebr_geoxml_line_new;
		break;
	case GEBR_GEOXML_DOCUMENT_TYPE_PROJECT:
		extension = "prj";
		new_func = (typeof(new_func)) gebr_geoxml_project_new;
		break;
	default:
		return NULL;
	}

	/* finaly create it and... */
	document = new_func();

	/* then set filename */
	filename = document_assembly_filename(extension);
	gebr_geoxml_document_set_filename(document, filename->str);
	g_string_free(filename, TRUE);

	/* get today's date */
	gebr_geoxml_document_set_date_created(document, gebr_iso_date());

	return document;
}

gboolean document_path_is_at_gebr_data_dir(const gchar *path)
{
	return g_str_has_prefix(path, gebr.config.data->str);
}

gboolean document_file_is_at_gebr_data_dir(const gchar *filename)
{
	GString *path;
	gboolean ret;

	path = document_get_path(filename);
	ret = g_file_test(path->str, G_FILE_TEST_IS_REGULAR);

	g_string_free(path, TRUE);

	return ret;
}

gboolean document_is_at_gebr_data_dir(GebrGeoXmlDocument * document)
{
	return document_file_is_at_gebr_data_dir(gebr_geoxml_document_get_filename(document));
}

int document_load(GebrGeoXmlDocument ** document, const gchar * filename)
{
	return document_load_with_parent(document, filename, NULL);
}

int document_load_with_parent(GebrGeoXmlDocument ** document, const gchar * filename, GtkTreeIter *parent)
{
	int ret;
	GString *path;

	path = document_get_path(filename);
	ret = document_load_path_with_parent(document, path->str, parent);
	/* save will make a xml format upgrade if necessary */
	if (!ret)
		document_save(*document, FALSE);
	g_string_free(path, TRUE);

	return ret;
}

int document_load_at(GebrGeoXmlDocument ** document, const gchar * filename, const gchar * directory)
{
	return document_load_at_with_parent(document, filename, directory, NULL);
}

int document_load_at_with_parent(GebrGeoXmlDocument ** document, const gchar * filename, const gchar * directory,
				 GtkTreeIter *parent)
{
	int ret;
	GString *path;

	path = g_string_new("");
	g_string_printf(path, "%s/%s", directory, filename);
	ret = document_load_path_with_parent(document, path->str, parent);
	g_string_free(path, TRUE);

	return ret;
}

int document_load_path(GebrGeoXmlDocument **document, const gchar * path)
{
	return document_load_path_with_parent(document, path, NULL);
}

int document_load_path_with_parent(GebrGeoXmlDocument **document, const gchar * path, GtkTreeIter *parent)
{
	/**
	 * \internal
	 * Callback to remove menu reference from program to maintain backward compatibility
	 */
	void  __document_discard_menu_ref_callback(GebrGeoXmlProgram * program, const gchar * filename, gint index)
	{
		GebrGeoXmlFlow *menu;
		GebrGeoXmlSequence *menu_program;

		menu = menu_load_ancient(filename);
		if (menu == NULL)
			return;

		/* go to menu's program index specified in flow */
		gebr_geoxml_flow_get_program(menu, &menu_program, index);
		gebr_geoxml_program_set_help(program, gebr_geoxml_program_get_help(GEBR_GEOXML_PROGRAM(menu_program)));

		gebr_geoxml_document_free(GEBR_GEOXML_DOC(menu));
	}	

	/**
	 * \internal
	 * Remove the first reference (if found) to \p path from \p parent.
	 * \p parent is a project or a line.
	 */
	void remove_parent_ref(GebrGeoXmlDocument *parent_document, const gchar *path)
	{
		GebrGeoXmlSequence * sequence;
		enum GEBR_GEOXML_DOCUMENT_TYPE type;

		gchar *basename;
		basename = g_path_get_basename(path);

		type = gebr_geoxml_document_get_type(parent_document);
		if (type == GEBR_GEOXML_DOCUMENT_TYPE_PROJECT)
			gebr_geoxml_project_get_line(GEBR_GEOXML_PROJECT(parent_document), &sequence, 0);
		else if (type == GEBR_GEOXML_DOCUMENT_TYPE_LINE)
			gebr_geoxml_line_get_flow(GEBR_GEOXML_LINE(parent_document), &sequence, 0);
		else
			return;
		for (; sequence != NULL; gebr_geoxml_sequence_next(&sequence)) {
			const gchar *i_src;

			if (type == GEBR_GEOXML_DOCUMENT_TYPE_PROJECT)
				i_src = gebr_geoxml_project_get_line_source(GEBR_GEOXML_PROJECT_LINE(sequence));
			else
				i_src = gebr_geoxml_line_get_flow_source(GEBR_GEOXML_LINE_FLOW(sequence));
			if (!strcmp(i_src, basename)) {
				gebr_geoxml_sequence_remove(sequence);
				document_save(parent_document, FALSE);
				break;
			}
		}

		g_free(basename);
	}

	int ret; 
	if (!(ret = gebr_geoxml_document_load(document, path, TRUE, g_str_has_suffix(path, ".flw") ?
					      __document_discard_menu_ref_callback : NULL)))
		return ret;

	GtkDialog *dialog;
	GString *string;
	GebrGeoXmlDocument *parent_document;
	gboolean free_document = FALSE;

	if (parent != NULL) {
		/* get XML (load if not yet loaded) from iter */
		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->store), parent,
				   PL_XMLPOINTER, &parent_document, -1);
		if (parent_document == NULL) {
			gchar *filename;

			gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->store), parent,
					   PL_FILENAME, &filename, -1);
			document_load(&parent_document, filename);
			free_document = parent_document != NULL;

			g_free(filename);
		}
	}

	string = g_string_new("");

	if (ret == GEBR_GEOXML_RETV_FILE_NOT_FOUND || ret == GEBR_GEOXML_RETV_PERMISSION_DENIED) {
		/* the final point already comes with gebr_geoxml_error_string */
		gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Can't load file at %s: %s"),
			     path, gebr_geoxml_error_string((enum GEBR_GEOXML_RETV)ret));
		if (parent != NULL && ret == GEBR_GEOXML_RETV_FILE_NOT_FOUND)
			remove_parent_ref(parent_document, path);
		goto out;
	}

	if (!gebr_geoxml_document_load(document, path, FALSE, NULL)) {
		const gchar *title;
		title = gebr_geoxml_document_get_title(*document);
		g_string_printf(string, "%s", title != NULL ? title : "");
		if (title != NULL) {
			g_string_prepend(string, "'");
			g_string_append(string, "' ");
		}
	}

	/* don't do document recovery for menus */
	if (g_str_has_suffix(path, ".mnu")) {
		/* the final point already comes with gebr_geoxml_error_string */
		gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Can't load menu %s at %s:\n%s"),
			     string->str, path, gebr_geoxml_error_string((enum GEBR_GEOXML_RETV)ret));
		goto out;
	}

	const gchar *document_name;
	if (*document == NULL)
		document_name = _("document");
	else switch (gebr_geoxml_document_get_type(*document)) {
	case GEBR_GEOXML_DOCUMENT_TYPE_PROJECT:
		document_name = _("project");
		break;
	case GEBR_GEOXML_DOCUMENT_TYPE_LINE:
		document_name = _("line");
		break;
	case GEBR_GEOXML_DOCUMENT_TYPE_FLOW:
		document_name = _("flow");
		break;
	default:
		goto out;
	}

	dialog = GTK_DIALOG(gtk_message_dialog_new(GTK_WINDOW(gebr.window),
						   (GtkDialogFlags)(GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL),
						   GTK_MESSAGE_ERROR,
						   GTK_BUTTONS_NONE,
						   "Could not load %s %s at %s.\nError: %s\n", document_name,
						   string->str, path,
						   gebr_geoxml_error_string((enum GEBR_GEOXML_RETV)ret)));
	g_string_printf(string, "Couldn't load %s", document_name);
	gtk_window_set_title(GTK_WINDOW(dialog), string->str); 

	/* for imported documents */
	if (!document_path_is_at_gebr_data_dir(path)) {
		gtk_dialog_add_button(dialog, GTK_STOCK_OK, 0);
		gtk_widget_show_all(GTK_WIDGET(dialog));
		gtk_dialog_run(dialog);
		goto out;
	}

	gtk_dialog_add_button(dialog, _("Ignore"), 0);
	GtkWidget * export = gtk_dialog_add_button(dialog, _("Export"), 1);
	gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(dialog->action_area), export, TRUE);
	gtk_dialog_add_button(dialog, _("Delete"), 2);

	gtk_widget_show_all(GTK_WIDGET(dialog));
	gint response;
	gboolean keep_dialog = FALSE;
	do switch ((response = gtk_dialog_run(dialog))) {
	case 1: { /* Export */
		gchar * export_path;
		GtkWidget *chooser_dialog;

		chooser_dialog = gebr_gui_save_dialog_new(_("Choose filename to export"),
							  GTK_WINDOW(gebr.window));
		export_path = gebr_gui_save_dialog_run(GEBR_GUI_SAVE_DIALOG(chooser_dialog));
		if (export_path) {
			GString *cmd_line = g_string_new(NULL);
			g_string_printf(cmd_line, "cp -f %s %s", path, export_path);
			g_free(export_path);

			gchar *standard_error;
			if (g_spawn_command_line_sync(cmd_line->str, NULL, &standard_error,
						      NULL, NULL)) {
				if (strlen(standard_error)) 
					gebr_message(GEBR_LOG_ERROR, TRUE, TRUE,
						     _("Failed to export file '%s':\n%s."),
						     path, standard_error);
				g_free(standard_error);
			}
			g_string_free(cmd_line, TRUE);
		}
		keep_dialog = TRUE;

		break;
	} case 2: { /* Delete */
		ret = GEBR_GEOXML_RETV_FILE_NOT_FOUND;
		if (parent != NULL)
			remove_parent_ref(parent_document, path);
		unlink(path);

		if (*document == NULL) 
			break;

		switch (gebr_geoxml_document_get_type(*document)) {
		case GEBR_GEOXML_DOCUMENT_TYPE_PROJECT: {
			GebrGeoXmlSequence *project_line;
			GebrGeoXmlDocument *orphans_project;

			orphans_project = document_new(GEBR_GEOXML_DOCUMENT_TYPE_PROJECT);
			gebr_geoxml_document_set_title(orphans_project, _("Orphans lines"));

			gebr_geoxml_project_get_line(GEBR_GEOXML_PROJECT(*document), &project_line, 0);
			for (; project_line != NULL; gebr_geoxml_sequence_next(&project_line)) {
				const gchar *src;

				src = gebr_geoxml_project_get_line_source(GEBR_GEOXML_PROJECT_LINE(project_line));
				gebr_geoxml_project_append_line(GEBR_GEOXML_PROJECT(orphans_project), src);
			}
			document_save(orphans_project, TRUE);
			project_load_with_lines(GEBR_GEOXML_PROJECT(orphans_project));

			break;
		} case GEBR_GEOXML_DOCUMENT_TYPE_LINE: {
			GebrGeoXmlSequence *line_flow;
			GebrGeoXmlDocument *orphans_line;

			orphans_line = document_new(GEBR_GEOXML_DOCUMENT_TYPE_LINE);
			gebr_geoxml_document_set_title(orphans_line, _("Orphans flows"));

			gebr_geoxml_line_get_flow(GEBR_GEOXML_LINE(*document), &line_flow, 0);
			for (; line_flow != NULL; gebr_geoxml_sequence_next(&line_flow)) {
				const gchar *src;
				
				src = gebr_geoxml_line_get_flow_source(GEBR_GEOXML_LINE_FLOW(line_flow)); 
				gebr_geoxml_line_append_flow(GEBR_GEOXML_LINE(orphans_line), src);
			}
			document_save(orphans_line, TRUE);

			if (parent == NULL) {
				free_document = TRUE;
				parent_document = document_new(GEBR_GEOXML_DOCUMENT_TYPE_PROJECT);
				gebr_geoxml_document_set_title(parent_document, _("Orphans flows"));
			}

			gebr_geoxml_project_append_line(GEBR_GEOXML_PROJECT(parent_document),
							gebr_geoxml_document_get_filename(orphans_line));
			document_save(parent_document, TRUE);

			if (parent == NULL)
				project_load_with_lines(GEBR_GEOXML_PROJECT(parent_document));
			else
				project_append_line_iter(parent, GEBR_GEOXML_LINE(orphans_line));

			break;
		} default:
			break;
		}

		break;
	}
	default:
		keep_dialog = FALSE;
		break;
	} while (keep_dialog);

	/* frees */
	if (*document) {
		gebr_geoxml_document_free(*document);
		*document = NULL;
	}
	gtk_widget_destroy(GTK_WIDGET(dialog));

out:	g_string_free(string, TRUE);
	if (free_document)
		gebr_geoxml_document_free(parent_document);
	return ret;
}

gboolean document_save_at(GebrGeoXmlDocument * document, const gchar * path, gboolean set_modified_date)
{
	gboolean ret = FALSE;

	if (set_modified_date)
		gebr_geoxml_document_set_date_modified(document, gebr_iso_date());

	ret = (gebr_geoxml_document_save(document, path) == GEBR_GEOXML_RETV_SUCCESS);
	if (!ret)
		gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Failed to save document '%s' file at '%s'."),
			     gebr_geoxml_document_get_title(document), path);
	return ret;
}

gboolean document_save(GebrGeoXmlDocument * document, gboolean set_modified_date)
{
	GString *path;
	gboolean ret = FALSE;

	path = document_get_path(gebr_geoxml_document_get_filename(document));
	ret = document_save_at(document, path->str, set_modified_date);

	g_string_free(path, TRUE);
	return ret;
}

void document_import(GebrGeoXmlDocument * document)
{
	GString *path;
	GString *new_filename;
	const gchar *extension;

	switch (gebr_geoxml_document_get_type(document)) {
	case GEBR_GEOXML_DOCUMENT_TYPE_FLOW:
		extension = "flw";
		flow_set_paths_to(GEBR_GEOXML_FLOW(document), FALSE);
		break;
	case GEBR_GEOXML_DOCUMENT_TYPE_LINE:
		extension = "lne";
		line_set_paths_to(GEBR_GEOXML_LINE(document), FALSE);
		break;
	case GEBR_GEOXML_DOCUMENT_TYPE_PROJECT:
		extension = "prj";
		break;
	default:
		return;
	}
	new_filename = document_assembly_filename(extension);

	path = document_get_path(new_filename->str);
	gebr_geoxml_document_set_filename(document, new_filename->str);
	/* TODO: check save */
	document_save_at(document, path->str, FALSE);

	g_string_free(path, TRUE);
	g_string_free(new_filename, TRUE);
}

GString *document_assembly_filename(const gchar * extension)
{
	time_t t;
	struct tm *lt;
	gchar date[21];

	GString *filename;
	GString *path;
	GString *aux;
	gchar *basename;

	/* initialization */
	filename = g_string_new(NULL);
	aux = g_string_new(NULL);

	/* get today's date */
	time(&t);
	lt = localtime(&t);
	strftime(date, 20, "%Y_%m_%d", lt);

	/* create it */
	g_string_printf(aux, "%s/%s_XXXXXX.%s", gebr.config.data->str, date, extension);
	path = gebr_make_unique_filename(aux->str);

	/* get only what matters: the filename */
	basename = g_path_get_basename(path->str);
	g_string_assign(filename, basename);

	/* frees */
	g_string_free(path, TRUE);
	g_string_free(aux, TRUE);
	g_free(basename);

	return filename;
}

GString *document_get_path(const gchar * filename)
{
	GString *path;

	path = g_string_new(NULL);
	g_string_printf(path, "%s/%s", gebr.config.data->str, filename);

	return path;
}

void document_delete(const gchar * filename)
{
	GString *path;

	path = document_get_path(filename);
	g_unlink(path->str);

	g_string_free(path, TRUE);
}
