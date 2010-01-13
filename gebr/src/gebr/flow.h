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

#ifndef __FLOW_H
#define __FLOW_H

#include "server.h"

gboolean flow_new(void);
void flow_free(void);
void flow_delete(gboolean confirm);
void flow_import(void);
void flow_export(void);
void flow_export_as_menu(void);
void flow_copy_from_dicts(GebrGeoXmlFlow * flow);
void flow_set_paths_to(GebrGeoXmlFlow * flow, gboolean relative);
void flow_run(struct server *server);
gboolean flow_revision_save(void);

void flow_program_remove(void);
void flow_program_move_top(void);
void flow_program_move_bottom(void);

void flow_copy(void);
void flow_paste(void);
void flow_program_copy(void);
void flow_program_paste(void);

#endif				//__FLOW_H
