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
#include "gebr.h"
#include <libgebr/intl.h>
#include "ui_moab.h"

static void populate_models (struct ui_moab * gui, struct server * server);

gboolean moab_setup_ui(gchar ** char_account, gchar ** char_class, struct server * server)
{
	gboolean ret;
	GtkWidget *label;
	GtkSizeGroup * group;
	GtkCellRenderer * cell;
	GtkWidget * cb_account, * cb_classes;
	GtkWidget * box, * hbox;
	struct ui_moab * gui;
	GtkTreeIter iter;

	ret = TRUE;
	gui = g_new(struct ui_moab, 1);
	gui->dialog = gtk_dialog_new_with_buttons(_("Moab Execute"), GTK_WINDOW(gebr.window), 
						  GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT, 
						  GTK_STOCK_EXECUTE, GTK_RESPONSE_ACCEPT,
						  GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
						  NULL);
	gui->account = gtk_list_store_new(1, G_TYPE_STRING);
	gui->classes = gtk_list_store_new(1, G_TYPE_STRING);

	box = GTK_DIALOG(gui->dialog)->vbox;

	group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	cb_account = gtk_combo_box_new();
	cell = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cb_account), cell, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(cb_account), cell, "text", 0);
	hbox= gtk_hbox_new(FALSE, 5);

	label = gtk_label_new(_("Account"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), cb_account, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);
	gtk_size_group_add_widget(group, label);

	cb_classes = gtk_combo_box_new();
	cell = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cb_classes), cell, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(cb_classes), cell, "text", 0);
	hbox= gtk_hbox_new(FALSE, 5);

	label = gtk_label_new(_("Classes"));
	gtk_box_pack_start(GTK_BOX(hbox),label, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), cb_classes, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);
	gtk_size_group_add_widget(group, label);


	gtk_combo_box_set_model(GTK_COMBO_BOX(cb_account), GTK_TREE_MODEL(gui->account));
	gtk_combo_box_set_model(GTK_COMBO_BOX(cb_classes), GTK_TREE_MODEL(gui->classes));

	populate_models(gui, server);

	gtk_widget_show_all(gui->dialog);

	if (gtk_dialog_run(GTK_DIALOG(gui->dialog)) != GTK_RESPONSE_ACCEPT) {
		ret = FALSE;
		goto out;
	}

	gtk_combo_box_get_active_iter(GTK_COMBO_BOX(cb_account), &iter);
	gtk_tree_model_get(GTK_TREE_MODEL(gui->account), &iter, 0, char_account, -1);

	gtk_combo_box_get_active_iter(GTK_COMBO_BOX(cb_classes), &iter);
	gtk_tree_model_get(GTK_TREE_MODEL(gui->classes), &iter, 0, char_class, -1);
out:
	gtk_widget_destroy(gui->dialog);
	gtk_list_store_clear(gui->account);
	gtk_list_store_clear(gui->classes);
	g_free(gui);
	return ret;
}

static void populate_models (struct ui_moab * gui, struct server * server)
{
	GtkTreeIter iter;
	gsize i;

	i = 0;
	while (server && server->account && server->account[i] != NULL) {
		gtk_list_store_append(gui->account, &iter);
		gtk_list_store_set(gui->account, &iter, 0, server->account[i], -1);
		i++;
	}
	
	i = 0;
	while (server && server->classes && server->classes[i] != NULL) {
		gtk_list_store_append(gui->classes, &iter);
		gtk_list_store_set(gui->classes, &iter, 0, server->classes[i], -1);
		i++;
	}
}
