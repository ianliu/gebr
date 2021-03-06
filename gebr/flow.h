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
 * @file flow.c Flow manipulation
 */

#ifndef __FLOW_H
#define __FLOW_H

#include <libgebr/gui/gebr-gui-html-viewer-widget.h>
#include <libgebr/comm/gebr-comm.h>

G_BEGIN_DECLS

typedef void (*GebrFlowModifyPathsFunc) (GString *path,
					 gpointer data);

/** 
 * Creates a new flow.
 */
void flow_new(void);
/** 
 * Frees the memory allocated to a flow.
 * Besides, update the detailed view of a flow in the interface.
 */
void flow_free(void);
/** 
 * Delete the selected flow in flow browser.
 */
void flow_delete(gboolean confirm);
/** 
 * Import flow from file to the current line.
 */
void flow_import(void);
/** 
 * Export selected(s) flow(s).
 */
void flow_export(void);
/**
 * Copy all values of parameters linked to dictionaries' parameters.
 */
void flow_copy_from_dicts(GebrGeoXmlFlow * flow);
/**
 * Change all paths to relative or absolute according to \p relative.
 */
void flow_set_paths_to_relative(GebrGeoXmlFlow * flow, GebrGeoXmlLine *line, gchar ***paths, gboolean relative);
/**
 * Clear all flow's paths
 */
void flow_set_paths_to_empty(GebrGeoXmlFlow * flow);

/**
 * Remove selected program from flow process.
 */
void flow_program_remove(void);

/*
 *Set the last snapshot modification date 
 * */
void
gebr_flow_set_snapshot_last_modify_date(const gchar *last_date);

/**
 * Make a revision from current flow.
 * Opens a dialog asking the user for a comment of it.
 */
gboolean flow_revision_save(void);

/**
 *
 */
gboolean flow_revision_remove(GebrGeoXmlFlow *flow,
                              gchar *id_remove,
                              gchar *parent_head,
                              GHashTable *hash);

/**
 *
 */
GHashTable *gebr_flow_revisions_hash_create(GebrGeoXmlFlow *flow);

/**
 *
 */
void gebr_flow_revisions_hash_free(GHashTable *revision);

gboolean gebr_flow_revisions_create_graph(GebrGeoXmlFlow *flow,
                                          GHashTable *revs,
                                          gchar ** png_filename);

/**
 * Move selected program to top in the processing flow.
 */
void flow_program_move_top(void);
/** 
 * Move selected program to bottom in the processing flow.
 */
void flow_program_move_bottom(void);

/**
 * Copy selected(s) flows(s) to clipboard.
 */
void flow_copy(void);
/** 
 * Paste flow(s) from clipboard.
 */
void flow_paste(void);
/** 
 * Copy selected(s) program(s) to clipboard.
 */
void flow_program_copy(void);

/** 
 * Paste program(s) from clipboard.
 */
void flow_program_paste(void);

void gebr_flow_modify_paths(GebrGeoXmlFlow *flow,
			    GebrFlowModifyPathsFunc func,
			    gboolean set_programs_unconfigured,
			    gpointer data);

/**
 *
 */
void gebr_flow_set_toolbar_sensitive(void);

G_END_DECLS

#endif				//__FLOW_H
