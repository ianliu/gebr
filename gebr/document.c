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

static void gebr_document_generate_flow_content(GebrGeoXmlDocument *document,
                                                GString *content,
                                                const gchar *inner_body,
                                                gboolean include_table,
                                                gboolean include_snapshots,
                                                gboolean include_comments,
                                                gboolean include_linepaths,
                                                const gchar *index,
                                                gboolean is_snapshot);

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

/**
 * gebr_document_report_get_styles:
 * @report: an html markup
 *
 * Returns: a list of strings containing all styles inside @report. It may return complete <style> tags and <link
 * rel='stylesheet' ... > tags. You must free all strings and the list itself.
 */
static GList *
gebr_document_report_get_styles(const gchar * report)
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

/**
 * gebr_document_report_get_styles_string:
 * See gebr_document_report_get_styles().
 * Returns: a newly allocated string containing the styles concatenated with line breaks.
 */
static gchar *
gebr_document_report_get_styles_string(const gchar * report)
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

/**
 * gebr_document_report_get_inner_body:
 * @report: an html markup
 *
 * Returns: a newly allocated string containing the inner
 * html of the body tag from @report.
 */
static gchar *
gebr_document_report_get_inner_body(const gchar * report)
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

gchar *gebr_document_report_get_styles_css(gchar *report,GString *css)
{
	gchar * styles = "";
	if (css->len != 0)
		styles = g_strdup_printf ("<link rel=\"stylesheet\" type=\"text/css\" href=\"file://%s/%s\" />",
					  LIBGEBR_STYLES_DIR, css->str);
	else
		styles = gebr_document_report_get_styles_string (report);

	return styles;
}

void gebr_document_create_section(GString *destiny,
                                  const gchar *source,
                                  const gchar *class_name)
{
	g_string_append_printf(destiny, "<div class=\"%s\">%s</div>\n ", class_name, source);
}

/**
 * gebr_document_generate_report:
 * @document:
 *
 * Returns: a newly allocated string containing the header for document
 */
static gchar *
gebr_document_generate_header(GebrGeoXmlDocument * document,
                                      gboolean is_internal,
                                      const gchar *index)
{
	if (index == NULL)
		index = "";
	GString * dump;
	GebrGeoXmlDocumentType type;

	type = gebr_geoxml_document_get_type (document);

	dump = g_string_new(NULL);

	/* Title and Description of Document */
	gchar *title = gebr_geoxml_document_get_title(document);

	gchar *description = gebr_geoxml_document_get_description(document);
	g_string_printf(dump,
	                "<a name=\"%s\"></a>\n"
			"<div class=\"title\">%s</div>\n"
			"<div class=\"description\">%s</div>\n",
			is_internal? index : "external", title, description);
	g_free(title);
	g_free(description);

	/* Credits */
	gchar *author = gebr_geoxml_document_get_author(document);
	gchar *email = gebr_geoxml_document_get_email(document);
	if (!*email)
		g_string_append_printf(dump,
		                       "<div class=\"credits\">\n"
		                       "  <span class=\"author\">%s</span>\n"
		                       "</div>\n",
		                       author);
	else
		g_string_append_printf(dump,
		                       "<div class=\"credits\">\n"
		                       "  <span class=\"author\">%s</span>\n"
		                       "  <span class=\"email\">%s</span>\n"
		                       "</div>\n",
		                       author, email);
	g_free(author);
	g_free(email);


	/* Date when generate this report */
	const gchar *date = gebr_localized_date(gebr_iso_date());
	g_string_append_printf(dump,
	                       "<div class=\"date\">%s%s</div>\n",
	                       _("Report generated at "),
	                       date);

	/* If Document is Line, include Maestro on header */
	if (type == GEBR_GEOXML_DOCUMENT_TYPE_LINE) {
		gchar *maestro = gebr_geoxml_line_get_maestro(GEBR_GEOXML_LINE(document));
		g_string_append_printf(dump,
		                       "<div class=\"maestro\">%s%s</div>\n",
		                       _("This line belongs to "),
		                       maestro);
		g_free(maestro);
	}

	return g_string_free(dump, FALSE);
}

