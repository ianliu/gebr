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

#ifndef __GEBR_GEOXML_FLOW_H
#define __GEBR_GEOXML_FLOW_H

#include <glib.h>

/**
 * \struct GebrGeoXmlFlow flow.h geoxml/flow.h
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
 * 	"GebrGeoXmlDocument" [ URL = "\ref document.h" ];
 * 	"GebrGeoXmlFlow" [ URL = "\ref flow.h" ];
 * 	"GebrGeoXmlSequence" [ URL = "\ref sequence.h" ];
 * 	"GebrGeoXmlProgram" [ URL = "\ref program.h" ];
 *
 * 	edge [
 * 		arrowhead = "normal"
 * 	]
 * 	"GebrGeoXmlDocument" -> "GebrGeoXmlFlow";
 * 	"GebrGeoXmlSequence" -> "GebrGeoXmlCategory";
 * 	"GebrGeoXmlSequence" -> "GebrGeoXmlProgram";
 *
 * 	edge [
 * 		arrowhead = "none"
 * 		taillabel = "0..*"
 * 	]
 * 	"GebrGeoXmlFlow" -> { "GebrGeoXmlCategory" "GebrGeoXmlProgram" };
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
 * Cast flow's document \p doc to GebrGeoXmlFlow
 */
#define GEBR_GEOXML_FLOW(doc) ((GebrGeoXmlFlow*)(doc))

/**
 * Cast from GebrGeoXmlSequence at \p seq to GebrGeoXmlRevision
 */
#define GEBR_GEOXML_REVISION(seq) ((GebrGeoXmlRevision*)(seq))

/**
 * Cast from GebrGeoXmlSequence at \p seq to GebrGeoXmlFlowServer
 */
#define GEBR_GEOXML_FLOW_SERVER(seq) ((GebrGeoXmlFlowServer*)(seq))

/**
 * The GebrGeoXmlFlow struct contains private data only, and should be accessed using the functions below.
 */
typedef struct gebr_geoxml_flow GebrGeoXmlFlow;

/**
 * The GebrGeoXmlCategory struct contains private data only, and should be accessed using the functions below.
 */
typedef struct gebr_geoxml_category GebrGeoXmlCategory;

/**
 * The GebrGeoXmlRevision struct contains private data only, and should be accessed using the functions below.
 */
typedef struct gebr_geoxml_revision GebrGeoXmlRevision;

/**
 * The GebrGeoXmlFlowServer struct contains private data only, and should be accessed using the functions below.
 */
typedef struct gebr_geoxml_flow_server GebrGeoXmlFlowServer;

#include "program.h"
#include "macros.h"
#include "sequence.h"
#include "object.h"

/**
 * Create a new empty flow and return a pointer to it.
 *
 * Returns NULL if memory couldn't be allocated.
 */
GebrGeoXmlFlow *gebr_geoxml_flow_new();

/**
 * Add all \p flow2's programs to the end of \p flow.
 * The help of each \p flow2's program is ignored on the copy.
 *
 * If \p flow or \p flow2 is NULL nothing is done.
 */
void gebr_geoxml_flow_add_flow(GebrGeoXmlFlow * flow, GebrGeoXmlFlow * flow2);

/**
 * Call \ref gebr_geoxml_program_foreach_parameter for each program of \p flow
 *
 * If \p flow is NULL nothing is done.
 */
void gebr_geoxml_flow_foreach_parameter(GebrGeoXmlFlow * flow, GebrGeoXmlCallback callback, gpointer user_data);

/**
 * Change the \p flow 's modified date to \p last_run
 *
 * If \p flow or \p last_run is NULL nothing is done.
 *
 * \see gebr_geoxml_document_get_date_modified
 */
void gebr_geoxml_flow_set_date_last_run(GebrGeoXmlFlow * flow, const gchar * last_run);

/**
 * Get the \p flow 's last modification date
 *
 * If \p flow is NULL returns NULL.
 *
 * \see gebr_geoxml_flow_set_date_modified
 */
