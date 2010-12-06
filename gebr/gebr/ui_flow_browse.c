/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or *(at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see
 *   <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <gdk/gdkkeysyms.h>

#include <glib/gi18n.h>
#include <libgebr/date.h>
#include <libgebr/gui/gebr-gui-utils.h>

#include "ui_flow_browse.h"
#include "gebr.h"
#include "document.h"
#include "line.h"
#include "flow.h"
#include "ui_flow.h"
#include "ui_help.h"
#include "callbacks.h"

/*
 * Prototypes
 */

static void flow_browse_load(void);
static void
flow_browse_on_row_activated(GtkTreeView * tree_view, GtkTreePath * path,
			     GtkTreeViewColumn * column, GebrUiFlowBrowse *ui_flow_browse);
static GtkMenu *flow_browse_popup_menu(GtkWidget * widget, GebrUiFlowBrowse *ui_flow_browse);
static void flow_browse_on_revision_revert_activate(GtkMenuItem * menu_item, GebrGeoXmlRevision * revision);
static void flow_browse_on_revision_delete_activate(GtkWidget * menu_item, GebrGeoXmlRevision * revision);
static void flow_browse_on_flow_move(void);

GebrUiFlowBrowse *flow_browse_setup_ui(GtkWidget * revisions_menu)
{
	GebrUiFlowBrowse *ui_flow_browse;

	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	GtkWidget *page;
	GtkWidget *hpanel;
	GtkWidget *frame;
	GtkWidget *scrolled_window;
	GtkWidget *infopage;
	GtkWidget *table;
	gint row;

	/* alloc */
	ui_flow_browse = g_new(GebrUiFlowBrowse, 1);
	ui_flow_browse->revisions_menu = revisions_menu;

	/* Create flow browse page */
	page = gtk_vbox_new(FALSE, 0);
	ui_flow_browse->widget = page;
	hpanel = gtk_hpaned_new();
	gtk_container_add(GTK_CONTAINER(page), hpanel);

	/*
	 * Left side: flow list
	 */
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_paned_pack1(GTK_PANED(hpanel), scrolled_window, FALSE, FALSE);
	gtk_widget_set_size_request(scrolled_window, 300, -1);

	ui_flow_browse->store = gtk_list_store_new(FB_N_COLUMN,
						   G_TYPE_STRING,	/* Name (title for libgeoxml) */
						   G_TYPE_STRING,	/* Filename */
						   G_TYPE_POINTER,	/* GebrGeoXmlFlow pointer */
						   G_TYPE_POINTER,	/* GebrGeoXmlLineFlow pointer */
						   G_TYPE_POINTER	/* Last queue hash table */);

	ui_flow_browse->view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ui_flow_browse->store));
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(ui_flow_browse->view), TRUE);
	gtk_container_add(GTK_CONTAINER(scrolled_window), ui_flow_browse->view);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_flow_browse->view)),
				    GTK_SELECTION_MULTIPLE);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ui_flow_browse->view), FALSE);
	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(ui_flow_browse->view),
						  (GebrGuiGtkPopupCallback) flow_browse_popup_menu, ui_flow_browse);
	gebr_gui_gtk_tree_view_set_gebr_geoxml_sequence_moveable(GTK_TREE_VIEW(ui_flow_browse->view),
								 FB_LINE_FLOW_POINTER,
								 (GebrGuiGtkTreeViewMoveSequenceCallback)
								 flow_browse_on_flow_move, NULL);
	g_signal_connect(ui_flow_browse->view, "row-activated", G_CALLBACK(flow_browse_on_row_activated),
			 ui_flow_browse);
	g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_flow_browse->view)), "changed",
			 G_CALLBACK(flow_browse_load), ui_flow_browse);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_flow_browse->view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", FB_TITLE);

	/*
	 * Right side: flow info
	 */
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_paned_pack2(GTK_PANED(hpanel), scrolled_window, TRUE, FALSE);
	frame = gtk_frame_new(_("Details"));
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), frame);
	infopage = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), infopage);

	/* Title */
	ui_flow_browse->info.title = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_flow_browse->info.title), 0, 0);
	gtk_box_pack_start(GTK_BOX(infopage), ui_flow_browse->info.title, FALSE, TRUE, 0);

	/* Description */
	ui_flow_browse->info.description = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_flow_browse->info.description), 0, 0);
	gtk_box_pack_start(GTK_BOX(infopage), ui_flow_browse->info.description, FALSE, TRUE, 10);

	ui_flow_browse->info.rev_num = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_flow_browse->info.rev_num), 0, 0);
	gtk_box_pack_start(GTK_BOX(infopage), ui_flow_browse->info.rev_num, FALSE, TRUE, 10);

	/* Dates */
	table = gtk_table_new(4, 2, FALSE), row = 0;
	gtk_box_pack_start(GTK_BOX(infopage), table, FALSE, TRUE, 10);

	ui_flow_browse->info.created_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_flow_browse->info.created_label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_flow_browse->info.created_label, 0, 1, row, row + 1,
			 (GtkAttachOptions)GTK_FILL, (GtkAttachOptions)GTK_FILL, 3, 3);
	ui_flow_browse->info.created = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_flow_browse->info.created), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_flow_browse->info.created, 1, 2, row, row + 1, (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 3, 3), row++;

	ui_flow_browse->info.modified_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_flow_browse->info.modified_label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_flow_browse->info.modified_label, 0, 1, row, row + 1,
			 (GtkAttachOptions)GTK_FILL, (GtkAttachOptions)GTK_FILL, 3, 3);
	ui_flow_browse->info.modified = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_flow_browse->info.modified), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_flow_browse->info.modified, 1, 2, row, row + 1,
			 (GtkAttachOptions)GTK_FILL, (GtkAttachOptions)GTK_FILL, 3, 3), row++;

	ui_flow_browse->info.lastrun_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_flow_browse->info.lastrun_label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_flow_browse->info.lastrun_label, 0, 1, row, row + 1,
			 (GtkAttachOptions)GTK_FILL, (GtkAttachOptions)GTK_FILL, 3, 3);
	ui_flow_browse->info.lastrun = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_flow_browse->info.lastrun), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_flow_browse->info.lastrun, 1, 2, row, row + 1, (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 3, 3);

	/* I/O */
	table = gtk_table_new(3, 2, FALSE), row = 0;
	gtk_box_pack_start(GTK_BOX(infopage), table, FALSE, TRUE, 10);

	ui_flow_browse->info.server_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_flow_browse->info.server_label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_flow_browse->info.server_label, 0, 1, row, row + 1,
			 (GtkAttachOptions)GTK_FILL, (GtkAttachOptions)GTK_FILL, 3, 3);
	ui_flow_browse->info.server = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_flow_browse->info.server), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_flow_browse->info.server, 1, 2, row, row + 1, (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 3, 3), row++;

	ui_flow_browse->info.input_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_flow_browse->info.input_label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_flow_browse->info.input_label, 0, 1, row, row + 1,
			 (GtkAttachOptions)GTK_FILL, (GtkAttachOptions)GTK_FILL, 3, 3);
	ui_flow_browse->info.input = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_flow_browse->info.input), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_flow_browse->info.input, 1, 2, row, row + 1, (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 3, 3), row++;

	ui_flow_browse->info.output_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_flow_browse->info.output_label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_flow_browse->info.output_label, 0, 1, row, row + 1,
			 (GtkAttachOptions)GTK_FILL, (GtkAttachOptions)GTK_FILL, 3,
			 3);
	ui_flow_browse->info.output = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_flow_browse->info.output), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_flow_browse->info.output, 1, 2, row, row + 1, (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 3, 3),
	    row++;

	ui_flow_browse->info.error_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_flow_browse->info.error_label), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_flow_browse->info.error_label, 0, 1, row, row + 1,
			 (GtkAttachOptions)GTK_FILL, (GtkAttachOptions)GTK_FILL, 3,
			 3);
	ui_flow_browse->info.error = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_flow_browse->info.error), 0, 0);
	gtk_table_attach(GTK_TABLE(table), ui_flow_browse->info.error, 1, 2, row, row + 1, (GtkAttachOptions)GTK_FILL,
			 (GtkAttachOptions)GTK_FILL, 3, 3);
	row++;

	/* Help */
	
	GtkWidget * hbox;
	hbox = gtk_hbox_new(FALSE, 0);

	ui_flow_browse->info.help_view = gtk_button_new_with_label(_("View report"));
	ui_flow_browse->info.help_edit = gtk_button_new_with_label(_("Edit flow commentaries"));

	gtk_box_pack_start(GTK_BOX(hbox), ui_flow_browse->info.help_view, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), ui_flow_browse->info.help_edit, TRUE, TRUE, 0);
	g_signal_connect(GTK_OBJECT(ui_flow_browse->info.help_view), "clicked",
			 G_CALLBACK(flow_browse_show_help), NULL);
	g_signal_connect(GTK_OBJECT(ui_flow_browse->info.help_edit), "clicked",
			 G_CALLBACK(flow_browse_edit_help), NULL);
	gtk_box_pack_end(GTK_BOX(infopage), hbox, FALSE, TRUE, 0);
	g_object_set(ui_flow_browse->info.help_view, "sensitive", FALSE, NULL);
	g_object_set(ui_flow_browse->info.help_edit, "sensitive", FALSE, NULL);

	/* Author */
	ui_flow_browse->info.author = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(ui_flow_browse->info.author), 0, 0);
	gtk_box_pack_end(GTK_BOX(infopage), ui_flow_browse->info.author, FALSE, TRUE, 0);

	return ui_flow_browse;
}

