/*   libgeoxml - An interface to describe seismic software in XML
 *   Copyright (C) 2007  Bráulio Barros de Oliveira (brauliobo@gmail.com)
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

#ifndef __GEOXML_PROGRAM_P_H
#define __GEOXML_PROGRAM_P_H

#include <gdome.h>

#include "program.h"
#include "program_parameter.h"

/**
 * \internal
 * Used by geoxml_program_new_parameter and geoxml_program_parameter_set_type
 */
GeoXmlProgramParameter *
__geoxml_program_new_parameter(GeoXmlProgram * program, GdomeElement * before, enum GEOXML_PARAMETERTYPE parameter_type);

#endif //__GEOXML_PROGRAM_P_H
