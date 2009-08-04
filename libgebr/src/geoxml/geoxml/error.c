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

#include "../../intl.h"

#include "error.h"

const gchar *
geoxml_error_string(enum GEOXML_RETV error)
{
	static const gchar * 	error_string_array [] = {
		N_("Success."),
		N_("Null pointer."),
		N_("Not enough memory."),
		N_("Can't find file or permission denied."),
		N_("Invalid element index."),
		N_("Invalid document syntax or structure."),
		N_("DTD specified in XML."),
		N_("Could not find or read DTD."),
		N_("Not a sequence."),
		N_("An operation on two sequences requires that both sequences are of the same type."),
		N_("Not an enum."),
		N_("Reference to itlself"),
		N_("Not first instance of the group"),
	};
	guint			index = -(guint)error;

	if (index > 11)
		return NULL;
	return error_string_array[index];
}

const gchar *
geoxml_error_explained_string(enum GEOXML_RETV error)
{
	static const gchar * 	error_string_array [] = {
		N_("The operation was done successfuly."),
		N_("One or more mandatory function arguments were null."),
		N_("Not enough memory. "
			"The library stoped after an unsucessful memory allocation."),
		N_("Can't access file. The file doesn't exist or there is not enough "
			"permission to read it."),
		N_("This element index does not exist in the sequence."),
		N_("Invalid XML syntax or it does not respect the structure "
			"defined on its corresponding document DTD."),
		N_("DTD was specified for the document. LibGeBR-GeoXml requires the document "
			"not to specify the DTD as it automatically find it and validate against."),
		N_("DTD for the corresponding document type (flow, line or project) and version "
			"could not be found or accessed on the system.\n"
			"You should verify if your installation is correct and if it supports "
			"this document version (you might need to update your software)."),
		N_("The argument(s) passed are not inside a sequence and therefore sequence operations "
			"doesn't apply. This is a type checking result error."),
		N_("An operation on two sequences requires that both sequences are of the same type."),
		N_("The function is only for enums. This is a type checking result error."),
		N_("The reference parameter references itself"),
		N_("The operation only applies to the first instance of the group")
	};
	guint			index = -(guint)error;

	if (index > 11)
		return NULL;
	return error_string_array[index];
}