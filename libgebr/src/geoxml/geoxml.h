/*   libgeoxml - An interface to describe seismic software in XML
 *   Copyright (C) 2007  Br√°ulio Barros de Oliveira (brauliobo@gmail.com)
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

#ifndef __LIBGEOXML_H
#define __LIBGEOXML_H

/**
 * \mainpage
 * \copydoc libgeoxml.h
 */

/**
 * \file libgeoxml.h
 * libgeoxml intends to be an abstraction way to specify the
 * the way programs understand seismic processing programs
 * as SU (Seismic Unix) and Madagascar
 * \dot
 * digraph libgeoxml {
 * 	fontname = "Bitstream Vera Sans"
 * 	fontsize = 8
 * 	size = "6"
 * 	node [
 * 	color = palegreen2, style = filled
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 8
 * 		shape = recor
 * 	]
 *
 * 	"GeoXmlDocument" [ URL = "\ref document.h" ];
 * 	"GeoXmlFlow" [ URL = "\ref flow.h" ];
 * 	"GeoXmlLine" [ URL = "\ref line.h" ];
 * 	"GeoXmlProject" [ URL = "\ref project.h" ];
 * 	"GeoXmlProgram" [ URL = "\ref program.h" ];
 * 	"GeoXmlProgramParameter" [ URL = "\ref program_parameter.h" ];
 * 	"GeoXmlCategory" [ URL = "\ref category.h" ];
 *
 * 	edge [
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 8
 * 	]
 * 	"GeoXmlDocument" -> { "GeoXmlFlow" "GeoXmlLine" "GeoXmlProject" };
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "0..*"
 * 	]
 * 	"GeoXmlFlow" -> { "GeoXmlCategory" "GeoXmlProgram" };
 * 	"GeoXmlProgram" -> "GeoXmlProgramParameter";
 * 	"GeoXmlLine" -> "GeoXmlLineFlow";
 * 	"GeoXmlProject" -> "GeoXmlProjectLine";
 * }
 * \enddot
 * \dot
 * digraph program {
 * 	fontname = "Bitstream Vera Sans"
 * 	fontsize = 8
 * 	size = "6"
 * 	node [
 * 		color = palegreen2, style = filled
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 8
 * 		shape = record
 * 	]
 *
 * 	"GeoXmlSequence" [ URL = "\ref sequence.h" ];
 * 	"GeoXmlProjectLine" [ URL = "\ref project.h" ];
 * 	"GeoXmlLineFlow" [ URL = "\ref line.h" ];
 * 	"GeoXmlProgram" [ URL = "\ref program.h" ];
 * 	"GeoXmlProgramParameter" [ URL = "\ref program_parameter.h" ];
 * 	"GeoXmlCategory" [ URL = "\ref category.h" ];
 *
 * 	edge [
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 8
 * 	]
 * 	"GeoXmlSequence" -> { "GeoXmlProjectLine" };
 * 	"GeoXmlSequence" -> { "GeoXmlLineFlow" };
 * 	"GeoXmlSequence" -> { "GeoXmlProgram" };
 * 	"GeoXmlSequence" -> { "GeoXmlProgramParameter" };
 * 	"GeoXmlSequence" -> { "GeoXmlCategory" };
 * }
 * \enddot
 * Discussions group: http://groups.google.com/group/gebr and http://groups.google.com/group/gebr-devel
 * \author
 * Br·ulio Barros de Oliveira (brauliobo@gmail.com)
 */

/* For more information, go to libgeoxml's documentation at
 * http://gebr.sf.net/libgeoxml/doc
 */

/* include all geoxml library's headers. */
#include <geoxml/error.h>
#include <geoxml/macros.h>
#include <geoxml/sequence.h>
#include <geoxml/document.h>
#include <geoxml/project.h>
#include <geoxml/line.h>
#include <geoxml/flow.h>
#include <geoxml/category.h>
#include <geoxml/program.h>
#include <geoxml/parameters.h>
#include <geoxml/parameter.h>
#include <geoxml/program_parameter.h>
#include <geoxml/parameter_group.h>

#endif //__LIBGEOXML_H
