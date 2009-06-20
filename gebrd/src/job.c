/*   GeBR Daemon - Process and control execution of flows
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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>
#include <glib/gstdio.h>


#include <libgebr/intl.h>
#include <libgebr/comm/protocol.h>
#include <libgebr/comm/gstreamsocket.h>
#include <libgebr/comm/gsocketaddress.h>
#include <libgebr/utils.h>
#include <libgebr/date.h>

#include "job.h"
#include "gebrd.h"

/*
 * Internal functions
 */

static gboolean
job_parse_parameters(struct job * job, GeoXmlParameters * parameters, GeoXmlProgram * program);
gboolean
check_for_readable_file(const gchar *file);
gboolean
check_for_write_permission(const gchar *file);
gboolean
check_for_binary(const gchar *binary);


static gboolean
job_parse_parameter(struct job * job, GeoXmlParameter * parameter, GeoXmlProgram * program)
{
	enum GEOXML_PARAMETERTYPE	type;
	GeoXmlProgramParameter *	program_parameter;

	type = geoxml_parameter_get_type(parameter);
	if (type == GEOXML_PARAMETERTYPE_GROUP) {
		GeoXmlSequence *	instance;
		gboolean		ret;

		geoxml_parameter_group_get_instance(GEOXML_PARAMETER_GROUP(parameter), &instance, 0);
		for (ret = TRUE; instance != NULL && ret == TRUE; geoxml_sequence_next(&instance)) {
			GeoXmlParameter *	selected;

			/* for an exclusive instance */
			selected = geoxml_parameters_get_selected(GEOXML_PARAMETERS(instance));
			if (selected != NULL)
				ret = job_parse_parameter(job, selected, program);
			else
				ret = job_parse_parameters(job, GEOXML_PARAMETERS(instance), program);
		}

		return ret;
	}

	program_parameter = GEOXML_PROGRAM_PARAMETER(parameter);
	switch (type) {
	case GEOXML_PARAMETERTYPE_STRING:
	case GEOXML_PARAMETERTYPE_INT:
	case GEOXML_PARAMETERTYPE_FLOAT:
	case GEOXML_PARAMETERTYPE_RANGE:
	case GEOXML_PARAMETERTYPE_FILE:
	case GEOXML_PARAMETERTYPE_ENUM: {
		GString *	value;

		value = geoxml_program_parameter_get_string_value(program_parameter, FALSE);
		if (strlen(value->str) > 0) {
			g_string_append_printf(job->cmd_line, "%s\"%s\" ",
				geoxml_program_parameter_get_keyword(program_parameter),
				value->str);
		} else {
			/* Check if this is a required parameter */
			if (geoxml_program_parameter_get_required(program_parameter)) {
				g_string_append_printf(job->issues,
					_("Required parameter '%s' of program '%s' not provided.\n"),
					geoxml_parameter_get_label(parameter),
					geoxml_program_get_title(program));

				return FALSE;
			}
		}

		g_string_free(value, TRUE);

		break;
	}
	case GEOXML_PARAMETERTYPE_FLAG:
		if (geoxml_program_parameter_get_first_boolean_value(program_parameter, FALSE) == TRUE)
			g_string_append_printf(job->cmd_line, "%s ", geoxml_program_parameter_get_keyword(program_parameter));

		break;
	default:
		g_string_append_printf(job->issues, _("Unknown parameter type.\n"));

		return FALSE;
	}

	return TRUE;
}

static gboolean
job_parse_parameters(struct job * job, GeoXmlParameters * parameters, GeoXmlProgram * program)
{
	GeoXmlSequence *	parameter;

	parameter = geoxml_parameters_get_first_parameter(parameters);
	for (; parameter != NULL; geoxml_sequence_next(&parameter))
		if (job_parse_parameter(job, GEOXML_PARAMETER(parameter), program) == FALSE)
			return FALSE;

	return TRUE;
}

static gboolean
job_add_program_parameters(struct job * job, GeoXmlProgram * program)
{
	return job_parse_parameters(job, geoxml_program_get_parameters(program), program);
}

