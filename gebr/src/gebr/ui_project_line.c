/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or * (at your option) any later version.
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

#include <stdlib.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libgebr/intl.h>
#include <libgebr/date.h>
#include <libgebr/utils.h>
#include <libgebr/gui/gebr-gui-utils.h>
#include <libgebr/gui/gebr-gui-save-dialog.h>

#include "ui_project_line.h"
#include "gebr.h"
#include "line.h"
#include "document.h"
#include "project.h"
#include "line.h"
#include "flow.h"
#include "ui_help.h"
#include "callbacks.h"
#include "../defines.h"

/*
 * Prototypes
 */

static void project_line_load(void);

static void project_line_on_row_activated(GtkTreeView * tree_view, GtkTreePath * path,
					  GtkTreeViewColumn * column, struct ui_project_line *ui_project_line);

static GtkMenu *project_line_popup_menu(GtkWidget * widget, struct ui_project_line *ui_project_line);

static gboolean line_reorder(GtkTreeView *tree_view, GtkTreeIter *source_iter, GtkTreeIter *target_iter,
			     GtkTreeViewDropPosition drop_position);

static gboolean line_can_reorder(GtkTreeView *tree_view, GtkTreeIter *source_iter, GtkTreeIter *target_iter,
				 GtkTreeViewDropPosition drop_position);

struct ui_project_line *project_line_setup_ui(void)
{
	struct ui_project_line *ui_project_line;

	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	GtkWidget *scrolled_window;
	GtkWidget *hpanel;
	GtkWidget *frame;
	GtkWidget *infopage;

	/* alloc */
	ui_project_line = g_new(struct ui_project_line, 1);

	/* Create projects/lines ui_project_line->widget */
	ui_project_line->widget = gtk_vbox_new(FALSE, 0);
	hpanel = gtk_hpaned_new();
	gtk_container_add(GTK_CONTAINER(ui_project_line->widget), hpanel);

	/* Left side */
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_paned_pack1(GTK_PANED(hpanel), scrolled_window, FALSE, FALSE);
	gtk_widget_set_size_request(scrolled_window, 300, -1);

	ui_project_line->store = gtk_tree_store_new(PL_N_COLUMN,
						    G_TYPE_STRING,
						    G_TYPE_STRING,
						    G_TYPE_POINTER);
	ui_project_line->view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ui_project_line->store));
	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(ui_project_line->view),
						  (GebrGuiGtkPopupCallback) project_line_popup_menu, ui_project_line);
	gebr_gui_gtk_tree_view_set_reorder_callback(GTK_TREE_VIEW(ui_project_line->view),
						    (GebrGuiGtkTreeViewReorderCallback) line_reorder,
						    (GebrGuiGtkTreeViewReorderCallback) line_can_reorder, NULL);

	g_signal_connect(ui_project_line->view, "row-activated",
			 G_CALLBACK(project_line_on_row_activated), ui_project_line);
	gtk_container_add(GTK_CONTAINER(scrolled_window), ui_project_line->view);

	/* Projects/lines column */
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Index"), renderer, NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_project_line->view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", PL_TITLE);
	gebr_gui_gtk_tree_view_fancy_search(GTK_TREE_VIEW(ui_project_line->view), PL_TITLE);
	g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_project_line->view)), "changed",
			 G_CALLBACK(project_line_load), ui_project_line);

	/* Right side */
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_paned_pack2(GTK_PANED(hpanel), scrolled_window, TRUE, FALSE);

	frame = gtk_frame_new(_("Details"));
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), frame);

	infopage = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), infopage);

	/* Title */
	ui_project_line->info.title = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_project_line->info.title), 0, 0);
	gtk_box_pack_start(GTK_BOX(infopage), ui_project_line->info.title, FALSE, TRUE, 0);

	/* Description */
	ui_project_line->info.description = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_project_line->info.description), 0, 0);
	gtk_box_pack_start(GTK_BOX(infopage), ui_project_line->info.description, FALSE, TRUE, 10);

	/* Number of lines */
	ui_project_line->info.numberoflines = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_project_line->info.numberoflines), 0, 0);
	gtk_box_pack_start(GTK_BOX(infopage), ui_project_line->info.numberoflines, FALSE, TRUE, 10);

	GtkWidget *table;
	table = gtk_table_new(4, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(infopage), table, FALSE, TRUE, 0);

	/* Dates */
	ui_project_line->info.created_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_project_line->info.created_label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_project_line->info.created_label, 0, 1, 0, 1, (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 3, 3);

	ui_project_line->info.created = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_project_line->info.created), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_project_line->info.created, 1, 2, 0, 1, (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 3, 3);

	ui_project_line->info.modified_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_project_line->info.modified_label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_project_line->info.modified_label, 0, 1, 1, 2, (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 3, 3);

	ui_project_line->info.modified = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_project_line->info.modified), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_project_line->info.modified, 1, 2, 1, 2, (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 3, 3);

	/* Paths for lines */
	ui_project_line->info.path_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_project_line->info.path_label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_project_line->info.path_label, 0, 1, 2, 3, (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 3, 3);

	ui_project_line->info.path = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_project_line->info.path), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_project_line->info.path, 1, 2, 2, 3, (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 3, 3);

	/* Help */
	GtkWidget * hbox;
	hbox = gtk_hbox_new(FALSE, 0);
	ui_project_line->info.help_view = gtk_button_new_with_label(_("View report"));
	ui_project_line->info.help_edit = gtk_button_new_with_label(_("Edit report"));
	gtk_box_pack_start(GTK_BOX(hbox), ui_project_line->info.help_view, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), ui_project_line->info.help_edit, TRUE, TRUE, 0);
	g_signal_connect(GTK_OBJECT(ui_project_line->info.help_view), "clicked",
			 G_CALLBACK(project_line_show_help), NULL);
	g_signal_connect(GTK_OBJECT(ui_project_line->info.help_edit), "clicked",
			 G_CALLBACK(project_line_edit_help), NULL);
	gtk_box_pack_end(GTK_BOX(infopage), hbox, FALSE, TRUE, 0);

	/* Author */
	ui_project_line->info.author = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_project_line->info.author), 0, 0);
	gtk_box_pack_end(GTK_BOX(infopage), ui_project_line->info.author, FALSE, TRUE, 0);

	return ui_project_line;
}