void flow_browse_info_update(void)
{
	if (gebr.flow == NULL) {
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.title), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.description), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.server_label), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.server), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.input_label), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.input), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.output_label), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.output), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.error_label), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.error), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.author), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.created), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.created_label), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.modified), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.modified_label), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.lastrun), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.lastrun_label), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.rev_num), "");

		g_object_set(gebr.ui_flow_browse->info.help_view, "sensitive", FALSE, NULL);
		g_object_set(gebr.ui_flow_browse->info.help_edit, "sensitive", FALSE, NULL);
		gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group, "flow_dump"), FALSE);

		navigation_bar_update();
		return;
	}

	gchar *markup;
	GString *text;

	/* Title in bold */
	markup = g_markup_printf_escaped("<b>%s</b>", gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(gebr.flow)));
	gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.title), markup);
	g_free(markup);
	/* Description in italic */
	markup = g_markup_printf_escaped("<i>%s</i>", gebr_geoxml_document_get_description(GEBR_GEOXML_DOC(gebr.flow)));
	gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.description), markup);
	g_free(markup);
	/* Date labels */
	markup = g_markup_printf_escaped("<b>%s</b>", _("Created:"));
	gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.created_label), markup);
	g_free(markup);
	markup = g_markup_printf_escaped("<b>%s</b>", _("Modified:"));
	gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.modified_label), markup);
	g_free(markup);
	markup = g_markup_printf_escaped("<b>%s</b>", _("Last run:"));
	gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.lastrun_label), markup);
	g_free(markup);
	/* Dates */
	gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.created),
			   gebr_localized_date(gebr_geoxml_document_get_date_created(GEBR_GEOXML_DOC(gebr.flow))));
	gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.modified),
			   gebr_localized_date(gebr_geoxml_document_get_date_modified(GEBR_GEOXML_DOC(gebr.flow))));
	gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.lastrun),
			   gebr_localized_date(gebr_geoxml_flow_get_date_last_run(gebr.flow)));

        /* Revisions */
        {
                char * str_tmp;
                glong nrevision = gebr_geoxml_flow_get_revisions_number(gebr.flow);

                switch (nrevision){
                case 0:
                        str_tmp = g_strdup( _("This flow has never had its state saved"));
                        break;
                case 1:
                        str_tmp = g_strdup(_("This flow had its state saved once"));
                        break;
                case 2:
                        str_tmp = g_strdup(_("This flow had its state saved twice"));
                        break;
                default:
                        str_tmp = g_strdup_printf(_("This flow had its state saved %ld times"), nrevision);
                }
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.rev_num), str_tmp);
		g_free(str_tmp);
	}

	/* I/O labels */
	markup = g_markup_printf_escaped("<b>%s</b>", _("Server:"));
	gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.server_label), markup);
	g_free(markup);
	markup = g_markup_printf_escaped("<b>%s</b>", _("Input:"));
	gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.input_label), markup);
	g_free(markup);
	markup = g_markup_printf_escaped("<b>%s</b>", _("Output:"));
	gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.output_label), markup);
	g_free(markup);
	markup = g_markup_printf_escaped("<b>%s</b>", _("Error log:"));
	gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.error_label), markup);
	g_free(markup);
	/* Server */
	if (gebr.flow_server != NULL) //avoid crash on exit
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.server),
				   server_get_name_from_address(gebr_geoxml_flow_server_get_address(gebr.flow_server)));
	else
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.input), _("(none)"));
	/* Input file */
	if (gebr.flow_server != NULL && strlen(gebr_geoxml_flow_server_io_get_input(gebr.flow_server)))
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.input),
				   gebr_geoxml_flow_server_io_get_input(gebr.flow_server));
	else
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.input), _("(none)"));
	/* Output file */
	if (gebr.flow_server != NULL && strlen(gebr_geoxml_flow_server_io_get_output(gebr.flow_server)))
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.output),
				   gebr_geoxml_flow_server_io_get_output(gebr.flow_server));
	else
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.output), _("(none)"));
	/* Error file */
	if (gebr.flow_server != NULL && strlen(gebr_geoxml_flow_server_io_get_error(gebr.flow_server)))
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.error),
				   gebr_geoxml_flow_server_io_get_error(gebr.flow_server));
	else
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.error), _("(none)"));

	/* Author and email */
	text = g_string_new(NULL);
	g_string_printf(text, "%s <%s>",
			gebr_geoxml_document_get_author(GEBR_GEOXML_DOC(gebr.flow)),
			gebr_geoxml_document_get_email(GEBR_GEOXML_DOC(gebr.flow)));
	gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.author), text->str);
	g_string_free(text, TRUE);

	/* Info button */
	gboolean help_exists = gebr.flow != NULL && (strlen(gebr_geoxml_document_get_help(GEBR_GEOXML_DOCUMENT(gebr.flow))) ? TRUE : FALSE);

	g_object_set(gebr.ui_flow_browse->info.help_view, "sensitive", help_exists, NULL);

	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group, "flow_view"), help_exists);

	g_object_set(gebr.ui_flow_browse->info.help_edit, "sensitive", TRUE, NULL);

	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group, "flow_dump"), TRUE);

	navigation_bar_update();
}

