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

static void flow_browse_on_row_activated(GtkTreeView * tree_view, GtkTreePath * path,
					 GtkTreeViewColumn * column, GebrUiFlowBrowse *ui_flow_browse);

static void revisions_on_row_activated(GtkTreeView * tree_view, GtkTreePath * path,
                                       GtkTreeViewColumn * column, GebrUiFlowBrowse *ui_flow_browse);

static GtkMenu *flow_browse_popup_menu(GtkWidget * widget, GebrUiFlowBrowse *ui_flow_browse);

static GtkMenu *revisions_popup_menu(GtkWidget * widget,
                                     GebrUiFlowBrowse *ui_flow_browse);

static void revisions_on_row_activated(GtkTreeView * tree_view,
                                       GtkTreePath * path,
                                       GtkTreeViewColumn * column,
                                       GebrUiFlowBrowse *fb);

static gboolean revision_on_key_pressed(GtkWidget *view,
                                 GdkEventKey *key,
                                 GebrUiFlowBrowse *fb);

static void flow_browse_on_flow_move(void);
static void update_speed_slider_sensitiveness(GebrUiFlowBrowse *ufb);


static void
flow_revisions_func(GtkTreeViewColumn *tree_column,
                    GtkCellRenderer *cell,
                    GtkTreeModel *model,
                    GtkTreeIter *iter,
                    gpointer data)
{
	gboolean active;
	const gchar *comment, *date;

	gtk_tree_model_get(model, iter,
	                   REV_COMMENT, &comment,
	                   REV_DATE, &date,
	                   REV_ACTIVE, &active,
	                   -1);

	gchar *text;
	if (active)
		text = g_markup_printf_escaped("<b>%s <span size='small'>(%s)</span> (current)</b>", comment, date);
	else
		text = g_markup_printf_escaped("%s <span size='small'>(%s)</span>", comment, date);

	g_object_set(cell, "markup", text, NULL);

	g_free(text);
}

static gboolean
on_revisions_focus_in(GtkWidget     *widget,
                      GdkEventFocus *event,
                      GebrUiFlowBrowse *fb)
{
	gtk_window_remove_accel_group(GTK_WINDOW(gebr.window), gebr.accel_group_array[gebr.last_notebook]);
	return FALSE;
}

static gboolean
on_revisions_focus_out(GtkWidget     *widget,
                      GdkEventFocus *event,
                      GebrUiFlowBrowse *fb)
{
	gtk_window_add_accel_group(GTK_WINDOW(gebr.window), gebr.accel_group_array[gebr.last_notebook]);
	return FALSE;

}