void project_line_info_update(void)
{
	gchar *markup;
	GString *text;

	gboolean is_project;

	if (!project_line_get_selected(NULL, DontWarnUnselection)) {
		gtk_label_set_text(GTK_LABEL(gebr.ui_project_line->info.title), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_project_line->info.description), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_project_line->info.created_label), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_project_line->info.created), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_project_line->info.modified_label), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_project_line->info.modified), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_project_line->info.path_label), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_project_line->info.path), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_project_line->info.numberoflines), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_project_line->info.author), "");

		g_object_set(gebr.ui_project_line->info.help_view, "sensitive", FALSE, NULL);
		g_object_set(gebr.ui_project_line->info.help_edit, "sensitive", FALSE, NULL);

		navigation_bar_update();
		return;
	}

	/* initialization */
	is_project =
	    (gebr_geoxml_document_get_type(gebr.project_line) == GEBR_GEOXML_DOCUMENT_TYPE_PROJECT) ? TRUE : FALSE;

	/* Title in bold */
	markup = is_project
	    ? g_markup_printf_escaped("<b>%s %s</b>", _("Project"), gebr_geoxml_document_get_title(gebr.project_line))
	    : g_markup_printf_escaped("<b>%s %s</b>", _("Line"), gebr_geoxml_document_get_title(gebr.project_line));
	gtk_label_set_markup(GTK_LABEL(gebr.ui_project_line->info.title), markup);
	g_free(markup);

	/* Description in italic */
	markup =
	    g_markup_printf_escaped("<i>%s</i>",
				    gebr_geoxml_document_get_description(GEBR_GEOXML_DOC(gebr.project_line)));
	gtk_label_set_markup(GTK_LABEL(gebr.ui_project_line->info.description), markup);
	g_free(markup);

	if (is_project) {
		gint nlines;

		nlines = gebr_geoxml_project_get_lines_number(gebr.project);
		markup = nlines != 0 ? nlines == 1 ? g_markup_printf_escaped(_("This project has 1 line"))
		    : g_markup_printf_escaped(_("This project has %d lines"), nlines)
		    : g_markup_printf_escaped(_("This project has no line"));

		gtk_label_set_markup(GTK_LABEL(gebr.ui_project_line->info.numberoflines), markup);
		g_free(markup);
	} else
		gtk_label_set_text(GTK_LABEL(gebr.ui_project_line->info.numberoflines), "");

	/* Date labels */
	markup = g_markup_printf_escaped("<b>%s</b>", _("Created:"));
	gtk_label_set_markup(GTK_LABEL(gebr.ui_project_line->info.created_label), markup);
	g_free(markup);
	markup = g_markup_printf_escaped("<b>%s</b>", _("Modified:"));
	gtk_label_set_markup(GTK_LABEL(gebr.ui_project_line->info.modified_label), markup);
	g_free(markup);

	/* Dates */
	gtk_label_set_text(GTK_LABEL(gebr.ui_project_line->info.created),
			   gebr_localized_date(gebr_geoxml_document_get_date_created(gebr.project_line)));
	gtk_label_set_text(GTK_LABEL(gebr.ui_project_line->info.modified),
			   gebr_localized_date(gebr_geoxml_document_get_date_modified(gebr.project_line)));

	/* Line's paths */
	if (!is_project) {
		GebrGeoXmlSequence *path;

		markup = g_markup_printf_escaped("<b>%s</b>", _("Paths:"));
		gtk_label_set_markup(GTK_LABEL(gebr.ui_project_line->info.path_label), markup);
		g_free(markup);

		text = g_string_new("");

		if (gebr_geoxml_line_get_path(gebr.line, &path, 0) == GEBR_GEOXML_RETV_SUCCESS){
			while (path) {
				const gchar * value;
				value = gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(path));
				g_string_append_printf(text, "%s\n", value);
				gebr_geoxml_sequence_next(&path);
			}
		}
		if (text->len == 0) {
			/* There is no path, put "None" */
			markup = g_markup_printf_escaped("<i>%s</i>", _("None"));
			gtk_label_set_markup(GTK_LABEL(gebr.ui_project_line->info.path), markup);
			g_free(markup);
		} else {
			gtk_label_set_text(GTK_LABEL(gebr.ui_project_line->info.path), text->str);
		}
		g_string_free(text, TRUE);
	} else {
		gtk_label_set_text(GTK_LABEL(gebr.ui_project_line->info.path_label), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_project_line->info.path), "");
	}


	/* Author and email */
	text = g_string_new(NULL);
	g_string_printf(text, "%s <%s>",
			gebr_geoxml_document_get_author(GEBR_GEOXML_DOC(gebr.project_line)),
			gebr_geoxml_document_get_email(GEBR_GEOXML_DOC(gebr.project_line)));
	gtk_label_set_text(GTK_LABEL(gebr.ui_project_line->info.author), text->str);
	g_string_free(text, TRUE);

	/* Info button */
	gboolean help_exists = gebr.project_line != NULL && (strlen(gebr_geoxml_document_get_help(GEBR_GEOXML_DOCUMENT(gebr.project_line)))? TRUE : FALSE);
	g_object_set(gebr.ui_project_line->info.help_view, "sensitive", help_exists, NULL);
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group, "project_line_view"), help_exists);
	g_object_set(gebr.ui_project_line->info.help_edit, "sensitive", TRUE, NULL);

	navigation_bar_update();
}

