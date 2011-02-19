/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
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
 * File: ui_server.c
 * Server related UI.
 *
 * Assembly servers list configuration dialog and
 * server select dialog for flow execution (TODO).
 */

#include <string.h>

#include <glib/gi18n.h>
#include <libgebr/gui/gebr-gui-utils.h>
#include <libgebr/gui/gebr-gui-enhanced-entry.h>

#include "ui_server.h"
#include "gebr.h"

#define RESPONSE_CONNECT_ALL	101
#define RESPONSE_DISCONNECT_ALL	102
#define RESPONSE_CLOSE		103

/*
 * Prototypes
 */

static void server_list_add(struct ui_server_list *ui_server_list, const gchar * address);

static void on_combo_changed(gpointer user_data);

static gboolean visible_func (GtkTreeModel *model,
                              GtkTreeIter  *iter,
                              gpointer      data);
/*
 * Section: Private
 * Private functions.
 */

/* Function: server_common_tooltip_callback
 * Callback for tree view tooltip
 */
static gboolean
server_common_tooltip_callback(GtkTreeView * tree_view, GtkTooltip * tooltip,
			       GtkTreeIter * iter, GtkTreeViewColumn * column, struct ui_server_common *ui)
{
	if (gtk_tree_view_get_column(tree_view, SERVER_STATUS_ICON) == column) {
		struct server *server;
		gboolean had_error = TRUE;

		gtk_tree_model_get(GTK_TREE_MODEL(ui->sort_store), iter, SERVER_POINTER, &server, -1);
		if (server->last_error->len) //after connected state
			gtk_tooltip_set_text(tooltip, server->last_error->str);
		else if (server->comm->last_error->len) //before connected state
			gtk_tooltip_set_text(tooltip, server->comm->last_error->str);
		else
			had_error = FALSE;

		return had_error;
	}
	if (gtk_tree_view_get_column(tree_view, SERVER_AUTOCONNECT) == column) {
		gboolean autoconnect;

		gtk_tree_model_get(GTK_TREE_MODEL(ui->sort_store), iter, SERVER_AUTOCONNECT, &autoconnect, -1);
		if (autoconnect)
			gtk_tooltip_set_text(tooltip, _("Autoconnect ON"));
		else
			gtk_tooltip_set_text(tooltip, _("Autoconnect OFF"));

		return TRUE;
	}

	return FALSE;
}

/* Function: server_common_connect
 * Callback for popup menu action
 */
static void server_common_connect(GtkMenuItem * menu_item, struct server *server)
{
	gebr_comm_server_connect(server->comm);
}

/* Function: server_common_disconnect
 * Callback for popup menu action
 */
static void server_common_disconnect(GtkMenuItem * menu_item, struct server *server)
{
	gebr_comm_server_disconnect(server->comm);
}

/* Function: server_common_autoconnect
 * Callback for popup menu action
 */
static void server_common_autoconnect_changed(GtkMenuItem * menu_item, struct server *server)
{
	gtk_list_store_set(gebr.ui_server_list->common.store, &server->iter,
			   SERVER_AUTOCONNECT, gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item)), -1);
}

/* Function: server_common_remove
 * Callback for popup menu action
 */
static void server_common_remove(GtkMenuItem * menu_item, struct server *server)
{
	server_free(server);
	ui_server_update_tags_combobox ();
}

/* Function: server_common_stop
 * Send server command to kill daemon
 */
static void server_common_stop(GtkMenuItem * menu_item, struct server *server)
{
	gebr_comm_server_kill(server->comm);
}

/* Function; server_common_popup_menu
 * Context menu for server tree view
 */
static GtkMenu *server_common_popup_menu(GtkWidget * widget, struct ui_server_common *ui_server_common)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;

	GtkWidget *menu;
	GtkWidget *menu_item;

	struct server *server;
	gboolean autoconnect;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ui_server_common->view));
	if (gtk_tree_selection_get_selected(selection, &model, &iter) == FALSE)
		return NULL;

	gtk_tree_model_get(model, &iter, SERVER_POINTER, &server, SERVER_AUTOCONNECT, &autoconnect, -1);
	menu = gtk_menu_new();

	/* connect */
	if (!server->comm->protocol->logged) {
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CONNECT, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect(menu_item, "activate", G_CALLBACK(server_common_connect), server);
	} else {
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DISCONNECT, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect(menu_item, "activate", G_CALLBACK(server_common_disconnect), server);
	}
	/* autoconnect */
	menu_item = gtk_check_menu_item_new_with_label(_("Autoconnect"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect(menu_item, "toggled", G_CALLBACK(server_common_autoconnect_changed), server);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), autoconnect);
	/* remove */
	if (!gebr_comm_server_is_local(server->comm)) {
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_REMOVE, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect(menu_item, "activate", G_CALLBACK(server_common_remove), server);
	}
	/* stop server */
	menu_item = gtk_image_menu_item_new_with_label(_("Stop server"));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),
				      gtk_image_new_from_stock(GTK_STOCK_STOP, GTK_ICON_SIZE_MENU));
	g_signal_connect(menu_item, "activate", G_CALLBACK(server_common_stop), server);

	gtk_widget_show_all(menu);

	return GTK_MENU(menu);
}

