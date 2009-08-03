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
gboolean
document_properties_setup_ui(GeoXmlDocument * document)
{
	GtkWidget *	dialog;
	gint		ret;
	GString *	dialog_title;
	GtkWidget *	table;
	GtkWidget *	label;
	GtkWidget *	help_show_button;
	GtkWidget *	help_hbox;
	GtkWidget *	title;
	GtkWidget *	description;
	GtkWidget *	help;
	GtkWidget *	author;
	GtkWidget *	email;

	if (document == NULL) {
		gebr_message(LOG_ERROR, TRUE, FALSE, _("Nothing selected"));
		return FALSE;
	}

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
	dialog = GTK_WIDGET(dialog);
	gtk_widget_set_size_request(dialog, 390, 260);

	table = gtk_table_new(5, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, TRUE, TRUE, 0);

	/* Title */
	label = gtk_label_new(_("Title"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	title = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), title, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 3, 3);
	/* read */
	gtk_entry_set_text(GTK_ENTRY(title), geoxml_document_get_title(document));

	/* Description */
	label = gtk_label_new(_("Description"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	description = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), description, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 3, 3);
	/* read */
	gtk_entry_set_text(GTK_ENTRY(description), geoxml_document_get_description(document));

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

	help = gtk_button_new_from_stock(GTK_STOCK_EDIT);
	gtk_box_pack_start(GTK_BOX(help_hbox), help, FALSE, FALSE, 0);
	g_signal_connect(GTK_OBJECT(help), "clicked",
		GTK_SIGNAL_FUNC(help_edit), document);
	g_object_set(G_OBJECT(help), "relief", GTK_RELIEF_NONE, NULL);

	/* Author */
	label = gtk_label_new(_("Author"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	author = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), author, 1, 2, 3, 4, GTK_FILL, GTK_FILL, 3, 3);
	/* read */
	gtk_entry_set_text(GTK_ENTRY(author), geoxml_document_get_author(document));

	/* User email */
	label = gtk_label_new(_("Email"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
	email = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5, GTK_FILL, GTK_FILL, 3, 3);
	gtk_table_attach(GTK_TABLE(table), email, 1, 2, 4, 5, GTK_FILL, GTK_FILL, 3, 3);
	/* read */
	gtk_entry_set_text(GTK_ENTRY(email), geoxml_document_get_email(document));

	gtk_widget_show_all(dialog);
	switch (gtk_dialog_run(GTK_DIALOG(dialog))) {
	case GTK_RESPONSE_OK: {
		GtkTreeIter			iter;

		const gchar *			old_title;
		const gchar *			new_title;
		gchar *				doc_type;

		enum GEOXML_DOCUMENT_TYPE	type;

		old_title = geoxml_document_get_title(document);
		new_title = gtk_entry_get_text(GTK_ENTRY(title));

		geoxml_document_set_title(document, new_title);
		geoxml_document_set_description(document, gtk_entry_get_text(GTK_ENTRY(description)));
		geoxml_document_set_author(document, gtk_entry_get_text(GTK_ENTRY(author)));
		geoxml_document_set_email(document, gtk_entry_get_text(GTK_ENTRY(email)));
		document_save(document);

		/* Update title in apropriated store */
		switch ((type = geoxml_document_get_type(document))) {
		case GEOXML_DOCUMENT_TYPE_PROJECT:
		case GEOXML_DOCUMENT_TYPE_LINE:
			project_line_get_selected(&iter, DontWarnUnselection);
			gtk_tree_store_set(gebr.ui_project_line->store, &iter,
				PL_TITLE, geoxml_document_get_title(document),
				-1);
			doc_type = (type == GEOXML_DOCUMENT_TYPE_PROJECT) ? "project" : "line";
			project_line_info_update();
			break;
		case GEOXML_DOCUMENT_TYPE_FLOW:
			flow_browse_get_selected(&iter, FALSE);
			gtk_list_store_set(gebr.ui_flow_browse->store, &iter,
				FB_TITLE, geoxml_document_get_title(document),
				-1);
			doc_type = "flow";
			flow_browse_info_update();
			break;
		default:
			break;
		}

		gebr_message(LOG_INFO, FALSE, TRUE, _("Properties of %s '%s' updated"), doc_type, old_title);
		if (strcmp(old_title, new_title) != 0)
			gebr_message(LOG_INFO, FALSE, TRUE, _("Renaming %s '%s' to '%s'"),
				doc_type, old_title, new_title);

		ret = TRUE;
		break;
	} default:
		ret = FALSE;
	}

	gtk_widget_destroy(GTK_WIDGET(dialog));
	g_string_free(dialog_title, TRUE);

	return ret;
}

/*
 * Section: Private
 * Private functions.
 */
