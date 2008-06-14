/*   libgebr - GeBR Library
 *   Copyright (C) 2007 GeBR core team (http://gebr.sourceforge.net)
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

#include "error.h"
#include "support.h"

const gchar *
geoxml_error_string(enum GEOXML_RETV error)
{
	gchar * 	error_string_array [] = {
		_("Success."),
		_("Null pointer."),
		_("Not enough memory."),
		_("Can't find file or permission denied."),
		_("Invalid element index."),
		_("Invalid document syntax or structure."),
		_("DTD specified in XML."),
		_("Could not find or read DTD."),
		_("Not a sequence."),
		_("An operation on two sequences requires that both sequences are of the same type."),
		_("Not an enum."),
		_("")
	};
	guint		index = -(guint)error;

	if (index > 11)
		return NULL;
	return error_string_array[index];
}

const gchar *
geoxml_error_explained_string(enum GEOXML_RETV error)
{
	gchar * 	error_string_array [] = {
		_("The operation was done successfuly."),
		_("One or more mandatory function arguments were null."),
		_("Not enough memory. "
			"The library stoped after an unsucessful memory allocation."),
		_("Can't access file. The file doesn't exist or there is not enough "
			"permission to read it."),
		_("This element index does not exist in the sequence."),
		_("Invalid XML syntax or it does not respect the structure "
			"defined on its corresponding document DTD."),
		_("DTD was specified for the document. LibGeBR-GeoXml requires the document "
			"not to specify the DTD as it automatically find it and validate against."),
		_("DTD for the corresponding document type (flow, line or project) and version "
			"could not be found or accessed on the system.\n"
			"You should verify if your installation is correct and if it supports "
			"this document version (you might need to update your software)."),
		_("The argument(s) passed are not inside a sequence and therefore sequence operations "
			"doesn't apply. This is a type checking result error."),
		_("An operation on two sequences requires that both sequences are of the same type."),
		_("The function is only for enums. This is a type checking result error."),
		_("")
	};
	guint		index = -(guint)error;

	if (index > 11)
		return NULL;
	return error_string_array[index];
}