gboolean flow_browse_get_selected(GtkTreeIter * iter, gboolean warn_unselected)
{
	if (!gebr_gui_gtk_tree_view_get_selected(GTK_TREE_VIEW(gebr.ui_flow_browse->view), iter)) {
		if (warn_unselected)
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("No flow selected"));
		return FALSE;
	}

	return TRUE;
}

void flow_browse_select_iter(GtkTreeIter * iter)
{
	gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(gebr.ui_flow_browse->view), iter);
}

void flow_browse_single_selection(void)
{
	gebr_gui_gtk_tree_view_turn_to_single_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
}

void flow_browse_load_revision(GebrGeoXmlRevision * revision, gboolean new)
{
	GString *label;
	gchar *date;
	gchar *comment;

	GtkWidget *submenu;
	GtkWidget *menu_item;
	GtkWidget *menu_item_rev;

	gebr_geoxml_flow_get_revision_data(revision, NULL, &date, &comment);
	label = g_string_new(NULL);
	g_string_printf(label, "%s: %s", date, comment);

	submenu = gtk_menu_new();
	menu_item_rev = gtk_menu_item_new_with_label(label->str);

	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_REVERT_TO_SAVED, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(submenu), menu_item);
	g_signal_connect(menu_item, "activate", G_CALLBACK(flow_browse_on_revision_revert_activate), revision);

	menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE, NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(submenu), menu_item);
	g_object_set_data(G_OBJECT(menu_item), "menu-item-to-be-removed", menu_item_rev);
	g_signal_connect(menu_item, "activate", G_CALLBACK(flow_browse_on_revision_delete_activate), revision);

	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item_rev), submenu);
	gtk_widget_show_all(submenu);
	gtk_widget_show(menu_item_rev);
	if (new)
		gtk_menu_shell_prepend(GTK_MENU_SHELL(gebr.ui_flow_browse->revisions_menu), menu_item_rev);
	else
		gtk_menu_shell_append(GTK_MENU_SHELL(gebr.ui_flow_browse->revisions_menu), menu_item_rev);
}

