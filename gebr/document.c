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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <time.h>
#include <unistd.h>
#include <stdlib.h>

#include <glib/gstdio.h>

#include <glib/gi18n.h>
#include <libgebr/utils.h>
#include <libgebr/date.h>
#include <libgebr/gui/gebr-gui-save-dialog.h>

#include "document.h"
#include "gebr.h"
#include "project.h"
#include "line.h"
#include "flow.h"
#include "menu.h"
#include "ui_help.h"
#include "gebr-report.h"

static GebrGeoXmlDocument *document_cache_check(const gchar *path)
{
	return g_hash_table_lookup(gebr.xmls_by_filename, path);
}

static void document_cache_add(const gchar *path, GebrGeoXmlDocument * document)
{
	GebrGeoXmlDocument *cached = document_cache_check(path);

	g_warn_if_fail(!cached || cached == document);

	g_hash_table_insert(gebr.xmls_by_filename, g_strdup(path), document);
}

GebrGeoXmlDocument *document_new(GebrGeoXmlDocumentType type)
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

int document_load(GebrGeoXmlDocument ** document, const gchar * filename, gboolean cache)
{
	return document_load_with_parent(document, filename, NULL, cache);
}

int document_load_with_parent(GebrGeoXmlDocument ** document, const gchar * filename, GtkTreeIter *parent, gboolean cache)
{
	int ret;
	GString *path;

	path = document_get_path(filename);
	ret = document_load_path_with_parent(document, path->str, parent, cache);
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
	ret = document_load_path_with_parent(document, path->str, parent, FALSE);
	g_string_free(path, TRUE);

	return ret;
}

int document_load_path(GebrGeoXmlDocument **document, const gchar * path)
{
	return document_load_path_with_parent(document, path, NULL, FALSE);
}

/*
 * Callback to remove menu reference from program to maintain backward compatibility
 */
static void
__document_discard_menu_ref_callback(GebrGeoXmlProgram * program, const gchar * filename, gint index)
{
	GebrGeoXmlFlow *menu;
	GebrGeoXmlSequence *menu_program;

	menu = menu_load_ancient(filename);
	if (menu == NULL)
		return;

	/* go to menu's program index specified in flow */
	gebr_geoxml_flow_get_program(menu, &menu_program, index);
	gchar *tmp_help_p = gebr_geoxml_program_get_help(GEBR_GEOXML_PROGRAM(menu_program));
	gebr_geoxml_program_set_help(program, tmp_help_p);
	g_free(tmp_help_p);

	document_free(GEBR_GEOXML_DOC(menu));
}	

/**
 * Remove the first reference (if found) to \p path from \p parent.
 * \p parent is a project or a line.
 */
static void
remove_parent_ref(GebrGeoXmlDocument *parent_document, const gchar *path)
{
	GebrGeoXmlSequence * sequence;
	GebrGeoXmlDocumentType type;

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
			document_save(parent_document, FALSE, FALSE);
			break;
		}
	}

	g_free(basename);
}

static const gchar *
get_document_name(GebrGeoXmlDocument *document)
{
	if (document == NULL)
		return _("document");

	switch (gebr_geoxml_document_get_type(document)) {
	case GEBR_GEOXML_DOCUMENT_TYPE_PROJECT: return _("project");
	case GEBR_GEOXML_DOCUMENT_TYPE_LINE: return _("line");
	case GEBR_GEOXML_DOCUMENT_TYPE_FLOW: return _("flow");
	default: return _("document");
	}
}

