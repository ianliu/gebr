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

G_BEGIN_DECLS

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
 * The GebrGeoXmlFlow struct contains private data only, and should be accessed using the functions below.
 */
typedef struct gebr_geoxml_flow GebrGeoXmlFlow;


/**
 *
 */
typedef enum {
	GEBR_GEOXML_FLOW_ERROR_NONE = 0,
	GEBR_GEOXML_FLOW_ERROR_NO_INPUT,
	GEBR_GEOXML_FLOW_ERROR_NO_OUTPUT,
	GEBR_GEOXML_FLOW_ERROR_NO_INFILE,
	GEBR_GEOXML_FLOW_ERROR_NO_VALID_PROGRAMS,
	GEBR_GEOXML_FLOW_ERROR_INVALID_INPUT,
	GEBR_GEOXML_FLOW_ERROR_LOOP_ONLY
} GebrGeoXmlFlowError;


/**
 * The GebrGeoXmlCategory struct contains private data only, and should be accessed using the functions below.
 */
typedef struct gebr_geoxml_category GebrGeoXmlCategory;

/**
 * The GebrGeoXmlRevision struct contains private data only, and should be accessed using the functions below.
 */
typedef struct gebr_geoxml_revision GebrGeoXmlRevision;

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
 * Sets the address of this flow server.
 */
void gebr_geoxml_flow_server_set_address(GebrGeoXmlFlow *flow, const gchar * address);

/**
 * Returns the address of this flow server.
 * The string must be freed with g_free() .
 */
const gchar *gebr_geoxml_flow_server_get_address(GebrGeoXmlFlow *flow);

/**
 * gebr_geoxml_flow_server_set_date_last_run:
 * @flow: a flow
 * @date: server last run date
 *
 * Sets the last run date for this server.
 *
 * @flow and @date should not be passed as NULL.
 *
 * See gebr_geoxml_flow_server_get_date_last_run().
 */
void gebr_geoxml_flow_server_set_date_last_run(GebrGeoXmlFlow *flow, const gchar * date);

/**
 * gebr_geoxml_flow_server_get_date_last_run:
 * @flow: a flow
 *
 * Retrieve the last run date for this server.
 *
 * @flow should not be passed as NULL.
 *
 */
const gchar *gebr_geoxml_flow_server_get_date_last_run(GebrGeoXmlFlow *flow);

/**
 * gebr_geoxml_flow_io_set_input:
 * @flow: a flow
 * @input: flow's input path
 *
 * Set the @flow input file path to @input. The input file is
 * the one used to gather the input sent by the last program of @flow.
 *
 * @flow and @input should not be passed as NULL.
 *
 * See gebr_geoxml_flow_server_io_get_input().
 */
void gebr_geoxml_flow_io_set_input(GebrGeoXmlFlow * flow, const gchar * input);

/**
 * gebr_geoxml_flow_io_set_output:
 * @flow: a flow
 * @output: flow's output path
 *
 * Set the @flow output file path to @output. The output file is
 * the one used to gather the output sent by the last program of @flow.
 *
 * @flow and @input should not be passed as NULL.
 *
 * See gebr_geoxml_flow_server_io_get_output().
 */
void gebr_geoxml_flow_io_set_output(GebrGeoXmlFlow * flow, const gchar * output);

/**
 * gebr_geoxml_flow_io_set_error:
 * @flow: a flow
 * @error: flow's error path
 *
 * Set the @flow error file path to @error. This should be the file
 * containing the error log, which might include program's stderr
 *
 * @flow and @input should not be passed as NULL.
 *
 * See gebr_geoxml_flow_server_io_get_error gebr_geoxml_program_set_stderr().
 */
void gebr_geoxml_flow_io_set_error(GebrGeoXmlFlow * flow, const gchar * error);

/**
 * gebr_geoxml_flow_io_get_input:
 * @flow: a flow
 *
 * Retrieves the input file path of @flow.
 * If the input file path was not set before, default will be an empty string "".
 *
 * @flow should not be passed as NULL.
 *
 * See gebr_geoxml_flow_server_io_set_input()
 */
const gchar *gebr_geoxml_flow_io_get_input(GebrGeoXmlFlow * flow);

/**
 * gebr_geoxml_flow_io_get_output:
 * @flow: a flow
 *
 * Retrieves the output file path of @flow.
 * If the output file path was not set before, default will be an empty string "".
 *
 * @flow should not be passed as NULL.
 *
 * See gebr_geoxml_flow_server_io_set_output().
 */
const gchar *gebr_geoxml_flow_io_get_output(GebrGeoXmlFlow * flow);

