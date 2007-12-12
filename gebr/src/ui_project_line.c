/*   GÍBR - An environment for seismic processing.
 *   Copyright(C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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

/*
 * File: ui_project_line.c
 * Builds the "Project and Lines" UI and distribute callbacks
 */

#include <string.h>

#include "ui_project_line.h"
#include "gebr.h"
#include "support.h"
#include "line.h"
#include "document.h"
#include "project.h"
#include "line.h"
#include "ui_help.h"

/*
 * Prototypes
 */

static void
project_line_rename(GtkCellRendererText * cell, gchar * path_string, gchar * new_text, struct ui_project_line * ui_project_line);


static void
project_line_load(void);

static void
project_line_show_help(void);


/*
 * Section: Public
 * Public functions.
 */

/*
 * Function: project_line_setup_ui
 * Assembly the project/lines widget.
 *
 * Return:
 * The structure containing relevant data.
 *
 * TODO:
 * Add an info summary about the project/line.
 */
struct ui_project_line *
project_line_setup_ui(void)
{
	struct ui_project_line *	ui_project_line;

	GtkTreeSelection *		selection;
	GtkTreeViewColumn *		col;
	GtkCellRenderer *		renderer;

	GtkWidget *			scrolledwin;
	GtkWidget *			hpanel;
	GtkWidget *			frame;
	GtkWidget *			infopage;

	/* alloc */
	ui_project_line = g_malloc(sizeof(struct ui_project_line));

	/* Create projects/lines ui_project_line->widget */
	ui_project_line->widget = gtk_vbox_new(FALSE, 0);
	hpanel = gtk_hpaned_new();
	gtk_container_add(GTK_CONTAINER(ui_project_line->widget), hpanel);

	/* Left side */
	scrolledwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_paned_pack1(GTK_PANED(hpanel), scrolledwin, FALSE, FALSE);
	gtk_widget_set_size_request(scrolledwin, 300, -1);

	ui_project_line->store = gtk_tree_store_new(PL_N_COLUMN,
						G_TYPE_STRING,  /* Name (title for libgeoxml) */
						G_TYPE_STRING); /* Filename */
	ui_project_line->view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ui_project_line->store));
	gtk_container_add(GTK_CONTAINER(scrolledwin), ui_project_line->view);

	/* Projects/lines column */
	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "editable", TRUE, NULL);
	col = gtk_tree_view_column_new_with_attributes(_("Index"), renderer, NULL);
	gtk_tree_view_column_set_sort_column_id(col, PL_TITLE);
	gtk_tree_view_column_set_sort_indicator(col, TRUE);

	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_project_line->view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", PL_TITLE);
	g_signal_connect(GTK_OBJECT(renderer), "edited",
			GTK_SIGNAL_FUNC(project_line_rename), ui_project_line);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_project_line->view));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);
	g_signal_connect(GTK_OBJECT(ui_project_line->view), "cursor-changed",
			GTK_SIGNAL_FUNC(project_line_load), ui_project_line);
	g_signal_connect(GTK_OBJECT(ui_project_line->view), "cursor-changed",
			GTK_SIGNAL_FUNC(line_load_flows), ui_project_line);
	g_signal_connect(GTK_OBJECT(ui_project_line->view), "cursor-changed",
			GTK_SIGNAL_FUNC(project_line_info_update), NULL);

	ui_project_line->selection_path = NULL;

	/* Right side */
	frame = gtk_frame_new(_("Details"));
	gtk_paned_pack2(GTK_PANED(hpanel), frame, TRUE, FALSE);

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

	/* Dates */
	GtkWidget *table;
	table = gtk_table_new(2, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(infopage), table, FALSE, TRUE, 0);

	ui_project_line->info.created_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_project_line->info.created_label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_project_line->info.created_label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 3, 3);

	ui_project_line->info.created = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_project_line->info.created), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_project_line->info.created, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 3, 3);

	ui_project_line->info.modified_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_project_line->info.modified_label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_project_line->info.modified_label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 3, 3);

	ui_project_line->info.modified = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_project_line->info.modified), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_project_line->info.modified, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 3, 3);

	/* Help */
	ui_project_line->info.help = gtk_button_new_from_stock(GTK_STOCK_INFO);
	gtk_box_pack_end(GTK_BOX(infopage), ui_project_line->info.help, FALSE, TRUE, 0);
	g_signal_connect(GTK_OBJECT(ui_project_line->info.help), "clicked",
			 GTK_SIGNAL_FUNC(project_line_show_help), ui_project_line);

	/* Author */
	ui_project_line->info.author = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_project_line->info.author), 0, 0);
	gtk_box_pack_end(GTK_BOX(infopage), ui_project_line->info.author, FALSE, TRUE, 0);


	return ui_project_line;
}

/*
 * Section: Private
 * Private functions.
 */

/*
 * Function: project_line_rename
 * Rename a projet or a line upon double click
 */
static void
project_line_rename(GtkCellRendererText * cell, gchar * path_string, gchar * new_text, struct ui_project_line * ui_project_line)
{
	GtkTreeIter		iter;
	gchar *			old_title;

	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(ui_project_line->store), &iter, path_string);
	old_title = (gchar *)geoxml_document_get_title(gebr.doc);

	/* was it really renamed? */
	if (g_ascii_strcasecmp(old_title, new_text) == 0)
		return;

	/* change it on the xml. */
	geoxml_document_set_title(gebr.doc, new_text);
	document_save(gebr.doc);

	/* store's change */
	gtk_tree_store_set(ui_project_line->store, &iter,
			PL_TITLE, new_text,
			-1);

	/* feedback */
	if (geoxml_document_get_type(gebr.doc) == GEOXML_DOCUMENT_TYPE_PROJECT)
		gebr_message(INFO, FALSE, TRUE, _("Project '%s' renamed to '%s'"), old_title, new_text);
	else
		gebr_message(INFO, FALSE, TRUE, _("Line '%s' renamed to '%s'"), old_title, new_text);
	project_line_info_update();
}