static void
job_send_clients_output(struct job * job, GString * output)
{
	GList *		link;

	/* FIXME: remove and find the real problem */
	if (!job->jid->len)
		return;

	link = g_list_first(gebrd.clients);
	while (link != NULL) {
		struct client *	client;

		client = (struct client *)link->data;
		protocol_send_data(client->protocol, client->stream_socket,
			protocol_defs.out_def, 2, job->jid->str, output->str);

		link = g_list_next(link);
	}
}

static void
job_process_add_output(struct job * job, GString * destination, GString * output)
{
	GString *	final_output;

	final_output = g_string_new(NULL);
	/* ensure UTF-8 encoding */
	if (g_utf8_validate(output->str, -1, NULL) == FALSE) {
		gchar *	converted;

		/* TODO: what else should be tried? */
		converted = g_simple_locale_to_utf8(output->str);
		if (converted == NULL) {
			g_string_assign(final_output, converted);
			gebrd_message(LOG_ERROR, _("Job '%s' sent output not in UTF-8."), job->title->str);
		} else {
			g_string_assign(final_output, converted);
			g_free(converted);
		}
	} else
		g_string_assign(final_output, output->str);

	g_string_append(destination, final_output->str);
	job_send_clients_output(job, final_output);

	g_string_free(final_output, TRUE);
}

static void
job_process_read_stdout(GProcess * process, struct job * job)
{
	GString *	stdout;

	stdout = g_process_read_stdout_string_all(process);
	job_process_add_output(job, job->output, stdout);

	g_string_free(stdout, TRUE);
}

static void
job_process_read_stderr(GProcess * process, struct job * job)
{
	GString *	stderr;

	stderr = g_process_read_stderr_string_all(process);
	job_process_add_output(job, job->output, stderr);

	g_string_free(stderr, TRUE);
}

static void
job_process_finished(GProcess * process, struct job * job)
{
	GList *	link;

	/* set the finish date */
	g_string_assign(job->finish_date, iso_date());

	/* what make it quit? */
	g_string_assign(job->status, (job->user_finished == FALSE) ? "finished" : "canceled");

	/* warn all client that this job finished */
	link = g_list_first(gebrd.clients);
	while (link != NULL) {
		struct client * client;

		client = (struct client *)link->data;
		protocol_send_data(client->protocol, client->stream_socket, protocol_defs.fin_def,
			3, job->jid->str, job->status->str, job->finish_date->str);

		link = g_list_next(link);
	}
}

static GString *
job_generate_id(void)
{
	GString *	jid;
	guint32		random;
	GList *		link;
	gboolean	unique;

	unique = TRUE;
	jid = g_string_new(NULL);
	do {
		random = g_random_int();
		g_string_printf(jid, "%u", random);

		/* check if it is unique */
		link = g_list_first(gebrd.jobs);
		while (link != NULL) {
			struct job *	job;

			job = (struct job *)link->data;
			if (!strcmp(job->jid->str, jid->str)) {
				unique = FALSE;
				break;
			}

			link = g_list_next(link);
		}
	} while (!unique);

	return jid;
}

/*
 * Public functions
 */