/**
 * gebr_geoxml_flow_io_get_error:
 * @flow: a flow
 *
 * Retrieves the error file path of @flow.
 * If the error file path was not set before, default will be an empty string "".
 *
 * @flow should not be passed as NULL.
 *
 * See gebr_geoxml_flow_server_io_set_error().
 *
 */
const gchar *gebr_geoxml_flow_io_get_error(GebrGeoXmlFlow * flow);

/**
 * gebr_geoxml_flow_append_program:
 * @flow: a flow
 *
 * Creates a new program associated and append to the list of programs
 * Provided for convenience
 *
 */
GebrGeoXmlProgram *gebr_geoxml_flow_append_program(GebrGeoXmlFlow * flow);

/**
 * gebr_geoxml_flow_get_program:
 * @flow: a flow
 * @program: a program
 * @index: the index of desired program
 *
 * Writes to @program the @index ieth category that @flow belong.
 * If an error occurred (either GEBR_GEOXML_RETV_INVALID_INDEX and GEBR_GEOXML_RETV_NULL_PTR)
 * the content of @program is assigned to NULL.
 *
 * If @flow is NULL nothing is done.
 *
 * Returns one of: GEBR_GEOXML_RETV_SUCCESS, GEBR_GEOXML_RETV_INVALID_INDEX, GEBR_GEOXML_RETV_NULL_PTR
 *
 * See gebr_geoxml_sequence_move gebr_geoxml_sequence_move_up(), gebr_geoxml_sequence_move_down(), gebr_geoxml_sequence_remove().
 */
int gebr_geoxml_flow_get_program(GebrGeoXmlFlow * flow, GebrGeoXmlSequence ** program, gulong index);

/**
 * gebr_geoxml_flow_get_programs_number:
 * @flow: a flow
 *
 * Get the number of programs @flow has.
 *
 * If @flow is NULL, returns -1.
 *
 */
glong gebr_geoxml_flow_get_programs_number(GebrGeoXmlFlow * flow);

/**
 * Return the first program using a mpi implementation at \p flow.
 * If there is no mpi program, return NULL.
 */
GebrGeoXmlProgram *gebr_geoxml_flow_get_first_mpi_program(GebrGeoXmlFlow * flow);

/**
 * gebr_geoxml_flow_append_category:
 * @flow: a flow
 * @name: the name for category
 *
 * Creates a new category and append it to the list of categories.
 * If the category already exist, returns it.
 *
 */
GebrGeoXmlCategory *gebr_geoxml_flow_append_category(GebrGeoXmlFlow * flow, const gchar * name);

/**
 * gebr_geoxml_flow_get_category:
 * @flow: a flow
 * @category: a category
 * @index: the index of desired category
 *
 * Writes to @category the @index ieth category that belongs to @flow.
 * If an error ocurred, the content of @category is assigned to NULL.
 *
 * Returns one of: GEBR_GEOXML_RETV_SUCCESS, GEBR_GEOXML_RETV_INVALID_INDEX, GEBR_GEOXML_RETV_NULL_PTR
 *
 * See gebr_geoxml_sequence_move(), gebr_geoxml_sequence_move_up gebr_geoxml_sequence_move_down(), gebr_geoxml_sequence_remove().
 */
int gebr_geoxml_flow_get_category(GebrGeoXmlFlow * flow, GebrGeoXmlSequence ** category, gulong index);

/**
 * gebr_geoxml_flow_get_categories_number:
 * @flow: a flow
 *
 * Get the number of categories that @flow has.
 *
 * If @flow is NULL returns -1.
 */
glong gebr_geoxml_flow_get_categories_number(GebrGeoXmlFlow * flow);

/**
 * Change all the flow data to the one stored at revision, except, of course,
 * the list of revisions.
 * Be aware that all \p flow data will be lost. If you don't want that, call
 * \ref gebr_geoxml_flow_append_revision before.
 *
 * If \p flow or \p revision is NULL nothing is done, and returns FALSE.
 * If it fails because the revision could not be loaded, returns FALSE.
 * If exists reports on the revisions, the selected one was merged with the 
 * current report and deleted from the revision
 * On success, return TRUE.
 */
gboolean gebr_geoxml_flow_change_to_revision(GebrGeoXmlFlow * flow, GebrGeoXmlRevision * revision, gboolean * report_merged);

/**
 * Creates a new revision with the current time append to the list of revisions
 * An revision is a way to keep the history of the flow changes. You can then restore
 * one revision with \ref gebr_geoxml_flow_change_to_revision
 *
 * If \p flow is NULL nothing is done.
 */
GebrGeoXmlRevision *gebr_geoxml_flow_append_revision(GebrGeoXmlFlow * flow, const gchar * comment);