/*
 * Function: project_line_load
 * Load a selected project or line from file
 *
 * The selected line or project is loaded from file, upon selection.
 */
static void
project_line_load(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;
	GtkTreeIter		project_iter;
	GtkTreePath *		path;

	gchar *			filename;
	gchar *                 title;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_project_line->view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE) {
		gebr_message(ERROR, TRUE, FALSE, _("Neither project nor line selected"));
		return;
	}

	/* Frees any previous project and line loaded */
	document_free();

	path = gtk_tree_model_get_path(GTK_TREE_MODEL(gebr.ui_project_line->store), &iter);
	gtk_tree_model_get(GTK_TREE_MODEL (gebr.ui_project_line->store), &iter,
			  PL_FILENAME, &filename,
			  PL_TITLE, &title,
			  -1);

	/* Was a line selected? */
	if (gtk_tree_path_get_depth(path) == 2) {
		/* Line load */
		gtk_tree_model_iter_parent(model, &project_iter, &iter);

		gebr.line = GEOXML_LINE(document_load(filename));
		if (gebr.line == NULL) {
			gebr_message(ERROR, TRUE, FALSE, _("Unable to load line '%s'"), title);
			gebr_message(ERROR, FALSE, TRUE, _("Unable to load line '%s' from file '%s'"), title, filename);
			goto out;
		}
		gebr.doc = GEOXML_DOC(gebr.line);

		/* free before reuse */
		g_free(filename);
		g_free(title);

		/* new get from the parent */
		gtk_tree_model_iter_parent(model, &project_iter, &iter);
		gtk_tree_model_get(GTK_TREE_MODEL (gebr.ui_project_line->store), &project_iter,
			  PL_FILENAME, &filename,
			  PL_TITLE, &title,
			  -1);
	} else
		gebr.line = NULL;

	/* Project load */
	gebr.project = GEOXML_PROJECT(document_load(filename));
	if (gebr.project == NULL) {
		gebr_message(ERROR, TRUE, FALSE, _("Unable to load project '%s'"), title);
		gebr_message(ERROR, FALSE, TRUE, _("Unable to load project '%s' from file '%s'"), title, filename);
		goto out;
	}
	if (gtk_tree_path_get_depth(path) == 1)
		gebr.doc = GEOXML_DOC(gebr.project);

out:	g_free(filename);
	g_free(title);
}

/*
 * Function: project_line_info_update
 * Update information shown about the selected project or line
 */
void
project_line_info_update(void)
{
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreeIter		iter;

	gchar *		        markup;
	GString *	        text;

	gboolean		is_project;

	if (gebr.doc == NULL) {
		gtk_label_set_text(GTK_LABEL(gebr.ui_project_line->info.title), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_project_line->info.description), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_project_line->info.created_label), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_project_line->info.created), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_project_line->info.modified_label), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_project_line->info.modified), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_project_line->info.numberoflines), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_project_line->info.author), "");

		g_object_set(gebr.ui_project_line->info.help, "sensitive", FALSE, NULL);
		return;
	}

	/* initialization */
	is_project = (geoxml_document_get_type(gebr.doc) == GEOXML_DOCUMENT_TYPE_PROJECT) ? TRUE : FALSE;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_project_line->view));
	gtk_tree_selection_get_selected(selection, &model, &iter);

	/* Title in bold */
	markup = is_project
		? g_markup_printf_escaped("<b>%s %s</b>", _("Project"), geoxml_document_get_title(gebr.doc))
		: g_markup_printf_escaped("<b>%s %s</b>", _("Line"), geoxml_document_get_title(gebr.doc));
	gtk_label_set_markup(GTK_LABEL(gebr.ui_project_line->info.title), markup);
	g_free(markup);

	/* Description in italic */
	markup = g_markup_printf_escaped("<i>%s</i>", geoxml_document_get_description(GEOXML_DOC(gebr.doc)));
	gtk_label_set_markup(GTK_LABEL(gebr.ui_project_line->info.description), markup);
	g_free(markup);

	if (is_project) {
		gint	nlines;

		nlines = gtk_tree_model_iter_n_children(model, &iter);
		markup = nlines != 0
			? nlines == 1
				? g_markup_printf_escaped(_("This project has 1 line"))
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
	gtk_label_set_text(GTK_LABEL(gebr.ui_project_line->info.created), geoxml_document_get_date_created(gebr.doc));
	gtk_label_set_text(GTK_LABEL(gebr.ui_project_line->info.modified), geoxml_document_get_date_modified(gebr.doc));

	/* Author and email */
	text = g_string_new(NULL);
	g_string_printf(text, "%s <%s>",
			geoxml_document_get_author(GEOXML_DOC(gebr.doc)),
			geoxml_document_get_email(GEOXML_DOC(gebr.doc)));
	gtk_label_set_text(GTK_LABEL(gebr.ui_project_line->info.author), text->str);
	g_string_free(text, TRUE);

	/* Info button */
	g_object_set(gebr.ui_project_line->info.help,
		"sensitive", strlen(geoxml_document_get_help(gebr.doc)) ? TRUE : FALSE, NULL);
}

static void
project_line_show_help(void)
{
	gchar * title;

	title = (geoxml_document_get_type(gebr.doc) == GEOXML_DOCUMENT_TYPE_PROJECT)
		? _("Project report") : _("Line report");
	help_show(geoxml_document_get_help(gebr.doc), title);

	return;
}
