/*   GêBR ME - GêBR Menu Editor
 *   Copyright (C) 2007 GêBR core team (http://gebr.sourceforge.net)
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

#include <gui/pixmaps.h>

#include <gui/utils.h>
#include <gui/valuesequenceedit.h>

#include "interface.h"
#include "gebrme.h"
#include "callbacks.h"
#include "support.h"
#include "menu.h"
#include "summary.h"
#include "category.h"
#include "program.h"

GtkWidget*
create_gebrme_window (void)
{
	GtkWidget *		gebrme_window;
	GtkWidget *		mainwindow_vbox;
	GtkWidget *		menubar;
	GtkWidget *		statusbar;

	GtkWidget *		flow_menuitem;
	GtkWidget *		flow_menuitem_menu;
	GtkWidget *		new_menuitem;
	GtkWidget *		open_menuitem;
	GtkWidget *		save_menuitem;
	GtkWidget *		save_as_menuitem;
	GtkWidget *		revert_menuitem;
	GtkWidget *		delete_menuitem;
	GtkWidget *		close_menuitem;
	GtkWidget *		separator_menuitem;
	GtkWidget *		quit_menuitem;

	GtkWidget *		edit_menuitem;
	GtkWidget *		edit_menuitem_menu;
// 	GtkWidget *		cut_menuitem;
// 	GtkWidget *		copy_menuitem;
// 	GtkWidget *		paste_menuitem;
	GtkWidget *		preferences_menuitem;

	GtkWidget *		help_menuitem;
	GtkWidget *		help_menuitem_menu;
	GtkWidget *		about_menuitem;

	GtkWidget *		toolbar;
	GtkIconSize		tmp_toolbar_icon_size;

	GtkWidget *		central_hbox;
	GtkWidget *		central_hpaned;

	GtkWidget *		menus_scrolledwindow;
	GtkWidget *		menus_treeview;

	GtkWidget *		edition_scrolledwindow;
	GtkWidget *		edition_viewport;
	GtkWidget *		edition_vbox;

	GtkWidget *		summary_expander;
	GtkWidget *		summary_label;
	GtkWidget *		summary_table;
	GtkWidget *		title_label;
	GtkWidget *		title_entry;
	GtkWidget *		description_label;
	GtkWidget *		description_entry;
	GtkWidget *		menuhelp_label;
	GtkWidget *		menuhelp_hbox;
	GtkWidget *		menuhelp_view_button;
	GtkWidget *		menuhelp_edit_button;
	GtkWidget *		author_label;
	GtkWidget *		author_entry;
	GtkWidget *		email_label;
	GtkWidget *		email_entry;

	GtkWidget *		categories_label;
	GtkWidget *		categories_combo;
	GtkWidget *		categories_sequence_edit;

	GtkWidget *		programs_frame;
	GtkWidget *		programs_hbox;
	GtkWidget *		programs_label;
	GtkWidget *		programs_add_button;
	GtkWidget *		programs_scrolledwindow;
	GtkWidget *		programs_vbox;

	GtkWidget *		widget;
	GtkWidget *		depth_hbox;
	GtkAccelGroup *		accel_group;
	GtkToolItem *		toolbutton;
	GtkTreeViewColumn *	col;
	GtkCellRenderer *	renderer;

	accel_group = gtk_accel_group_new ();

	/*
	 * Main interface
	 */

	gtk_window_set_default_icon (pixmaps_gebr_icon_16x16());
	gebrme_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gebrme.window = gebrme_window;
	gtk_window_set_title(GTK_WINDOW (gebrme_window), _("GÃªBR ME"));
	gtk_widget_set_size_request(gebrme_window, 800, 800);
	gebrme.about = about_setup_ui("GÃªBRME", _("Flow describer for GÃªBR"));

	g_signal_connect(gebrme_window, "delete_event",
			GTK_SIGNAL_FUNC (gebrme_quit),
			NULL);
	g_signal_connect(gebrme_window, "show",
			GTK_SIGNAL_FUNC (gebrme_init),
			NULL);

	mainwindow_vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show(mainwindow_vbox);
	gtk_container_add(GTK_CONTAINER(gebrme_window), mainwindow_vbox);

	/*
	 * Actions
	 */

	gebrme.save_action = gtk_action_new("save_action",
		NULL, NULL, GTK_STOCK_SAVE);
	/* TODO: */