static void on_tags_edited (GtkCellRendererText *cell,
			    gchar *pathstr,
			    gchar *new_text,
			    GtkTreeModel *model)
{
	gchar *tags;
	GtkTreeIter iter;
	GtkTreePath *model_path;
	GtkTreePath *filter_path;
	GtkTreePath *sorted_path;
	struct server *server;
	gboolean ret;

	ret = gtk_tree_model_get_iter_from_string (
			GTK_TREE_MODEL (gebr.ui_server_list->common.sort_store),
			&iter, pathstr);

	if (!ret)
		return;

	// This code resolves the pathstr variable to the original tree model.
	// Welcome to Hell.

	sorted_path = gtk_tree_model_get_path (
			GTK_TREE_MODEL (gebr.ui_server_list->common.sort_store),
			&iter);

	filter_path = gtk_tree_model_sort_convert_path_to_child_path (
			GTK_TREE_MODEL_SORT (gebr.ui_server_list->common.sort_store),
			sorted_path);

	model_path = gtk_tree_model_filter_convert_path_to_child_path (
			GTK_TREE_MODEL_FILTER (gebr.ui_server_list->common.filter),
			filter_path);

	ret = gtk_tree_model_get_iter (model, &iter, model_path);

	if (!ret)
		return;

	gtk_tree_model_get (model, &iter,
			    SERVER_POINTER, &server,
			    SERVER_TAGS, &tags,
			    -1);

	if (!g_str_equal (tags, new_text))
		ui_server_set_tags (server, new_text);
}

/* Function: server_common_setup
 * Setup common server dialogs stuff
 */
static void server_common_setup(struct ui_server_common *ui_server_common)
{
	GtkWidget *scrolled_window;
	GtkWidget *view;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	ui_server_common->add_local_button = NULL;
	gtk_widget_set_size_request(ui_server_common->dialog, 580, 300);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	ui_server_common->widget = scrolled_window;
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ui_server_common->sort_store));
	gtk_container_add(GTK_CONTAINER(scrolled_window), view);
	ui_server_common->view = view;
	gebr_gui_gtk_tree_view_set_popup_callback(GTK_TREE_VIEW(view),
						  (GebrGuiGtkPopupCallback) server_common_popup_menu, ui_server_common);
#if GTK_CHECK_VERSION(2,12,0)
	gebr_gui_gtk_tree_view_set_tooltip_callback(GTK_TREE_VIEW(view),
						    (GebrGuiGtkTreeViewTooltipCallback) server_common_tooltip_callback,
						    ui_server_common);
#endif

	renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "pixbuf", SERVER_STATUS_ICON);

	renderer = gtk_cell_renderer_toggle_new();
	col = gtk_tree_view_column_new_with_attributes(_("AC"), renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "active", SERVER_AUTOCONNECT);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Address"), renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", SERVER_NAME);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("CPU"), renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", SERVER_CPU);
	gtk_tree_view_column_set_sort_column_id(col, SERVER_CPU);
	gtk_tree_view_column_set_sort_indicator(col, TRUE);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Memory"), renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", SERVER_MEM);
	gtk_tree_view_column_set_sort_column_id(col, SERVER_MEM);
	gtk_tree_view_column_set_sort_indicator(col, TRUE);

	renderer = gtk_cell_renderer_text_new();
	g_object_set (renderer, "editable", TRUE, NULL);
	g_signal_connect (renderer, "edited",
			  G_CALLBACK (on_tags_edited), ui_server_common->store);
	col = gtk_tree_view_column_new_with_attributes(_("Groups"), renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", SERVER_TAGS);
	gtk_tree_view_column_set_expand (col, TRUE);
}

/*
 * Function: server_common_actions
 * Take the appropriate action when the server dialog emmits
 * a response signal.
 */
