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

#ifndef __GEBR_GEOXML_ERROR_H
#define __GEBR_GEOXML_ERROR_H

#include <glib.h>

G_BEGIN_DECLS

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
 * an error ocurred. Zero (GEBR_GEOXML_RETV_SUCCESS) means the operation succed
 * on its execution. In this case the argument passed by reference is assigned to its properly value.
 */

/**
 * Enumeration for possible return values of complex functions.
 */
enum GEBR_GEOXML_RETV {

	GEBR_GEOXML_RETV_SUCCESS = 0,

	/**
	 * One of the arguments is a null pointer.
	 *
	 * Returned by all complex functions.
	 */
	GEBR_GEOXML_RETV_NULL_PTR = -1,

	/**
	 * An memory allocation operation could not be made.
	 *
	 * Returned by complex functions that requires modification.
	 */
	GEBR_GEOXML_RETV_NO_MEMORY = -2,

	/**
	 * A file could not be read on document load because it does not exist. 
	 * Returned by 'gebr_geoxml_flow_load' and 'gebr_geoxml_flow_validate'
	 */
	GEBR_GEOXML_RETV_FILE_NOT_FOUND = -3,

	/**
	 * A file could not be read or written because it there is no enough permission to access (read or write
	 * depending on the function).
	 * Returned by 'gebr_geoxml_flow_load', 'gebr_geoxml_flow_validate' and 'gebr_geoxml_flow_save'.
	 */
	GEBR_GEOXML_RETV_PERMISSION_DENIED = -4,

	/**
	 * The index is equal or greater than the list.
	 */
	GEBR_GEOXML_RETV_INVALID_INDEX = -5,

	/**
	 * The parsed flow is invalid acording to its DTD.
	 */
	GEBR_GEOXML_RETV_INVALID_DOCUMENT = -6,

	/**
	 * libgeoxml documents must not specify DTDs, as it uses the attribute version of root element to find the
	 * appropriate the DTD. DTDs are kept in /usr/share/libgeoxml (specifically at ${datarootdir}/libgeoxml, run
	 * '--configure --help' for more information).
	 */
	GEBR_GEOXML_RETV_DTD_SPECIFIED = -7,

	/**
	 * Can't read DTD equivalent to document's version in libgeoxml data folder (${datarootdir}/libgeoxml, run
	 * '--configure --help' for more information). Verify if it exists or if you read permissions for it.
	 */
	GEBR_GEOXML_RETV_CANT_ACCESS_DTD = -8,

	/**
	 * The function parameter is not a sequence.
	 */
	GEBR_GEOXML_RETV_NOT_A_SEQUENCE = -9,

	/**
	 * The user tried to mix two elements of different sequences.
	 */
	GEBR_GEOXML_RETV_DIFFERENT_SEQUENCES = -10,

	/**
	 * The parameter passed is not a enum parameter.
	 */
	GEBR_GEOXML_RETV_PARAMETER_NOT_ENUM = -11,

	/**
	 * The user tried to create a parameter that reference to itself.
	 */
	GEBR_GEOXML_RETV_REFERENCE_TO_ITSELF = -12,

	/**
	 * The operation could not be made because it is not the
	 * first instance of a group, aka the master instance.
	 */
	GEBR_GEOXML_RETV_NOT_MASTER_INSTANCE = -13,

	/**
	 * The version of the loaded document is newer than GêBR.
	 */
	GEBR_GEOXML_RETV_NEWER_VERSION = -14,

	/**
	 * Number of return flags.
	 */
	GEBR_GEOXML_RETV_LENGTH = 15,
};

/**
 * Returns an one line string corresponding to \p error
 * \see gebr_geoxml_error_explained_string
 */
const gchar *gebr_geoxml_error_string(enum GEBR_GEOXML_RETV error);

/**
 * Returns a string corresponding to \p error. The text
 * explain with details \p error, and can be multilined.
 * \see gebr_geoxml_error_string
 */
const gchar *gebr_geoxml_error_explained_string(enum GEBR_GEOXML_RETV error);

G_END_DECLS
#endif				//__GEBR_GEOXML_ERROR_H
