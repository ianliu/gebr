/*   GÍBR - An environment for seismic processing.
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * File: cb_flowcomp.c
 * Callbacks to flow components
 */

#include <geoxml.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "cb_flowcomp.h"
#include "gebr.h"
#include "cb_flow.h"
#include "widgets.h"
#include "callbacks.h"
#include "ui_prop.h"

const gchar * no_flow_comp_selected_error = "No flow component selected";

void flow_component_properties_set      (void)
{
	GtkTreeIter		iter;
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	gchar *			name;
	gchar			message[STRMAX];

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (W.fseq_view));
	if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
		log_message(INTERFACE, no_flow_comp_selected_error, TRUE);
		return;
	}

	gtk_tree_model_get (model, &iter, FSEQ_TITLE_COLUMN, &name, -1);

	sprintf (message, "Configuring flow component '%s'", name);
	log_message(INTERFACE, message, TRUE);

	progpar_config_window();

	g_free (name);
}

void
flow_component_selected	(void)
{
	GtkTreeIter		iter;
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreePath *		path;
	GeoXmlProgram *		program;
	gchar *			status;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (W.fseq_view));
	if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
		log_message(INTERFACE, no_flow_comp_selected_error, TRUE);
		return;
	}

	path = gtk_tree_model_get_path(model, &iter);
	geoxml_flow_get_program(flow, &program, gtk_tree_path_get_indices(path)[0]);
	status = (gchar *)geoxml_program_get_status(program);

	if (!g_ascii_strcasecmp(status, "configured"))
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (W.configured_menuitem), TRUE);
	else if (!g_ascii_strcasecmp(status, "disabled"))
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (W.disabled_menuitem), TRUE);
	else if (!g_ascii_strcasecmp(status, "unconfigured"))
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (W.unconfigured_menuitem), TRUE);

	gtk_tree_path_free(path);
}

void flow_component_set_status	(GtkMenuItem *menuitem)
{
	GtkTreeIter		iter;
	GtkTreeSelection *	selection;
	GtkTreeModel *		model;
	GtkTreePath *		path;
	GtkWidget *		status_menuitem;
	GeoXmlProgram *		program;
	GdkPixbuf *		pixbuf;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (W.fseq_view));
	if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
		log_message(INTERFACE, no_flow_comp_selected_error, TRUE);
		return;
	}

	status_menuitem = GTK_WIDGET(menuitem);
	path = gtk_tree_model_get_path(model, &iter);
	geoxml_flow_get_program(flow, &program, gtk_tree_path_get_indices(path)[0]);
	gtk_tree_path_free(path);

	if (status_menuitem == W.configured_menuitem) {
		geoxml_program_set_status(program, "configured");
		pixbuf = W.pixmaps.configured_icon;
	} else if (status_menuitem == W.disabled_menuitem) {
		geoxml_program_set_status(program, "disabled");
		pixbuf = W.pixmaps.disabled_icon;
	}
	else if (status_menuitem == W.unconfigured_menuitem) {
		geoxml_program_set_status(program, "unconfigured");
		pixbuf = W.pixmaps.unconfigured_icon;
	} else
		return;

	gtk_list_store_set (W.fseq_store, &iter,
			    FSEQ_STATUS_COLUMN, pixbuf, -1);

	flow_save();
}