static void
gebr_document_generate_project_index_content(GebrGeoXmlDocument *document,
                                             GString *index_content)
{
	GebrGeoXmlSequence *lines;

	gebr_geoxml_project_get_line(GEBR_GEOXML_PROJECT(document), &lines, 0);
	while (lines) {
		const gchar *lname;
		GebrGeoXmlDocument *line;

		lname = gebr_geoxml_project_get_line_source(GEBR_GEOXML_PROJECT_LINE(lines));
		document_load(&line, lname, FALSE);

		gchar *title = gebr_geoxml_document_get_title(line);
		gchar *link = g_strdup(title);
		link = g_strdelimit(link, " ", '_');
		gchar *description = gebr_geoxml_document_get_description(line);
		g_string_append_printf (index_content,
		                        "      <li>\n"
		                        "        <span class=\"title\"><a href=\"#%s\">%s</a></span> - \n"
		                        "        <span class=\"description\">%s</span>\n"
		                        "      </li>\n",
		                        link, title, description);
		g_free(title);
		g_free(description);
		g_free(link);
		gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(line));
		gebr_geoxml_sequence_next (&lines);
	}
}

static void
gebr_document_generate_line_index_content(GebrGeoXmlDocument *document,
                                          GString *index_content)
{
	gint i = 1;
	GebrGeoXmlSequence *flows;

	gebr_geoxml_line_get_flow (GEBR_GEOXML_LINE (document), &flows, 0);
	while (flows) {
		const gchar *fname;
		GebrGeoXmlDocument *flow;

		fname = gebr_geoxml_line_get_flow_source (GEBR_GEOXML_LINE_FLOW (flows));
		document_load(&flow, fname, FALSE);

		gchar *title = gebr_geoxml_document_get_title(flow);

		gchar *link = g_strdup_printf("%d", i);

		gchar *description = gebr_geoxml_document_get_description(flow);
		g_string_append_printf (index_content,
		                        "      <li>\n"
		                        "        <span class=\"title\"><a href=\"#%s\">%s</a></span> - \n"
		                        "        <span class=\"description\">%s</span>\n"
		                        "      </li>\n",
		                        link, title, description);

		i++;

		g_free(title);
		g_free(description);
		g_free(link);
		gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
		gebr_geoxml_sequence_next (&flows);
	}
}

static void
gebr_document_generate_line_paths(GebrGeoXmlDocument *document,
                                  GString *content)
{
	GebrGeoXmlLine *line = GEBR_GEOXML_LINE(document);

	if (gebr_geoxml_line_get_paths_number(line) > 0) {
		// Comment for translators: HTML header for detailed report
		g_string_append_printf(content,
		                       "<div class=\"paths\">\n"
		                       "  <table>\n"
		                       "    <caption>%s</caption>\n"
		                       "    <tbody>\n",
		                       _("Line paths"));

		GString *buf = g_string_new(NULL);
		gchar ***paths = gebr_geoxml_line_get_paths(GEBR_GEOXML_LINE(document));
		for (gint i = 0; paths[i]; i++) {
			if (!*paths[i][0])
				continue;

			if (!g_strcmp0(paths[i][1], "HOME")) {
				g_string_append_printf(content,
				                       "   <tr>\n"
				                       "     <td class=\"type\">&lt;%s&gt;</td>\n"
				                       "     <td class=\"value\">%s</td>\n"
				                       "   </tr>\n",
				                       paths[i][1], paths[i][0]);
				continue;
			}

			gchar *resolved = gebr_resolve_relative_path(paths[i][0], paths);
			g_string_append_printf(buf,
			                       "   <tr>"
			                       "     <td class=\"type\">&lt;%s&gt;</td>\n"
			                       "     <td class=\"value\">%s</td>\n"
			                       "   </tr>",
			                       paths[i][1], resolved);
			g_free(resolved);
		}
		g_string_append(content, buf->str);
		g_string_append(content,
		                "    </tbody>\n"
				"  </table>\n"
		                "</div>\n");

		g_string_free(buf, TRUE);
		gebr_pairstrfreev(paths);
	}
}

static void
gebr_document_generate_flow_index_content(GebrGeoXmlDocument *document,
                                          GString *index_content,
                                          const gchar *index)
{
	gboolean has_index = (index != NULL);
	gint i = 1;
	gchar *flow_title = gebr_geoxml_document_get_title(document);
	GebrGeoXmlSequence *program;

	gebr_geoxml_flow_get_program (GEBR_GEOXML_FLOW(document), &program, 0);
	while (program) {
		GebrGeoXmlProgram * prog;
		GebrGeoXmlProgramStatus status;
		gchar *title;
		gchar *description;

		prog = GEBR_GEOXML_PROGRAM (program);
		status = gebr_geoxml_program_get_status (prog);
		title = gebr_geoxml_program_get_title (prog);
		description = gebr_geoxml_program_get_description(prog);

		gchar *link = g_strdup_printf("%s%s%d",
		                              has_index? index : "",
		                              has_index? "." : "",
		                              i);

		if (status == GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED)
			g_string_append_printf(index_content,
			                       "      <li>\n"
			                       "        <span class=\"title\"><a href=\"#%s\">%s</a></span> - \n"
			                       "        <span class=\"description\">%s</span>\n"
			                       "      </li>\n",
			                       link, title, description);

		i++;

		g_free(title);
		g_free(description);
		g_free(link);
		gebr_geoxml_sequence_next (&program);
	}
	g_free(flow_title);
}