// 	gtk_action_set_accel_group(gebrme.save_action, accel_group);
// 	gtk_action_set_accel_path(gebrme.save_action, "<GêBR>File/Save");
// 	gtk_action_connect_accelerator(gebrme.save_action);
	g_signal_connect(gebrme.save_action, "activate",
		(GCallback)on_save_activate, NULL);

	gebrme.revert_action = gtk_action_new("revert_action",
		NULL, NULL, GTK_STOCK_REVERT_TO_SAVED);
	g_signal_connect(gebrme.revert_action, "activate",
		(GCallback)on_revert_activate, NULL);

	/*
	 * Menus
	 */

	menubar = gtk_menu_bar_new ();
	gtk_widget_show(menubar);
	gtk_box_pack_start (GTK_BOX (mainwindow_vbox), menubar, FALSE, FALSE, 0);

	flow_menuitem = gtk_menu_item_new_with_mnemonic (_("_Flow"));
	gtk_widget_show(flow_menuitem);
	gtk_container_add(GTK_CONTAINER(menubar), flow_menuitem);

	flow_menuitem_menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (flow_menuitem), flow_menuitem_menu);

	new_menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_NEW, accel_group);
	gtk_widget_show(new_menuitem);
	gtk_container_add(GTK_CONTAINER(flow_menuitem_menu), new_menuitem);
	g_signal_connect(new_menuitem, "activate",
		(GCallback)on_new_activate, NULL);
	open_menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN, accel_group);
	gtk_widget_show(open_menuitem);
	gtk_container_add(GTK_CONTAINER(flow_menuitem_menu), open_menuitem);
	g_signal_connect(open_menuitem, "activate",
		(GCallback)on_open_activate, NULL);

	separator_menuitem = gtk_separator_menu_item_new ();
	gtk_widget_show(separator_menuitem);
	gtk_container_add(GTK_CONTAINER(flow_menuitem_menu), separator_menuitem);
	save_menuitem = gtk_action_create_menu_item(gebrme.save_action);
	gtk_widget_show(save_menuitem);
	gtk_container_add(GTK_CONTAINER(flow_menuitem_menu), save_menuitem);
	save_as_menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_SAVE_AS, accel_group);
	gtk_widget_show(save_as_menuitem);
	gtk_container_add(GTK_CONTAINER(flow_menuitem_menu), save_as_menuitem);
	g_signal_connect(save_as_menuitem, "activate",
		(GCallback)on_save_as_activate, NULL);
	revert_menuitem = gtk_action_create_menu_item(gebrme.revert_action);
	gtk_widget_show(revert_menuitem);
	gtk_container_add(GTK_CONTAINER(flow_menuitem_menu), revert_menuitem);

	separator_menuitem = gtk_separator_menu_item_new ();
	gtk_widget_show(separator_menuitem);
	gtk_container_add(GTK_CONTAINER(flow_menuitem_menu), separator_menuitem);
	delete_menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE, accel_group);
	gtk_widget_show(delete_menuitem);
	gtk_container_add(GTK_CONTAINER(flow_menuitem_menu), delete_menuitem);
	g_signal_connect(delete_menuitem, "activate",
		(GCallback)on_delete_activate, NULL);
	close_menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_CLOSE, accel_group);
	gtk_widget_show(close_menuitem);
	gtk_container_add(GTK_CONTAINER(flow_menuitem_menu), close_menuitem);
	g_signal_connect(close_menuitem, "activate",
		(GCallback)on_close_activate, NULL);
	separator_menuitem = gtk_separator_menu_item_new ();

	gtk_widget_show(separator_menuitem);
	gtk_container_add(GTK_CONTAINER(flow_menuitem_menu), separator_menuitem);
	gtk_widget_set_sensitive (separator_menuitem, FALSE);
	quit_menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, accel_group);
	gtk_widget_show(quit_menuitem);
	gtk_container_add(GTK_CONTAINER(flow_menuitem_menu), quit_menuitem);
	g_signal_connect(quit_menuitem, "activate",
		(GCallback)gebrme_quit, NULL);

	edit_menuitem = gtk_menu_item_new_with_mnemonic (_("_Edit"));
	gtk_widget_show(edit_menuitem);
	gtk_container_add(GTK_CONTAINER(menubar), edit_menuitem);

	edit_menuitem_menu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (edit_menuitem), edit_menuitem_menu);