int document_load_path_with_parent(GebrGeoXmlDocument **document, const gchar * path, GtkTreeIter *parent, gboolean cache)
{
	if (cache) {
		*document = document_cache_check(path);
		if (*document)
			return GEBR_GEOXML_RETV_SUCCESS;
	}

	int ret = gebr_geoxml_document_load(document, path, TRUE, g_str_has_suffix(path, ".flw") ?
					    __document_discard_menu_ref_callback : NULL);

	if (ret == GEBR_GEOXML_RETV_SUCCESS) {
		if (cache)
			document_cache_add(path, *document);
		return GEBR_GEOXML_RETV_SUCCESS;
	}

	GString *string;
	GebrGeoXmlDocument *parent_document = NULL;
	gboolean free_document = FALSE;

	if (parent != NULL) {
		/* get XML (load if not yet loaded) from iter */
		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->store), parent,
				   PL_XMLPOINTER, &parent_document, -1);
		if (parent_document == NULL) {
			gchar *filename;

			gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->store), parent,
					   PL_FILENAME, &filename, -1);
			document_load(&parent_document, filename, FALSE);
			free_document = parent_document != NULL;

			g_free(filename);
		}
	}

	string = g_string_new("");

	if (ret == GEBR_GEOXML_RETV_FILE_NOT_FOUND || ret == GEBR_GEOXML_RETV_PERMISSION_DENIED) {
		/* the final point already comes with gebr_geoxml_error_string */
		gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Cannot load the file at %s: %s"),
			     path, gebr_geoxml_error_string((enum GEBR_GEOXML_RETV)ret));
		if (parent != NULL && ret == GEBR_GEOXML_RETV_FILE_NOT_FOUND)
			remove_parent_ref(parent_document, path);
		goto out;
	}

	const gchar *title = NULL;
	if (!gebr_geoxml_document_load(document, path, FALSE, NULL))
		title = gebr_geoxml_document_get_title(*document);

	/* don't do document recovery for menus */
	if (g_str_has_suffix(path, ".mnu")) {
		/* the final point already comes with gebr_geoxml_error_string */
		if (title != NULL)
			gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Cannot load menu '%s' at %s:\n%s"),
				     title, path, gebr_geoxml_error_string((enum GEBR_GEOXML_RETV)ret));
		else
			gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Cannot load the Menu at %s:\n%s"),
				     path, gebr_geoxml_error_string((enum GEBR_GEOXML_RETV)ret));

		goto out;
	}

	const gchar *document_name = get_document_name(*document);

	gchar *fname;
	fname = g_path_get_basename(path);
	g_string_printf(string, "Error loading '%s': %s",
			fname, gebr_geoxml_error_string((enum GEBR_GEOXML_RETV)ret));
	gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, string->str);

	GtkWidget *dialog;
	dialog = gtk_message_dialog_new_with_markup(GTK_WINDOW(gebr.window),
						    (GtkDialogFlags)(GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL),
						    GTK_MESSAGE_ERROR, GTK_BUTTONS_NONE, NULL); 
	g_string_printf(string, "Could not load %s", document_name);
	gtk_window_set_title(GTK_WINDOW(dialog), string->str); 

	if (title != NULL)
		g_string_printf(string, "Couldn't load %s '%s'", document_name, title);
	else
		g_string_printf(string, "Couldn't load %s", document_name);
	g_string_prepend(string, "<span weight='bold' size='large'>");
	g_string_append(string, "</span>");
	gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog), string->str);
	if (parent_document == NULL) 
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
							 _("The file %s could not be loaded.\n%s"),
							 fname, gebr_geoxml_error_string((enum GEBR_GEOXML_RETV)ret));
	else {
		GString *title = g_string_new("");
		if (gebr_geoxml_document_get_title(parent_document) != NULL)
			g_string_printf(title, " '%s'", gebr_geoxml_document_get_title(parent_document));
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
							 _("The file %s from %s%s (file %s) could not be loaded.\n%s"),
							 fname, get_document_name(parent_document), title->str,
							 gebr_geoxml_document_get_filename(parent_document),
							 gebr_geoxml_error_string((enum GEBR_GEOXML_RETV)ret));
	}

	/* for imported documents, simpler dialog is shown */
	if (!document_path_is_at_gebr_data_dir(path)) {
		gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_OK, 0);
		gtk_widget_show_all(dialog);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		goto out;
	}

	gtk_dialog_add_button(GTK_DIALOG(dialog), _("Ignore"), 0);
	GtkWidget * export = gtk_dialog_add_button(GTK_DIALOG(dialog), _("Export"), 1);
	gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(gtk_dialog_get_action_area(GTK_DIALOG(dialog))), export, TRUE);
	gtk_dialog_add_button(GTK_DIALOG(dialog), _("Delete"), 2);

	gtk_widget_show_all(dialog);
	gint response;
	gboolean keep_dialog = FALSE;

	do switch ((response = gtk_dialog_run(GTK_DIALOG(dialog)))) {
	case 1: { /* Export */
		gchar * export_path;
		GtkWidget *chooser_dialog;

		chooser_dialog = gebr_gui_save_dialog_new(_("Choose the filename to export"),
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
						     _("Failed to export the file '%s':\n%s."),
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
			gebr_geoxml_document_set_title(orphans_project, _("Orphan Lines"));

			gebr_geoxml_project_get_line(GEBR_GEOXML_PROJECT(*document), &project_line, 0);
			for (; project_line != NULL; gebr_geoxml_sequence_next(&project_line)) {
				const gchar *src;

				src = gebr_geoxml_project_get_line_source(GEBR_GEOXML_PROJECT_LINE(project_line));
				gebr_geoxml_project_append_line(GEBR_GEOXML_PROJECT(orphans_project), src);
			}
			document_save(orphans_project, TRUE, FALSE);
			project_load_with_lines(GEBR_GEOXML_PROJECT(orphans_project));

			break;
		} case GEBR_GEOXML_DOCUMENT_TYPE_LINE: {
			GebrGeoXmlSequence *line_flow;
			GebrGeoXmlDocument *orphans_line;

			orphans_line = document_new(GEBR_GEOXML_DOCUMENT_TYPE_LINE);
			gebr_geoxml_document_set_title(orphans_line, _("Orphan Flows"));

			gebr_geoxml_line_get_flow(GEBR_GEOXML_LINE(*document), &line_flow, 0);
			for (; line_flow != NULL; gebr_geoxml_sequence_next(&line_flow)) {
				const gchar *src;
				
				src = gebr_geoxml_line_get_flow_source(GEBR_GEOXML_LINE_FLOW(line_flow)); 
				gebr_geoxml_line_append_flow(GEBR_GEOXML_LINE(orphans_line), src);
			}
			document_save(orphans_line, TRUE, FALSE);

			if (parent == NULL) {
				free_document = TRUE;
				parent_document = document_new(GEBR_GEOXML_DOCUMENT_TYPE_PROJECT);
				gebr_geoxml_document_set_title(parent_document, _("Orphan Flows"));
			}

			gebr_geoxml_project_append_line(GEBR_GEOXML_PROJECT(parent_document),
							gebr_geoxml_document_get_filename(orphans_line));
			document_save(parent_document, TRUE, FALSE);

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
		document_free(*document);
		*document = NULL;
	}
	gtk_widget_destroy(dialog);

out:	g_string_free(string, TRUE);
	if (free_document)
		document_free(parent_document);
	return ret;
}

gboolean document_save_at(GebrGeoXmlDocument * document, const gchar * path, gboolean set_modified_date, gboolean cache, gboolean compress)
{
	gboolean ret = FALSE;

	if (set_modified_date)
		gebr_geoxml_document_set_date_modified(document, gebr_iso_date());

	ret = (gebr_geoxml_document_save(document, path, compress) == GEBR_GEOXML_RETV_SUCCESS);
	if (!ret)
		gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Failed to save the document '%s' at '%s'."),
			     gebr_geoxml_document_get_title(document), path);
	if (cache)
		document_cache_add(path, document);
	
	return ret;
}