const gchar *gebr_geoxml_flow_get_date_last_run(GebrGeoXmlFlow * flow);

/**
 * Creates a new server as a child of servers tag.
 *
 * \see gebr_geoxml_flow_new_server
 */
GebrGeoXmlFlowServer *gebr_geoxml_flow_append_server(GebrGeoXmlFlow * flow);

/**
 * Writes into server the ieth server from 'servers' tag.
 * If an error ocurred, the content of server is assigned to NULL.
 * If flow is NULL nothing is done.
 * Returns one of: GEBR_GEOXML_RETV_SUCCESS, GEBR_GEOXML_RETV_INVALID_INDEX, GEBR_GEOXML_RETV_NULL_PTR
 */
int gebr_geoxml_flow_get_server(GebrGeoXmlFlow * flow, GebrGeoXmlSequence ** server, gulong index);

/**
 * Sets the address of this flow server.
 */
void gebr_geoxml_flow_server_set_address(GebrGeoXmlFlowServer * server, const gchar * address);

/**
 * Returns the address of this flow server.
 * The string must be freed with g_free() .
 */
const gchar *gebr_geoxml_flow_server_get_address(GebrGeoXmlFlowServer * server);

/**
 * Gets the last ran server.
 */
GebrGeoXmlFlowServer *gebr_geoxml_flow_servers_get_last_run(GebrGeoXmlFlow * flow);

/**
 * Returns the GebrGeoXmlFlowServer that has \p address
 * \p input, \p output and/or \p error may be NULL which causes them to be ignored
 * If not found returns NULL.
 *
 * If \p flow or \p address is NULL returns NULL.
 */
GebrGeoXmlFlowServer *gebr_geoxml_flow_servers_query(GebrGeoXmlFlow * flow, const gchar * address,
						     const gchar * input, const gchar * output, const gchar * error);

/**
 * Get the number of servers \p flow has.
 *
 * If \p flow is NULL returns -1.
 */
glong gebr_geoxml_flow_get_servers_number(GebrGeoXmlFlow * flow);

/**
 * Set the flow \p server input file path to \p input.
 *
 * If \p server or \p input is NULL nothing is done.
 *
 * \see gebr_geoxml_flow_server_io_get_input gebr_geoxml_flow_io_get_input
 */
void gebr_geoxml_flow_server_io_set_input(GebrGeoXmlFlowServer * server, const gchar * input);

/**
 * Set the flow \p server output file path to \p output.
 *
 * If \p server or \p output is NULL nothing is done.
 *
 * \see gebr_geoxml_flow_server_io_get_output gebr_geoxml_flow_io_get_output
 */
void gebr_geoxml_flow_server_io_set_output(GebrGeoXmlFlowServer * server, const gchar * output);

/**
 * Set the \p server error file path to \p error.
 *
 * If \p server or \p error is NULL nothing is done.
 *
 * \see gebr_geoxml_flow_server_io_get_error gebr_geoxml_flow_io_get_error gebr_geoxml_program_set_stderr
 */
void gebr_geoxml_flow_server_io_set_error(GebrGeoXmlFlowServer * server, const gchar * error);

/**
 * Retrieves the input file path of \p server.
 *
 * If \p server is NULL returns NULL.
 *
 * \see gebr_geoxml_flow_io_set_input
 */
const gchar *gebr_geoxml_flow_server_io_get_input(GebrGeoXmlFlowServer * server);

/**
 * Retrieves the output file path of \p server.
 *
 * If \p server is NULL returns NULL.
 *
 * \see gebr_geoxml_flow_io_set_output
 */
const gchar *gebr_geoxml_flow_server_io_get_output(GebrGeoXmlFlowServer * server);

/**
 * Retrieves the error file path of \p server.
 *
 * If \p server is NULL returns NULL.
 *
 * \see gebr_geoxml_flow_server_io_set_error
 */
