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

#ifndef __LIBGEBR_GEOXML_ERROR_H
#define __LIBGEBR_GEOXML_ERROR_H

#include <glib.h>

/**
 * \file error.h
 * Error values used by libgeoxml
 *
 * In geoxml was adopted an intuitive error handling mechanism.
 *
 * Simple functions doesn't require error codes so they only check
 * nullity of pointers arguments; if one
 * of them are equal to NULL a NULL pointer return if the function
 * returns a pointer or the function does nothing if the function doesn't return a value.
 *
 * Complex functions returns an integer value, which can be some of the
 * values (depending on it) of this enumeration. * Negatives values means
 * an error ocurred. Zero (GEOXML_RETV_SUCCESS) means the operation succed
 * on its execution. In this case the argument passed by reference is assigned to its properly value.
 */

/**
 * Enumeration for possible return values of complex functions.
 */
enum GEOXML_RETV {
	GEOXML_RETV_SUCCESS			= 0,

	/**
	 * One of the arguments is a null pointer.
	 *
	 * Returned by all complex functions.
	 */
	GEOXML_RETV_NULL_PTR			= -1,
	/**
	 * An memory allocation operation could not be made.
	 *
	 * Returned by complex functions that requires modification.
	 */
	GEOXML_RETV_NO_MEMORY			= -2,
	/**
	 * A file could not be read neither because it
	 * does not exist or because there is no enough permission
	 * to access (read or write depending on the function).
	 *
	 * Returned by 'geoxml_flow_load', 'geoxml_flow_validate' and 'geoxml_flow_save'.
	 */
	GEOXML_RETV_CANT_ACCESS_FILE		= -3,
	/**
	 * The index is equal or greater than the list.
	 */
	GEOXML_RETV_INVALID_INDEX		= -4,
	/**
	 * The parsed flow is invalid acording to its DTD.
	 */
	GEOXML_RETV_INVALID_DOCUMENT		= -5,
	/**
	 * libgeoxml documents must not specify DTDs, as it uses the attribute version of root element to find the appropriate the DTD. DTDs are kept in /usr/share/libgeoxml (specifically at ${datarootdir}/libgeoxml, run '--configure --help' for more information).
	 */
	GEOXML_RETV_DTD_SPECIFIED		= -6,
	/**
	 * Can't read DTD equivalent to document's version in libgeoxml data folder (${datarootdir}/libgeoxml, run '--configure --help' for more information). Verify if it exists or if you read permissions for it.
	 */
	GEOXML_RETV_CANT_ACCESS_DTD		= -7,
 	/**
	 * The function parameter is not a sequence.
	 */
	GEOXML_RETV_NOT_A_SEQUENCE		= -8,
	/**
	 * The user tried to mix two elements of different sequences.
	 */
	GEOXML_RETV_DIFFERENT_SEQUENCES		= -9,
	/**
	 * The parameter passed is not a enum parameter.
	 */
	GEOXML_RETV_PARAMETER_NOT_ENUM		= -10,
	/**
	 * The action could not be done because group
	 * container of the parameter was instatiated.
	 */
	GEOXML_RETV_MORE_THAN_ONE_INSTANCES	= -11,
};

/**
 * Returns an one line string corresponding to \p error
 * \see geoxml_error_explained_string
 */
const gchar *
geoxml_error_string(enum GEOXML_RETV error);

/**
 * Returns a string corresponding to \p error. The text
 * explain with details \p error, and can be multilined.
 * \see geoxml_error_string
 */
const gchar *
geoxml_error_explained_string(enum GEOXML_RETV error);

#endif //__LIBGEBR_GEOXML_ERROR_H