/**
 * \internal
 * Load a selected flow from file when selected in "Flow Browse".
 */
static void flow_browse_load(void)
{
	GtkTreeIter iter;
	GtkTreeIter server_iter;

	gchar *filename;
	gchar *title;

	GebrGeoXmlSequence *revision;
	GebrGeoXmlFlowServer *flow_server;

	flow_free();
	if (!flow_browse_get_selected(&iter, FALSE))
		return;

	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group, "flow_change_revision"),
				 gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection
									(GTK_TREE_VIEW(gebr.ui_flow_browse->view))) >
				 1 ? FALSE : TRUE);

	/* load its filename and title */
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter,
			   FB_FILENAME, &filename,
			   FB_TITLE, &title,
			   FB_XMLPOINTER, &gebr.flow,
			   -1);

	/* free previous flow and load it */
	gtk_widget_set_sensitive(gebr.ui_flow_edition->queue_combobox, TRUE);
	gtk_widget_set_sensitive(gebr.ui_flow_edition->server_combobox, TRUE);
	flow_edition_load_components();

	/* load revisions */
	gebr_geoxml_flow_get_revision(gebr.flow, &revision, 0);
	for (; revision != NULL; gebr_geoxml_sequence_next(&revision))
		flow_browse_load_revision(GEBR_GEOXML_REVISION(revision), FALSE);

	/* if there are no local servers, introduce local server */
	if (gebr_geoxml_flow_get_servers_number(gebr.flow) == 0) {
		flow_server = gebr_geoxml_flow_append_server(gebr.flow);
		gebr_geoxml_flow_server_set_address(flow_server, "127.0.0.1");
		gebr_geoxml_flow_server_io_set_input(flow_server, gebr_geoxml_flow_io_get_input(gebr.flow));
		gebr_geoxml_flow_server_io_set_output(flow_server, gebr_geoxml_flow_io_get_output(gebr.flow));
		gebr_geoxml_flow_server_io_set_error(flow_server, gebr_geoxml_flow_io_get_error(gebr.flow));
		gebr_geoxml_flow_io_set_input(gebr.flow, "");
		gebr_geoxml_flow_io_set_output(gebr.flow, "");
		gebr_geoxml_flow_io_set_error(gebr.flow, "");
		document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), FALSE, TRUE);
	} else {
		GebrGeoXmlSequence *first_server;

		gebr_geoxml_flow_get_server(gebr.flow, &first_server, 0);
		flow_server = GEBR_GEOXML_FLOW_SERVER(first_server);
	}

	/* select last edited server */
	if (server_find_address(gebr_geoxml_flow_server_get_address(flow_server), &server_iter))
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(gebr.ui_flow_edition->server_combobox), &server_iter);
	else
		gtk_combo_box_set_active(GTK_COMBO_BOX(gebr.ui_flow_edition->server_combobox), 0);
	flow_edition_on_server_changed();

	flow_browse_info_update();

	g_free(filename);
	g_free(title);
}