static void server_common_actions(GtkDialog * dialog, gint arg1, struct ui_server_common *ui_server_common)
{
	switch (arg1) {
	case RESPONSE_CONNECT_ALL:{
			GtkTreeIter iter;

			gebr_gui_gtk_tree_model_foreach(iter, GTK_TREE_MODEL(ui_server_common->store)) {
				struct server *server;

				gtk_tree_model_get(GTK_TREE_MODEL(ui_server_common->store), &iter,
						   SERVER_POINTER, &server, -1);

				if (gebr_comm_server_is_logged(server->comm) == FALSE)
					gebr_comm_server_connect(server->comm);
			}

			break;
		}
	case RESPONSE_DISCONNECT_ALL:{
			GtkTreeIter iter;

			gebr_gui_gtk_tree_model_foreach(iter, GTK_TREE_MODEL(ui_server_common->store)) {
				struct server *server;

				gtk_tree_model_get(GTK_TREE_MODEL(ui_server_common->store), &iter,
						   SERVER_POINTER, &server, -1);

				if (gebr_comm_server_is_logged(server->comm) == TRUE)
					gebr_comm_server_disconnect(server->comm);
			}

			break;
		}
	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		if (dialog != GTK_DIALOG(gebr.ui_server_list->common.dialog))
			break;
	case RESPONSE_CLOSE:	/* Only for server list */
		gtk_widget_hide(gebr.ui_server_list->common.dialog);
		gebr_config_save(FALSE);
		break;
	default:
		break;
	}
}

/*
 * Function: server_list_add
 * Callback to add a server to the server list
 */
static void server_list_add(struct ui_server_list *ui_server_list, const gchar * address)
{
	GtkTreeIter iter;

	/* check if it is already in list */
	gebr_gui_gtk_tree_model_foreach(iter, GTK_TREE_MODEL(ui_server_list->common.store)) {
		struct server *server;

		gtk_tree_model_get(GTK_TREE_MODEL(ui_server_list->common.store), &iter, SERVER_POINTER, &server, -1);

		if (!strcmp(server->comm->address->str, address)) {
			gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(ui_server_list->common.view), &iter);
			return;
		}
	}

	/* finally */
	server_new(address, TRUE, "");
}

/* Function: on_add_clicked
 * Call for entry activation
 */
static void on_entry_activate(GtkEntry * entry, struct ui_server_list *ui_server_list)
{
	server_list_add(ui_server_list, gebr_gui_enhanced_entry_get_text(GEBR_GUI_ENHANCED_ENTRY(entry)));
	gtk_entry_set_text(entry, "");
}

/* Function: on_add_clicked
 * Call for add button
 */
static void on_add_clicked(GtkButton * button, struct ui_server_list *ui_server_list)
{
	GtkEntry *entry;

	g_object_get(button, "user-data", &entry, NULL);

	if (g_str_equal( _("Type here [user@]serverhostname"), gtk_entry_get_text(entry)))
		return;

	on_entry_activate(entry, ui_server_list);
}

static void on_combo_changed (gpointer user_data)
{
	gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (gebr.ui_server_list->common.filter));
}

static gboolean visible_func (GtkTreeModel *model,
                              GtkTreeIter  *iter,
			      gpointer      data)
{
	gchar *tag;
	GtkTreeIter active;
	GtkComboBox *combo;
	struct server *server;

	combo = GTK_COMBO_BOX (gebr.ui_server_list->common.combo);

	if (!gtk_combo_box_get_active_iter (combo, &active))
		return TRUE;

	// Means we selected 'All servers'
	if (gtk_combo_box_get_active (combo) == 0)
		return TRUE;

	gtk_tree_model_get (model, iter, SERVER_POINTER, &server, -1);
	gtk_tree_model_get (gtk_combo_box_get_model (combo), &active,
			    0, &tag, -1);

	return ui_server_has_tag (server, tag);
}

/*
 * Section: Public
 * Public functions.
 */

/*
 * Function: server_list_setup_ui
 * Assembly the servers configurations dialog.
 *
 * Return:
 * The structure containing relevant data.
 */
struct ui_server_list *server_list_setup_ui(void)
{
	struct ui_server_list *ui_server_list;

	GtkWidget *dialog;
	GtkWidget *button;
	GtkWidget *hbox;
	GtkWidget *entry;

	ui_server_list = g_new(struct ui_server_list, 1);
	ui_server_list->common.store = gtk_list_store_new(SERVER_N_COLUMN,
							  GDK_TYPE_PIXBUF,	/* Status icon */
							  G_TYPE_BOOLEAN,	/* Autoconnect */
							  G_TYPE_STRING,	/* Server name */
							  G_TYPE_POINTER,	/* Server pointer */
							  G_TYPE_STRING,	/* Tag List    */
							  G_TYPE_STRING,	/* CPU Info    */
							  G_TYPE_STRING 	/* Memory Info */
							 );

