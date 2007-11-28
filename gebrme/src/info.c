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

#include "info.h"
#include "gebrme.h"
#include "help.h"
#include "menu.h"

void
info_title_changed(GtkEntry * entry)
{
	geoxml_document_set_title(GEOXML_DOC(gebrme.current), gtk_entry_get_text(entry));
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

void
info_description_changed(GtkEntry * entry)
{
	geoxml_document_set_description(GEOXML_DOC(gebrme.current), gtk_entry_get_text(entry));
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

void
info_help_view(void)
{
	help_show(geoxml_document_get_help(GEOXML_DOC(gebrme.current)));
}

void
info_help_edit(void)
{
	GString *	help;

	help = help_edit(geoxml_document_get_help(GEOXML_DOC(gebrme.current)));
	geoxml_document_set_help(GEOXML_DOC(gebrme.current), help->str);
	g_string_free(help, TRUE);
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

void
info_author_changed(GtkEntry * entry)
{
	geoxml_document_set_author(GEOXML_DOC(gebrme.current), gtk_entry_get_text(entry));
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}

void
info_email_changed(GtkEntry * entry)
{
	geoxml_document_set_email(GEOXML_DOC(gebrme.current), gtk_entry_get_text(entry));
	menu_saved_status_set(MENU_STATUS_UNSAVED);
}
