/*   libgebr - GÍBR Library
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

#ifndef __LIBGEBR_GEOXML_H
#define __LIBGEBR_GEOXML_H

/**
 * \mainpage
 * \copydoc geoxml.h
 */

/**
 * \file geoxml.h
 * libgeoxml intends to be an abstraction way to specify the
 * the way programs understand seismic processing programs
 * as SU (Seismic Unix) and Madagascar
 * \dot
 * digraph geoxml {
 * 	fontname = "Bitstream Vera Sans"
 * 	fontsize = 8
 * 	size = "6"
 * 	node [
 * 	color = palegreen2, style = filled
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 8
 * 		shape = record
 * 	]
 * 	edge [
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 8
 * 	]
 *
 * 	"GeoXmlDocument" [ URL = "\ref document.h" ];
 * 	"GeoXmlProject" [ URL = "\ref project.h" ];
 * 	"GeoXmlProjectLine" [ URL = "\ref GeoXmlProjectLine" ];
 * 	"GeoXmlLine" [ URL = "\ref line.h" ];
 * 	"GeoXmlLineFlow" [ URL = "\ref GeoXmlLineFlow" ];
 * 	"GeoXmlFlow" [ URL = "\ref flow.h" ];
 * 	"GeoXmlProgram" [ URL = "\ref program.h" ];
 * 	"GeoXmlParameters" [ URL = "\ref parameters.h" ];
 * 	"GeoXmlParameter" [ URL = "\ref parameter.h" ];
 * 	"GeoXmlProgramParameter" [ URL = "\ref program_parameter.h" ];
 * 	"GeoXmlParameterGroup" [ URL = "\ref parameter_group.h" ];
 * 	"GeoXmlCategory" [ URL = "\ref category.h" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GeoXmlDocument" -> { "GeoXmlFlow" "GeoXmlLine" "GeoXmlProject" };
 * 	"GeoXmlParameter" -> "GeoXmlProgramParameter";
 * 	"GeoXmlParameter" -> "GeoXmlParameterGroup";
 * 	"GeoXmlParameters" -> "GeoXmlParameterGroup";
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "0..*"
 * 	]
 * 	"GeoXmlFlow" -> { "GeoXmlCategory" "GeoXmlProgram" };
 * 	"GeoXmlLine" -> "GeoXmlLineFlow";
 * 	"GeoXmlProject" -> "GeoXmlProjectLine";
 * 	"GeoXmlParameters" -> "GeoXmlParameter";
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "1"
 * 	]
 * 	"GeoXmlProgram" -> "GeoXmlParameters";
 * }
 * \enddot
 * \dot
 * digraph sequence {
 * 	fontname = "Bitstream Vera Sans"
 * 	fontsize = 8
 * 	size = "6"
 * 	node [
 * 		color = palegreen2, style = filled
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 8
 * 		shape = record
 * 	]
 * 	edge [
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 8
 * 	]
 *
 * 	"GeoXmlSequence" [ URL = "\ref sequence.h" ];
 * 	"GeoXmlProjectLine" [ URL = "\ref GeoXmlProjectLine" ];
 * 	"GeoXmlLineFlow" [ URL = "\ref GeoXmlLineFlow" ];
 * 	"GeoXmlProgram" [ URL = "\ref program.h" ];
 * 	"GeoXmlParameter" [ URL = "\ref parameter.h" ];
 * 	"GeoXmlCategory" [ URL = "\ref category.h" ];
 * 	"GeoXmlValueSequence" [ URL = "\ref value_sequence.h" ];
 * 	"GeoXmlEnumOption" [ URL = "\ref GeoXmlEnumOption" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GeoXmlSequence" -> { "GeoXmlProjectLine" };
 * 	"GeoXmlSequence" -> { "GeoXmlLineFlow" };
 * 	"GeoXmlSequence" -> { "GeoXmlCategory" };
 * 	"GeoXmlSequence" -> { "GeoXmlProgram" };
 * 	"GeoXmlSequence" -> { "GeoXmlParameter" };
 * 	"GeoXmlSequence" -> { "GeoXmlValueSequence" };
 * 	"GeoXmlValueSequence" -> { "GeoXmlEnumOption" };
 * }
 * \enddot
 * Discussions group: http://groups.google.com/group/gebr and http://groups.google.com/group/gebr-devel
 * \author
 * Br·ulio Barros de Oliveira (brauliobo@gmail.com)
 */

/* For more information, go to libgebr-geoxml's documentation at
 * http://gebr.sf.net/doc/libgebr/geoxml
 */

/* include all geoxml library's headers. */
#include <geoxml/error.h>
#include <geoxml/macros.h>
#include <geoxml/document.h>
#include <geoxml/project.h>
#include <geoxml/line.h>
#include <geoxml/flow.h>
#include <geoxml/sequence.h>
#include <geoxml/value_sequence.h>
#include <geoxml/category.h>
#include <geoxml/program.h>
#include <geoxml/parameters.h>
#include <geoxml/parameter.h>
#include <geoxml/program_parameter.h>
#include <geoxml/parameter_group.h>

#endif //__LIBGEBR_GEOXML_H
