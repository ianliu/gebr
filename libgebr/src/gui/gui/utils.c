/*   libgebr - GêBR Library
 *   Copyright (C) 2007 GÃªBR core team (http://gebr.sourceforge.net)
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

#include "utils.h"

/*
 * Internal functions
 */

// void
// save_widget_browse_button_clicked(GtkWidget * button, GtkWidget * entry)
// {
// 	GtkWidget *	chooser_dialog;
//
// 	chooser_dialog = gtk_file_chooser_dialog_new(	"Choose file", NULL,
// 							GTK_FILE_CHOOSER_ACTION_SAVE,
// 							GTK_STOCK_OK, GTK_RESPONSE_OK,
// 							GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
// 							NULL);
//
// 	switch (gtk_dialog_run(GTK_DIALOG(chooser_dialog))) {
// 	case GTK_RESPONSE_OK:
// 		gtk_entry_set_text(GTK_ENTRY(entry), gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser_dialog)));
// 		break;
// 	default:
// 		break;
// 	}
//
// 	gtk_widget_destroy(GTK_WIDGET(chooser_dialog));
// }
//
// /*
//  * Library functions
//  */
// gebr_save_widget_t
// save_widget_create(void)
// {
// 	gebr_save_widget_t save_widget;
//
// 	save_widget.hbox = gtk_hbox_new(FALSE, 10);
//
// 	/* entry */
// 	save_widget.entry = gtk_entry_new();
// 	gtk_box_pack_start(GTK_BOX (save_widget.hbox), save_widget.entry, TRUE, TRUE, 0);
//
// 	/* browse button */
// 	save_widget.browse_button = gtk_button_new_from_stock(GTK_STOCK_OPEN);
// 	gtk_box_pack_start(GTK_BOX (save_widget.hbox), save_widget.browse_button, FALSE, TRUE, 0);
// 	g_signal_connect (GTK_OBJECT (save_widget.browse_button), "clicked",
// 			  G_CALLBACK (save_widget_browse_button_clicked), save_widget.entry);
//
// 	return save_widget;
// }