GebrUiFlowBrowse *flow_browse_setup_ui()
{
	GebrUiFlowBrowse *ui_flow_browse;

	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	GtkWidget *page;
	GtkWidget *hpanel;
	GtkWidget *scrolled_window;
	GtkWidget *scrolled_window_info;
	GtkWidget *scrolled_window_rev;
	GtkWidget *infopage;

	/* alloc */
	ui_flow_browse = g_new(GebrUiFlowBrowse, 1);

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
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_flow_browse->view));
	g_signal_connect(selection, "changed", G_CALLBACK(flow_browse_load), NULL);
	g_signal_connect_swapped(selection, "changed", G_CALLBACK(update_speed_slider_sensitiveness), ui_flow_browse);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_flow_browse->view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", FB_TITLE);


	/*
	 * Right side: flow info tab
	 */
	scrolled_window_info = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window_info),
	                               GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	/* Get glade file */
	ui_flow_browse->info.builder_flow = gtk_builder_new();
	gtk_builder_add_from_file(ui_flow_browse->info.builder_flow, GEBR_GLADE_DIR "/flow-properties.glade", NULL);

	infopage = gtk_vbox_new(FALSE, 0);
	GtkWidget *infopage_flow = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "main"));

	ui_flow_browse->info_window = infopage_flow;

	gtk_box_pack_start(GTK_BOX(infopage), infopage_flow, FALSE, TRUE, 0);

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);

	if (maestro && gebr_maestro_server_get_state(maestro) == SERVER_STATE_LOGGED)
		ui_flow_browse->warn_window = gtk_label_new(_("No Line is selected\n"));
	else
		ui_flow_browse->warn_window = gtk_label_new(_("The Maestro of this Line is disconnected,\nthen you cannot edit flows.\n"
							      "Try changing its maestro or connecting it."));
	gtk_widget_set_sensitive(ui_flow_browse->warn_window, FALSE);
	gtk_box_pack_start(GTK_BOX(infopage), ui_flow_browse->warn_window, TRUE, TRUE, 0);

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window_info), infopage);


	/* Title */
	ui_flow_browse->info.title = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "label_title"));

	/* Description */
	ui_flow_browse->info.description = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "label_description"));

	/* Revision */
	ui_flow_browse->info.rev_num = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "desc_revision"));

	/* Dates */
	ui_flow_browse->info.created_label = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "created_title"));
	ui_flow_browse->info.created = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "created_label"));

	ui_flow_browse->info.modified_label = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "modified_title"));
	ui_flow_browse->info.modified = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "modified_label"));

	/* Last execution */
	ui_flow_browse->info.lastrun = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "lastrun_label"));

	/* I/O */
	ui_flow_browse->info.input_label = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "input_title"));
	ui_flow_browse->info.input = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "input_label"));

	ui_flow_browse->info.output_label = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "output_title"));
	ui_flow_browse->info.output = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "output_label"));

	ui_flow_browse->info.error_label = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "log_title"));
	ui_flow_browse->info.error = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "log_label"));

	/* Author */
	ui_flow_browse->info.author = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "linkbutton_email"));

	/* Help */
	GtkWidget * hbox_help;
	hbox_help = gtk_hbox_new(FALSE, 0);

	ui_flow_browse->info.help_view = gtk_button_new_with_label(_("View Report"));
	ui_flow_browse->info.help_edit = gtk_button_new_with_label(_("Edit Comments"));

	gtk_box_pack_start(GTK_BOX(hbox_help), ui_flow_browse->info.help_view, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox_help), ui_flow_browse->info.help_edit, TRUE, TRUE, 0);
	g_signal_connect(GTK_OBJECT(ui_flow_browse->info.help_view), "clicked",
			 G_CALLBACK(flow_browse_show_help), NULL);
	g_signal_connect(GTK_OBJECT(ui_flow_browse->info.help_edit), "clicked",
			 G_CALLBACK(flow_browse_edit_help), NULL);

	gtk_box_pack_end(GTK_BOX(infopage), hbox_help, FALSE, TRUE, 0);
	g_object_set(ui_flow_browse->info.help_view, "sensitive", FALSE, NULL);
	g_object_set(ui_flow_browse->info.help_edit, "sensitive", FALSE, NULL);

	/*
	 * Right side: revisions tab
	 */
	GtkWidget *revpage;

	scrolled_window_rev = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window_rev),
	                               GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	revpage = gtk_vbox_new(FALSE, 0);

	ui_flow_browse->rev_main = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "main_rev"));

	ui_flow_browse->revpage_main = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "revisions_main"));

	ui_flow_browse->revpage_warn = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "revisions_warn"));

	gtk_widget_show(ui_flow_browse->revpage_main);
	gtk_widget_hide(ui_flow_browse->revpage_warn);

	ui_flow_browse->rev_view = GTK_WIDGET(gtk_builder_get_object(ui_flow_browse->info.builder_flow, "rev_treeview"));

	ui_flow_browse->rev_store = gtk_tree_store_new(REV_N_COLUMN,
	                                               G_TYPE_STRING,	/* Comment */
	                                               G_TYPE_STRING,	/* Date */
	                                               G_TYPE_POINTER,	/* GebrGeoXmlRevision pointer */
	                                               G_TYPE_BOOLEAN	/* Current */
	                                               );

	gtk_tree_view_set_model(GTK_TREE_VIEW(ui_flow_browse->rev_view), GTK_TREE_MODEL(ui_flow_browse->rev_store));
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(ui_flow_browse->rev_view), TRUE);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_flow_browse->rev_view)),
	                            GTK_SELECTION_SINGLE);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ui_flow_browse->rev_view), TRUE);
	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(ui_flow_browse->rev_view),
	                                          (GebrGuiGtkPopupCallback) revisions_popup_menu, ui_flow_browse);

	g_signal_connect(ui_flow_browse->rev_view, "row-activated", G_CALLBACK(revisions_on_row_activated),
	                 ui_flow_browse);
	g_signal_connect(ui_flow_browse->rev_view, "key-press-event", G_CALLBACK(revision_on_key_pressed),
	                 ui_flow_browse);
	g_signal_connect(ui_flow_browse->rev_view, "focus-in-event", G_CALLBACK(on_revisions_focus_in),
	                ui_flow_browse);
	g_signal_connect(ui_flow_browse->rev_view, "focus-out-event", G_CALLBACK(on_revisions_focus_out),
	                ui_flow_browse);


	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_flow_browse->rev_view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", REV_COMMENT);
	gtk_tree_view_column_set_cell_data_func(col, renderer, flow_revisions_func,
	                                        NULL, NULL);

	gtk_box_pack_start(GTK_BOX(revpage), ui_flow_browse->rev_main, TRUE, TRUE, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window_rev), revpage);


	/**
	 * Create Notebook
	 */
	GtkWidget *notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrolled_window_info, gtk_label_new(_("Details")));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrolled_window_rev, gtk_label_new(_("Revisions")));

	gtk_paned_pack2(GTK_PANED(hpanel), notebook, TRUE, FALSE);

	return ui_flow_browse;
}