gboolean project_line_get_selected(GtkTreeIter * _iter, enum ProjectLineSelectionType check_type)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	gboolean is_line;
	static const gchar *no_line_selected;
	static const gchar *no_project_selected;

	no_line_selected = _("Please select a line.");
	no_project_selected = _("Please select a project.");
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_project_line->view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE) {
		switch (check_type) {
		case DontWarnUnselection:
			break;
		case ProjectSelection:
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, no_project_selected);
			break;
		case LineSelection:
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, no_line_selected);
			break;
		case ProjectLineSelection:
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Please select a project or a line."));
			break;
		}
		return FALSE;
	}
	if (_iter != NULL)
		*_iter = iter;
	GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(gebr.ui_project_line->store), &iter);
	is_line = gtk_tree_path_get_depth(path) == 2 ? TRUE : FALSE;
	gtk_tree_path_free(path);
	if (check_type == LineSelection && !is_line) {
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, no_line_selected);
		return FALSE;
	}
	if (check_type == ProjectSelection && is_line) {
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, no_project_selected);
		return FALSE;
	}

	return TRUE;
}

void project_line_select_iter(GtkTreeIter * iter)
{
	gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(gebr.ui_project_line->view), iter);
}

void project_line_import(void)
{
	GtkWidget *chooser_dialog;
	GtkFileFilter *file_filter;

	gchar *filename;
	gboolean is_project;

	GString *tmp_dir;
	GString *command;
	GString *command_line;
	gint exit_status;
	GError *error;
	gchar *output;
	GList *line_paths_creation_sugest = NULL;

	GebrGeoXmlDocument *document;
	GtkTreeIter iter;
	gchar **files;
	int i;

	command = g_string_new(NULL);
	command_line = g_string_new(NULL);
	error = NULL;

	chooser_dialog = gtk_file_chooser_dialog_new(_("Choose project/line to open"),
						     GTK_WINDOW(gebr.window),
						     GTK_FILE_CHOOSER_ACTION_OPEN,
						     GTK_STOCK_OPEN, GTK_RESPONSE_YES,
						     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(chooser_dialog), TRUE);
	file_filter = gtk_file_filter_new();
	gtk_file_filter_set_name(file_filter, _("Project or line (*.prjz *.lnez)"));
	gtk_file_filter_add_pattern(file_filter, "*.prjz");
	gtk_file_filter_add_pattern(file_filter, "*.lnez");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_dialog), file_filter);

	/* show file chooser */
	gtk_widget_show(chooser_dialog);
	if (gtk_dialog_run(GTK_DIALOG(chooser_dialog)) != GTK_RESPONSE_YES)
		goto out3;
	filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser_dialog));
	if (g_str_has_suffix(filename, ".prjz"))
		is_project = TRUE;
	else if (g_str_has_suffix(filename, ".lnez")) {
		is_project = FALSE;
		if (!project_line_get_selected(NULL, ProjectLineSelection))
			goto out2;
	} else {
		gebr_message(GEBR_LOG_ERROR, FALSE, TRUE, _("Unrecognized file type."));
		goto out2;
	}

	/**
	 * \internal
	 * Import line with basename \p line_filename inside \p at_dir.
	 * Also import its flows.
	 * Return error from #document_load_path.
	 */
	int line_import(GtkTreeIter *project_iter, GebrGeoXmlLine ** line, const gchar * line_filename, const gchar * at_dir)
	{
		GebrGeoXmlSequence *i;
		int ret;

		if ((ret = document_load_at((GebrGeoXmlDocument**)line, line_filename, at_dir)))
			return ret;

		document_import(GEBR_GEOXML_DOCUMENT(*line));
		/* check for paths that could be created; */
		GebrGeoXmlSequence *line_path;
		gebr_geoxml_line_get_path(*line, &line_path, 0);
		for (; line_path != NULL; gebr_geoxml_sequence_next(&line_path)) {
			const gchar *path = gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(line_path));
			if (gebr_path_is_at_home(path) && !g_file_test(path, G_FILE_TEST_EXISTS))
				line_paths_creation_sugest = g_list_prepend(line_paths_creation_sugest, g_strdup(path));
		}

		gebr_geoxml_line_get_flow(*line, &i, 0);
		while (i != NULL) {
			GebrGeoXmlFlow *flow;

			GebrGeoXmlSequence * next = i;
			gebr_geoxml_sequence_next(&next);

			int ret = document_load_at_with_parent((GebrGeoXmlDocument**)(&flow),
							       gebr_geoxml_line_get_flow_source(GEBR_GEOXML_LINE_FLOW(i)),
							       at_dir, project_iter);
			if (ret) {
				i = next;
				continue;
			}

			document_import(GEBR_GEOXML_DOCUMENT(flow));
			gebr_geoxml_line_set_flow_source(GEBR_GEOXML_LINE_FLOW(i),
							 gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(flow)));
			document_save(GEBR_GEOXML_DOCUMENT(flow), FALSE, TRUE); /* this flow is cached */

			i = next;
		}
		document_save(GEBR_GEOXML_DOCUMENT(*line), FALSE, FALSE);

		return ret;
	}

	char *quoted_dir;
	char *quoted_fil;
	char *quoted_com;

	tmp_dir = gebr_temp_directory_create();

	quoted_dir = g_shell_quote(tmp_dir->str);
	quoted_fil = g_shell_quote(filename);

	g_string_printf(command_line, "cd %s; tar xzfv %s", quoted_dir, quoted_fil);
	quoted_com = g_shell_quote(command_line->str);

	g_string_printf(command, "bash -c %s", quoted_com);

	if (!g_spawn_command_line_sync(command->str, &output, NULL, &exit_status, &error))
		goto err;
	if (exit_status)
		goto err;
	files = g_strsplit(output, "\n", 0);
	for (i = 0; files[i] != NULL; ++i) {
		if (is_project && g_str_has_suffix(files[i], ".prj")) {
			GebrGeoXmlProject *project;
			GebrGeoXmlSequence *project_line;

			if (document_load_at((GebrGeoXmlDocument**)(&project), files[i], tmp_dir->str))
				continue;
			document_import(GEBR_GEOXML_DOCUMENT(project));
			iter = project_append_iter(project);

			gebr_geoxml_project_get_line(project, &project_line, 0);
			while (project_line != NULL) {
				GebrGeoXmlLine *line;

				GebrGeoXmlSequence * next = project_line;
				gebr_geoxml_sequence_next(&next);

				int ret = line_import(&iter, &line, gebr_geoxml_project_get_line_source
						      (GEBR_GEOXML_PROJECT_LINE(project_line)), tmp_dir->str);
				if (ret) {
					project_line = next;
					continue;
				}
				gebr_geoxml_project_set_line_source(GEBR_GEOXML_PROJECT_LINE(project_line),
								    gebr_geoxml_document_get_filename
								    (GEBR_GEOXML_DOCUMENT(line)));

				project_append_line_iter(&iter, line);
				document_save(GEBR_GEOXML_DOCUMENT(line), FALSE, FALSE);
				
				project_line = next;
			}

			document = GEBR_GEOXML_DOCUMENT(project);
		} else if (!is_project && g_str_has_suffix(files[i], ".lne")) {
			GebrGeoXmlLine *line;
			GtkTreeIter parent;

			line_import(&parent, &line, files[i], tmp_dir->str);
			if (line == NULL)
				continue;
			gebr_geoxml_project_append_line(gebr.project,
							gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(line)));
			document_save(GEBR_GEOXML_DOCUMENT(gebr.project), TRUE, FALSE);

			project_line_get_selected(&iter, DontWarnUnselection);
			parent = iter;
			if (!gtk_tree_model_iter_parent(GTK_TREE_MODEL(gebr.ui_project_line->store), &parent, &iter))
				parent = iter;
			iter = project_append_line_iter(&parent, line);

			document = GEBR_GEOXML_DOCUMENT(line);
		} else
			document = NULL;

		if (document != NULL) {
			project_line_select_iter(&iter);

			GString *new_title = g_string_new(NULL);
			g_string_printf(new_title, _("%s (Imported)"), gebr_geoxml_document_get_title(document));
			gtk_tree_store_set(gebr.ui_project_line->store, &iter, PL_TITLE, new_title->str, -1);
			gebr_geoxml_document_set_title(document, new_title->str);
			g_string_free(new_title, TRUE);

			document_save(document, FALSE, FALSE);
		}
	}

	if (line_paths_creation_sugest != NULL) {
		GString *paths = g_string_new("");
		for (GList *i = line_paths_creation_sugest; i != NULL; i = g_list_next(i))
			g_string_append_printf(paths, "\n%s", (gchar*)i->data);

		if (gebr_gui_confirm_action_dialog(_("Create directories"),
						   _("There are some line paths localed on your home directory that"
						     " do not exist. Do you want to create following folders:%s"), paths->str)) {
			GString *cmd_line = g_string_new(NULL);
			for (GList *i = line_paths_creation_sugest; i != NULL; i = g_list_next(i)) {
				if (g_file_test (i->data, G_FILE_TEST_EXISTS))
					continue;

				if (g_mkdir_with_parents (i->data, 0755) != 0) {
					GtkWidget * warning;
					warning = gtk_message_dialog_new_with_markup (GTK_WINDOW (gebr.window),
										      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
										      GTK_MESSAGE_WARNING,
										      GTK_BUTTONS_OK,
										      "<span size='larger' weight='bold'>%s %s</span>",
										      _("Could not create the directory"),
										      (gchar*)i->data);

					gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (warning),
										    _("The directory <i>%s</i> could not be created. "
										      "Certify you have the rights to perform this operation."),
										    (gchar*)i->data);

					gtk_dialog_run (GTK_DIALOG (warning));
					gtk_widget_destroy (warning);
				}
			}
			g_string_free(cmd_line, TRUE);
		}

		g_string_free(paths, TRUE);
	}

	gebr_temp_directory_destroy(tmp_dir);
	gebr_message(GEBR_LOG_INFO, FALSE, TRUE, _("Import successful."));
	g_strfreev(files);
	goto out;

 err:	gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Failed to import."));

