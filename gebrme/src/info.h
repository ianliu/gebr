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

#ifndef __INFO_H
#define __INFO_H

#include <gtk/gtk.h>

void
info_title_changed(GtkEntry * entry);

void
info_description_changed(GtkEntry * entry);

void
info_help_view(void);

void
info_help_edit(void);

void
info_author_changed(GtkEntry * entry);

void
info_email_changed(GtkEntry * entry);

#endif //__INFO_H