void flow_browse_info_update(void)
{
	if (gebr.flow == NULL) {
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.title), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.description), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.input_label), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.input), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.output_label), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.output), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.error_label), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.error), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.created), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.created_label), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.modified), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.modified_label), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.lastrun), "");
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.rev_num), "");

		gtk_link_button_set_uri(GTK_LINK_BUTTON(gebr.ui_flow_browse->info.author), "");
		gtk_button_set_label(GTK_BUTTON(gebr.ui_flow_browse->info.author), "");

		g_object_set(gebr.ui_flow_browse->info.help_view, "sensitive", FALSE, NULL);
		g_object_set(gebr.ui_flow_browse->info.help_edit, "sensitive", FALSE, NULL);

		navigation_bar_update();
		return;
	}

	gchar *markup;

	/* Title in bold */
	markup = g_markup_printf_escaped("<span size='xx-large'>%s</span>", gebr_geoxml_document_get_title(GEBR_GEOXML_DOC(gebr.flow)));
	gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.title), markup);
	g_free(markup);

	/* Description in italic */
	markup = g_markup_printf_escaped("<span size='large'><i>%s</i></span>", gebr_geoxml_document_get_description(GEBR_GEOXML_DOC(gebr.flow)));
	gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.description), markup);
	g_free(markup);

	/* Date labels */
	markup = g_markup_printf_escaped("<b>%s</b>", _("Created:"));
	gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.created_label), markup);
	g_free(markup);
	markup = g_markup_printf_escaped("<b>%s</b>", _("Modified:"));
	gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.modified_label), markup);
	g_free(markup);

	/* Dates */
	gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.created),
			   gebr_localized_date(gebr_geoxml_document_get_date_created(GEBR_GEOXML_DOC(gebr.flow))));
	gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.modified),
			   gebr_localized_date(gebr_geoxml_document_get_date_modified(GEBR_GEOXML_DOC(gebr.flow))));

        /* Revisions */
        {
                gchar * str_tmp;
                gchar *date;
                gchar *comment;

                if (gebr_geoxml_flow_get_parent_revision(gebr.flow, &date, &comment, NULL))
                	str_tmp = g_markup_printf_escaped(_("<b>Revision of origin:</b>  %s <span size='small'>(saved in %s)</span>"), comment, date);
                else
                	str_tmp = g_strdup(_("This Flow has never had its state saved"));

                gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.rev_num), str_tmp);
                g_free(str_tmp);
	}

        /* Server */
        gchar *last_text;

        const gchar *last_run = gebr_geoxml_flow_get_date_last_run(gebr.flow);
        if (!last_run || !*last_run) {
        	last_text = g_strdup(_("This flow was never executed"));
        } else {
        	gchar *group;
        	gebr_geoxml_flow_server_get_group(gebr.flow, NULL, &group);
        	if (group && !*group) {
        		g_free(group);
        		GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro(gebr.maestro_controller);
        		group = g_strdup_printf("Maestro %s", gebr_maestro_server_get_address(maestro));
        	}

        	last_text = g_markup_printf_escaped("Last execution in %s at <b>%s</b>", gebr_localized_date(last_run), group);

        	g_free(group);
        }

	gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.lastrun), last_text);
        g_free(last_text);

	/* I/O labels */
	markup = g_markup_printf_escaped("<b>%s</b>", _("Input:"));
	gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.input_label), markup);
	g_free(markup);
	markup = g_markup_printf_escaped("<b>%s</b>", _("Output:"));
	gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.output_label), markup);
	g_free(markup);
	markup = g_markup_printf_escaped("<b>%s</b>", _("Log:"));
	gtk_label_set_markup(GTK_LABEL(gebr.ui_flow_browse->info.error_label), markup);
	g_free(markup);

	/* Input file */
	gchar *input_file = gebr_geoxml_flow_io_get_input_real(gebr.flow);
	if (strlen(input_file))
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.input), input_file);
	else
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.input), _("(none)"));
	g_free(input_file);

	/* Output file */
	gchar *output_file = gebr_geoxml_flow_io_get_output_real(gebr.flow);
	if (strlen(output_file))
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.output), output_file);
	else
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.output), _("(none)"));
	g_free(output_file);

	/* Error file */
	gchar *error_file = gebr_geoxml_flow_io_get_error(gebr.flow);
	if (strlen(error_file))
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.error), error_file);
	else
		gtk_label_set_text(GTK_LABEL(gebr.ui_flow_browse->info.error), _("(none)"));
	g_free(error_file);

	/* Author and email */
	gchar *tmp;
	tmp = g_strconcat("mailto:", gebr_geoxml_document_get_email(GEBR_GEOXML_DOCUMENT(gebr.flow)), NULL);
	gtk_link_button_set_uri(GTK_LINK_BUTTON(gebr.ui_flow_browse->info.author), tmp);
	g_free(tmp);

	tmp = g_strconcat(gebr_geoxml_document_get_author(GEBR_GEOXML_DOCUMENT(gebr.flow)), " <",
	                  gebr_geoxml_document_get_email(GEBR_GEOXML_DOCUMENT(gebr.flow)), ">", NULL);
	gtk_button_set_label(GTK_BUTTON(gebr.ui_flow_browse->info.author), tmp);
	g_free(tmp);

	/* Info button */
	if (gebr.flow != NULL){
		g_object_set(gebr.ui_flow_browse->info.help_view, "sensitive", TRUE, NULL);
		g_object_set(gebr.ui_flow_browse->info.help_edit, "sensitive", TRUE, NULL);
	}

	navigation_bar_update();
}

