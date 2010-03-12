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

#include "document.h"
#include "gebr.h"
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

int document_load(GebrGeoXmlDocument ** document, const gchar * filename)
{
	int ret;
	GString *path;

	path = document_get_path(filename);
	ret = document_load_path(document, path->str);
	/* save will make a xml format upgrade if necessary */
	document_save(*document, FALSE);
	g_string_free(path, TRUE);

	return ret;
}

int document_load_at(GebrGeoXmlDocument ** document, const gchar * filename, const gchar * directory)
{
	int ret;
	GString *path;

	path = g_string_new("");
	g_string_printf(path, "%s/%s", directory, filename);
	ret = document_load_path(document, path->str);
	g_string_free(path, TRUE);

	return ret;
}

/**
 * \internal
 * Callback to remove menu reference from program to maintain backward compatibility
 */
static void  __document_discard_menu_ref_callback(GebrGeoXmlProgram * program, const gchar * filename, gint index)
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

int document_load_path(GebrGeoXmlDocument **document, const gchar * path)
{
	int ret;

	if ((ret = gebr_geoxml_document_load(document, path, TRUE, g_str_has_suffix(path, ".flw") ?
					     __document_discard_menu_ref_callback : NULL)) < 0) {
		GtkDialog *dialog;
		GString *string;

		string = g_string_new("");

		if (!gebr_geoxml_document_load(document, path, FALSE, NULL)) {
			const gchar *title;
			title = gebr_geoxml_document_get_title(*document);
			g_string_printf(string, "%s", title != NULL ? title : "");
			if (title != NULL)
				g_string_append(string, " ");
		}
		
		/* don't do document recovery for menus */
		if (g_str_has_suffix(path, ".mnu")) {
			gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Can't load menu %sat %s: %s."),
				     string, path, gebr_geoxml_error_string((enum GEBR_GEOXML_RETV)ret));
			g_string_free(string, TRUE);
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
			document_name = _("document");
		}

		dialog = GTK_DIALOG(gtk_message_dialog_new(GTK_WINDOW(gebr.window),
							   GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
							   GTK_MESSAGE_QUESTION,
							   GTK_BUTTONS_NONE,
							   "Could not load %s %sat %s.\nError: %s\n", document_name,
							   string->str, path,
							   gebr_geoxml_error_string((enum GEBR_GEOXML_RETV)ret)));
		g_string_printf(string, "Couldn't load %s", document_name);
		gtk_window_set_title(GTK_WINDOW(dialog), string->str); 
		gtk_dialog_add_button(dialog, _("Ignore"), 0);
		if (ret != GEBR_GEOXML_RETV_CANT_ACCESS_FILE)
			gtk_dialog_add_button(dialog, _("Export"), 1);

		gtk_widget_show_all(GTK_WIDGET(dialog));
		gint response;
		gboolean keep_dialog;
		do switch ((response = gtk_dialog_run(dialog))) {
		case 1: {
			GtkWidget *chooser_dialog;

			chooser_dialog = gtk_file_chooser_dialog_new(_("Choose filename to export"),
								     GTK_WINDOW(gebr.window),
								     GTK_FILE_CHOOSER_ACTION_SAVE,
								     GTK_STOCK_SAVE, GTK_RESPONSE_YES,
								     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);

			gtk_widget_show_all(chooser_dialog);
			if (gtk_dialog_run(GTK_DIALOG(chooser_dialog)) != GTK_RESPONSE_YES) {
				keep_dialog = TRUE;
				goto out;
			}
			gchar *export_path;
			export_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser_dialog));
			document_save_at(*document, export_path, FALSE);
			g_free(export_path);

			keep_dialog = FALSE;
			gtk_widget_destroy(chooser_dialog);
			break;
		} case 0:
		default:
			unlink(path);
			keep_dialog = FALSE;
			break;
		} while (keep_dialog);
		
		/* frees */
		if (*document) {
			gebr_geoxml_document_free(*document);
			document = NULL;
		}
		g_string_free(string, TRUE);
		gtk_widget_destroy(GTK_WIDGET(dialog));
	}

out:	return ret;
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

	/* TODO: check save */
	path = document_get_path(new_filename->str);
	gebr_geoxml_document_set_filename(document, new_filename->str);
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