void
gebr_document_generate_index(GebrGeoXmlDocument *document,
                             GString *content,
                             GebrGeoXmlDocumentType type,
                             const gchar *index,
                             const gchar *description)
{
	GString *index_content = g_string_new(NULL);

	if (type == GEBR_GEOXML_DOCUMENT_TYPE_PROJECT)
		gebr_document_generate_project_index_content(document, index_content);

	else if (type == GEBR_GEOXML_DOCUMENT_TYPE_LINE)
		gebr_document_generate_line_index_content(document, index_content);

	else if (type == GEBR_GEOXML_DOCUMENT_TYPE_FLOW)
		gebr_document_generate_flow_index_content(document, index_content, index);

	else
		g_warn_if_reached();

	g_string_append_printf(content,
	                       "<div class=\"index\">\n"
	                       "  %s\n"
	                       "  <ul>\n"
	                       "    %s\n"
	                       "  </ul>\n"
	                       "</div>\n",
	                       description, index_content->str);

	g_string_free(index_content, TRUE);
}

static void
gebr_document_generate_project_tables_content(GebrGeoXmlDocument *document,
                                              GString *tables_content)
{
	/* Variables table */
	gebr_generate_variables_value_table(document, TRUE, TRUE, tables_content, _("Project"));
}

static void
gebr_document_generate_line_tables_content(GebrGeoXmlDocument *document,
                                           GString *tables_content)
{
	/* Variables table */
	gebr_generate_variables_value_table(GEBR_GEOXML_DOCUMENT(gebr.project), TRUE, FALSE, tables_content, _("Project"));
	gebr_generate_variables_value_table(document, FALSE, TRUE, tables_content, _("Line"));
}

static void
gebr_document_generate_flow_tables_content(GebrGeoXmlDocument *document,
                                           GString *tables_content)
{
	/* Variables table */
	gebr_generate_variables_value_table(GEBR_GEOXML_DOCUMENT(gebr.project), TRUE, FALSE, tables_content, _("Project"));
	gebr_generate_variables_value_table(GEBR_GEOXML_DOCUMENT(gebr.line), FALSE, FALSE, tables_content, _("Line"));
	gebr_generate_variables_value_table(document, FALSE, TRUE, tables_content, _("Flow"));

	/* I/O table */
	gebr_flow_generate_io_table(GEBR_GEOXML_FLOW(document), tables_content);
}

static void
gebr_document_generate_flow_programs(GebrGeoXmlDocument *document,
                                     GString *content,
                                     const gchar *index)
{
	gebr_flow_generate_parameter_value_table(GEBR_GEOXML_FLOW (document), content, index, FALSE);
}

static void
gebr_document_generate_tables(GebrGeoXmlDocument *document,
                              GString *content,
                              GebrGeoXmlDocumentType type)
{
	GString *tables_content = g_string_new(NULL);

	if (type == GEBR_GEOXML_DOCUMENT_TYPE_PROJECT)
		gebr_document_generate_project_tables_content(document, tables_content);

	else if (type == GEBR_GEOXML_DOCUMENT_TYPE_LINE)
		gebr_document_generate_line_tables_content(document, tables_content);

	else if (type == GEBR_GEOXML_DOCUMENT_TYPE_FLOW)
		gebr_document_generate_flow_tables_content(document, tables_content);

	else
		g_warn_if_reached();

	g_string_append_printf(content,
	                       "<div class=\"tables\">\n"
	                       "  %s\n"
	                       "</div>\n",
	                       tables_content->str);

	g_string_free(tables_content, TRUE);
}

