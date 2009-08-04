/*   libgebr - GeBR Library
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

#include <string.h>

#include <gdome.h>

#include "clipboard.h"
#include "xml.h"
#include "types.h"

extern GdomeDocument *	clipboard_document;

void
geoxml_clipboard_clear(void)
{
	GdomeElement *	document_element;
	GdomeElement *	element;

	document_element = gdome_doc_documentElement(clipboard_document, &exception);
	while ((element = __geoxml_get_first_element(document_element, "*")))
		gdome_el_removeChild(document_element, (GdomeNode*)element, &exception);
}

void
geoxml_clipboard_copy(GeoXmlObject * object)
{
	if (object == NULL)
		return;

	gdome_el_appendChild(gdome_doc_documentElement(clipboard_document, &exception),
		gdome_doc_importNode(clipboard_document, (GdomeNode*)object, TRUE, &exception), &exception);
}

GeoXmlObject *
geoxml_clipboard_paste(GeoXmlObject * object)
{
	if (object == NULL)
		return NULL;

	static const gchar *		child_parent [][2] = {
		{"program", "flow"},
		{"parameter", "program"},
		{NULL, NULL}
	};
	GdomeElement *			paste_element;
	GdomeElement *			container_element;
	GeoXmlObject *			first_paste = NULL;

	container_element = gdome_n_nodeType((GdomeNode*)object, &exception) == GDOME_DOCUMENT_NODE
		? gdome_doc_documentElement((GdomeDocument*)object, &exception) : (GdomeElement*)object;
	paste_element = __geoxml_get_first_element(
		gdome_doc_documentElement(clipboard_document, &exception), "*");
	for (; paste_element != NULL; paste_element = __geoxml_next_element(paste_element)) {
		for (int i = 0; child_parent[i][0] != NULL; ++i) {
			if (!strcmp(gdome_el_tagName(paste_element, &exception)->str, child_parent[i][0]) &&
			!strcmp(gdome_el_tagName(container_element, &exception)->str, child_parent[i][1])) {
				GdomeNode *	imported;

				imported = gdome_doc_importNode(
					gdome_el_ownerDocument(container_element, &exception),
					(GdomeNode*)paste_element, TRUE, &exception);
				gdome_el_appendChild(container_element, imported, &exception);
				__geoxml_element_reassign_ids((GdomeElement*)imported);
				if (first_paste == NULL)
					first_paste = GEOXML_OBJECT(imported);
			}
		}
	}

	return first_paste;
}
