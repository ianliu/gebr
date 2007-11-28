/*   GÍBR - An environment for seismic processing.
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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

#ifndef _GEBR_H_
#define _GEBR_H_

#define STRMAX 2048

#include <geoxml.h>
#include "widgets.h"

extern GeoXmlFlow *	flow;
extern gebrw_t		W;

enum msg_type {
	START, END, ACTION, SERVER, ERROR, WARNING, INTERFACE
};

void
gebr_init(void);

gboolean
gebr_quit       (GtkWidget *widget,
		 GdkEvent  *event,
		 gpointer   user_data);

int
gebr_config_load(int argc, char ** argv);

int
gebr_config_reload(void);

int
gebr_config_save(void);

void
log_message(enum msg_type type, const gchar * message, gboolean in_statusbar);

#endif //_GEBR_H_