/**
 */
void flow_browse_show_help(void)
{
	gebr_help_show(GEBR_GEOXML_OBJECT(gebr.flow), FALSE);
}

/**
 */
void flow_browse_edit_help(void)
{
	gebr_help_edit_document(GEBR_GEOXML_DOC(gebr.flow));
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);
}

/**
 * \internal
 * Go to flow components tab
 */
static void
flow_browse_on_row_activated(GtkTreeView * tree_view, GtkTreePath * path,
			     GtkTreeViewColumn * column, GebrUiFlowBrowse *ui_flow_browse)
{
	gebr.config.current_notebook = 2;
	gtk_notebook_set_current_page(GTK_NOTEBOOK(gebr.notebook), gebr.config.current_notebook);
}

/**
 * \internal
 * Build popup menu
 */
static GtkMenu *flow_browse_popup_menu(GtkWidget * widget, GebrUiFlowBrowse *ui_flow_browse)
{
	GtkWidget *menu;
	GtkWidget *menu_item;

	GtkTreeIter iter;

	/* no line, no new flow possible */
	if (gebr.line == NULL)
		return NULL;

	menu = gtk_menu_new();

	if (!flow_browse_get_selected(&iter, FALSE)) {
		gtk_container_add(GTK_CONTAINER(menu),
				  gtk_action_create_menu_item(gtk_action_group_get_action
							      (gebr.action_group, "flow_new")));
		gtk_container_add(GTK_CONTAINER(menu),
				  gtk_action_create_menu_item(gtk_action_group_get_action
							      (gebr.action_group, "flow_paste")));
		goto out;
	}

	/* Move top */
	if (gebr_gui_gtk_list_store_can_move_up(ui_flow_browse->store, &iter) == TRUE) {
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_GOTO_TOP, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect(menu_item, "activate", G_CALLBACK(line_move_flow_top), NULL);
	}
	/* Move bottom */
	if (gebr_gui_gtk_list_store_can_move_down(ui_flow_browse->store, &iter) == TRUE) {
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_GOTO_BOTTOM, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect(menu_item, "activate", G_CALLBACK(line_move_flow_bottom), NULL);
	}
	/* separator */
	if (gebr_gui_gtk_list_store_can_move_up(ui_flow_browse->store, &iter) == TRUE ||
	    gebr_gui_gtk_list_store_can_move_down(ui_flow_browse->store, &iter) == TRUE)
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group, "flow_new")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group, "flow_copy")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group, "flow_paste")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group, "flow_delete")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group, "flow_properties")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group, "flow_view")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group, "flow_edit")));

	menu_item = gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group, "flow_change_revision"));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), ui_flow_browse->revisions_menu);
	gtk_container_add(GTK_CONTAINER(menu), menu_item);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
	/* gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group, "flow_io"))); */
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group, "flow_execute")));

 out:	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}