out:	g_free(output);
out2:	g_free(filename);
out3:	gtk_widget_destroy(chooser_dialog);
	g_list_foreach(line_paths_creation_sugest, (GFunc)g_free, NULL);
	g_list_free(line_paths_creation_sugest);
	g_string_free(command, TRUE);
}

void project_line_export(void)
{
	GString *command;
	GString *filename;
	GString *tmpdir;
	const gchar *extension;
	gchar *tmp;
	gchar *current_dir;

	GtkWidget *chooser_dialog;
	GtkWidget *check_box;
	const gchar *check_box_label;
	GtkFileFilter *file_filter;

	if (!project_line_get_selected(NULL, ProjectLineSelection))
		return;
	
	file_filter = gtk_file_filter_new();
	if (gebr_geoxml_document_get_type(gebr.project_line) == GEBR_GEOXML_DOCUMENT_TYPE_PROJECT) {
		gtk_file_filter_set_name(file_filter, _("Project (*.prjz)"));
		gtk_file_filter_add_pattern(file_filter, "*.prjz");
		extension = ".prjz";
		check_box_label = _("Make this project user-independent");
	} else {
		gtk_file_filter_set_name(file_filter, _("Line (*.lnez)"));
		gtk_file_filter_add_pattern(file_filter, "*.lnez");
		extension = ".lnez";
		check_box_label = _("Make this line user-independent");
	}

	/* run file chooser */
	check_box = gtk_check_button_new_with_label(_(check_box_label));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_box), TRUE);
	chooser_dialog = gebr_gui_save_dialog_new(_("Choose filename to save"), GTK_WINDOW(gebr.window));
	gebr_gui_save_dialog_set_default_extension(GEBR_GUI_SAVE_DIALOG(chooser_dialog), extension);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser_dialog), file_filter);
	gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(chooser_dialog), check_box);

	/* show file chooser */
	tmp = gebr_gui_save_dialog_run(GEBR_GUI_SAVE_DIALOG(chooser_dialog));
	if (!tmp)
		return;
	
	command = g_string_new("");
	filename = g_string_new("");
	tmpdir = gebr_temp_directory_create();

	void parse_line(GebrGeoXmlLine * _line) {
		GebrGeoXmlSequence *j;
		GebrGeoXmlLine *line;

		line = GEBR_GEOXML_LINE(gebr_geoxml_document_clone(GEBR_GEOXML_DOCUMENT(_line)));

		line_set_paths_to(line,	gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_box)));
		g_string_printf(filename, "%s/%s", tmpdir->str, gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(line)));
		document_save_at(GEBR_GEOXML_DOCUMENT(line), filename->str, FALSE, FALSE);

		gebr_geoxml_line_get_flow(line, &j, 0);
		for (; j != NULL; gebr_geoxml_sequence_next(&j)) {
			const gchar *flow_filename;
			GebrGeoXmlFlow *flow;

			flow_filename = gebr_geoxml_line_get_flow_source(GEBR_GEOXML_LINE_FLOW(j));
			if (document_load((GebrGeoXmlDocument**)(&flow), flow_filename, FALSE))
				continue;

			flow_set_paths_to(flow,	gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_box)));
			g_string_printf(filename, "%s/%s", tmpdir->str, flow_filename);
			document_save_at(GEBR_GEOXML_DOCUMENT(flow), filename->str, FALSE, FALSE);

			document_free(GEBR_GEOXML_DOCUMENT(flow));
		}

		document_free(GEBR_GEOXML_DOCUMENT(line));
	}
	
	if (gebr_geoxml_document_get_type(gebr.project_line) == GEBR_GEOXML_DOCUMENT_TYPE_PROJECT) {
		GebrGeoXmlSequence *i;

		g_string_printf(filename, "%s/%s", tmpdir->str, gebr_geoxml_document_get_filename(GEBR_GEOXML_DOCUMENT(gebr.project)));
		document_save_at(GEBR_GEOXML_DOCUMENT(gebr.project), filename->str, FALSE, FALSE);

		gebr_geoxml_project_get_line(gebr.project, &i, 0);
		for (; i != NULL; gebr_geoxml_sequence_next(&i)) {
			GebrGeoXmlLine *line;

			if (document_load((GebrGeoXmlDocument**)(&line),
					  gebr_geoxml_project_get_line_source(GEBR_GEOXML_PROJECT_LINE(i)), FALSE))
				continue;

			parse_line(line);
			document_free(GEBR_GEOXML_DOCUMENT(line));
		}
	} else
		parse_line(gebr.line);

	current_dir = g_get_current_dir();
	g_chdir(tmpdir->str);
	char *quoted;
	quoted = g_shell_quote(tmp);
	g_string_printf(command, "tar czf %s *", quoted);

	if (system(command->str))
		gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Could not export."));
	else
		gebr_message(GEBR_LOG_INFO, TRUE, TRUE, _("Export succesful."));
	g_chdir(current_dir);
	g_free(current_dir);

	g_free(tmp);
	g_string_free(command, TRUE);
	g_string_free(filename, TRUE);
	gebr_temp_directory_destroy(tmpdir);
}