const gchar *gebr_geoxml_flow_server_io_get_error(GebrGeoXmlFlowServer * server);

/**
 * Sets the last run date for this server.
 *
 * \see gebr_geoxml_flow_server_get_date_last_run
 */
void gebr_geoxml_flow_server_set_date_last_run(GebrGeoXmlFlowServer * server, const gchar * date);

/**
 * Retrieve the last run date for this server.
 *
 * \see gebr_geoxml_flow_server_get_date_last_run
 */
const gchar *gebr_geoxml_flow_server_get_date_last_run(GebrGeoXmlFlowServer * server);

/**
 * Retrieve from the list of servers the server which
 * has the newest the last run date.
 */
GebrGeoXmlFlowServer *gebr_geoxml_flow_server_get_last_runned_server(GebrGeoXmlFlow * flow);

/**
 * Sets the flow io tag from the corresponding server tag.
 * This is done by copying all values from the server io tag
 * to the flow io.
 */
void gebr_geoxml_flow_io_set_from_server(GebrGeoXmlFlow * flow, GebrGeoXmlFlowServer * server);

/**
 * Retrieves the \p server input file path*
 *
 * If \p server or \p input is NULL nothing is done.
 *
 * \see gebr_geoxml_flow_server_io_get_input
 */
void gebr_geoxml_flow_io_set_input(GebrGeoXmlFlow * flow, const gchar * input);

/**
 * Set the \p flow output file path to \p output. The output file is
 * the one used to gather the output sent by the last program of \p flow.
 *
 * If \p flow or \p output is NULL nothing is done.
 *
 * \see gebr_geoxml_flow_io_get_output
 */
void gebr_geoxml_flow_io_set_output(GebrGeoXmlFlow * flow, const gchar * output);

/**
 * Set the \p flow error file path to \p error. This should be the file
 * containing the error log, which might include program's stderr
 *
 * If \p flow or \p error is NULL nothing is done.
 *
 * \see gebr_geoxml_flow_io_get_error gebr_geoxml_program_set_stderr
 */
void gebr_geoxml_flow_io_set_error(GebrGeoXmlFlow * flow, const gchar * error);

/**
 * Retrieves the input file path of \p flow.
 *
 * If \p flow is NULL returns NULL.
 *
 * \see gebr_geoxml_flow_io_set_input
 */
const gchar *gebr_geoxml_flow_io_get_input(GebrGeoXmlFlow * flow);

/**
 * Retrieves the output file path of \p flow.
 *
 * If \p flow is NULL returns NULL.
 *
 * \see gebr_geoxml_flow_io_set_output
 */
const gchar *gebr_geoxml_flow_io_get_output(GebrGeoXmlFlow * flow);

/**
 * Retrieves the error file path of \p flow.
 *
 * If \p flow is NULL returns NULL.
 *
 * \see gebr_geoxml_flow_io_set_error
 */
const gchar *gebr_geoxml_flow_io_get_error(GebrGeoXmlFlow * flow);

/**
 * Creates a new program associated and append to the list of programs
 * Provided for convenience
 *
 * \see gebr_geoxml_flow_new_program
 */
GebrGeoXmlProgram *gebr_geoxml_flow_append_program(GebrGeoXmlFlow * flow);

/**
 * Writes to \p program the \p index ieth category that \p flow belong.
 * If an error ocurred, the content of \p program is assigned to NULL.
 *
 * If \p flow is NULL nothing is done.
 *
 * Returns one of: GEBR_GEOXML_RETV_SUCCESS, GEBR_GEOXML_RETV_INVALID_INDEX, GEBR_GEOXML_RETV_NULL_PTR
 *
 * \see gebr_geoxml_sequence_move gebr_geoxml_sequence_move_up gebr_geoxml_sequence_move_down gebr_geoxml_sequence_remove
 */
int gebr_geoxml_flow_get_program(GebrGeoXmlFlow * flow, GebrGeoXmlSequence ** program, gulong index);