// 	cut_menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, accel_group);
// 	gtk_widget_show(cut_menuitem);
// 	gtk_container_add(GTK_CONTAINER(edit_menuitem_menu), cut_menuitem);
// 	g_signal_connect(cut_menuitem, "activate",
// 		(GCallback)on_cut_activate, NULL);
// 	copy_menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_COPY, accel_group);
// 	gtk_widget_show(copy_menuitem);
// 	gtk_container_add(GTK_CONTAINER(edit_menuitem_menu), copy_menuitem);
// 	g_signal_connect(copy_menuitem, "activate",
// 		(GCallback)on_copy_activate, NULL);
// 	paste_menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_PASTE, accel_group);
// 	gtk_widget_show(paste_menuitem);
// 	gtk_container_add(GTK_CONTAINER(edit_menuitem_menu), paste_menuitem);
// 	g_signal_connect(paste_menuitem, "activate",
// 		(GCallback)on_paste_activate, NULL);

	preferences_menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, accel_group);
	gtk_widget_show(preferences_menuitem);
	gtk_container_add(GTK_CONTAINER(edit_menuitem_menu), preferences_menuitem);
	g_signal_connect(preferences_menuitem, "activate",
		(GCallback)on_preferences_activate, NULL);

	help_menuitem = gtk_menu_item_new_with_mnemonic (_("_Help"));
	gtk_menu_item_right_justify(GTK_MENU_ITEM(help_menuitem));
	gtk_widget_show(help_menuitem);
	gtk_container_add(GTK_CONTAINER (menubar), help_menuitem);

	help_menuitem_menu = gtk_menu_new ();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM (help_menuitem), help_menuitem_menu);

	about_menuitem = gtk_image_menu_item_new_from_stock("gtk-about", accel_group);
	gtk_widget_show(about_menuitem);
	gtk_container_add(GTK_CONTAINER(help_menuitem_menu), about_menuitem);
	g_signal_connect(about_menuitem, "activate",
		(GCallback)on_about_activate, NULL);

	toolbar = gtk_toolbar_new ();
	gtk_widget_show(toolbar);
	gtk_box_pack_start (GTK_BOX (mainwindow_vbox), toolbar, FALSE, FALSE, 0);
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_BOTH);
	tmp_toolbar_icon_size = gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar));

	toolbutton = gtk_tool_button_new_from_stock (GTK_STOCK_NEW);
	gtk_toolbar_insert (GTK_TOOLBAR(toolbar), toolbutton, -1);
	gtk_widget_show(GTK_WIDGET(toolbutton));
	g_signal_connect(toolbutton, "clicked",
		(GCallback)menu_new, NULL);
	toolbutton = gtk_tool_button_new_from_stock(GTK_STOCK_OPEN);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolbutton, -1);
	gtk_widget_show(GTK_WIDGET(toolbutton));
	g_signal_connect(toolbutton, "clicked",
		(GCallback)on_open_activate, NULL);
	toolbutton = GTK_TOOL_ITEM(gtk_action_create_tool_item(gebrme.save_action));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolbutton, -1);
	gtk_widget_show(GTK_WIDGET(toolbutton));
	g_signal_connect(toolbutton, "clicked",
		(GCallback)on_save_activate, NULL);
	toolbutton = gtk_tool_button_new_from_stock(GTK_STOCK_SAVE_AS);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolbutton, -1);
	gtk_widget_show(GTK_WIDGET(toolbutton));
	g_signal_connect(toolbutton, "clicked",
		(GCallback)on_save_as_activate, NULL);
	toolbutton = GTK_TOOL_ITEM(gtk_action_create_tool_item(gebrme.revert_action));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolbutton, -1);
	gtk_widget_show(GTK_WIDGET(toolbutton));
	toolbutton = gtk_tool_button_new_from_stock(GTK_STOCK_CLOSE);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolbutton, -1);
	gtk_widget_show(GTK_WIDGET(toolbutton));
	g_signal_connect(toolbutton, "clicked",
		(GCallback)on_close_activate, NULL);

	central_hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_show(central_hbox);
	gtk_box_pack_start (GTK_BOX (mainwindow_vbox), central_hbox, TRUE, TRUE, 0);

	central_hpaned = gtk_hpaned_new ();
	gtk_widget_show(central_hpaned);
	gtk_box_pack_start (GTK_BOX (central_hbox), central_hpaned, TRUE, TRUE, 0);
	gtk_paned_set_position (GTK_PANED (central_hpaned), 160);

	menus_scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show(menus_scrolledwindow);
	gtk_paned_pack1 (GTK_PANED (central_hpaned), menus_scrolledwindow, FALSE, TRUE);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (menus_scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (menus_scrolledwindow), GTK_SHADOW_IN);

	gebrme.menus_liststore = gtk_list_store_new (MENU_N_COLUMN,
						     GDK_TYPE_PIXBUF,
						     G_TYPE_STRING,
						     G_TYPE_POINTER,
						     G_TYPE_STRING);
	menus_treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (gebrme.menus_liststore));
	gebrme.menus_treeview = menus_treeview;
	gtk_widget_show(menus_treeview);
	gtk_container_add(GTK_CONTAINER(menus_scrolledwindow), menus_treeview);
	g_signal_connect(menus_treeview, "cursor-changed",
		(GCallback)menu_selected, NULL);

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(menus_treeview), FALSE);
	renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width(col, 24);
	gtk_tree_view_column_add_attribute(col, renderer, "pixbuf", MENU_STATUS);
	gtk_tree_view_append_column(GTK_TREE_VIEW(menus_treeview), col);
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_column_add_attribute(col, renderer, "text", MENU_FILENAME);
	gtk_tree_view_append_column(GTK_TREE_VIEW(menus_treeview), col);

	edition_scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show(edition_scrolledwindow);
	gtk_paned_pack2 (GTK_PANED (central_hpaned), edition_scrolledwindow, TRUE, TRUE);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (edition_scrolledwindow), GTK_POLICY_NEVER, GTK_POLICY_NEVER);

	edition_viewport = gtk_viewport_new (NULL, NULL);
	gtk_widget_show(edition_viewport);
	gtk_container_add(GTK_CONTAINER(edition_scrolledwindow), edition_viewport);

	edition_vbox = gtk_vbox_new (FALSE, 5);
	gtk_widget_show(edition_vbox);
	gtk_container_add(GTK_CONTAINER(edition_viewport), edition_vbox);

	summary_expander = gtk_expander_new (NULL);
	depth_hbox = create_depth(summary_expander);
	gtk_widget_show(summary_expander);
	gtk_box_pack_start(GTK_BOX (edition_vbox), summary_expander, FALSE, FALSE, 0);
	gtk_expander_set_expanded(GTK_EXPANDER (summary_expander), TRUE);
	summary_label = gtk_label_new(_("Summary"));
	gtk_widget_show(summary_label);
	gtk_expander_set_label_widget(GTK_EXPANDER(summary_expander), summary_label);

	summary_table = gtk_table_new(6, 2, FALSE);
	gtk_widget_show(summary_table);
	gtk_container_add(GTK_CONTAINER (depth_hbox), summary_table);
	gtk_table_set_row_spacings(GTK_TABLE(summary_table), 5);
	gtk_table_set_col_spacings(GTK_TABLE(summary_table), 5);

	title_label = gtk_label_new (_("Title:"));
	gtk_widget_show(title_label);
	gtk_table_attach(GTK_TABLE(summary_table), title_label, 0, 1, 0, 1,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (title_label), 0, 0.5);

	title_entry = gtk_entry_new ();
	gebrme.title_entry = title_entry;
	gtk_widget_show(title_entry);
	gtk_table_attach(GTK_TABLE(summary_table), title_entry, 1, 2, 0, 1,
			(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
	g_signal_connect(title_entry, "changed",
			(GCallback)summary_title_changed,
			NULL);

	description_label = gtk_label_new (_("Description:"));
	gtk_widget_show(description_label);
	gtk_table_attach (GTK_TABLE (summary_table), description_label, 0, 1, 1, 2,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (description_label), 0, 0.5);

	description_entry = gtk_entry_new ();
	gebrme.description_entry = description_entry;
	gtk_widget_show(description_entry);
	gtk_table_attach (GTK_TABLE (summary_table), description_entry, 1, 2, 1, 2,
			(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
	g_signal_connect(description_entry, "changed",
			(GCallback)summary_description_changed,
			NULL);

	menuhelp_label = gtk_label_new (_("Help"));
	gtk_widget_show(menuhelp_label);
	gtk_table_attach (GTK_TABLE (summary_table), menuhelp_label, 0, 1, 2, 3,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (menuhelp_label), 0, 0.5);

	menuhelp_hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(menuhelp_hbox);
	gtk_table_attach (GTK_TABLE (summary_table), menuhelp_hbox, 1, 2, 2, 3,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
	menuhelp_view_button = gtk_button_new_from_stock (GTK_STOCK_OPEN);
	gtk_widget_show(menuhelp_view_button);
	gtk_box_pack_start(GTK_BOX(menuhelp_hbox), menuhelp_view_button, FALSE, FALSE, 0);
	g_signal_connect(menuhelp_view_button, "clicked",
			GTK_SIGNAL_FUNC (summary_help_view),
			NULL);
	g_object_set(G_OBJECT(menuhelp_view_button), "relief", GTK_RELIEF_NONE, NULL);

	menuhelp_edit_button = gtk_button_new_from_stock (GTK_STOCK_EDIT);
	gtk_widget_show(menuhelp_edit_button);
	gtk_box_pack_start(GTK_BOX(menuhelp_hbox), menuhelp_edit_button, FALSE, FALSE, 5);
	g_signal_connect(menuhelp_edit_button, "clicked",
			GTK_SIGNAL_FUNC (summary_help_edit),
			NULL);
	g_object_set(G_OBJECT(menuhelp_edit_button), "relief", GTK_RELIEF_NONE, NULL);

	author_label = gtk_label_new (_("Author:"));
	gtk_widget_show(author_label);
	gtk_table_attach (GTK_TABLE (summary_table), author_label, 0, 1, 3, 4,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (author_label), 0, 0.5);

	author_entry = gtk_entry_new ();
	gebrme.author_entry = author_entry;
	gtk_widget_show(author_entry);
	gtk_table_attach (GTK_TABLE (summary_table), author_entry, 1, 2, 3, 4,
			(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
	g_signal_connect(author_entry, "changed",
			(GCallback)summary_author_changed,
			NULL);

	email_label = gtk_label_new (_("Email:"));
	gtk_widget_show(email_label);
	gtk_table_attach (GTK_TABLE (summary_table), email_label, 0, 1, 4, 5,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (email_label), 0, 0.5);

	email_entry = gtk_entry_new ();
	gebrme.email_entry = email_entry;
	gtk_widget_show(email_entry);
	gtk_table_attach (GTK_TABLE (summary_table), email_entry, 1, 2, 4, 5,
			(GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions)(0), 0, 0);
	g_signal_connect(email_entry, "changed",
			(GCallback)summary_email_changed,
			NULL);

	categories_label = gtk_label_new(_("Categories:"));
	gtk_widget_show(categories_label);
	widget = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(widget), categories_label, FALSE, FALSE, 0);
	gtk_table_attach(GTK_TABLE(summary_table), widget, 0, 1, 5, 6,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(GTK_FILL), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(categories_label), 0, 0.5);

	categories_combo = gtk_combo_box_entry_new_text();
	gebrme.categories_combo = categories_combo;
	gtk_widget_show(categories_combo);
	gtk_combo_box_append_text(GTK_COMBO_BOX(categories_combo), "Data Compression");
	gtk_combo_box_append_text(GTK_COMBO_BOX(categories_combo), "Editing, Sorting and Manipulation");
	gtk_combo_box_append_text(GTK_COMBO_BOX(categories_combo), "Filtering, Transforms and Attributes");
	gtk_combo_box_append_text(GTK_COMBO_BOX(categories_combo), "Gain, NMO, Stack and Standard Processes");
	gtk_combo_box_append_text(GTK_COMBO_BOX(categories_combo), "Graphics");
	gtk_combo_box_append_text(GTK_COMBO_BOX(categories_combo), "Import/Export");
	gtk_combo_box_append_text(GTK_COMBO_BOX(categories_combo), "Inversion");
	gtk_combo_box_append_text(GTK_COMBO_BOX(categories_combo), "Migration and Dip Moveout");
	gtk_combo_box_append_text(GTK_COMBO_BOX(categories_combo), "Multiple Supression");
	gtk_combo_box_append_text(GTK_COMBO_BOX(categories_combo), "Seismic Unix");
	gtk_combo_box_append_text(GTK_COMBO_BOX(categories_combo), "Simulation and Model Building");
	gtk_combo_box_append_text(GTK_COMBO_BOX(categories_combo), "Utilities");
	/* TODO: GtkComboBoxEntry doesn't have activate signal */
// 	g_signal_connect(GTK_OBJECT(categories_combo), "activate",
// 		GTK_SIGNAL_FUNC(category_add), NULL);

	categories_sequence_edit = value_sequence_edit_new(categories_combo);
	gebrme.categories_sequence_edit = categories_sequence_edit;
	gtk_widget_show(categories_sequence_edit);
	g_signal_connect(GTK_OBJECT(categories_sequence_edit), "add-request",
		GTK_SIGNAL_FUNC(category_add), NULL);
	g_signal_connect(GTK_OBJECT(categories_sequence_edit), "changed",
		GTK_SIGNAL_FUNC(category_changed), NULL);
	gtk_table_attach(GTK_TABLE(summary_table), categories_sequence_edit, 1, 2, 5, 6,
			(GtkAttachOptions)(GTK_FILL),
			(GtkAttachOptions)(GTK_FILL), 0, 0);
	gtk_widget_show(categories_sequence_edit);

	/* Programs label and add button */
	programs_hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_show(programs_hbox);
	programs_frame = gtk_frame_new("");
	gtk_widget_show(programs_frame);
	gtk_frame_set_label_widget(GTK_FRAME(programs_frame), programs_hbox);
	gtk_box_pack_start(GTK_BOX(edition_vbox), programs_frame, TRUE, TRUE, 0);
	/* label */
	programs_label = gtk_label_new(_("Programs"));
	gtk_box_pack_start(GTK_BOX(programs_hbox), programs_label, FALSE, FALSE, 0);
	gtk_widget_show(GTK_WIDGET(programs_label));
	/* button */
	programs_add_button = gtk_button_new_from_stock (GTK_STOCK_ADD);
	gtk_box_pack_start(GTK_BOX(programs_hbox), programs_add_button, FALSE, FALSE, 10);
	gtk_widget_show(GTK_WIDGET(programs_add_button));
	g_signal_connect(programs_add_button, "clicked",
		(GCallback)program_add, NULL);
	g_object_set(G_OBJECT(programs_add_button), "relief", GTK_RELIEF_NONE, NULL);

	/* Programs' depth */
	depth_hbox = create_depth(programs_frame);

	/* Program view */
	programs_scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show(programs_scrolledwindow);
	gtk_box_pack_start(GTK_BOX (depth_hbox), programs_scrolledwindow, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (programs_scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	/* TODO: make it work and see the look */
// 	GdkColor white_color = {0xFFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
// 	gtk_widget_modify_bg(programs_scrolledwindow, GTK_STATE_NORMAL, &white_color);
// 	gtk_widget_modify_bg(programs_scrolledwindow, GTK_STATE_ACTIVE, &white_color);
// 	gtk_widget_modify_bg(programs_scrolledwindow, GTK_STATE_PRELIGHT, &white_color);
// 	gtk_widget_modify_bg(programs_scrolledwindow, GTK_STATE_SELECTED, &white_color);
// 	gtk_widget_modify_bg(programs_scrolledwindow, GTK_STATE_INSENSITIVE, &white_color);

	programs_vbox = gtk_vbox_new(FALSE, 0);
	gebrme.programs_vbox = programs_vbox;
	gtk_widget_show(programs_vbox);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(programs_scrolledwindow), programs_vbox);
	g_object_set(G_OBJECT(GTK_BIN(programs_scrolledwindow)->child), "shadow-type", GTK_SHADOW_NONE, NULL);

	statusbar = gtk_statusbar_new ();
	gebrme.statusbar = statusbar;
	gtk_widget_show(statusbar);
	gtk_box_pack_start (GTK_BOX (mainwindow_vbox), statusbar, FALSE, FALSE, 0);

	gtk_window_add_accel_group (GTK_WINDOW (gebrme_window), accel_group);
	g_signal_connect(gebrme_window, "destroy_event",
			(GCallback)gebrme_quit,
			NULL);

	return gebrme_window;
}