void project_line_free(void)
{
	gebr.project_line = NULL;
	gebr.project = NULL;
	gebr.line = NULL;

	gtk_list_store_clear(gebr.ui_flow_browse->store);
	flow_free();

	project_line_info_update();
}


/**
 * \internal
 * Load the selected project or line from file.
 */
static void project_line_load(void)
{
	GtkTreeIter iter;
	GtkTreeIter child;

	gboolean is_line;
	gchar *project_filename;
	gchar *line_filename;

	project_line_free();
	if (!project_line_get_selected(&iter, DontWarnUnselection))
		return;

	GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(gebr.ui_project_line->store), &iter);
	is_line = gtk_tree_path_get_depth(path) == 2 ? TRUE : FALSE;
	gtk_tree_path_free(path);

	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group, "project_line_dump"), is_line);

	if (is_line == TRUE) {
		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->store), &iter,
				   PL_FILENAME, &line_filename, -1);

		child = iter;
		gtk_tree_model_iter_parent(GTK_TREE_MODEL(gebr.ui_project_line->store), &iter, &child);
		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->store), &iter,
				   PL_FILENAME, &project_filename, -1);
	} else {
		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->store), &iter,
				   PL_FILENAME, &project_filename, -1);
	}

	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->store), &iter,
			   PL_XMLPOINTER, &gebr.project, -1);
	if (is_line == TRUE) {
		gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_project_line->store), &child,
				   PL_XMLPOINTER, &gebr.line, -1);

		gebr.project_line = GEBR_GEOXML_DOC(gebr.line);
		line_load_flows();
	} else {
		gebr.project_line = GEBR_GEOXML_DOC(gebr.project);
		gebr.line = NULL;
	}

	project_line_info_update();

	g_free(project_filename);
	if (is_line == TRUE)
		g_free(line_filename);
}