gboolean flow_browse_get_selected(GtkTreeIter * iter, gboolean warn_unselected)
{
	if (!gebr_gui_gtk_tree_view_get_selected(GTK_TREE_VIEW(gebr.ui_flow_browse->view), iter)) {
		if (warn_unselected)
			gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("No Flow selected"));
		return FALSE;
	}
	return TRUE;
}

void flow_browse_reload_selected(void)
{
	flow_browse_load();
}

void flow_browse_select_iter(GtkTreeIter * iter)
{
	gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(gebr.ui_flow_browse->view), iter);
}

void flow_browse_single_selection(void)
{
	gebr_gui_gtk_tree_view_turn_to_single_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view));
}

static void
create_revisions_model_recursive(GHashTable *revs,
                                 gchar *id,
                                 gchar *id_active,
                                 GtkTreeIter *parent,
                                 GebrGeoXmlFlow *flow,
                                 GebrUiFlowBrowse *fb)
{
	GList *childrens;
	gchar *comment;
	gchar *date;
	GtkTreeIter iter;
	GebrGeoXmlRevision *revision = gebr_geoxml_flow_get_revision_by_id(flow, id);

	if (!revision)
		return;

	gebr_geoxml_flow_get_revision_data(revision, NULL, &date, &comment, NULL);

	gtk_tree_store_insert(fb->rev_store, &iter, parent, 0);
	gtk_tree_store_set(fb->rev_store, &iter,
	                   REV_COMMENT, comment,
	                   REV_DATE, date,
	                   REV_XMLPOINTER, revision,
	                   REV_ACTIVE, g_strcmp0(id_active, id) == 0? TRUE : FALSE,
			   -1);


	childrens = g_hash_table_lookup(revs, id);

	if (!childrens)
		return;

	for (GList *i = childrens; i; i = i->next) {
		gchar *id_child = i->data;
		create_revisions_model_recursive(revs, id_child, id_active, &iter, flow, fb);
	}

	g_free(comment);
	g_free(date);
	gebr_geoxml_object_unref(revision);
}