	ui_server_list->common.filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(ui_server_list->common.store), NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(ui_server_list->common.filter), visible_func, NULL, NULL);

	ui_server_list->common.sort_store = gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(ui_server_list->common.filter));

	dialog = gtk_dialog_new_with_buttons(_("Servers configuration"),
					     GTK_WINDOW(gebr.window),
					     (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), NULL);
	button = gtk_dialog_add_button(GTK_DIALOG(dialog), _("Co_nnect all"), RESPONSE_CONNECT_ALL);
	gtk_button_set_image(GTK_BUTTON(button),
			     gtk_image_new_from_stock(GTK_STOCK_CONNECT, GTK_ICON_SIZE_SMALL_TOOLBAR));
	button = gtk_dialog_add_button(GTK_DIALOG(dialog), _("_Disconnect all"), RESPONSE_DISCONNECT_ALL);
	gtk_button_set_image(GTK_BUTTON(button),
			     gtk_image_new_from_stock(GTK_STOCK_DISCONNECT, GTK_ICON_SIZE_SMALL_TOOLBAR));
	button = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CLOSE, RESPONSE_CLOSE);
	g_signal_connect(dialog, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);
	ui_server_list->common.dialog = dialog;
	/* Take the apropriate action when a button is pressed */
	g_signal_connect(dialog, "response", G_CALLBACK(server_common_actions), &ui_server_list->common);

	server_common_setup(&ui_server_list->common);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), ui_server_list->common.widget, TRUE, TRUE, 0);

	ui_server_list->common.combo_store = gtk_list_store_new(1, G_TYPE_STRING);
	ui_server_list->common.combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(ui_server_list->common.combo_store));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), ui_server_list->common.combo, FALSE, TRUE, 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(ui_server_list->common.combo), -1);

	GtkCellRenderer *renderer;
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(ui_server_list->common.combo), renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(ui_server_list->common.combo), renderer, "text", 0);
	g_signal_connect(ui_server_list->common.combo, "changed", G_CALLBACK(on_combo_changed), NULL);

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, TRUE, 0);

	entry = gebr_gui_enhanced_entry_new_with_empty_text(_("Type here [user@]serverhostname"));
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
	g_signal_connect(GTK_OBJECT(entry), "activate", G_CALLBACK(on_entry_activate), ui_server_list);

	button = gtk_button_new_from_stock(GTK_STOCK_ADD);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	g_signal_connect(GTK_OBJECT(button), "clicked", G_CALLBACK(on_add_clicked), ui_server_list);
	g_object_set(button, "user-data", entry, NULL);

	return ui_server_list;
}

/* Function: server_list_show
 * Show _ui_server_list_ taking appropriate actions
 */
void server_list_show(struct ui_server_list *ui_server_list)
{
	gtk_widget_show_all(ui_server_list->common.dialog);
}

/*
 * Function: server_list_updated_status
 * Update status of _server_ in store
 *
 */
void server_list_updated_status(struct server *server)
{
	GdkPixbuf *status_icon;
	GtkTreePath *path;

	status_icon = (server->last_error->len || server->comm->last_error->len)
	    ? gebr.pixmaps.stock_warning : (server->comm->protocol->logged == TRUE)
	    ? gebr.pixmaps.stock_connect : gebr.pixmaps.stock_disconnect;

	gtk_list_store_set(gebr.ui_server_list->common.store, &server->iter, SERVER_STATUS_ICON, status_icon, -1);

	/* update view */
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(gebr.ui_server_list->common.store), &server->iter);
	gtk_tree_model_row_changed(GTK_TREE_MODEL(gebr.ui_server_list->common.store), path, &server->iter);
}

gchar **ui_server_list_tag (struct server *server)
{
	gchar *tags;
	gchar **tag_list;
	GtkTreeModel *model;

	model = GTK_TREE_MODEL (gebr.ui_server_list->common.store);

	gtk_tree_model_get(model, &server->iter, SERVER_TAGS, &tags, -1);
	tag_list = g_strsplit(tags, ",", 0);
	g_free (tags);

	return tag_list;
}

GList *ui_server_servers_with_tag (const gchar *tag) {
	GList *list = NULL;
	GtkTreeIter iter;
	GtkTreeModel *model;

	model = GTK_TREE_MODEL (gebr.ui_server_list->common.store);

	gebr_gui_gtk_tree_model_foreach (iter, model) {
		struct server *server;
		gchar *tags;
		gchar **tag_list;

		gtk_tree_model_get(model, &iter,
				   SERVER_POINTER, &server,
				   SERVER_TAGS, &tags,
				   -1);

		tag_list = g_strsplit (tags, ",", 0);	
		for (gint i = 0; tag_list[i]; i++) {
			if (g_str_equal (tag_list[i], tag)) {
				list = g_list_prepend (list, server);
				break;
			}
		}
		g_free (tags);
		g_strfreev (tag_list);
	}

	return g_list_reverse (list);
}