gboolean
job_new(struct job ** _job, struct client * client, GString * xml)
{
	struct job *		job;

	GeoXmlFlow *		flow;
	GeoXmlDocument *	document;
	GeoXmlSequence *	program;
	gulong			nprog;

	gboolean		has_error_output_file;
	gboolean		previous_stdout;
	guint			issue_number;
	gboolean		success;

	/* initialization */
	issue_number = 0;
	job = g_malloc(sizeof(struct job));
	*job = (struct job) {
		.process = g_process_new(),
		.flow = NULL,
		.user_finished = FALSE,
		.hostname = g_string_new(""),
		.status = g_string_new(""),
		.jid = job_generate_id(),
		.title = g_string_new(""),
		.start_date = g_string_new(""),
		.finish_date = g_string_new(""),
		.issues = g_string_new(""),
		.cmd_line = g_string_new(""),
		.output = g_string_new(""),
	};
	*_job = job;

	int ret;
	ret = geoxml_document_load_buffer(&document, xml->str);
	if (ret < 0) {
		switch (ret) {
		case GEOXML_RETV_DTD_SPECIFIED:
			g_string_append_printf(job->issues,
				_("DTD specified. The <DOCTYPE ...> must not appear in XML.\n"
				"libgeoxml will find the appopriated DTD installed from version.\n"));
			break;
		case GEOXML_RETV_INVALID_DOCUMENT:
			g_string_append_printf(job->issues,
				_("Invalid document. It has a sintax error or doesn't match the DTD.\n"
				"In this case see the errors above.\n"));
			break;
		case GEOXML_RETV_CANT_ACCESS_FILE:
			g_string_append_printf(job->issues,
				_("Can't access file. The file doesn't exist or there is not enough "
				"permission to read it.\n"));
			break;
		case GEOXML_RETV_CANT_ACCESS_DTD:
			g_string_append_printf(job->issues,
				_("Can't access dtd. The file's DTD couldn't not be found.\n"
				"It may be a newer version not support by this version of libgeoxml "
				"or there is an installation problem.\n"));
			break;
		case GEOXML_RETV_NO_MEMORY:
			g_string_append_printf(job->issues,
				_("Not enough memory. The library stoped after an unsucessful memory allocation.\n"));
			break;
		default:
			g_string_append_printf(job->issues,
				_("Unspecified error %d.\n"
				"See library documentation at http://sites.google.com/site/gebrproject.\n"), ret);
			break;
		}
		goto err;
	}

	flow = GEOXML_FLOW(document);
	job->flow = flow;
	has_error_output_file = strlen(geoxml_flow_io_get_error(flow)) ? TRUE : FALSE;
	nprog = geoxml_flow_get_programs_number(flow);
	if (nprog == 0) {
		g_string_append_printf(job->issues, _("Empty flow.\n"));
		goto err;
	}

	/* Check if there is configured programs */
	geoxml_flow_get_program(flow, &program, 0);
	while (program != NULL && strcmp(geoxml_program_get_status(GEOXML_PROGRAM(program)), "configured") != 0) {
		g_string_append_printf(job->issues, _("%u) Skiping disabled/unconfigured program '%s.'\n"),
			++issue_number, geoxml_program_get_title(GEOXML_PROGRAM(program)));

		geoxml_sequence_next(&program);
	}
	if (program == NULL) {
		g_string_append_printf(job->issues, _("No configured programs.\n"));
		goto err;
	}

	/*
	 * First program
	 */
	/* Start wi.hwithout stdin */
	if (geoxml_program_get_stdin(GEOXML_PROGRAM(program))) {
		if (strlen(geoxml_flow_io_get_input(flow)) == 0) {
			g_string_append_printf(job->issues, _("No input file selected.\n"));
			goto err;
		}

		/* Input file */
		if (check_for_readable_file(geoxml_flow_io_get_input(flow))) {
			g_string_append_printf(job->issues,
						_("Input file %s not present or not accessable.\n"),
						geoxml_flow_io_get_input(flow));
			goto err;
		}

		g_string_append_printf(job->cmd_line, "<\"%s\" ", geoxml_flow_io_get_input(flow));

	}
	/* Binary followed by an space */
	g_string_append_printf(job->cmd_line, "%s ", geoxml_program_get_binary(GEOXML_PROGRAM(program)));
	if (job_add_program_parameters(job, GEOXML_PROGRAM(program)) == FALSE)
		goto err;
	/* check for error file output */
	if (has_error_output_file && geoxml_program_get_stderr(GEOXML_PROGRAM(program))) {
		if (check_for_write_permission(geoxml_flow_io_get_error(flow))) {
			g_string_append_printf(job->issues,
				_("Write permission to %s not granted.\n"),
				geoxml_flow_io_get_error(flow));
			goto err;
		}

		g_string_append_printf(job->cmd_line, "2>> \"%s\" ", geoxml_flow_io_get_error(flow));
        }

	/*
	 * Others programs
	 */
	previous_stdout = geoxml_program_get_stdout(GEOXML_PROGRAM(program));
	geoxml_sequence_next(&program);
	while (program != NULL) {
		/* Skiping disabled/unconfigured programs */
		if (strcmp(geoxml_program_get_status(GEOXML_PROGRAM(program)), "configured") != 0) {
			g_string_append_printf(job->issues, _("%u) Skiping disabled/unconfigured program '%s'.\n"),
				++issue_number, geoxml_program_get_title(GEOXML_PROGRAM(program)));

			geoxml_sequence_next(&program);
			continue;
		}

		/* How to connect chainned programs */
		int chain_option = geoxml_program_get_stdin(GEOXML_PROGRAM(program)) + (previous_stdout << 1);
		switch (chain_option) {
		case 0: /* Previous does not write to stdin and current does not carry about */
			g_string_append_printf(job->cmd_line, "; %s ", geoxml_program_get_binary(GEOXML_PROGRAM(program)));
			break;
		case 1: /* Previous does not write to stdin but current expect something */
			g_string_append_printf(job->issues, _("Broken flow before %s (no input).\n"),
				geoxml_program_get_title(GEOXML_PROGRAM(program)));
			goto err;
		case 2: /* Previous does write to stdin but current does not carry about */
			g_string_append_printf(job->issues, _("Broken flow before %s (unexpected output).\n"),
				geoxml_program_get_title(GEOXML_PROGRAM(program)));
			goto err;
		case 3: /* Both talk to each other */
			g_string_append_printf(job->cmd_line, "| %s ", geoxml_program_get_binary(GEOXML_PROGRAM(program)));
			break;
		default:
			break;
		}

		if (job_add_program_parameters(job, GEOXML_PROGRAM(program)) == FALSE)
			goto err;
		if (has_error_output_file && geoxml_program_get_stderr(GEOXML_PROGRAM(program)))
			g_string_append_printf(job->cmd_line, "2>> \"%s\" ", geoxml_flow_io_get_error(flow));

		previous_stdout = geoxml_program_get_stdout(GEOXML_PROGRAM(program));
		geoxml_sequence_next(&program);
	}

	if (has_error_output_file == FALSE)
		g_string_append_printf(job->issues,
			_("No error file selected; error output merged with standard output.\n"));
	if (previous_stdout) {
		if (strlen(geoxml_flow_io_get_output(flow)) == 0)
			g_string_append_printf(job->issues, _("Proceeding without output file.\n"));
		else {
                        if (check_for_write_permission(geoxml_flow_io_get_output(flow))) {
				g_string_append_printf(job->issues,
					_("Write permission to %s not granted.\n"),
					geoxml_flow_io_get_output(flow));
				goto err;
			}
			g_string_append_printf(job->cmd_line, ">\"%s\"", geoxml_flow_io_get_output(flow));
                }
        }

	/* success exit */
	success = TRUE;
	goto out;

err:	g_string_assign(job->jid, "0");
	g_string_assign(job->status, "failed");
        g_string_assign(job->cmd_line, "");
	success = FALSE;

out:
	/* hostname and flow title */
	g_string_assign(job->hostname, client->protocol->hostname->str);
	g_string_assign(job->title, geoxml_document_get_title(document));
	/* set the start date */
	g_string_assign(job->start_date, iso_date());

	return success;
}

