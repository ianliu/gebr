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

#ifndef __LIBGEBR_GEOXML_FLOW_H
#define __LIBGEBR_GEOXML_FLOW_H

#include <glib.h>

/**
 * \struct GeoXmlFlow flow.h geoxml/flow.h
 * \brief
 * A sequence of programs.
 * \dot
 * digraph flow {
 * 	fontname = "Bitstream Vera Sans"
 * 	fontsize = 9
 * 	size = "6"
 * 	node [
 * 		color = palegreen2, style = filled
 * 		fontname = "Bitstream Vera Sans"
 *   fontsize = 9
 * 		shape = record
 * 	]
 * 	edge [
 * 		fontname = "Bitstream Vera Sans"
 *   fontsize = 9
 * 	]
 *
 * 	"GeoXmlDocument" [ URL = "\ref document.h" ];
 * 	"GeoXmlFlow" [ URL = "\ref flow.h" ];
 * 	"GeoXmlSequence" [ URL = "\ref sequence.h" ];
 * 	"GeoXmlProgram" [ URL = "\ref program.h" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GeoXmlDocument" -> "GeoXmlFlow";
 * 	"GeoXmlSequence" -> "GeoXmlCategory";
 * 	"GeoXmlSequence" -> "GeoXmlProgram";
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

/**
 * The GeoXmlCategory struct contains private data only, and should be accessed using the functions below.
 */
typedef struct geoxml_category GeoXmlCategory;

#include "program.h"
#include "macros.h"
#include "sequence.h"

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
 * Change the \p flow 's modified date to \p last_run
 *
 * If \p flow or \p last_run is NULL nothing is done.
 *
 * \see geoxml_document_get_date_modified
 */
void
geoxml_flow_set_date_last_run(GeoXmlFlow * flow, const gchar * last_run);

/**
 * Set the \p flow input file path to \p input. The input file
 * is used to start the flow, by loading it on the first program of \p flow.
 *
 * If \p flow or \p input is NULL nothing is done.
 *
 * \see geoxml_flow_io_get_input
 */
void
geoxml_flow_io_set_input(GeoXmlFlow * flow, const gchar * input);

/**
 * Set the \p flow output file path to \p output. The output file is
 * the one used to gather the output sent by the last program of \p flow.
 *
 * If \p flow or \p output is NULL nothing is done.
 *
 * \see geoxml_flow_io_get_output
 */
void
geoxml_flow_io_set_output(GeoXmlFlow * flow, const gchar * output);

/**
 * Set the \p flow error file path to \p error. This should be the file
 * containing the error log, which might include program's stderr
 *
 * If \p flow or \p error is NULL nothing is done.
 *
 * \see geoxml_flow_io_get_error geoxml_program_set_stderr
 */
void
geoxml_flow_io_set_error(GeoXmlFlow * flow, const gchar * error);

/**
 * Get the \p flow 's last modification date
 *
 * If \p flow is NULL returns NULL.
 *
 * \see geoxml_flow_set_date_modified
 */
const gchar *
geoxml_flow_get_date_last_run(GeoXmlFlow * flow);

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
 * Creates a new program associated and append to the list of programs
 * Provided for convenience
 *
 * \see geoxml_flow_new_program
 */
GeoXmlProgram *
geoxml_flow_append_program(GeoXmlFlow * flow);

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
geoxml_flow_get_program(GeoXmlFlow * flow, GeoXmlSequence ** program, gulong index);

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
 * Creates a new category and append it to the list of categories.
 * Provided for convenience.
 *
 * \see geoxml_flow_new_category
 */
GeoXmlCategory *
geoxml_flow_append_category(GeoXmlFlow * flow, const gchar * name);

/**
 * Writes to \p category the \p index ieth category that belongs to \p flow.
 * If an error ocurred, the content of \p category is assigned to NULL.
 *
 * Returns one of: GEOXML_RETV_SUCCESS, GEOXML_RETV_INVALID_INDEX, GEOXML_RETV_NULL_PTR
 *
 * \see geoxml_sequence_move geoxml_sequence_move_up geoxml_sequence_move_down geoxml_sequence_remove
 */
int
geoxml_flow_get_category(GeoXmlFlow * flow, GeoXmlSequence ** category, gulong index);

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
 * Use \ref geoxml_sequence_remove instead. Kept only for backwards compatible and should not be used in newly written code
 */
void GEOXML_DEPRECATED
geoxml_flow_remove_category(GeoXmlFlow * flow, GeoXmlCategory * category);



#endif //__LIBGEBR_GEOXML_FLOW_H
