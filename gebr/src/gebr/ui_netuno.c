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
#include "ui_netuno.h"

static void populate_models (struct ui_netuno * gui);
static GList * netuno_get_accounts (void);
static GList * netuno_get_classes(void);

gboolean netuno_setup_ui(gchar ** char_account, gchar ** char_class)
{
	gboolean ret;
	GtkWidget *label;
	GtkSizeGroup * group;
	GtkCellRenderer * cell;
	GtkWidget * cb_account, * cb_classes;
	GtkWidget * box, * hbox;
	struct ui_netuno * gui;
	GtkTreeIter iter;

	ret = TRUE;
	gui = g_new(struct ui_netuno, 1);
	gui->dialog = gtk_dialog_new_with_buttons(_("Netuno Execute"), GTK_WINDOW(gebr.window), 
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

	populate_models(gui);

	gtk_widget_show_all(gui->dialog);

	if (gtk_dialog_run(GTK_DIALOG(gui->dialog)) != GTK_RESPONSE_ACCEPT){
		ret = FALSE;
		goto out;
	}

	gtk_combo_box_get_active_iter(GTK_COMBO_BOX(cb_account), &iter);
	gtk_tree_model_get(GTK_TREE_MODEL(gui->account), &iter, 0, char_account, -1);

	gtk_combo_box_get_active_iter(GTK_COMBO_BOX(cb_classes), &iter);
	gtk_tree_model_get(GTK_TREE_MODEL(gui->classes), &iter, 0, char_class, -1);
out:
	gtk_widget_destroy(gui->dialog);
	g_free(gui);
	return ret;
}

static GList * netuno_get_accounts (void)
{
	return NULL;
}

static GList * netuno_get_classes(void)
{
	return NULL;
}

static void populate_models (struct ui_netuno * gui)
{
	GList * acc, * cla;
	GtkTreeIter iter;

	acc = netuno_get_accounts();
	cla = netuno_get_classes();

	while (acc != NULL)
	{
		gtk_list_store_append(gui->account, &iter);
		gtk_list_store_set(gui->account, &iter, 0, acc->data, -1);
		acc = acc->next;
	}
	
	while (cla != NULL)
	{
		gtk_list_store_append(gui->classes, &iter);
		gtk_list_store_set(gui->classes, &iter, 0, cla->data, -1);
		cla = cla->next;
	}
}
