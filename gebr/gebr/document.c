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

#include <glib/gi18n.h>
#include <libgebr/utils.h>
#include <libgebr/date.h>
#include <libgebr/defines.h>
#include <libgebr/gui/gebr-gui-save-dialog.h>

#include "document.h"
#include "gebr.h"
#include "project.h"
#include "line.h"
#include "flow.h"
#include "menu.h"
#include "ui_help.h"

static GebrGeoXmlDocument *document_cache_check(const gchar *path)
{
	return g_hash_table_lookup(gebr.xmls_by_filename, path);
}

static void document_cache_add(const gchar *path, GebrGeoXmlDocument * document)
{
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

int document_load_path_with_parent(GebrGeoXmlDocument **document, const gchar * path, GtkTreeIter *parent, gboolean cache)
{
	/*
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

		document_free(GEBR_GEOXML_DOC(menu));
	}	

	/**
	 * \internal
	 * Remove the first reference (if found) to \p path from \p parent.
	 * \p parent is a project or a line.
	 */
	void remove_parent_ref(GebrGeoXmlDocument *parent_document, const gchar *path)
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

	if (cache && (*document = document_cache_check(path)) != NULL) {
		return GEBR_GEOXML_RETV_SUCCESS;
	}
	int ret; 
	if (!(ret = gebr_geoxml_document_load(document, path, TRUE, g_str_has_suffix(path, ".flw") ?
					      __document_discard_menu_ref_callback : NULL))) {
		if (cache)
			document_cache_add(path, *document);
		return ret;
	}

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
			document_load(&parent_document, filename, FALSE);
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

	const gchar *title = NULL;
	if (!gebr_geoxml_document_load(document, path, FALSE, NULL))
		title = gebr_geoxml_document_get_title(*document);

	/* don't do document recovery for menus */
	if (g_str_has_suffix(path, ".mnu")) {
		/* the final point already comes with gebr_geoxml_error_string */
		if (title != NULL)
			gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Can't load menu '%s' at %s:\n%s"),
				     title, path, gebr_geoxml_error_string((enum GEBR_GEOXML_RETV)ret));
		else
			gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Can't load menu at %s:\n%s"),
				     path, gebr_geoxml_error_string((enum GEBR_GEOXML_RETV)ret));

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
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
						 _("The file %s could not be loaded.\n%s"),
						 fname, gebr_geoxml_error_string((enum GEBR_GEOXML_RETV)ret));

	/* for imported documents */
	if (!document_path_is_at_gebr_data_dir(path)) {
		gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_OK, 0);
		gtk_widget_show_all(dialog);
		gtk_dialog_run(GTK_DIALOG(dialog));
		goto out;
	}

	gtk_dialog_add_button(GTK_DIALOG(dialog), _("Ignore"), 0);
	GtkWidget * export = gtk_dialog_add_button(GTK_DIALOG(dialog), _("Export"), 1);
	gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(GTK_DIALOG(dialog)->action_area), export, TRUE);
	gtk_dialog_add_button(GTK_DIALOG(dialog), _("Delete"), 2);

	gtk_widget_show_all(dialog);
	gint response;
	gboolean keep_dialog = FALSE;
	do switch ((response = gtk_dialog_run(GTK_DIALOG(dialog)))) {
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
			document_save(orphans_project, TRUE, FALSE);
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
			document_save(orphans_line, TRUE, FALSE);

			if (parent == NULL) {
				free_document = TRUE;
				parent_document = document_new(GEBR_GEOXML_DOCUMENT_TYPE_PROJECT);
				gebr_geoxml_document_set_title(parent_document, _("Orphans flows"));
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

gboolean document_save_at(GebrGeoXmlDocument * document, const gchar * path, gboolean set_modified_date, gboolean cache)
{
	gboolean ret = FALSE;

	if (set_modified_date)
		gebr_geoxml_document_set_date_modified(document, gebr_iso_date());

	ret = (gebr_geoxml_document_save(document, path) == GEBR_GEOXML_RETV_SUCCESS);
	if (!ret)
		gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Failed to save document '%s' file at '%s'."),
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
	ret = document_save_at(document, path->str, set_modified_date, cache);

	g_string_free(path, TRUE);
	return ret;
}

void document_free(GebrGeoXmlDocument * document)
{
	void foreach(gpointer key, gpointer value)
	{
		if (value == (gpointer)document)
			g_hash_table_remove(gebr.xmls_by_filename, key);
	}
	g_hash_table_foreach(gebr.xmls_by_filename, (GHFunc)foreach, NULL);

	gebr_geoxml_document_free(document);
}

void document_import(GebrGeoXmlDocument * document)
{
	GString *path;
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
	GString *new_filename = document_assembly_filename(extension);
	path = document_get_path(new_filename->str);
	gebr_geoxml_document_set_filename(document, new_filename->str);
	g_string_free(new_filename, TRUE);

	document_save_at(document, path->str, FALSE, TRUE);

	g_string_free(path, TRUE);
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

static GList *
generate_list_from_match_info(GMatchInfo * match)
{
	GList * list = NULL;
	while (g_match_info_matches(match)) {
		list = g_list_prepend(list, g_match_info_fetch(match, 0));
		g_match_info_next(match, NULL);
	}
	return g_list_reverse(list);
}

GList * gebr_document_report_get_styles(const gchar * report)
{
	GList * list;
	GRegex * links;
	GRegex * styles;
	GMatchInfo * match;

	links = g_regex_new("<\\s*link\\s+rel\\s*=\\s*\"stylesheet\"[^>]*>",
			    G_REGEX_DOTALL, 0, NULL);

	styles = g_regex_new("<style[^>]*>.*?<\\/style>",
			     G_REGEX_DOTALL, 0, NULL);

	g_regex_match(links, report, 0, &match);
	list = generate_list_from_match_info(match);
	g_match_info_free(match);

	g_regex_match(styles, report, 0, &match);
	list = g_list_concat(list, generate_list_from_match_info(match));
	g_match_info_free(match);

	g_regex_unref(links);
	g_regex_unref(styles);

	return list;
}

gchar * gebr_document_report_get_styles_string(const gchar * report)
{
	GList * list, * i;
	GString * string;

	list = gebr_document_report_get_styles(report);

	if (list)
		string = g_string_new(list->data);
	else
		return g_strdup ("");

	i = list->next;
	while (i) {
		g_string_append_c(string, '\n');
		g_string_append(string, i->data);
		i = i->next;
	}

	g_list_foreach(list, (GFunc)g_free, NULL);
	g_list_free(list);

	return g_string_free(string, FALSE);
}

gchar * gebr_document_report_get_inner_body(const gchar * report)
{
	GRegex * body;
	gchar * inner_body;
	GMatchInfo * match;

	body = g_regex_new("<body[^>]*>(.*?)<\\/body>",
			   G_REGEX_DOTALL, 0, NULL);

	if (!g_regex_match(body, report, 0, &match)) {
		g_regex_unref(body);
		return NULL;
	}

	inner_body = g_match_info_fetch(match, 1);

	g_match_info_free(match);
	g_regex_unref(body);
	return inner_body;
}

gchar * gebr_document_generate_report (GebrGeoXmlDocument *document)
{
	GebrGeoXmlSequence *line_flow;
	GebrGeoXmlObjectType type;
	const gchar * title;
	const gchar * report;
	gchar * detailed_html = "";
	gchar * inner_body = "";
	gchar * styles = "";
	gchar * header = "";
	GString * content;

	content = g_string_new (NULL);

	type = gebr_geoxml_object_get_type(GEBR_GEOXML_OBJECT(document));
	if (type == GEBR_GEOXML_OBJECT_TYPE_PROGRAM) 
		return NULL;

	title = gebr_geoxml_document_get_title(document);
	report = gebr_geoxml_document_get_help(document);
	inner_body = gebr_document_report_get_inner_body(report);

	if (type == GEBR_GEOXML_OBJECT_TYPE_LINE) {
		if (gebr.config.detailed_line_css->len != 0)
			styles = g_strdup_printf ("<link rel=\"stylesheet\" type=\"text/css\" href=\"file://%s/%s\" />",
						  LIBGEBR_STYLES_DIR, gebr.config.detailed_line_css->str);
		else
			styles = gebr_document_report_get_styles_string(report);

		header = gebr_line_generate_header(document);

		if (gebr.config.detailed_line_include_report && inner_body)
			g_string_append (content, inner_body);

		if (gebr.config.detailed_line_include_flow_report) {
			gebr_geoxml_line_get_flow(GEBR_GEOXML_LINE(document), &line_flow, 0);

			for (; line_flow != NULL; gebr_geoxml_sequence_next(&line_flow)) {
				GebrGeoXmlFlow *flow;
				gboolean include_table;
				const gchar *filename = gebr_geoxml_line_get_flow_source(GEBR_GEOXML_LINE_FLOW(line_flow));

				include_table = gebr.config.detailed_line_parameter_table != GEBR_PARAM_TABLE_NO_TABLE;
				document_load((GebrGeoXmlDocument**)(&flow), filename, FALSE);
				gchar * flow_cont = gebr_flow_get_detailed_report(flow, include_table, FALSE);
				g_string_append(content, flow_cont);
				g_free(flow_cont);
				gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
			}
		}
	} else if (type == GEBR_GEOXML_OBJECT_TYPE_FLOW) {
		if (gebr.config.detailed_flow_css->len != 0)
			styles = g_strdup_printf ("<link rel=\"stylesheet\" type=\"text/css\" href=\"file://%s/%s\" />",
						  LIBGEBR_STYLES_DIR, gebr.config.detailed_flow_css->str);
		else
			styles = gebr_document_report_get_styles_string (report);

		header = gebr_flow_generate_header(GEBR_GEOXML_FLOW(document), TRUE);

		if (gebr.config.detailed_flow_include_report && inner_body)
			g_string_append_printf (content, "<div class='gebr-geoxml-flow'>%s</div>\n", inner_body);

		gchar * params;
		if (gebr.config.detailed_flow_parameter_table == GEBR_PARAM_TABLE_NO_TABLE)
			params = g_strdup("");
		else
			params = gebr_flow_generate_parameter_value_table (GEBR_GEOXML_FLOW (document));
		g_string_append_printf (content, "<div class='gebr-geoxml-flow'>%s</div>\n", params);
		g_free (params);
	} else if (type == GEBR_GEOXML_OBJECT_TYPE_PROJECT) {
		g_free (inner_body);
		return g_strdup (report);
	} else {
		g_free (inner_body);
		g_return_val_if_reached (NULL);
	}

	detailed_html = gebr_generate_report(title, styles, header, content->str);

	g_free(header);
	g_free(styles);
	g_free(inner_body);
	g_string_free(content, TRUE);

	return detailed_html;
}

gchar * gebr_document_get_css_header_field (const gchar * filename, const gchar * field)
{
	GRegex * regex = NULL;
	GMatchInfo * match_info = NULL;
	gchar * search_pattern = NULL;
	GError * error = NULL;
	gchar * contents = NULL;
	gchar * word = NULL;
	GString * escaped_pattern = NULL;
	GString * tmp_escaped = NULL;
	gint i = 0;

	escaped_pattern = g_string_new(g_regex_escape_string (field, -1));
	tmp_escaped = g_string_new("");

	/* g_regex_escape_string don't escape '-',
	 * so we need to do it manualy.
	 */
	for (i = 0; escaped_pattern->str[i] != '\0'; i++)
		if (escaped_pattern->str[i]  != '-')
			tmp_escaped = g_string_append_c(tmp_escaped, escaped_pattern->str[i]);
		else
			tmp_escaped = g_string_append(tmp_escaped, "\\-");
	
	g_string_printf (escaped_pattern, "%s", tmp_escaped->str);
	
	g_string_free(tmp_escaped, TRUE);

	search_pattern = g_strdup_printf("@%s:.*", escaped_pattern->str);

	g_file_get_contents (filename, &contents, NULL, &error);
	
	regex = g_regex_new (search_pattern, G_REGEX_CASELESS, 0, NULL);
	g_regex_match_full (regex, contents, -1, 0, 0, &match_info, &error);

	if (g_match_info_matches(match_info) == TRUE)
	{
		word = g_match_info_fetch (match_info, 0);
		word = g_strstrip(word);
		g_regex_unref(regex);
		search_pattern = g_strdup_printf("[^@%s:].*", escaped_pattern->str);
		regex = g_regex_new (search_pattern, G_REGEX_CASELESS, 0, NULL);
		g_regex_match_full (regex, word, -1, 0, 0, &match_info, &error);
		word = g_match_info_fetch (match_info, 0);
		word = g_strstrip(word);
	}
	
	g_free(search_pattern);	
	g_free(contents);	
	g_match_info_free (match_info);
	g_regex_unref (regex);
	g_string_free(escaped_pattern, TRUE);
	if (error != NULL)
	{
		g_printerr ("Error while matching: %s\n", error->message);
		g_error_free (error);
	}

	return word;
}