/** 
 * \internal
 * Change flow to selected revision
 */
static void flow_browse_on_revision_revert_activate(GtkMenuItem * menu_item, GebrGeoXmlRevision * revision)
{
	if (gebr_gui_confirm_action_dialog(_("Backup current state?"),
					   _("You are about to revert to a previous state. "
					     "The current flow will be lost after this action. "
					     "Do you want to save the current flow state?")) && !flow_revision_save())
		return;

	gchar *date;
	gchar *comment;
	gboolean report_merged = FALSE;

	gebr_geoxml_flow_get_revision_data(revision, NULL, &date, &comment);
	if (!gebr_geoxml_flow_change_to_revision(gebr.flow, revision, &report_merged)) {
		document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Could not revert to state '%s' ('%s')."), comment, date);
		return;
	}

	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);

	if (report_merged)
		gebr_message(GEBR_LOG_INFO, TRUE, TRUE, _("Reverted to state '%s' ('%s'), and merged report to current"), comment, date);
	else
		gebr_message(GEBR_LOG_INFO, TRUE, TRUE, _("Reverted to state '%s' ('%s')."), comment, date);
	flow_browse_load();
}

/**
 */
static void flow_browse_on_revision_delete_activate(GtkWidget * widget, GebrGeoXmlRevision * revision)
{
	gboolean response;
	gpointer menu_item;
	response = gebr_gui_confirm_action_dialog(_("Remove this revision permanently?"),
						  _("If you choose to remove this revision, "
						    "you will not be able to recover it later."));
	if (response) {
		gebr_geoxml_sequence_remove(GEBR_GEOXML_SEQUENCE(revision));
		document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, TRUE);
		menu_item = g_object_get_data(G_OBJECT(widget), "menu-item-to-be-removed");
		gtk_widget_destroy(GTK_WIDGET(menu_item));
		flow_browse_info_update();
	}
}

/**
 * \internal
 * Saves the current selected line.
 */
static void flow_browse_on_flow_move(void)
{
	document_save(GEBR_GEOXML_DOC(gebr.line), TRUE, TRUE);
}