static void
flow_browse_create_revisions_model(GebrGeoXmlFlow *flow,
                                   GebrUiFlowBrowse *fb)
{
	gchar *id_root;
	gchar *id_active;
	GHashTable *revs = gebr_flow_revisions_hash_create(flow);

	id_root = gebr_geoxml_flow_revisions_get_root_id(revs);
	id_active = gebr_geoxml_document_get_parent_id(GEBR_GEOXML_DOCUMENT(flow));

	gtk_tree_store_clear(gebr.ui_flow_browse->rev_store);

	create_revisions_model_recursive(revs, id_root, id_active, NULL, flow, fb);

	gtk_tree_view_expand_all(GTK_TREE_VIEW(gebr.ui_flow_browse->rev_view));

//	gebr_flow_revisions_hash_free(revs);
}

static void
flow_browse_add_revisions_graph(GebrGeoXmlFlow *flow,
                                GebrUiFlowBrowse *fb)
{
	GHashTable *revs = gebr_flow_revisions_hash_create(flow);
	gchar *png_filename;

	if (gebr_flow_revisions_create_graph(flow, revs, &png_filename)) {
		GtkImage *image = GTK_IMAGE(gtk_builder_get_object(fb->info.builder_flow, "rev_image"));
		gtk_image_set_from_file(image, png_filename);
	}

	g_free(png_filename);

//	gebr_flow_revisions_hash_free(revs);
}


/**
 * \internal
 * Load a selected flow from file when selected in "Flow Browser".
 */
static void flow_browse_load(void)
{
	GtkTreeIter iter;

	gchar *filename;
	gchar *title;

	GebrGeoXmlSequence *revision;

	flow_free();

	gebr_flow_set_toolbar_sensitive();

	if (!flow_browse_get_selected(&iter, FALSE))
		return;

	gint nrows = gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(gebr.ui_flow_browse->view)));
	gtk_action_set_sensitive(gtk_action_group_get_action(gebr.action_group_flow, "flow_change_revision"), nrows > 1? FALSE : TRUE);

	/* load its filename and title */
	gtk_tree_model_get(GTK_TREE_MODEL(gebr.ui_flow_browse->store), &iter,
			   FB_FILENAME, &filename,
			   FB_TITLE, &title,
			   FB_XMLPOINTER, &gebr.flow,
			   -1);

	if (gebr.validator)
		gebr_validator_update(gebr.validator);

	/* free previous flow and load it */
	flow_edition_load_components();

	/* load revisions */
	gboolean has_revision = FALSE;
	gebr_geoxml_flow_get_revision(gebr.flow, &revision, 0);
	for (; revision != NULL && !has_revision; gebr_geoxml_sequence_next(&revision))
		has_revision = TRUE;

	/* Create model for Revisions */
	if (has_revision) {
		gtk_widget_show(gebr.ui_flow_browse->revpage_main);
		gtk_widget_hide(gebr.ui_flow_browse->revpage_warn);
		flow_browse_create_revisions_model(gebr.flow,
		                                   gebr.ui_flow_browse);

		flow_browse_add_revisions_graph(gebr.flow,
		                                gebr.ui_flow_browse);
	} else {
		gtk_widget_hide(gebr.ui_flow_browse->revpage_main);
		gtk_widget_show(gebr.ui_flow_browse->revpage_warn);
	}


	gebr_flow_edition_select_group_for_flow(gebr.ui_flow_edition,
						gebr.flow);

	flow_browse_info_update();

	g_free(filename);
	g_free(title);
}

