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

#ifndef __GEBR_GEOXML_PROGRAM_P_H
#define __GEBR_GEOXML_PROGRAM_P_H

#include <gdome.h>
#include <glib.h>

#include "program.h"
#include "program-parameter.h"

G_BEGIN_DECLS

typedef struct {
	gchar *flavor;
	gchar *n_processes;
} MpiKeywords;

/**
 * \internal
 * Used by gebr_geoxml_program_new_parameter and gebr_geoxml_program_parameter_set_type
 */
GebrGeoXmlProgramParameter *__gebr_geoxml_program_new_parameter(GebrGeoXmlProgram * program, GdomeElement * before,
								GebrGeoXmlParameterType parameter_type);

void gebr_geoxml_program_mpi_add_tags(GebrGeoXmlProgram *self,
				      const gchar *mpi);

void gebr_geoxml_program_mpi_remove_tags(GebrGeoXmlProgram *self);

MpiKeywords *get_mpi_keywords_for_flavor(const gchar *flavor);

G_END_DECLS
#endif				//__GEBR_GEOXML_PROGRAM_P_H
