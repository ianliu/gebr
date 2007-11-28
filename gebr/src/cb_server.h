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

#ifndef _GEBR_CB_SERVER_H_
#define _GEBR_CB_SERVER_H_

#include <gtk/gtk.h>
#include "gebr.h"

#define GEBR_SERVER_CLOSE  101
#define GEBR_SERVER_REMOVE 102

void
server_dialog_actions                (GtkDialog *dialog,
				      gint       arg1,
				      gpointer   user_data);

void
server_add    (GtkEntry   *entry,
	       gpointer   *data );

void
server_remove    (void);

#endif //_GEBR_CB_SERVER_H_
