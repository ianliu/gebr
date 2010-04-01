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

gboolean moab_setup_ui(gchar ** char_account, struct server * server, gboolean parallel_program)
{
	gboolean ret;
	GtkWidget * box;
	GtkWidget * hbox;
	GtkWidget * dialog;
	GtkWidget * label;
	GtkWidget * cb_account;
	GtkCellRenderer * cell;
	GtkTreeIter iter;

	ret = TRUE;
	dialog = gtk_dialog_new_with_buttons(_("Moab execution parameters"), GTK_WINDOW(gebr.window), 
					     GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT, 
					     GTK_STOCK_EXECUTE, GTK_RESPONSE_ACCEPT,
					     GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
					     NULL);
	box = GTK_DIALOG(dialog)->vbox;

	cb_account = gtk_combo_box_new();
	cell = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cb_account), cell, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(cb_account), cell, "text", 0);
	hbox = gtk_hbox_new(FALSE, 5);

	label = gtk_label_new(_("Account"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), cb_account, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);

	if (parallel_program) {
		/* We should be able to ask for the number of processes (np) to run the parallel program(s). */
		GtkWidget *hbox_np = gtk_hbox_new(FALSE, 5);
		GtkWidget *label_np = gtk_label_new(_("Number of processes"));
		GtkWidget *entry_np = gtk_entry_new();
		gtk_box_pack_start(GTK_BOX(hbox_np), label_np, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(hbox_np), entry_np, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(box), hbox_np, TRUE, TRUE, 0);
	}

	gtk_combo_box_set_model(GTK_COMBO_BOX(cb_account), GTK_TREE_MODEL(server->accounts_model));
	gtk_combo_box_set_active(GTK_COMBO_BOX(cb_account), 0);

	gtk_widget_show_all(dialog);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT) {
		ret = FALSE;
		goto out;
	}

	gtk_combo_box_get_active_iter(GTK_COMBO_BOX(cb_account), &iter);
	gtk_tree_model_get(GTK_TREE_MODEL(server->accounts_model), &iter, 0, char_account, -1);
out:
	gtk_widget_destroy(dialog);
	return ret;
}
