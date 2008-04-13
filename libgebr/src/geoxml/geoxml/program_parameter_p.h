/*   libgebr - G�BR Library
 *   Copyright (C) 2007 G�BR core team (http://gebr.sourceforge.net)
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
 * Reseting a program parameter default is not only settting it to a empty string.
 * In fact, this will break XML, as some default are implemented as enumerated attributes.
 *
 */
void
__geoxml_program_parameter_reset_default(GeoXmlProgramParameter * parameter);

#endif //__LIBGEBR_GEOXML_PROGRAM_PARAMETER_P_H
