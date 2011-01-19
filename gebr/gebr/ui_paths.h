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

#ifndef __UI_PATH_H
#define __UI_PATH_H

#include <libgebr/gui/gebr-gui-value-sequence-edit.h>

G_BEGIN_DECLS

gchar * gebr_stripdir(gchar * dir, gchar *buf, gint maxlen);
gboolean path_save(void);

void path_add(GebrGuiValueSequenceEdit * sequence_edit);

/**
 * path_renamed:
 * @self:
 * @old_text:
 * @new_text:
 *
 * Callback for #GebrGuiSequenceEdit::renamed signal.
 */
gboolean path_renamed (GebrGuiValueSequenceEdit * self, const gchar * old_text, const gchar * new_text);

G_END_DECLS
#endif				//__UI_PATH_H