static void
gebr_document_generate_flow_revisions_content(GebrGeoXmlDocument *flow,
                                              GString *content,
                                              const gchar *index,
                                              gboolean include_tables,
                                              gboolean include_snapshots)
{
	gboolean has_index = (index != NULL);
	gint i = 1;
	gboolean include_comments;
	gboolean has_snapshots = FALSE;
	GString *snap_content = g_string_new(NULL);
	gchar *flow_title = gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(flow));

	GebrGeoXmlSequence *seq;
	gebr_geoxml_flow_get_revision(GEBR_GEOXML_FLOW(flow), &seq, 0);
	for (; seq; gebr_geoxml_sequence_next(&seq)) {
		has_snapshots = TRUE;
		gchar *rev_xml;
		gchar *comment;
		gchar *date;
		GebrGeoXmlDocument *revdoc;

		gebr_geoxml_flow_get_revision_data(GEBR_GEOXML_REVISION(seq), &rev_xml, &date, &comment, NULL);

		if (gebr_geoxml_document_load_buffer(&revdoc, rev_xml) != GEBR_GEOXML_RETV_SUCCESS) {
			g_free(rev_xml);
			g_free(comment);
			g_free(date);
			g_warn_if_reached();
		}

		gchar *link = g_strdup_printf("%s%s%s%d",
		                              "snap",
		                              has_index? index : "",
		                              has_index? "." : "",
		                              i);

		g_string_append_printf(snap_content,
		                       "  <div class=\"snapshot\">\n"
		                       "    <div class=\"header\">"
		                       "      <a name=\"%s\"></a>\n"
		                       "      <div class=\"title\">%s</div>\n"
		                       "      <div class=\"description\">%s%s</div>\n"
		                       "    </div>\n",
		                       link, comment, _("Snapshot taken on "), date);

		gchar *report = gebr_geoxml_document_get_help(revdoc);
		gchar *snap_inner_body = gebr_document_report_get_inner_body(report);

		include_comments = gebr.config.detailed_flow_include_report;

		gebr_document_generate_flow_content(revdoc, snap_content, snap_inner_body,
		                                    include_tables, include_snapshots,
		                                    include_comments, FALSE, link, TRUE);

		g_string_append(snap_content,
		                "  </div>");

		i++;

		g_free(rev_xml);
		g_free(comment);
		g_free(link);
		g_free(date);
		g_free(report);
		g_free(snap_inner_body);
		gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(revdoc));
	}

	if (has_snapshots)
		g_string_append_printf(content,
		                       "<div class=\"snapshots\">\n"
		                       "  %s"
		                       "</div>\n",
		                       snap_content->str);

	g_string_free(snap_content, TRUE);
	g_free(flow_title);
}

static void
gebr_document_generate_flow_content(GebrGeoXmlDocument *document,
                                    GString *content,
                                    const gchar *inner_body,
                                    gboolean include_table,
                                    gboolean include_snapshots,
                                    gboolean include_comments,
                                    gboolean include_linepaths,
                                    const gchar *index,
                                    gboolean is_snapshot)
{
	gboolean has_snapshots = (gebr_geoxml_flow_get_revisions_number(GEBR_GEOXML_FLOW(document)) > 0);

	if (has_snapshots && include_snapshots)
		gebr_flow_generate_flow_revisions_index(GEBR_GEOXML_FLOW(document), content, index);

	if (include_comments && inner_body)
		gebr_document_create_section(content, inner_body, "comments");

	gebr_document_generate_index(document, content, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, index,
	                             is_snapshot? _("Snapshot composed by the program(s):") : _("Flow composed by the program(s):"));

	if (include_table) {
		if (include_linepaths)
			gebr_document_generate_line_paths(GEBR_GEOXML_DOCUMENT(gebr.line), content);
		gebr_document_generate_tables(document, content, GEBR_GEOXML_DOCUMENT_TYPE_FLOW);
		gebr_document_generate_flow_programs(document, content, index);
	}

	if (has_snapshots && include_snapshots)
		gebr_document_generate_flow_revisions_content(document, content, index, include_table, include_comments);
}

