/*   GÍBR - An environment for seismic processing.
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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
	FB_N_COLUMN
};

struct ui_flow_browse {
	GtkWidget *		widget;

	GtkListStore *		store;
	GtkWidget *		view;

	struct ui_flow_browse_info {
		GtkWidget *	title;
		GtkWidget *	description;

		GtkWidget *	input_label;
		GtkWidget *	input;
		GtkWidget *	output_label;
		GtkWidget *	output;
		GtkWidget *	error_label;
		GtkWidget *	error;

		GtkWidget *	help;
		GtkWidget *	author;
	} info;
};

struct ui_flow_browse *
flow_browse_setup_ui(void);

void
flow_browse_info_update(void);

#endif //__UI_FLOW_BROWSE_H