void
job_free(struct job * job)
{
	GList *	link;

	link = g_list_find(gebrd.jobs, job);
	/* job that failed to run are not added to this list */
	if (link != NULL)
		gebrd.jobs = g_list_delete_link(gebrd.jobs, link);

	/* free data */
	g_process_free(job->process);
	geoxml_document_free(GEOXML_DOC(job->flow));
	g_string_free(job->hostname, TRUE);
	g_string_free(job->status, TRUE);
	g_string_free(job->jid, TRUE);
	g_string_free(job->title, TRUE);
	g_string_free(job->start_date, TRUE);
	g_string_free(job->finish_date, TRUE);
	g_string_free(job->issues, TRUE);
	g_string_free(job->cmd_line, TRUE);
	g_string_free(job->output, TRUE);
	g_free(job);
}

void
job_run_flow(struct job * job, struct client * client)
{
	GString *		cmd_line;
	GeoXmlSequence *	program;
	gchar *                 locale_str;
        gsize                   __attribute__ ((unused)) bytes_written;

	/* initialization */
	cmd_line = g_string_new(NULL);
        locale_str = g_filename_from_utf8(job->cmd_line->str, -1, NULL, &bytes_written, NULL);

	/* command-line */
	if (client->display->len) {
		if (client->is_local)
			g_string_printf(cmd_line, "bash -l -c 'export DISPLAY=%s; %s'",
				client->display->str, locale_str);
		else
			g_string_printf(cmd_line, "bash -l -c 'export DISPLAY=127.0.0.1%s; %s'",
				client->display->str, locale_str);
	} else {
		g_string_printf(cmd_line, "bash -l -c '%s'", locale_str);
	}
	gebrd_message(LOG_DEBUG, "Client '%s' flow about to run: %s",
		client->protocol->hostname->str, cmd_line->str);

	gebrd.jobs = g_list_append(gebrd.jobs, job);
	g_signal_connect(job->process, "ready-read-stdout",
			G_CALLBACK(job_process_read_stdout), job);
	g_signal_connect(job->process, "ready-read-stderr",
			G_CALLBACK(job_process_read_stderr), job);
	g_signal_connect(job->process, "finished",
			G_CALLBACK(job_process_finished), job);
	g_process_start(job->process, cmd_line);
	g_string_assign(job->status, "running");

	/* for program that waits stdin EOF (like sfmath) */
	geoxml_flow_get_program(job->flow, &program, 0);
	if (geoxml_program_get_stdin(GEOXML_PROGRAM(program)) == FALSE)
		g_process_close_stdin(job->process);

	/* frees */
	g_string_free(cmd_line, TRUE);
        g_free(locale_str);
}