/**
 * @revision should not be NULL.
 * If \p flow is NULL nothing is done.
 */
void gebr_geoxml_flow_set_revision_data(GebrGeoXmlRevision * revision, const gchar * flow, const gchar * date, const gchar * comment);

/**
 * gebr_geoxml_flow_get_revision:
 * @flow: a flow
 * @revision: a revision
 * @index: the index of desired program
 *
 * Writes to @revision the @index ieth revision that belongs @flow.
 * If an error occurred (either GEBR_GEOXML_RETV_INVALID_INDEX and GEBR_GEOXML_RETV_NULL_PTR)
 * the content of @revision is assigned to NULL.
 *
 * If @flow is NULL nothing is done.
 *
 * Returns one of: GEBR_GEOXML_RETV_SUCCESS, GEBR_GEOXML_RETV_INVALID_INDEX, GEBR_GEOXML_RETV_NULL_PTR
 *
 * See gebr_geoxml_sequence_move gebr_geoxml_sequence_move_up gebr_geoxml_sequence_move_down gebr_geoxml_sequence_remove
 */
int gebr_geoxml_flow_get_revision(GebrGeoXmlFlow * flow, GebrGeoXmlSequence ** revision, gulong index);

/**
 * Get information of \p revision. The flow is stored at \p flow and can be
 * loaded with gebr_geoxml_document_load_buffer. \p date receive the date of creation of \p revision.
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

/**
 * Determine if a flow can be executed or not. If it can't, return
 * the correct type of error.
 */
GebrGeoXmlFlowError gebr_geoxml_flow_validade(GebrGeoXmlFlow * flow, gchar ** program_title);

/**
 * gebr_geoxml_flow_has_control_program:
 * @flow:
 *
 * If this flow has a program marked as a control, this function returns %TRUE. Otherwise returns %FALSE.
 * The purpose of this function is to allow only one control for each flow. It is not possible to set a
 * program to be a control, only if you directly edit the XML file of a flow.
 *
 * Returns: %TRUE if @flow has a program which is a control, %FALSE otherwise.
 */
gboolean gebr_geoxml_flow_has_control_program (GebrGeoXmlFlow *flow);

/*
 * gebr_geoxml_flow_insert_iter_dict:
 * @flow: a #GebrGeoXmlFlow
 *
 * Inserts the `iter' keyword into @flow's dictionary if its not defined there already.
 *
 * Returns: %TRUE if `iter' dictionary was inserted, %FALSE otherwise.
 */
gboolean gebr_geoxml_flow_insert_iter_dict (GebrGeoXmlFlow *flow);

/*
 * gebr_geoxml_flow_insert_iter_dict:
 * @flow: a #GebrGeoXmlFlow
 *
 * Remove the `iter' keyword into @flow's dictionary.
 *
 */
void gebr_geoxml_flow_remove_iter_dict (GebrGeoXmlFlow *flow);

/**
 * gebr_geoxml_flow_get_control_program:
 * @flow:
 *
 * If this flow has a program marked as a control, this function returns %GebrGeoxmlProgram. Otherwise returns %NULL.
 * The purpose of this function is to allow only one control for each flow. It is not possible to set a
 * program to be a control, only if you directly edit the XML file of a flow.
 *
 * Returns: %GebrGeoxmlProgram if @flow has a program which is a control, %NULL otherwise.
 */
GebrGeoXmlProgram *gebr_geoxml_flow_get_control_program (GebrGeoXmlFlow *flow);

/**
 * gebr_geoxml_flow_io_set_output_append:
 * @flow: a #GebrGeoXmlFlow
 * @setting: %TRUE to make output append
 */
void gebr_geoxml_flow_io_set_output_append(GebrGeoXmlFlow *flow, gboolean setting);

/**
 * gebr_geoxml_flow_io_get_output_append:
 * @flow: a #GebrGeoXmlFlow
 *
 * Returns: %TRUE if output appends, %FALSE otherwise
 */
gboolean gebr_geoxml_flow_io_get_output_append(GebrGeoXmlFlow *flow);

/**
 * gebr_geoxml_flow_io_set_error_append:
 * @flow: a #GebrGeoXmlFlow
 * @setting: %TRUE to make error append
 */
void gebr_geoxml_flow_io_set_error_append(GebrGeoXmlFlow *flow, gboolean setting);

/**
 * gebr_geoxml_flow_io_get_error_append:
 * @flow: a #GebrGeoXmlFlow
 *
 * Returns: %TRUE if error appends, %FALSE otherwise
 */
gboolean gebr_geoxml_flow_io_get_error_append(GebrGeoXmlFlow *flow);

G_END_DECLS

#endif				//__GEBR_GEOXML_FLOW_H