/**
 * \internal
 */
static void project_line_on_row_activated(GtkTreeView * tree_view, GtkTreePath * path,
					  GtkTreeViewColumn * column, struct ui_project_line *ui)
{
	GtkTreeIter iter;

	project_line_get_selected(&iter, DontWarnUnselection);

	if (gtk_tree_store_iter_depth(ui->store, &iter) == 0) {
		GtkTreePath *path;
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(ui->store), &iter);

		if (gtk_tree_view_row_expanded(GTK_TREE_VIEW(ui->view), path)) {
			gtk_tree_view_collapse_row(GTK_TREE_VIEW(ui->view), path);
		} else {
			gtk_tree_view_expand_row(GTK_TREE_VIEW(ui->view), path, FALSE);
		}
		gtk_tree_path_free(path);
		return;
	}

	gebr.config.current_notebook = 1;
	gtk_notebook_set_current_page(GTK_NOTEBOOK(gebr.notebook), gebr.config.current_notebook);
}

/**
 * \internal
 */
static GtkMenu *project_line_popup_menu(GtkWidget * widget, struct ui_project_line *ui_project_line)
{
	GtkWidget *menu;
	GtkWidget *menu_item;

	menu = gtk_menu_new();

	/* new project */
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group, "project_line_new_project")));

	if (!project_line_get_selected(NULL, DontWarnUnselection)) {
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

		menu_item = gtk_menu_item_new_with_label(_("Collapse all"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_collapse_all), ui_project_line->view);

		menu_item = gtk_menu_item_new_with_label(_("Expand all"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_expand_all), ui_project_line->view);
		goto out;
	}

	/* new line */
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group, "project_line_new_line")));
	/* properties */
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group, "project_line_properties")));
	/* delete */
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group, "project_line_delete")));

	/* view report */
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group, "project_line_view")));

	/* edit report */
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action
						      (gebr.action_group, "project_line_edit")));

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

	menu_item = gtk_menu_item_new_with_label(_("Collapse all"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_collapse_all), ui_project_line->view);

	menu_item = gtk_menu_item_new_with_label(_("Expand all"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect_swapped(menu_item, "activate", G_CALLBACK(gtk_tree_view_expand_all), ui_project_line->view);

 out:	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}


/**
 * \internal
 * 
 * Lines and projects reordering callback.
 */
static gboolean
line_reorder(GtkTreeView *tree_view, GtkTreeIter *source_iter, GtkTreeIter *target_iter,
	     GtkTreeViewDropPosition drop_position)
{
	GtkTreeIter source_iter_parent, target_iter_parent;
	GtkTreeIter new_iter;
	gboolean source_is_line, target_is_line;
	gchar *source_line_filename = NULL, *target_line_filename = NULL;
	gchar *source_project_filename = NULL, *target_project_filename = NULL;
	
	GtkTreeModel *model = GTK_TREE_MODEL(gebr.ui_project_line->store);

	/* If the iters have parents, they refer to lines. Otherwise, they refer to projects. */
	source_is_line = gtk_tree_model_iter_parent(model, &source_iter_parent, source_iter);
	target_is_line = gtk_tree_model_iter_parent(model, &target_iter_parent, target_iter);

	/* Get all projects and lines filenames (sources and targets). */
	if (source_is_line) {
		gtk_tree_model_get(model, source_iter, PL_FILENAME, &source_line_filename, -1);
		gtk_tree_model_get(model, &source_iter_parent, PL_FILENAME, &source_project_filename, -1);
	}

	if (target_is_line) {
		gtk_tree_model_get(model, target_iter, PL_FILENAME, &target_line_filename, -1);
		gtk_tree_model_get(model, &target_iter_parent, PL_FILENAME, &target_project_filename, -1);
	}
	else {
		/* Target iter is a project. Thus, we get its filename only (line is unknown in this case). */
		gtk_tree_model_get(model, target_iter, PL_FILENAME, &target_project_filename, -1);
	}


	/* Drop cases: */
	if (!source_is_line && !target_is_line) /* Source and target are lines. Nothing to do.*/
		return TRUE;

	GtkTreeStore *store = gebr.ui_project_line->store;

	if (source_is_line && target_is_line) {
		gboolean drop_before;
		drop_before = (drop_position == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE || drop_position == GTK_TREE_VIEW_DROP_BEFORE);

		if (drop_before) {
			gtk_tree_store_insert_before(store, &new_iter, NULL, target_iter);
		}
		else { /* GTK_TREE_VIEW_DROP_INTO_OR_AFTER || GTK_TREE_VIEW_DROP_AFTER */
			gtk_tree_store_insert_after(store, &new_iter, NULL, target_iter);
		}
		gebr_gui_gtk_tree_model_iter_copy_values(model, &new_iter, source_iter);
		gtk_tree_store_remove(store, source_iter);
		
		project_line_move(source_project_filename, source_line_filename, target_project_filename, target_line_filename, drop_before);
		project_line_select_iter(&new_iter);

		return TRUE;
	}

	if (source_is_line && !target_is_line) { /* Target is a project. */
		gtk_tree_store_append(store, &new_iter, target_iter);
		gebr_gui_gtk_tree_model_iter_copy_values(model, &new_iter, source_iter);
		gtk_tree_store_remove(store, source_iter);

		project_line_move(source_project_filename, source_line_filename, target_project_filename, NULL, FALSE);
		project_line_select_iter(&new_iter);

		return TRUE;
	}

	return FALSE;
}


/**
 * \internal
 *
 * Lines and projects reordering acceptance callback.
 */
static gboolean
line_can_reorder(GtkTreeView *tree_view, GtkTreeIter *source_iter, GtkTreeIter *target_iter,
		 GtkTreeViewDropPosition drop_position)
{
	GtkTreeIter source_iter_parent, target_iter_parent;
	gboolean source_is_line, target_is_line;

	/* If the iters have parents, they refer to lines. Otherwise, they refer to projects. */
	source_is_line = gtk_tree_model_iter_parent(GTK_TREE_MODEL(gebr.ui_project_line->store), &source_iter_parent, source_iter);
	target_is_line = gtk_tree_model_iter_parent(GTK_TREE_MODEL(gebr.ui_project_line->store), &target_iter_parent, target_iter);

	if (source_is_line && target_is_line) /* Source and target are lines. */
		return TRUE;

	if (!source_is_line && target_is_line) /* Source is a project. */
		return FALSE;

	if (source_is_line && !target_is_line) /* Target is a project. */
		return drop_position == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE || drop_position == GTK_TREE_VIEW_DROP_INTO_OR_AFTER;

	/* Source and target are projects. */
	return FALSE;
}

/**
 */
void project_line_show_help(void)
{
	const gchar * title;
	GebrGeoXmlObject * object;

	object = GEBR_GEOXML_OBJECT(gebr.project_line);
	if (gebr_geoxml_document_get_type(gebr.project_line) == GEBR_GEOXML_DOCUMENT_TYPE_PROJECT)
		title = _("Project report");
	else
		title = _("Line report");
	gebr_help_show(object, FALSE, title);
}

void project_line_edit_help(void)
{
	gebr_help_edit_document(GEBR_GEOXML_DOC(gebr.project_line));
	document_save(GEBR_GEOXML_DOCUMENT(gebr.project_line), TRUE, FALSE);
}

static void on_detailed_line_css_changed (GtkComboBox * combobox)
{
	gchar *text;
	text = gtk_combo_box_get_active_text (combobox);
	if (gtk_combo_box_get_active (combobox) == 0) {
		g_string_assign (gebr.config.detailed_line_css, "");
	} else {
		g_string_assign (gebr.config.detailed_line_css, text);
	}
	g_free (text);
}

static void on_detailed_line_include_report_toggled (GtkToggleButton *toggle)
{
	gboolean toggled;
	toggled = gtk_toggle_button_get_active (toggle);
	gebr.config.detailed_line_include_report = toggled;
}

static void on_detailed_line_include_flow_report_toggled (GtkToggleButton *button, GtkWidget *widget)
{
	gboolean toggled;
	toggled = gtk_toggle_button_get_active (button);
	gtk_widget_set_sensitive(widget, toggled);
	gebr.config.detailed_line_include_flow_report = toggled;
}

static void on_detailed_line_include_flow_params_toggled (GtkToggleButton * button)
{
	gboolean toggled;
	toggled = gtk_toggle_button_get_active(button);
	gebr.config.detailed_line_include_flow_params = toggled;
}

GtkWidget *
gebr_project_line_print_dialog_custom_tab()
{
	GtkWidget * hbox_combo;
	GtkWidget * vbox;
	GtkWidget * frame;
	GtkWidget * alignment;
	GtkWidget * detailed_line_css_combo;
	GtkWidget * detailed_line_include_report;
	GtkWidget * detailed_line_include_flow_report;
	GtkWidget * detailed_line_include_flow_params;
	GtkWidget * css_combo_label;
	GDir * directory;
	GError * error = NULL;
	const gchar * filename = NULL;
	gint active = 0, i = 0;

	css_combo_label = gtk_label_new(_("Style"));
	gtk_widget_show(css_combo_label);
	detailed_line_css_combo = gtk_combo_box_new_text();

	directory = g_dir_open(GEBR_STYLES_DIR, 0, &error);
	if (error != NULL) {
		gebr_message(GEBR_LOG_ERROR, TRUE, TRUE, _("Unable to read file: %s"), error->message);
		g_error_free(error);
		return NULL;
	}

	gtk_combo_box_append_text (GTK_COMBO_BOX (detailed_line_css_combo), _("Report style"));
	filename = g_dir_read_name(directory);
	while (filename != NULL) {
		if (fnmatch("*.css", filename, 1) == 0) {
			gtk_combo_box_append_text (GTK_COMBO_BOX (detailed_line_css_combo), filename);
			if (strcmp (filename, gebr.config.detailed_line_css->str) == 0)
				active = i + 1;
			i++;
		}
		filename = g_dir_read_name(directory);
	}
	gtk_widget_show(detailed_line_css_combo);

	detailed_line_include_report = gtk_check_button_new_with_label(_("Include user's report"));
	detailed_line_include_flow_report = gtk_check_button_new_with_label(_("Include flow reports"));
	detailed_line_include_flow_params = gtk_check_button_new_with_label(_("Include parameter/value table"));

	g_signal_connect(detailed_line_css_combo, "changed",
			 G_CALLBACK(on_detailed_line_css_changed), NULL);

	g_signal_connect(detailed_line_include_report, "toggled",
			 G_CALLBACK(on_detailed_line_include_report_toggled), NULL);

	g_signal_connect(detailed_line_include_flow_report, "toggled",
			 G_CALLBACK(on_detailed_line_include_flow_report_toggled), detailed_line_include_flow_params);

	g_signal_connect(detailed_line_include_flow_params, "toggled",
			 G_CALLBACK(on_detailed_line_include_flow_params_toggled), NULL);

	gtk_combo_box_set_active(GTK_COMBO_BOX(detailed_line_css_combo), active);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(detailed_line_include_report), gebr.config.detailed_line_include_report);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(detailed_line_include_flow_report), gebr.config.detailed_line_include_flow_report);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(detailed_line_include_flow_params), gebr.config.detailed_line_include_flow_params); 
	gtk_widget_set_sensitive (detailed_line_include_flow_params, gebr.config.detailed_line_include_flow_report);

	frame = gtk_frame_new(NULL);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 10);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);

	vbox = gtk_vbox_new(FALSE, 0);
	hbox_combo = gtk_hbox_new(FALSE, 0);

	gtk_box_pack_start(GTK_BOX(hbox_combo), css_combo_label, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_combo), detailed_line_css_combo, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), detailed_line_include_report, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), detailed_line_include_flow_report, FALSE, TRUE, 0);

	alignment = gtk_alignment_new(0, 0, 1, 1);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 10, 0);
	gtk_container_add (GTK_CONTAINER (alignment), detailed_line_include_flow_params);
	gtk_box_pack_start(GTK_BOX(vbox), alignment, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox_combo, FALSE, TRUE, 0);

	gtk_container_add(GTK_CONTAINER(frame),vbox);
	gtk_widget_show_all(frame);
	return frame;
}