/**
 * Get the number of programs \p flow has.
 *
 * If \p flow is NULL returns -1.
 */
glong gebr_geoxml_flow_get_programs_number(GebrGeoXmlFlow * flow);

/**
 * Return the first program using a mpi implementation at \p flow.
 * If there is no mpi program, return NULL.
 */
GebrGeoXmlProgram *gebr_geoxml_flow_get_first_mpi_program(GebrGeoXmlFlow * flow);

/**
 * Creates a new category and append it to the list of categories.
 * Provided for convenience.
 *
 * \see gebr_geoxml_flow_new_category
 */
GebrGeoXmlCategory *gebr_geoxml_flow_append_category(GebrGeoXmlFlow * flow, const gchar * name);

/**
 * Writes to \p category the \p index ieth category that belongs to \p flow.
 * If an error ocurred, the content of \p category is assigned to NULL.
 *
 * Returns one of: GEBR_GEOXML_RETV_SUCCESS, GEBR_GEOXML_RETV_INVALID_INDEX, GEBR_GEOXML_RETV_NULL_PTR
 *
 * \see gebr_geoxml_sequence_move gebr_geoxml_sequence_move_up gebr_geoxml_sequence_move_down gebr_geoxml_sequence_remove
 */
int gebr_geoxml_flow_get_category(GebrGeoXmlFlow * flow, GebrGeoXmlSequence ** category, gulong index);

/**
 * Get the number of categories that \p flow has.
 *
 * If \p flow is NULL returns -1.
 */
glong gebr_geoxml_flow_get_categories_number(GebrGeoXmlFlow * flow);

/**
 * Change all the flow data to the one stored at revision, except, of course,
 * the list of revisions.
 * Be aware that all \p flow data will be lost. If you don't want that, call
 * \ref gebr_geoxml_flow_append_revision before.
 *
 * If \p flow or \p revision is NULL nothing is done.
 * If it fails because the revision could not be loaded, returns FALSE.
 * On success, return TRUE.
 */
gboolean gebr_geoxml_flow_change_to_revision(GebrGeoXmlFlow * flow, GebrGeoXmlRevision * revision);

/**
 * Creates a new revision with the current time append to the list of revisions
 * An revision is a way to keep the history of the flow changes. You can then restore
 * one revision with \ref gebr_geoxml_flow_change_to_revision
 *
 * If \p flow is NULL nothing is done.
 */
GebrGeoXmlRevision *gebr_geoxml_flow_append_revision(GebrGeoXmlFlow * flow, const gchar * comment);

/**
 * Writes to \p revision the \p index ieth revision that \p flow has.
 * If an error ocurred, the content of \p revision is assigned to NULL.
 *
 * If \p flow is NULL nothing is done.
 *
 * Returns one of: GEBR_GEOXML_RETV_SUCCESS, GEBR_GEOXML_RETV_INVALID_INDEX, GEBR_GEOXML_RETV_NULL_PTR
 *
 * \see gebr_geoxml_sequence_move gebr_geoxml_sequence_move_up gebr_geoxml_sequence_move_down gebr_geoxml_sequence_remove
 */
int gebr_geoxml_flow_get_revision(GebrGeoXmlFlow * flow, GebrGeoXmlSequence ** revision, gulong index);

/**
 * Get information of \p revision. The flow is stored at \p flow and can be
 * loaded with gebr_geoxml_document_load_buffer. \p receive the date of creation of \p revision.
 * A NULL value of \p flow or \p date or \p comment mean not set.
 * Any of the string should be freed.
 *
 * If \p revision in NULL nothing is done.
 */
void gebr_geoxml_flow_get_revision_data(GebrGeoXmlRevision * revision, gchar ** flow, gchar ** date, gchar ** comment);

/**
 * Get the number of revisions \p flow has.
 *
 * If \p flow is NULL returns -1.
 */
glong gebr_geoxml_flow_get_revisions_number(GebrGeoXmlFlow * flow);

#endif				//__GEBR_GEOXML_FLOW_H