static void
gebr_document_generate_internal_flow(GebrGeoXmlDocument *document,
                                     GString *content)
{
	gint i = 1;
	GebrGeoXmlSequence *line_flow;

	gebr_geoxml_line_get_flow(GEBR_GEOXML_LINE(document), &line_flow, 0);

	g_string_append(content,
	                "      <div class=\"contents\">\n");

	for (; line_flow != NULL; gebr_geoxml_sequence_next(&line_flow)) {
		GebrGeoXmlFlow *flow;
		gboolean include_table;
		gboolean include_snapshots;
		gboolean include_comments;

		include_table = gebr.config.detailed_line_parameter_table != GEBR_PARAM_TABLE_NO_TABLE;
		include_snapshots = gebr.config.detailed_line_include_revisions_report;
		include_comments = gebr.config.detailed_line_include_report;

		const gchar *filename = gebr_geoxml_line_get_flow_source(GEBR_GEOXML_LINE_FLOW(line_flow));
		document_load((GebrGeoXmlDocument**)(&flow), filename, FALSE);
		gebr_validator_set_document(gebr.validator,(GebrGeoXmlDocument**)(&flow), GEBR_GEOXML_DOCUMENT_TYPE_FLOW, TRUE);

		gchar *report = gebr_geoxml_document_get_help(GEBR_GEOXML_DOCUMENT(flow));
		gchar *flow_inner_body = gebr_document_report_get_inner_body(report);

		gchar *index = g_strdup_printf("%d", i);

		GString *flow_content = g_string_new(NULL);
		gchar *header = gebr_document_generate_header(GEBR_GEOXML_DOCUMENT(flow), TRUE, index);
		gebr_document_generate_flow_content(GEBR_GEOXML_DOCUMENT(flow), flow_content,
		                                    flow_inner_body, include_table,
		                                    include_snapshots, include_comments, FALSE, index, FALSE);

		gchar *internal_html = gebr_generate_content_report("flow", header, flow_content->str);

		g_string_append_printf(content,
		                       "        %s",
		                       internal_html);

		i++;

		g_free(flow_inner_body);
		g_free(report);
		g_free(internal_html);
		g_free(index);
		g_string_free(flow_content, TRUE);

		gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
	}
	g_string_append(content,
	                "      </div>\n");

	gebr_validator_set_document(gebr.validator, (GebrGeoXmlDocument**)(&gebr.flow), GEBR_GEOXML_DOCUMENT_TYPE_FLOW, TRUE);
}

static void
gebr_document_generate_line_content(GebrGeoXmlDocument *document,
                                    GString *content,
                                    const gchar *inner_body,
                                    gboolean include_table)
{
	if (gebr.config.detailed_line_include_report && inner_body)
		gebr_document_create_section(content, inner_body, "comments");

	gebr_document_generate_index(document, content,
	                             GEBR_GEOXML_DOCUMENT_TYPE_LINE, NULL,
	                             _("Line composed by the Flow(s):"));

	if (include_table) {
		gebr_document_generate_line_paths(document, content);
		gebr_document_generate_tables(document, content, GEBR_GEOXML_DOCUMENT_TYPE_LINE);
	}

	if (gebr.config.detailed_line_include_flow_report)
		gebr_document_generate_internal_flow(document, content);
}

gchar * gebr_document_generate_report (GebrGeoXmlDocument *document)
{
	GebrGeoXmlObjectType type;
	gchar * title;
	gchar * report;
	gchar * detailed_html = "";
	gchar * inner_body = "";
	gchar * styles = "";
	gchar * header = "";
	gchar *scope;
	GString * content;

	content = g_string_new (NULL);

	type = gebr_geoxml_object_get_type(GEBR_GEOXML_OBJECT(document));
	if (type == GEBR_GEOXML_OBJECT_TYPE_PROGRAM) 
		return NULL;

	title = gebr_geoxml_document_get_title(document);
	report = gebr_geoxml_document_get_help(document);
	inner_body = gebr_document_report_get_inner_body(report);
	header = gebr_document_generate_header(document, FALSE, NULL);

	if (type == GEBR_GEOXML_OBJECT_TYPE_LINE) {
		scope = g_strdup(_("line"));

		styles = gebr_document_report_get_styles_css(report, gebr.config.detailed_line_css);

		gboolean include_table = gebr.config.detailed_line_parameter_table != GEBR_PARAM_TABLE_NO_TABLE;

		gebr_document_generate_line_content(document, content, inner_body, include_table);

	} else if (type == GEBR_GEOXML_OBJECT_TYPE_FLOW) {
		scope = g_strdup(_("flow"));

		styles = gebr_document_report_get_styles_css(report, gebr.config.detailed_flow_css);

		gboolean include_table = gebr.config.detailed_flow_parameter_table != GEBR_PARAM_TABLE_NO_TABLE;
		gboolean include_snapshots = gebr.config.detailed_flow_include_revisions_report;
		gboolean include_comments = gebr.config.detailed_flow_include_report;

		gebr_document_generate_flow_content(document, content, inner_body,
		                                    include_table, include_snapshots,
		                                    include_comments, TRUE, NULL, FALSE);

	} else if (type == GEBR_GEOXML_OBJECT_TYPE_PROJECT) {
		scope = g_strdup(_("project"));
		g_free (inner_body);
		return report;
	} else {
		g_free (inner_body);
		g_free (report);
		g_return_val_if_reached (NULL);
	}

	detailed_html = gebr_generate_report(title, styles, scope, header, content->str);

	g_free(header);
	g_free(styles);
	g_free(inner_body);
	g_string_free(content, TRUE);
	g_free (report);
	g_free(title);
	g_free(scope);

	return detailed_html;
}