gchar * gebr_line_generate_header(GebrGeoXmlDocument * document)
{
	GString * dump;
	GebrGeoXmlLine *line;
	GebrGeoXmlDocumentType type;
	GebrGeoXmlSequence *sequence;

	type = gebr_geoxml_document_get_type (document);
	g_return_val_if_fail (type == GEBR_GEOXML_DOCUMENT_TYPE_LINE, NULL);

	dump = g_string_new(NULL);
	g_string_printf(dump,
			"<h1>%s</h1>\n<h2>%s</h2>\n",
			gebr_geoxml_document_get_title(document),
			gebr_geoxml_document_get_description(document));

	g_string_append_printf(dump,
			       "<p class=\"credits\">%s <span style=\"gebr-author\">%s</span> "
			       "<span class=\"gebr-email\">%s</span>, "
			       "<span class=\"gebr-date\">%s</span></p>\n",
			       // Comment for translators:
			       // "By" as in "By John McClane"
			       _("By"),
			       gebr_geoxml_document_get_author(document),
			       gebr_geoxml_document_get_email(document),
			       gebr_localized_date(gebr_iso_date()));
			

	line = GEBR_GEOXML_LINE (document);
	if (gebr_geoxml_line_get_paths_number(line) > 0) {
		GebrGeoXmlSequence *line_path;

		// Comment for translators: HTML header for detailed report
		g_string_append_printf (dump, "<p>%s</p>\n<ul>\n", _("Line paths:"));

		gebr_geoxml_line_get_path(GEBR_GEOXML_LINE(document), &line_path, 0);
		for (; line_path != NULL; gebr_geoxml_sequence_next(&line_path)) {
			g_string_append_printf(dump, "   <li>%s</li>\n",
					       gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(line_path)));
		}
		g_string_append(dump, "</ul>\n");
	}

	g_string_append_printf (dump, "<p>%s</p>\n<ul>\n", _("Line with flow(s):"));
	gebr_geoxml_line_get_flow (GEBR_GEOXML_LINE (document), &sequence, 0);
	while (sequence) {
		const gchar *fname;
		GebrGeoXmlDocument *flow;

		fname = gebr_geoxml_line_get_flow_source (GEBR_GEOXML_LINE_FLOW (sequence));
		document_load(&flow, fname, FALSE);
		g_string_append_printf (dump, "   <li>%s</li>\n", gebr_geoxml_document_get_title (flow));
		gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(flow));
		gebr_geoxml_sequence_next (&sequence);
	}
	g_string_append (dump, "</ul>\n");

	return g_string_free(dump, FALSE);
}
