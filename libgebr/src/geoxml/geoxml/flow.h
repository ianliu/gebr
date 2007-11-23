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

#ifndef __LIBGEOXML_FLOW_H
#define __LIBGEOXML_FLOW_H

#include <glib.h>

/**
 * \struct GeoXmlFlow flow.h libgeoxml/flow.h
 * \brief
 * A sequence of programs.
 * \dot
 * digraph flow {
 * 	fontname = "Bitstream Vera Sans"
 * 	fontsize = 8
 * 	size = "6"
 * 	node [
 *		color = palegreen2, style = filled
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 8
 * 		shape = record
 * 	]
 *
 * 	"GeoXmlDocument" [ URL = "\ref document.h" ];
 * 	"GeoXmlFlow" [ URL = "\ref flow.h" ];
 * 	"GeoXmlProgram" [ URL = "\ref program.h" ];
 * 	"GeoXmlCategory" [ URL = "\ref category.h" ];
 *
 * 	edge [
 * 		fontname = "Bitstream Vera Sans"
 * 		fontsize = 8
 * 	]
 * 	"GeoXmlDocument" -> { "GeoXmlFlow" };
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "0..*"
 * 	]
 * 	"GeoXmlFlow" -> { "GeoXmlCategory" "GeoXmlProgram" };
 * }
 * \enddot
 * \see flow.h
 */

/**
 * \file flow.h
 * A sequence of programs.
 *
 * Many seismic processing works involves the execution a chain - or a flow -
 * of programs, where each output of one program is the input of the next.
 *
 *
 */

/**
 * Cast flow's document \p doc to GeoXmlFlow
 */
#define GEOXML_FLOW(doc) ((GeoXmlFlow*)(doc))

/**
 * The GeoXmlFlow struct contains private data only, and should be accessed using the functions below.
 */
typedef struct geoxml_flow GeoXmlFlow;

#include "program.h"
#include "category.h"
#include "macros.h"

/**
 * Create a new empty flow and return a pointer to it.
 *
 * Returns NULL if memory couldn't be allocated.
 */
GeoXmlFlow *
geoxml_flow_new();

/**
 * Add all \p flow2's programs to the end of \p flow.
 * The help of each \p flow2's program is ignored on the copy.
 *
 * If \p flow or \p flow2 is NULL nothing is done.
 */
void
geoxml_flow_add_flow(GeoXmlFlow * flow, GeoXmlFlow * flow2);

/**
 * Set the \p flow input file path to \p input. The input file
 * is used to start the flow, by loading it on the first program of \p flow.
 *
 * If \p flow is NULL nothing is done.
 *
 * \see geoxml_flow_io_get_input
 */
void
geoxml_flow_io_set_input(GeoXmlFlow * flow, const gchar * input);

/**
 * Set the \p flow output file path to \p output. The output file is
 * the one used to gather the output sent by the last program of \p flow.
 *
 * If \p flow is NULL nothing is done.
 *
 * \see geoxml_flow_io_get_output
 */
void
geoxml_flow_io_set_output(GeoXmlFlow * flow, const gchar * output);

/**
 * Set the \p flow error file path to \p error. This should be the file
 * containing the error log, which might include program's stderr
 *
 * If \p flow is NULL nothing is done.
 *
 * \see geoxml_flow_io_get_error geoxml_program_set_stderr
 */
void
geoxml_flow_io_set_error(GeoXmlFlow * flow, const gchar * error);

/**
 * Retrieves the input file path of \p flow.
 *
 * If \p flow is NULL returns NULL.
 *
 * \see geoxml_flow_io_set_input
 */
const gchar *
geoxml_flow_io_get_input(GeoXmlFlow * flow);

/**
 * Retrieves the output file path of \p flow.
 *
 * If \p flow is NULL returns NULL.
 *
 * \see geoxml_flow_io_set_output
 */
const gchar *
geoxml_flow_io_get_output(GeoXmlFlow * flow);

/**
 * Retrieves the error file path of \p flow.
 *
 * If \p flow is NULL returns NULL.
 *
 * \see geoxml_flow_io_set_error
 */
const gchar *
geoxml_flow_io_get_error(GeoXmlFlow * flow);

