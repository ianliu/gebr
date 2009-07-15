/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
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
 * File: ui_document.c
 */

#include <string.h>

#include <libgebr/intl.h>

#include "ui_document.h"
#include "gebr.h"
#include "flow.h"
#include "ui_help.h"
#include "document.h"
#include "ui_project_line.h"

/*
 * Prototypes
 */

static void
document_properties_actions(GtkDialog * dialog, gint arg1, struct ui_document_properties * ui_document_properties);

/*
 * Section: Public
 * Public functions.
 */

/*
 * Function: document_properties_setup_ui
 * Show the _document_ properties in a dialog
 * Create the user interface for editing _document_(flow, line or project) properties,
 * like author, email, report, etc.
 *
 * Return:
 * The structure containing relevant data. It will be automatically freed when the
 * dialog closes.
 */
struct ui_document_properties *
document_properties_setup_ui(GeoXmlDocument * document)
{
	struct ui_document_properties *	ui_document_properties;

	GtkWidget *			dialog;
	GtkWidget *			table;
	GtkWidget *			label;
        GtkWidget *			help_show_button;
	GtkWidget *			help_hbox;
	GString *                       dialog_title;


	if (document == NULL) {
		gebr_message(LOG_ERROR, TRUE, FALSE, _("Nothing selected"));
		return NULL;
	}

	/* alloc */
	ui_document_properties = g_malloc(sizeof(struct ui_document_properties));
	ui_document_properties->document = document;

	dialog_title = g_string_new(NULL);
	switch (geoxml_document_get_type(document)) {
	case GEOXML_DOCUMENT_TYPE_PROJECT:
		g_string_printf(dialog_title, _("Properties for project '%s'"), geoxml_document_get_title(document));
		break;
	case GEOXML_DOCUMENT_TYPE_LINE:
		g_string_printf(dialog_title, _("Properties for line '%s'"), geoxml_document_get_title(document));
		break;
	case GEOXML_DOCUMENT_TYPE_FLOW:
		g_string_printf(dialog_title, _("Properties for flow '%s'"), geoxml_document_get_title(document));
		break;
	default:
		g_string_printf(dialog_title, _("Properties for document '%s'"), geoxml_document_get_title(document));
		break;
	}

	dialog = gtk_dialog_new_with_buttons(dialog_title->str,
		GTK_WINDOW(gebr.window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		NULL);
	ui_document_properties->dialog = GTK_WIDGET(dialog);
	gtk_widget_set_size_request(dialog, 390, 260);
	g_signal_connect(dialog, "response",
		G_CALLBACK(document_properties_actions), ui_document_properties);
	g_signal_connect(dialog, "delete_event",
		GTK_SIGNAL_FUNC(gtk_widget_destroy), NULL);

	table = gtk_table_new(5, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, TRUE, TRUE, 0);

	/* Title */
	label = gtk_label_new(_("Title"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	ui_document_properties->title = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), ui_document_properties->title, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
	/* read */
	gtk_entry_set_text(GTK_ENTRY(ui_document_properties->title), geoxml_document_get_title(document));

	/* Description */
	label = gtk_label_new(_("Description"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	ui_document_properties->description = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), ui_document_properties->description, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	/* read */
	gtk_entry_set_text(GTK_ENTRY(ui_document_properties->description), geoxml_document_get_description(document));

	/* Report */
	label = gtk_label_new(_("Report"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 3, 3);
	help_hbox = gtk_hbox_new(FALSE,0);
	gtk_table_attach(GTK_TABLE(table), help_hbox, 1, 2, 2, 3,
		(GtkAttachOptions)(GTK_FILL),
		(GtkAttachOptions)(0), 0, 0);
	help_show_button = gtk_button_new_from_stock(GTK_STOCK_OPEN);
	gtk_box_pack_start(GTK_BOX(help_hbox), help_show_button, FALSE, FALSE, 0);
	g_signal_connect(GTK_OBJECT(help_show_button), "clicked",
                         (GCallback)help_show_callback, document);
	g_object_set(G_OBJECT(help_show_button), "relief", GTK_RELIEF_NONE, NULL);

	ui_document_properties->help = gtk_button_new_from_stock(GTK_STOCK_EDIT);
	gtk_box_pack_start(GTK_BOX(help_hbox), ui_document_properties->help, FALSE, FALSE, 0);
	g_signal_connect(GTK_OBJECT(ui_document_properties->help), "clicked",
			GTK_SIGNAL_FUNC(help_edit), document);
	g_object_set(G_OBJECT(ui_document_properties->help), "relief", GTK_RELIEF_NONE, NULL);

	/* Author */
	label = gtk_label_new(_("Author"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	ui_document_properties->author = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), ui_document_properties->author, 1, 2, 3, 4, GTK_FILL, GTK_FILL, 3, 3);
	/* read */
	gtk_entry_set_text(GTK_ENTRY(ui_document_properties->author), geoxml_document_get_author(document));

	/* User email */
	label = gtk_label_new(_("Email"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	ui_document_properties->email = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), ui_document_properties->email, 1, 2, 4, 5, GTK_FILL, GTK_FILL, 3, 3);
	/* read */
	gtk_entry_set_text(GTK_ENTRY(ui_document_properties->email), geoxml_document_get_email(document));

	/* frees */
	g_string_free(dialog_title, TRUE);

	gtk_widget_show_all(ui_document_properties->dialog);

	return ui_document_properties;
}

/*
 * Section: Private
 * Private functions.
 */

/*
 * Function: document_properties_actions
 * Take the appropriate action when the document properties dialog emmits
 * a response signal.
 */
static void
document_properties_actions(GtkDialog * dialog, gint arg1, struct ui_document_properties * ui_document_properties)
{
	switch (arg1) {
	case GTK_RESPONSE_OK: {
		GtkTreeSelection *		selection;
		GtkTreeModel *			model;
		GtkTreeIter			iter;

		const gchar *			old_title;
		const gchar *			new_title;
		gchar *				doc_type;

		enum GEOXML_DOCUMENT_TYPE	type;

		old_title = geoxml_document_get_title(ui_document_properties->document);
		new_title = gtk_entry_get_text(GTK_ENTRY(ui_document_properties->title));

		geoxml_document_set_title(ui_document_properties->document, new_title);
		geoxml_document_set_description(ui_document_properties->document,
			gtk_entry_get_text(GTK_ENTRY(ui_document_properties->description)));
		geoxml_document_set_author(ui_document_properties->document,
			gtk_entry_get_text(GTK_ENTRY(ui_document_properties->author)));
		geoxml_document_set_email(ui_document_properties->document,
			gtk_entry_get_text(GTK_ENTRY(ui_document_properties->email)));
		document_save(ui_document_properties->document);

		/* Update title in apropriated store */
		switch ((type = geoxml_document_get_type(ui_document_properties->document))) {
		case GEOXML_DOCUMENT_TYPE_PROJECT:
		case GEOXML_DOCUMENT_TYPE_LINE:
			project_line_get_selected(&iter, FALSE);
			gtk_tree_store_set(gebr.ui_project_line->store, &iter,
				PL_TITLE, geoxml_document_get_title(ui_document_properties->document),
				-1);
			doc_type = (type == GEOXML_DOCUMENT_TYPE_PROJECT) ? "project" : "line";
			project_line_info_update();
			break;
		case GEOXML_DOCUMENT_TYPE_FLOW:
			selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
			gtk_tree_selection_get_selected(selection, &model, &iter);
			gtk_list_store_set(gebr.ui_flow_browse->store, &iter,
				FB_TITLE, geoxml_document_get_title(ui_document_properties->document),
				-1);
			doc_type = "flow";
			flow_browse_info_update();
			break;
		default:
			goto out;
		}

		gebr_message(LOG_INFO, FALSE, TRUE, _("Properties of %s '%s' updated"), doc_type, old_title);
		if (strcmp(old_title, new_title) != 0)
			gebr_message(LOG_INFO, FALSE, TRUE, _("Renaming %s '%s' to '%s'"),
				doc_type, old_title, new_title);

		break;
	} default:
		break;
	}

	/* frees */
out:	gtk_widget_destroy(GTK_WIDGET(ui_document_properties->dialog));
	g_free(ui_document_properties);
}