gboolean ui_server_has_tag (struct server *server, const gchar *tag)
{
	gchar *tags;
	gchar **tag_list;
	GtkTreeModel *model;
	gboolean retval = FALSE;

	model = GTK_TREE_MODEL (gebr.ui_server_list->common.store);

	gtk_tree_model_get (model, &server->iter, SERVER_TAGS, &tags, -1);
	tag_list = g_strsplit (tags, ",", 0);
	for (gint i = 0; !retval && tag_list[i]; i++)
		if (g_str_equal (tag_list[i], tag))
			retval = TRUE;

	g_strfreev (tag_list);
	g_free (tags);

	return retval;
}

/*
 * Sorts @tags_str and removes duplicate tags.
 * Free the returned value with g_free().
 */
static gchar *sort_and_remove_doubles (const gchar *tags_str)
{
	gchar **tagsv;
	GString *tags;
	GPtrArray *array;

	tagsv = g_strsplit (tags_str, ",", 0);
	array = g_ptr_array_new ();
	tags  = g_string_new ("");

	for (int i = 0; tagsv[i]; i++) {
		gchar *tag = g_strstrip (tagsv[i]);
		if (strlen (tag) > 0)
			g_ptr_array_add (array, tag);
	}

	int sort (gchar **v1, gchar **v2) {
		return g_strcmp0 (*v1, *v2);
	}

	tags = g_string_new ("");

	// Sort the array so we can remove duplicates
	g_ptr_array_sort (array, (GCompareFunc) sort);

	for (int i = 0; i < (gint)array->len - 1; i++) {
		const gchar *tag1 = g_ptr_array_index (array, i);
		const gchar *tag2 = g_ptr_array_index (array, i+1);
		if (!g_str_equal (tag1, tag2))
			g_string_append_printf (tags, "%s,", tag1);
	}

	if (array->len)
		g_string_append (tags, g_ptr_array_index (array, array->len-1));

	g_strfreev (tagsv);
	g_ptr_array_free (array, FALSE);
	return g_string_free (tags, FALSE);
}

void ui_server_set_tags (struct server *server, const gchar *str)
{
	gchar *tags;

	tags = sort_and_remove_doubles (str);
	gtk_list_store_set (gebr.ui_server_list->common.store, &server->iter,
			    SERVER_TAGS, tags,
			    -1);
	g_free (tags);

	ui_server_update_tags_combobox ();
}

gchar **ui_server_get_all_tags (void)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GString *concat;
	gchar *tags;
	gchar **retval;

	concat = g_string_new ("");
	model = GTK_TREE_MODEL (gebr.ui_server_list->common.store);

	gebr_gui_gtk_tree_model_foreach (iter, model) {
		gtk_tree_model_get (model, &iter, SERVER_TAGS, &tags, -1);
		g_string_append_printf (concat, "%s,", tags);
		g_free (tags);
	}

	tags = sort_and_remove_doubles (concat->str);
	retval =  g_strsplit (tags, ",", 0);

	g_free (tags);
	g_string_free (concat, TRUE);

	return retval;
}

void ui_server_update_tags_combobox (void)
{
	int active = -1;
	gchar **tags;
	GtkTreeIter iter;
	GtkComboBox *combo;
	GtkTreeModel *model;
	gchar *selected = NULL;

	combo = GTK_COMBO_BOX (gebr.ui_server_list->common.combo);
	model = GTK_TREE_MODEL (gebr.ui_server_list->common.combo_store);

	if (gtk_combo_box_get_active_iter (combo, &iter))
		gtk_tree_model_get (model, &iter, 0, &selected, -1);

	gtk_list_store_clear (gebr.ui_server_list->common.combo_store);
	tags = ui_server_get_all_tags ();
	gtk_list_store_append (gebr.ui_server_list->common.combo_store, &iter);
	gtk_list_store_set (gebr.ui_server_list->common.combo_store,
			    &iter, 0, _("All Servers"), -1);

	for (int i = 0; tags[i]; i++) {
		gtk_list_store_append (gebr.ui_server_list->common.combo_store, &iter);
		gtk_list_store_set (gebr.ui_server_list->common.combo_store,
				    &iter, 0, tags[i], -1);

		if (active == -1 && selected && g_str_equal (tags[i], selected))
			active = i;
	}

	gtk_combo_box_set_active (combo, active);
}
