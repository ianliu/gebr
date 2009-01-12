/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
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

#ifndef __LIBGEBR_GEOXML_PROGRAM_PARAMETER_P_H
#define __LIBGEBR_GEOXML_PROGRAM_PARAMETER_P_H

#include "program_parameter.h"

/**
 * \internal
 * Assign to all values (or default one according to \p default_value) the value \p value.
 * An empty value means reset.
 */
void
__geoxml_program_parameter_set_all_value(GeoXmlProgramParameter * program_parameter,
	gboolean default_value, const gchar * value);

#endif //__LIBGEBR_GEOXML_PROGRAM_PARAMETER_P_H
