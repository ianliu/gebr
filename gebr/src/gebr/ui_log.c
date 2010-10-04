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

/*
 * File: ui_log.c
 * Responsible for UI for viewing the log file
 */

#include <string.h>

#include <libgebr/intl.h>
#include <libgebr/gui/gebr-gui-utils.h>
#include <libgebr/date.h>

#include "ui_log.h"
#include "gebr.h"

/*
 * Prototypes
 */

/*
 * Section: Public
 * Public functions.
 */

/*
 * Function: log_setup_ui
 * Assembly the job control page.
 *
 * Return:
 * The structure containing relevant data.
 *
 */
struct ui_log *log_setup_ui(void)
{
	struct ui_log *ui_log;

	GtkWidget *scrolled_win;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	ui_log = g_new(struct ui_log, 1);
	ui_log->widget = gtk_expander_new("");

	/*
	 * Store/View
	 */
	scrolled_win = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(GTK_WIDGET(scrolled_win), 180, 130);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(ui_log->widget), scrolled_win);

	ui_log->store = gtk_list_store_new(GEBR_LOG_N_COLUMN, GDK_TYPE_PIXBUF,	/* Icon         */
					   G_TYPE_STRING,	/* Date         */
					   G_TYPE_STRING);	/* struct job   */
	ui_log->view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(ui_log->store));
	gtk_container_add(GTK_CONTAINER(scrolled_win), ui_log->view);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(ui_log->view), FALSE);

	renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes("", renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_log->view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "pixbuf", GEBR_LOG_TYPE_ICON);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Date"), renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_log->view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "markup", GEBR_LOG_DATE);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(_("Message"), renderer, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(ui_log->view), col);
	gtk_tree_view_column_add_attribute(col, renderer, "text", GEBR_LOG_MESSAGE);

	return ui_log;
}

/*
 * Section: Private
 * Private functions.
 */

void log_set_message(struct ui_log *ui_log, const gchar * message)
{
	gchar *msga;
	gchar *ptr;
	GString *msgb;

	msga = g_strndup(message, 80);
	msga = g_strdelimit(g_strchug(msga), "\n", ' ');

	ptr = msga;
	while (*ptr != '\0') {
		if (g_ascii_iscntrl(*ptr))
			*ptr = ' ';
		ptr++;
	}

	msgb = g_string_new(msga);
	if (strlen(message) > 80)
		g_string_append(msgb, "...");
	gtk_expander_set_label(GTK_EXPANDER(ui_log->widget), msgb->str);

	g_free(msga);
	g_string_free(msgb, TRUE);
}

void gebr_log_add_message_to_list(struct ui_log *ui_log, struct gebr_log_message *message)
{
	GtkTreeIter iter;
	GdkPixbuf *pixbuf;
	GString *markuped_date;

	/* date */
	markuped_date = g_string_new(NULL);
	g_string_printf(markuped_date, "<small>%s</small>", gebr_localized_date(message->date->str));
	/* icon type */
	switch (message->type) {
	case GEBR_LOG_START:
		pixbuf = gebr.pixmaps.stock_go_forward;
		break;
	case GEBR_LOG_END:
		pixbuf = gebr.pixmaps.stock_go_back;
		break;
	case GEBR_LOG_INFO:
		pixbuf = gebr.pixmaps.stock_info;
		break;
	case GEBR_LOG_ERROR:
		pixbuf = gebr.pixmaps.stock_cancel;
		break;
	case GEBR_LOG_WARNING:
		pixbuf = gebr.pixmaps.stock_warning;
		break;
	default:
		return;
	}
	/* add */
	gtk_list_store_append(ui_log->store, &iter);
	gtk_list_store_set(ui_log->store, &iter,
			   GEBR_LOG_TYPE_ICON, pixbuf,
			   GEBR_LOG_DATE, markuped_date->str, GEBR_LOG_MESSAGE, message->message->str, -1);
	gebr_gui_gtk_tree_view_select_iter(GTK_TREE_VIEW(ui_log->view), &iter);

	/* frees */
	g_string_free(markuped_date, TRUE);
}
