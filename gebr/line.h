/*   GeBR - An environment for seismic processing.
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
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

/**
 * \file line.c
 * Lines manipulation functions
 */

#ifndef __LINE_H
#define __LINE_H

#include <gtk/gtk.h>

#include <libgebr/geoxml/geoxml.h>

/**
 * Error types for import and base entry
 */
typedef enum {
	OK_ENTRY,
	EMPTY_ENTRY,
	NOT_ABSOLUTE_ENTRY
} GebrErrorEntry;

G_BEGIN_DECLS

/**
 * on_line_callback_base_entry_press:
 *
 * Callback used on Assistant and properties for Line.
 */
void on_line_callback_base_entry_press(GtkEntry            *entry,
                                       GtkEntryIconPosition icon_pos,
                                       GdkEvent            *event,
                                       gpointer             user_data);

/**
 * on_line_callback_import_entry_press:
 *
 * Callback used on Assistant and properties for Line.
 */
void on_line_callback_import_entry_press(GtkEntry            *entry,
                                         GtkEntryIconPosition icon_pos,
                                         GdkEvent            *event,
                                         gpointer             user_data);

/**
 * on_line_callback_base_focus_out:
 *
 * Callback used on Assistant and properties for Line.
 */
gboolean on_line_callback_base_focus_out(GtkWidget *widget,
                                         GdkEvent  *event);

/**
 * Create a new line.
 * If \p silent is TRUE then no message and dialogs are displayed to user.
 */
void line_new(void);
/**
 * Delete the selected line.
 */
gboolean line_delete(GtkTreeIter * iter, gboolean warn_user);
/**
 * Change all paths in \p line to relative or absolute according \p relative.
 */
void line_set_paths_to_relative(GebrGeoXmlLine * line, gboolean relative);
/**
 * Clear all paths
 */
void line_set_paths_to_empty(GebrGeoXmlLine *flow);

/** 
 * Append \p flow to flow browse and return the appended iter.
 */
GtkTreeIter line_append_flow_iter(GebrGeoXmlFlow * flow, GebrGeoXmlLineFlow * line_flow);
/** 
 * Load flows associated to the selected line.
 * Called only by project_line_load.
 */
void line_load_flows(void);

/** 
 * Move flow top
 */
void line_move_flow_top(void);
/** 
 * Move flow bottom
 */
void line_move_flow_bottom(void);

/**
 *
 */
void on_properties_entry_changed(GtkEntry *entry, GtkWidget *ok_button);

G_END_DECLS
#endif				//__LINE_H
