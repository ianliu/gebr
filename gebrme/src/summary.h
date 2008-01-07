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

#ifndef __SUMMARY_H
#define __SUMMARY_H

#include <gtk/gtk.h>

void
summary_title_changed(GtkEntry * entry);

void
summary_description_changed(GtkEntry * entry);

void
summary_help_view(void);

void
summary_help_edit(void);

void
summary_author_changed(GtkEntry * entry);

void
summary_email_changed(GtkEntry * entry);

#endif //__SUMMARY_H
