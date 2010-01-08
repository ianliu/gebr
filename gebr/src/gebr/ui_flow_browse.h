/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation, either version 3 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see
 *   <http://www.gnu.org/licenses/>.
 */

#ifndef __UI_FLOW_BROWSE_H
#define __UI_FLOW_BROWSE_H

#include <gtk/gtk.h>

/* Store fields */
enum {
	FB_TITLE = 0,
	FB_FILENAME,
	FB_LINE_FLOW_POINTER,
	FB_N_COLUMN
};

struct ui_flow_browse {
	GtkWidget *widget;

	GtkListStore *store;
	GtkWidget *view;

	GtkWidget *revisions_menu;

	struct ui_flow_browse_info {
		GtkWidget *title;
		GtkWidget *description;

		GtkWidget *created_label;
		GtkWidget *created;
		GtkWidget *modified_label;
		GtkWidget *modified;
		GtkWidget *lastrun_label;
		GtkWidget *lastrun;

		GtkWidget *server_label;
		GtkWidget *server;
		GtkWidget *input_label;
		GtkWidget *input;
		GtkWidget *output_label;
		GtkWidget *output;
		GtkWidget *error_label;
		GtkWidget *error;

		GtkWidget *help;
		GtkWidget *author;
	} info;
};

struct ui_flow_browse *flow_browse_setup_ui(GtkWidget * revisions_menu);

void flow_browse_info_update(void);

gboolean flow_browse_get_selected(GtkTreeIter * iter, gboolean warn_unselected);

void flow_browse_select_iter(GtkTreeIter * iter);

void flow_browse_single_selection(void);

void flow_browse_load_revision(GebrGeoXmlRevision * revision, gboolean new);

#endif				//__UI_FLOW_BROWSE_H