gboolean document_save(GebrGeoXmlDocument * document, gboolean set_modified_date, gboolean cache)
{
	GString *path;
	gboolean ret = FALSE;

	path = document_get_path(gebr_geoxml_document_get_filename(document));
	ret = document_save_at(document, path->str, set_modified_date, cache, TRUE);
	g_string_free(path, TRUE);
	return ret;
}

static void
foreach_document_free(gpointer key, gpointer value, gpointer document)
{
	if (value == document)
		g_hash_table_remove(gebr.xmls_by_filename, key);
}

void document_free(GebrGeoXmlDocument * document)
{
	g_hash_table_foreach(gebr.xmls_by_filename, foreach_document_free, document);
	gebr_geoxml_document_free(document);
}

void document_import(GebrGeoXmlDocument * document, gboolean save)
{
	const gchar *extension;

	switch (gebr_geoxml_document_get_type(document)) {
	case GEBR_GEOXML_DOCUMENT_TYPE_FLOW: {
		gchar ***paths = gebr_geoxml_line_get_paths(gebr.line);
		extension = "flw";
		flow_set_paths_to_relative(GEBR_GEOXML_FLOW(document), gebr.line, paths, FALSE);
		break;
	}
	case GEBR_GEOXML_DOCUMENT_TYPE_LINE:
		extension = "lne";
		break;
	case GEBR_GEOXML_DOCUMENT_TYPE_PROJECT:
		extension = "prj";
		break;
	default:
		g_warn_if_reached();
		return;
	}
	GString *new_filename = document_assembly_filename(extension);
	gebr_geoxml_document_set_filename(document, new_filename->str);

	if (save) {
		GString *path = document_get_path(new_filename->str);
		document_save_at(document, path->str, FALSE, TRUE, TRUE);
		g_string_free(path, TRUE);
	}

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

gchar *
gebr_document_generate_report(GebrGeoXmlDocument *document)
{
	gboolean is_flow;

	if (gebr_geoxml_document_get_type(document) == GEBR_GEOXML_DOCUMENT_TYPE_FLOW)
		is_flow = TRUE;
	else
		is_flow = FALSE;

	GebrReport *report = gebr_report_new(document);
	gebr_report_set_maestro_controller(report, gebr.maestro_controller);
	gebr_report_set_validator(report, gebr.validator);
	gebr_report_set_dictionary_documents(report,
			GEBR_GEOXML_DOCUMENT(gebr.line),
			GEBR_GEOXML_DOCUMENT(gebr.project));

	if (is_flow) {
		gebr_report_set_css_url(report, gebr.config.detailed_flow_css->str);
		gebr_report_set_include_commentary(report, gebr.config.detailed_flow_include_report);
		gebr_report_set_include_revisions(report, gebr.config.detailed_flow_include_revisions_report);
		gebr_report_set_detailed_parameter_table(report, gebr.config.detailed_flow_parameter_table);
	} else {
		gebr_report_set_css_url(report, gebr.config.detailed_line_css->str);
		gebr_report_set_include_commentary(report, gebr.config.detailed_line_include_report);
		gebr_report_set_include_revisions(report, gebr.config.detailed_line_include_revisions_report);
		gebr_report_set_detailed_parameter_table(report, gebr.config.detailed_line_parameter_table);
		gebr_report_set_include_flow_report(report, gebr.config.detailed_line_include_flow_report);
	}

	return gebr_report_generate(report);
}