static void
update_speed_slider_sensitiveness(GebrUiFlowBrowse *ufb)
{
	gboolean sensitive = FALSE;
	GtkTreeModel *model;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ufb->view));
	GList *rows = gtk_tree_selection_get_selected_rows(selection, &model);

	for (GList *i = rows; i; i = i->next) {
		GtkTreeIter iter;
		GebrGeoXmlFlow *flow;
		gtk_tree_model_get_iter(model, &iter, i->data);
		gtk_tree_model_get(model, &iter, FB_XMLPOINTER, &flow, -1);
		gebr_validator_set_document(gebr.validator, (GebrGeoXmlDocument**) &flow, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, FALSE);
		gboolean parallel = gebr_geoxml_flow_is_parallelizable(flow, gebr.validator);
		gebr_validator_set_document(gebr.validator, (GebrGeoXmlDocument**) &gebr.flow, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, FALSE);
		GebrGeoXmlProgram *prog = gebr_geoxml_flow_get_first_mpi_program(flow);
		gboolean has_mpi = (gebr_geoxml_program_get_status(prog) == GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED);
		gebr_geoxml_object_unref(prog);

		if (parallel || has_mpi) {
			sensitive = TRUE;
			break;
		}

	}
	if (gebr.line)
		gebr_flow_set_toolbar_sensitive();

	g_list_foreach(rows, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(rows);

	gtk_widget_set_sensitive(ufb->speed_slider, sensitive);
	gebr_interface_update_speed_sensitiveness(ufb->speed_button,
						  ufb->speed_slider,
						  ufb->ruler,
						  sensitive);
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
 * Change flow to revision activated
 */
static void
gebr_flow_browse_revision_revert(GtkTreePath *path,
                                 GebrUiFlowBrowse *fb)
{
	if (gebr_gui_confirm_action_dialog(_("Backup current state?"),
	                                   _("You are about to revert to a previous state. "
	                                		   "The current Flow will be lost after this action. "
	                                		   "Do you want to save the current Flow's state?")))
		if (!flow_revision_save())
			return;

	gchar *date = NULL;
	gchar *comment = NULL;
	gboolean report_merged = FALSE;
	GtkTreeIter iter;
	GebrGeoXmlRevision *revision;

	gchar *path_str = gtk_tree_path_to_string(path);
	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(fb->rev_store), &iter, path_str);

	gtk_tree_model_get(GTK_TREE_MODEL(fb->rev_store), &iter,
	                   REV_COMMENT, &comment,
	                   REV_DATE, &date,
	                   REV_XMLPOINTER, &revision,
	                   -1);

	g_free(path_str);

	if (!gebr_geoxml_flow_change_to_revision(gebr.flow, revision, &report_merged)) {
		document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, FALSE);
		gebr_message(GEBR_LOG_ERROR, TRUE, FALSE, _("Could not revert to state '%s' ('%s')."), comment, date);
		g_free(date);
		g_free(comment);
		return;
	}
	document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, FALSE);

	if (report_merged)
		gebr_message(GEBR_LOG_INFO, TRUE, TRUE, _("Reverted to state '%s' ('%s'), and merged reports"), comment, date);
	else
		gebr_message(GEBR_LOG_INFO, TRUE, TRUE, _("Reverted to state '%s' ('%s')."), comment, date);

	flow_browse_load();
	gebr_validator_force_update(gebr.validator);
	flow_browse_get_selected(&iter, FALSE);
	gtk_list_store_set(gebr.ui_flow_browse->store, &iter,
	                   FB_TITLE, gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(gebr.flow)), -1);
	flow_browse_info_update();

	g_free(date);
	g_free(comment);
}

static void
revisions_on_row_activated(GtkTreeView * tree_view,
                           GtkTreePath * path,
                           GtkTreeViewColumn * column,
                           GebrUiFlowBrowse *fb)
{
	gebr_flow_browse_revision_revert(path, fb);
}

static gboolean
gebr_flow_browse_revision_delete(GebrUiFlowBrowse *fb)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *paths;
	GtkTreeIter iter;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (fb->rev_view));
	paths = gtk_tree_selection_get_selected_rows (selection, &model);

	if (paths && paths->next != NULL) {
		g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (paths);
		return FALSE;
	}

	gboolean response;
	response = gebr_gui_confirm_action_dialog(_("Remove this revision permanently?"),
	                                          _("If you choose to remove this revision "
	                                        		  "you will not be able to recover it later."));
	if (response) {
		gchar *path_str = gtk_tree_path_to_string(paths->data);
		gtk_tree_model_get_iter_from_string(model, &iter, path_str);

		g_free(path_str);
		g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (paths);

		GebrGeoXmlRevision *revision;

		gtk_tree_model_get(model, &iter,
		                   REV_XMLPOINTER, &revision, -1);

		gchar *id;
		gchar *flow_xml;
		GebrGeoXmlDocument *revdoc;

		gebr_geoxml_flow_get_revision_data(revision, &flow_xml, NULL, NULL, &id);

		if (gebr_geoxml_document_load_buffer(&revdoc, flow_xml) != GEBR_GEOXML_RETV_SUCCESS) {
			g_free(flow_xml);
			g_free(id);
			return FALSE;
		}
		g_free(flow_xml);

		gchar *parent_id = gebr_geoxml_document_get_parent_id(revdoc);
		gchar *head_parent = gebr_geoxml_document_get_parent_id(GEBR_GEOXML_DOCUMENT(gebr.flow));

		gebr_geoxml_document_free(revdoc);

		GHashTable *hash_rev = gebr_flow_revisions_hash_create(gebr.flow);

		gboolean change_head_parent = flow_revision_remove(gebr.flow, id, head_parent, hash_rev);

		if (change_head_parent)
			gebr_geoxml_document_set_parent_id(GEBR_GEOXML_DOCUMENT(gebr.flow), parent_id);

		gebr_flow_revisions_hash_free(hash_rev);

		document_save(GEBR_GEOXML_DOCUMENT(gebr.flow), TRUE, FALSE);

		flow_browse_load();

		return TRUE;
	}

	return FALSE;
}