struct job *
job_find(GString * jid)
{
	GList *		link;
	struct job *	job;

	job = NULL;
	link = g_list_first(gebrd.jobs);
	while (link != NULL) {
		struct job * i;

		i = (struct job *)link->data;
		if (!strcmp(i->jid->str, jid->str)) {
			job = i;
			break;
		}

		link = g_list_next(link);
	}

	return job;
}

void
job_clear(struct job * job)
{
	if (g_process_is_running(job->process) == FALSE)
		job_free(job);
}

void
job_end(struct job * job)
{
	job->user_finished = TRUE;
	g_process_terminate(job->process);
}

void
job_kill(struct job * job)
{
	job->user_finished = TRUE;
	g_process_kill(job->process);
}

void
job_list(struct client * client)
{
	GList *		link;

	link = g_list_first(gebrd.jobs);
	while (link != NULL) {
		struct job *	job;

		job = (struct job *)link->data;
		protocol_send_data(client->protocol, client->stream_socket, protocol_defs.job_def, 9,
			job->jid->str, job->status->str, job->title->str,
			job->start_date->str, job->finish_date->str, job->hostname->str,
			job->issues->str, job->cmd_line->str, job->output->str);

		link = g_list_next(link);
	}
}

void
job_send_clients_job_notify(struct job * job)
{
	GList *		link;

	link = g_list_first(gebrd.clients);
	while (link != NULL) {
		struct client * client;

		client = (struct client *)link->data;
		protocol_send_data(client->protocol, client->stream_socket, protocol_defs.job_def, 9,
			job->jid->str, job->status->str, job->title->str,
			job->start_date->str, job->finish_date->str, job->hostname->str,
			job->issues->str, job->cmd_line->str, job->output->str);

		link = g_list_next(link);
	}
}

/* Several tests to ensure flow executability */
/* All of them return TRUE upon error */
gboolean
check_for_readable_file(const gchar *file)
{
        return (g_access(file, F_OK | R_OK) == -1 ? TRUE: FALSE);
        
}

gboolean
check_for_write_permission(const gchar *file)
{

        gboolean  ret;
        gchar *   dir;

        dir = g_path_get_dirname(file);
        ret = (g_access(dir, W_OK) == -1 ? TRUE: FALSE);
        g_free(dir);
        
        return ret;
}

gboolean
check_for_binary(const gchar *binary)
{
        return (g_find_program_in_path(binary) == NULL ? TRUE : FALSE);
}