/**
 * Creates a new program associated with \p flow.
 * \p program is append to the list of programs.
 *
 * If \p flow is NULL nothing is done.
 *
 * \see geoxml_sequence_move geoxml_sequence_move_up geoxml_sequence_move_down geoxml_sequence_remove
 */
GeoXmlProgram *
geoxml_flow_new_program(GeoXmlFlow * flow);

/**
 * Writes to \p program the \p index ieth category that \p flow belong.
 * If an error ocurred, the content of \p program is assigned to NULL.
 *
 * If \p flow is NULL nothing is done.
 *
 * Returns one of: GEOXML_RETV_SUCCESS, GEOXML_RETV_INVALID_INDEX, GEOXML_RETV_NULL_PTR
 *
 * \see geoxml_sequence_move geoxml_sequence_move_up geoxml_sequence_move_down geoxml_sequence_remove
 */
int
geoxml_flow_get_program(GeoXmlFlow * flow, GeoXmlProgram ** program, gulong index);

/**
 * Get the number of programs \p flow has.
 *
 * If \p flow is NULL returns -1.
 */
glong
geoxml_flow_get_programs_number(GeoXmlFlow * flow);

/**
 * Creates a new category named as \p name in \p flow and returns a pointer to it.
 * \p category is appended to the list of categories.
 *
 * If \p flow is NULL nothing is done.
 *
 * \see geoxml_sequence_move geoxml_sequence_move_up geoxml_sequence_move_down geoxml_sequence_remove
 */
GeoXmlCategory *
geoxml_flow_new_category(GeoXmlFlow * flow, const gchar * name);

/**
 * Writes to \p category the \p index ieth category that \p flow belong.
 * If an error ocurred, the content of \p category is assigned to NULL.
 *
 * If \p flow is NULL nothing is done.
 *
 * Returns one of: GEOXML_RETV_SUCCESS, GEOXML_RETV_INVALID_INDEX, GEOXML_RETV_NULL_PTR
 *
 * \see geoxml_sequence_move geoxml_sequence_move_up geoxml_sequence_move_down geoxml_sequence_remove
 */
int
geoxml_flow_get_category(GeoXmlFlow * flow, GeoXmlCategory ** category, gulong index);

/**
 * Get the number of categories that \p flow has.
 *
 * If \p flow is NULL returns -1.
 */
glong
geoxml_flow_get_categories_number(GeoXmlFlow * flow);

/**
 * \deprecated
 * Use \ref geoxml_sequence_remove instead. Kept only for backwards compatible and should not be used in newly written code
 */
void GEOXML_DEPRECATED
geoxml_flow_remove_program(GeoXmlFlow * flow, GeoXmlProgram * program);

/**
 * \deprecated
 * Use \ref geoxml_sequence_move instead. Kept only for backwards compatible and should not be used in newly written code
 */
void GEOXML_DEPRECATED
geoxml_flow_move_program(GeoXmlFlow * flow, GeoXmlProgram * program, GeoXmlProgram * before_program);

/**
 * \deprecated
 * Use \ref geoxml_sequence_move_up instead. Kept only for backwards compatible and should not be used in newly written code
 */
int GEOXML_DEPRECATED
geoxml_flow_move_program_up(GeoXmlFlow * flow, GeoXmlProgram * program);

/**
 * \deprecated
 * Use \ref geoxml_sequence_move_down instead. Kept only for backwards compatible and should not be used in newly written code
 */
int GEOXML_DEPRECATED
geoxml_flow_move_program_down(GeoXmlFlow * flow, GeoXmlProgram * program);

/**
 * \deprecated
 * Renamed to \ref geoxml_flow_new_category . Kept only for backwards compatible and should not be used in newly written code
 */
GeoXmlCategory * GEOXML_DEPRECATED
geoxml_flow_append_category(GeoXmlFlow * flow, const gchar * name);

/**
 * \deprecated
 * Use \ref geoxml_sequence_remove instead. Kept only for backwards compatible and should not be used in newly written code
 */
void GEOXML_DEPRECATED
geoxml_flow_remove_category(GeoXmlFlow * flow, GeoXmlCategory * category);



#endif //__LIBGEOXML_FLOW_H