static gboolean
revision_on_key_pressed(GtkWidget *view,
                        GdkEventKey *key,
                        GebrUiFlowBrowse *fb)
{
	if (key->keyval != GDK_Delete)
		return FALSE;

	return gebr_flow_browse_revision_delete(fb);
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

	GebrMaestroServer *maestro = gebr_maestro_controller_get_maestro_for_line(gebr.maestro_controller, gebr.line);
	if (!maestro || gebr_maestro_server_get_state(maestro) != SERVER_STATE_LOGGED)
		return NULL;

	menu = gtk_menu_new();

	if (!flow_browse_get_selected(&iter, FALSE)) {
		gtk_container_add(GTK_CONTAINER(menu),
				  gtk_action_create_menu_item(gtk_action_group_get_action
							      (gebr.action_group_flow, "flow_new")));
		gtk_container_add(GTK_CONTAINER(menu),
				  gtk_action_create_menu_item(gtk_action_group_get_action
							      (gebr.action_group_flow, "flow_paste")));
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
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_new")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_copy")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_paste")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_delete")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_properties")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_view")));
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_edit")));

	menu_item = gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_change_revision"));
	gtk_container_add(GTK_CONTAINER(menu), menu_item);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
	gtk_container_add(GTK_CONTAINER(menu),
			  gtk_action_create_menu_item(gtk_action_group_get_action(gebr.action_group_flow, "flow_execute")));

 out:	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}

void
on_revision_delete_activate(GtkMenuItem *menuitem,
                            GebrUiFlowBrowse *fb)
{
	gebr_flow_browse_revision_delete(fb);
}

void
on_revision_revert_activate(GtkMenuItem *menuitem,
                            GebrUiFlowBrowse *fb)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *paths;
	GtkTreePath *path;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (fb->rev_view));
	paths = gtk_tree_selection_get_selected_rows (selection, &model);

	if (paths && paths->next)
		return;

	path = paths->data;
	gebr_flow_browse_revision_revert(path, fb);

	g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (paths);
}

static GtkMenu *
revisions_popup_menu(GtkWidget * widget,
                     GebrUiFlowBrowse *fb)
{
	GtkWidget *menu;
	GtkWidget *menu_item;

	/* no line, no new flow possible */
	if (gebr.flow == NULL)
		return NULL;

	GtkTreeSelection *selection;
	GList *paths;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
	paths = gtk_tree_selection_get_selected_rows (selection, NULL);

	if (!paths)
		return NULL;

	menu = gtk_menu_new();

	/* Revert Revision */
	menu_item = gtk_image_menu_item_new_with_label(_("Revert"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(menu_item, "activate", G_CALLBACK(on_revision_revert_activate), fb);

	/* Delete Revision */
	menu_item = gtk_image_menu_item_new_with_label(_("Remove"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(menu_item, "activate", G_CALLBACK(on_revision_delete_activate), fb);

	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}

/**
 * \internal
 * Saves the current selected line.
 */
static void flow_browse_on_flow_move(void)
{
	document_save(GEBR_GEOXML_DOC(gebr.line), TRUE, TRUE);
}

void
gebr_flow_browse_hide(GebrUiFlowBrowse *self)
{
	return;
}

void
gebr_flow_browse_show(GebrUiFlowBrowse *self)
{
	if (gebr.line)
		gebr_flow_set_toolbar_sensitive();

	update_speed_slider_sensitiveness(self);

	if (gebr.config.niceness == 0)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->nice_button_high), TRUE);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->nice_button_low), TRUE);

	flow_browse_info_update();
}
