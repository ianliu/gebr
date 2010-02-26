/*   GeBR Daemon - Process and control execution of flows
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

#define _XOPEN_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <gdome.h>

#include <libgebr/intl.h>
#include <libgebr/comm/protocol.h>
#include <libgebr/comm/streamsocket.h>
#include <libgebr/comm/socketaddress.h>
#include <libgebr/utils.h>
#include <libgebr/date.h>

#include "job.h"
#include "gebrd.h"
#include "queues.h"

static gboolean job_parse_parameters(struct job *job, GebrGeoXmlParameters * parameters, GebrGeoXmlProgram * program);

static void job_process_finished(GebrCommProcess * process, struct job *job);

static gboolean check_for_readable_file(const gchar * file);

static gboolean check_for_write_permission(const gchar * file);

static gboolean check_for_binary(const gchar * binary);

static void job_send_signal_on_moab(const char * signal, struct job * job);

static gboolean job_parse_parameter(struct job *job, GebrGeoXmlParameter * parameter, GebrGeoXmlProgram * program)
{
	enum GEBR_GEOXML_PARAMETER_TYPE type;
	GebrGeoXmlProgramParameter *program_parameter;

	type = gebr_geoxml_parameter_get_type(parameter);
	if (type == GEBR_GEOXML_PARAMETER_TYPE_GROUP) {
		GebrGeoXmlSequence *instance;
		gboolean ret;

		gebr_geoxml_parameter_group_get_instance(GEBR_GEOXML_PARAMETER_GROUP(parameter), &instance, 0);
		for (ret = TRUE; instance != NULL && ret == TRUE; gebr_geoxml_sequence_next(&instance)) {
			GebrGeoXmlParameter *selected;

			/* for an exclusive instance */
			selected = gebr_geoxml_parameters_get_selected(GEBR_GEOXML_PARAMETERS(instance));
			if (selected != NULL)
				ret = job_parse_parameter(job, selected, program);
			else
				ret = job_parse_parameters(job, GEBR_GEOXML_PARAMETERS(instance), program);
		}

		return ret;
	}

	program_parameter = GEBR_GEOXML_PROGRAM_PARAMETER(parameter);
	switch (type) {
	case GEBR_GEOXML_PARAMETER_TYPE_STRING:
	case GEBR_GEOXML_PARAMETER_TYPE_INT:
	case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
	case GEBR_GEOXML_PARAMETER_TYPE_RANGE:
	case GEBR_GEOXML_PARAMETER_TYPE_FILE:
	case GEBR_GEOXML_PARAMETER_TYPE_ENUM:{
			GString *value;
			
			value = gebr_geoxml_program_parameter_get_string_value(program_parameter, FALSE);

			if (strlen(value->str) > 0) {
				gchar *quoted;
				quoted = g_shell_quote(value->str);
				g_string_append_printf(job->cmd_line, "%s%s ",
						       gebr_geoxml_program_parameter_get_keyword(program_parameter),
						       quoted);
				g_free(quoted);
			} else {
				/* Check if this is a required parameter */
				if (gebr_geoxml_program_parameter_get_required(program_parameter)) {
					g_string_append_printf(job->issues,
							       _("Required parameter '%s' of program '%s' not provided.\n"),
							       gebr_geoxml_parameter_get_label(parameter),
							       gebr_geoxml_program_get_title(program));

					return FALSE;
				}
			}

			g_string_free(value, TRUE);

			break;
		}
	case GEBR_GEOXML_PARAMETER_TYPE_FLAG:
		if (gebr_geoxml_program_parameter_get_first_boolean_value(program_parameter, FALSE) == TRUE)
			g_string_append_printf(job->cmd_line, "%s ",
					       gebr_geoxml_program_parameter_get_keyword(program_parameter));

		break;
	default:
		g_string_append_printf(job->issues, _("Unknown parameter type.\n"));

		return FALSE;
	}

	return TRUE;
}

static gboolean job_parse_parameters(struct job *job, GebrGeoXmlParameters * parameters, GebrGeoXmlProgram * program)
{
	GebrGeoXmlSequence *parameter;

	parameter = gebr_geoxml_parameters_get_first_parameter(parameters);
	for (; parameter != NULL; gebr_geoxml_sequence_next(&parameter))
		if (job_parse_parameter(job, GEBR_GEOXML_PARAMETER(parameter), program) == FALSE)
			return FALSE;

	return TRUE;
}

static gboolean job_add_program_parameters(struct job *job, GebrGeoXmlProgram * program)
{
	return job_parse_parameters(job, gebr_geoxml_program_get_parameters(program), program);
}

static void job_send_clients_output(struct job *job, GString * output)
{
	GList *link;

	/* FIXME: remove and find the real problem */
	if (!job->jid->len)
		return;

	link = g_list_first(gebrd.clients);
	while (link != NULL) {
		struct client *client;

		client = (struct client *)link->data;
		gebr_comm_protocol_send_data(client->protocol, client->stream_socket,
					     gebr_comm_protocol_defs.out_def, 2, job->jid->str, output->str);

		link = g_list_next(link);
	}
}

static void job_process_add_output(struct job *job, GString * destination, GString * output)
{
	GString *final_output;

	final_output = g_string_new(NULL);
	/* ensure UTF-8 encoding */
	if (g_utf8_validate(output->str, -1, NULL) == FALSE) {
		gchar *converted;

		/* TODO: what else should be tried? */
		converted = gebr_locale_to_utf8(output->str);
		if (converted == NULL) {
			g_string_assign(final_output, converted);
			gebrd_message(GEBR_LOG_ERROR, _("Job '%s' sent output not in UTF-8."), job->title->str);
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


static void moab_process_read_stdout(GebrCommProcess *process, struct job *job)
{
	GString *stdout;
	stdout = gebr_comm_process_read_stdout_string_all(process);
	job_process_add_output(job, job->output, stdout);
	g_string_free(stdout, TRUE);
}

static void job_process_read_stdout(GebrCommProcess * process, struct job *job)
{
	GString *stdout;

	stdout = gebr_comm_process_read_stdout_string_all(process);
	job_process_add_output(job, job->output, stdout);

	g_string_free(stdout, TRUE);
}

static void job_process_read_stderr(GebrCommProcess * process, struct job *job)
{
	GString *stderr;

	stderr = gebr_comm_process_read_stderr_string_all(process);
	job_process_add_output(job, job->output, stderr);

	g_string_free(stderr, TRUE);
}

/**
 * \internal
 * Change status in \p job structure
 * \see job_notify_status
 */
static void job_set_status(struct job *job, enum JobStatus status)
{
	const gchar * enum_to_string [] = {
		"unknown", "queued", "failed", "running", "finished", "canceled", "requeued", NULL };

	g_string_assign(job->status_string, enum_to_string[status]);
	if (status != JOB_STATUS_REQUEUED)
		job->status = status;	
}

void job_notify_status(struct job *job, enum JobStatus status, const gchar *parameter)
{
	GList *link;

	job_set_status(job, status);

	/* warn all client of the new status */
	link = g_list_first(gebrd.clients);
	while (link != NULL) {
		struct client *client;

		client = (struct client *)link->data;
		gebr_comm_protocol_send_data(client->protocol, client->stream_socket, gebr_comm_protocol_defs.sta_def,
					     3, job->jid->str, job->status_string->str, parameter);

		link = g_list_next(link);
	}
}

static gboolean job_set_status_finished(struct job *job)
{
	if (gebrd_get_server_type() == GEBR_COMM_SERVER_TYPE_MOAB) {
		if (job->tail_process != NULL){
			gebr_comm_process_free(job->tail_process);
			job->tail_process = NULL;
		} else
			return FALSE;
	
		GIOChannel * file;
		GError * error;
		GString * output;
		gchar * buffer;
		gsize length;

		error = NULL;
		output = g_string_new(NULL);
		g_string_printf(output, "%s/STDIN.o%s", getenv("HOME"), job->moab_jid->str);
		file = g_io_channel_new_file(output->str, "r", &error);
		if (file == NULL)
			goto out2;
		if (g_io_channel_seek_position(file, job->output->len, G_SEEK_SET, &error) == G_IO_STATUS_EOF)
			goto out;
		g_io_channel_read_to_end(file, &buffer, &length, &error);
		g_string_printf(output, "%s", buffer);
		job_process_add_output(job, job->output, output);

		g_free(buffer);
out:		g_io_channel_shutdown(file, FALSE, &error);
out2:		g_string_free(output, TRUE);
	}

	/* set the finish date */
	g_string_assign(job->finish_date, gebr_iso_date());

	job_notify_status(job, (job->user_finished == FALSE) ? JOB_STATUS_FINISHED : JOB_STATUS_CANCELED, job->finish_date->str);

	/* Run next flow on queue */
	if (gebrd_queues_has_next(job->queue->str)) 
		gebrd_queues_step_queue(job->queue->str);
	else
		gebrd_queues_set_queue_busy(job->queue->str, FALSE);
	return TRUE;
}

static void job_process_finished(GebrCommProcess * process, struct job *job)
{
	job_set_status_finished(job);
}

static GString *job_generate_id(void)
{
	GString *jid;
	guint32 random;
	GList *link;
	gboolean unique;

	unique = TRUE;
	jid = g_string_new(NULL);
	do {
		random = g_random_int();
		g_string_printf(jid, "%u", random);

		/* check if it is unique */
		link = g_list_first(gebrd.jobs);
		while (link != NULL) {
			struct job *job;

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

/**
 * \internal
 */
gboolean job_moab_checkjob_pooling(struct job * job)
{
	GString *cmd_line = NULL;
	gchar *std_out = NULL;
	gchar *std_err = NULL;
	gint exit_status;
	gboolean ret = FALSE;
	GString *moab_status;
	enum JobStatus moab_status_enum;

	GdomeDOMImplementation *dom_impl = NULL;
	GdomeDocument *doc = NULL;
	GdomeElement *element = NULL;
	GdomeException exception;
	GdomeDOMString *attribute_name;

	cmd_line = g_string_new("");
	moab_status = g_string_new("");
	g_string_printf(cmd_line, "checkjob %s --format=xml", job->moab_jid->str);
	if (g_spawn_command_line_sync(cmd_line->str, &std_out, &std_err, &exit_status, NULL) == FALSE)
		goto err3;

	if ((dom_impl = gdome_di_mkref()) == NULL)
		goto err3;
	if ((doc = gdome_di_createDocFromMemory(dom_impl, std_out, GDOME_LOAD_PARSING, &exception)) == NULL) {
		goto err2;
	}
	if ((element = gdome_doc_documentElement(doc, &exception)) == NULL) {
		goto err;
	}
	if ((element = (GdomeElement *)gdome_el_firstChild(element, &exception)) == NULL) {
		goto err;
	}

	attribute_name = gdome_str_mkref("EState");
	g_string_assign(moab_status, (gdome_el_getAttribute(element, attribute_name, &exception))->str);
	gdome_str_unref(attribute_name);
	ret = TRUE;

	moab_status_enum = JOB_STATUS_UNKNOWN;
	if (!strcmp(moab_status->str, "Idle"))
		moab_status_enum = JOB_STATUS_QUEUED;	
	else if (!strcmp(moab_status->str, "Starting"));
	else if (!strcmp(moab_status->str, "Running"))
		moab_status_enum = JOB_STATUS_RUNNING;
	else if (!strcmp(moab_status->str, "Completed"))
		moab_status_enum = JOB_STATUS_FINISHED;

	/* status changed */
	if (moab_status_enum != JOB_STATUS_UNKNOWN && moab_status_enum != job->status) {
		if (moab_status_enum == JOB_STATUS_FINISHED)
			ret = job_set_status_finished(job);
		else
			job_notify_status(job, moab_status_enum, "");
       	}

err:	gdome_doc_unref(doc, &exception);
err2:	gdome_di_unref(dom_impl, &exception);

err3:	g_string_free(cmd_line, TRUE);
	g_string_free(moab_status, TRUE);
	g_free(std_out);
	g_free(std_err);

	return ret;
}

enum JobStatus job_translate_status(GString * status)
{
	enum JobStatus translated_status;

	if (!strcmp(status->str, "unknown"))
		translated_status = JOB_STATUS_UNKNOWN;
	else if (!strcmp(status->str, "queued"))
		translated_status = JOB_STATUS_QUEUED;
	else if (!strcmp(status->str, "failed"))
		translated_status = JOB_STATUS_FAILED;
	else if (!strcmp(status->str, "running"))
		translated_status = JOB_STATUS_RUNNING;
	else if (!strcmp(status->str, "finished"))
		translated_status = JOB_STATUS_FINISHED;
	else if (!strcmp(status->str, "canceled"))
		translated_status = JOB_STATUS_CANCELED;
	else if (!strcmp(status->str, "requeued"))
		translated_status = JOB_STATUS_REQUEUED;
	else
		translated_status = JOB_STATUS_UNKNOWN;

	return translated_status;
}

/*
 * Public functions
 */

gboolean job_new(struct job ** _job, struct client * client, GString * queue, GString * account, GString * xml)
{
	struct job *job;

	GebrGeoXmlFlow *flow;
	GebrGeoXmlDocument *document;
	GebrGeoXmlSequence *program;
	gulong nprog;

	gboolean has_error_output_file;
	gboolean previous_stdout;
	guint issue_number;
	gboolean success;

	/* initialization */
	issue_number = 0;
	job = g_malloc(sizeof(struct job));
	*job = (struct job) {
		.process = gebr_comm_process_new(),
		.flow = NULL,
		.user_finished = FALSE,
		.hostname = g_string_new(client->protocol->hostname->str),
		.display = g_string_new(client->display->str),
		.server_location = client->server_location,
		.status_string = g_string_new(""),
		.jid = job_generate_id(),
		.title = g_string_new(""),
		.start_date = g_string_new(""),
		.finish_date = g_string_new(""),
		.issues = g_string_new(""),
		.cmd_line = g_string_new(""),
		.output = g_string_new(""),
		.queue  = g_string_new(queue->str),
		.moab_account = g_string_new(account->str),
		.moab_jid = g_string_new("")
	};
	*_job = job;

	int ret;
	gchar * quoted;

	ret = gebr_geoxml_document_load_buffer(&document, xml->str);
	if (ret < 0) {
		switch (ret) {
		case GEBR_GEOXML_RETV_DTD_SPECIFIED:
			g_string_append_printf(job->issues,
					       _("DTD specified. The <DOCTYPE ...> must not appear in XML.\n"
						 "libgeoxml will find the appopriated DTD installed from version.\n"));
			break;
		case GEBR_GEOXML_RETV_INVALID_DOCUMENT:
			g_string_append_printf(job->issues,
					       _("Invalid document. It has a sintax error or doesn't match the DTD.\n"
						 "In this case see the errors above.\n"));
			break;
		case GEBR_GEOXML_RETV_CANT_ACCESS_FILE:
			g_string_append_printf(job->issues,
					       _("Can't access file. The file doesn't exist or there is not enough "
						 "permission to read it.\n"));
			break;
		case GEBR_GEOXML_RETV_CANT_ACCESS_DTD:
			g_string_append_printf(job->issues,
					       _("Can't access dtd. The file's DTD couldn't not be found.\n"
						 "It may be a newer version not support by this version of libgeoxml "
						 "or there is an installation problem.\n"));
			break;
		case GEBR_GEOXML_RETV_NO_MEMORY:
			g_string_append_printf(job->issues,
					       _("Not enough memory. The library stoped after an unsucessful memory allocation.\n"));
			break;
		default:
			g_string_append_printf(job->issues,
					       _("Unspecified error %d.\n"
						 "See library documentation at http://sites.google.com/site/gebrproject.\n"),
					       ret);
			break;
		}
		goto err;
	}

	flow = GEBR_GEOXML_FLOW(document);
	job->flow = flow;
	has_error_output_file = strlen(gebr_geoxml_flow_io_get_error(flow)) ? TRUE : FALSE;
	nprog = gebr_geoxml_flow_get_programs_number(flow);
	if (nprog == 0) {
		g_string_append_printf(job->issues, _("Empty flow.\n"));
		goto err;
	}

	/* Check if there is configured programs */
	gebr_geoxml_flow_get_program(flow, &program, 0);
	while (program != NULL &&
	       gebr_geoxml_program_get_status(GEBR_GEOXML_PROGRAM(program)) != GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED) {
		g_string_append_printf(job->issues, _("%u) Skiping disabled/unconfigured program '%s.'\n"),
				       ++issue_number, gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(program)));

		gebr_geoxml_sequence_next(&program);
	}
	if (program == NULL) {
		g_string_append_printf(job->issues, _("No configured programs.\n"));
		goto err;
	}

	/*
	 * First program
	 */
	/* Start wi.hwithout stdin */
	if (gebr_geoxml_program_get_stdin(GEBR_GEOXML_PROGRAM(program))) {
		if (strlen(gebr_geoxml_flow_io_get_input(flow)) == 0) {
			g_string_append_printf(job->issues, _("No input file selected.\n"));
			goto err;
		}

		/* Input file */
		if (check_for_readable_file(gebr_geoxml_flow_io_get_input(flow))) {
			g_string_append_printf(job->issues,
					       _("Input file %s not present or not accessable.\n"),
					       gebr_geoxml_flow_io_get_input(flow));
			goto err;
		}

		quoted = g_shell_quote(gebr_geoxml_flow_io_get_input(flow));
		g_string_append_printf(job->cmd_line, "<%s ", quoted);
		g_free(quoted);

	}
	/* Binary followed by an space */
	g_string_append_printf(job->cmd_line, "%s ", gebr_geoxml_program_get_binary(GEBR_GEOXML_PROGRAM(program)));
	if (job_add_program_parameters(job, GEBR_GEOXML_PROGRAM(program)) == FALSE)
		goto err;
	/* check for error file output */
	if (has_error_output_file && gebr_geoxml_program_get_stderr(GEBR_GEOXML_PROGRAM(program))) {
		if (check_for_write_permission(gebr_geoxml_flow_io_get_error(flow))) {
			g_string_append_printf(job->issues,
					       _("Write permission to %s not granted.\n"),
					       gebr_geoxml_flow_io_get_error(flow));
			goto err;
		}

		g_string_append_printf(job->cmd_line, "2>> \"%s\" ", gebr_geoxml_flow_io_get_error(flow));
	}

	/*
	 * Other programs
	 */
	previous_stdout = gebr_geoxml_program_get_stdout(GEBR_GEOXML_PROGRAM(program));
	gebr_geoxml_sequence_next(&program);
	while (program != NULL) {
		/* Skiping disabled/unconfigured programs */
		if (gebr_geoxml_program_get_status(GEBR_GEOXML_PROGRAM(program)) != GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED) {
			g_string_append_printf(job->issues, _("%u) Skiping disabled/unconfigured program '%s'.\n"),
					       ++issue_number,
					       gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(program)));

			gebr_geoxml_sequence_next(&program);
			continue;
		}

		/* How to connect chainned programs */
		int chain_option = gebr_geoxml_program_get_stdin(GEBR_GEOXML_PROGRAM(program)) + (previous_stdout << 1);
		switch (chain_option) {
		case 0:	/* Previous does not write to stdin and current does not carry about */
			g_string_append_printf(job->cmd_line, "; %s ",
					       gebr_geoxml_program_get_binary(GEBR_GEOXML_PROGRAM(program)));
			break;
		case 1:	/* Previous does not write to stdin but current expect something */
			g_string_append_printf(job->issues, _("Broken flow before %s (no input).\n"),
					       gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(program)));
			goto err;
		case 2:	/* Previous does write to stdin but current does not carry about */
			g_string_append_printf(job->issues, _("Broken flow before %s (unexpected output).\n"),
					       gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(program)));
			goto err;
		case 3:	/* Both talk to each other */
			g_string_append_printf(job->cmd_line, "| %s ",
					       gebr_geoxml_program_get_binary(GEBR_GEOXML_PROGRAM(program)));
			break;
		default:
			break;
		}

		if (job_add_program_parameters(job, GEBR_GEOXML_PROGRAM(program)) == FALSE)
			goto err;
		if (has_error_output_file && gebr_geoxml_program_get_stderr(GEBR_GEOXML_PROGRAM(program))){

			quoted = g_shell_quote(gebr_geoxml_flow_io_get_error(flow));
			g_string_append_printf(job->cmd_line, "2>> %s ", quoted);
			g_free(quoted);
		}

		previous_stdout = gebr_geoxml_program_get_stdout(GEBR_GEOXML_PROGRAM(program));
		gebr_geoxml_sequence_next(&program);
	}

	if (has_error_output_file == FALSE)
		g_string_append_printf(job->issues,
				       _("No error file selected; error output merged with standard output.\n"));
	if (previous_stdout) {
		if (strlen(gebr_geoxml_flow_io_get_output(flow)) == 0)
			g_string_append_printf(job->issues, _("Proceeding without output file.\n"));
		else {
			if (check_for_write_permission(gebr_geoxml_flow_io_get_output(flow))) {
				g_string_append_printf(job->issues,
						       _("Write permission to %s not granted.\n"),
						       gebr_geoxml_flow_io_get_output(flow));
				goto err;
			}

			quoted = g_shell_quote(gebr_geoxml_flow_io_get_output(flow));
			g_string_append_printf(job->cmd_line, ">%s", quoted);
			g_free(quoted);
		}
	}

	/* success exit */
	success = TRUE;
	goto out;

 err:	g_string_assign(job->jid, "0");
	job_set_status(job, JOB_STATUS_FAILED);
	g_string_assign(job->cmd_line, "");
	success = FALSE;

 out:
	/* hostname and flow title */
	g_string_assign(job->title, gebr_geoxml_document_get_title(document));
	/* set the start date */
	g_string_assign(job->start_date, gebr_iso_date());

	return success;
}

void job_free(struct job *job)
{
	GList *link;

	link = g_list_find(gebrd.jobs, job);
	/* job that failed to run are not added to this list */
	if (link != NULL)
		gebrd.jobs = g_list_delete_link(gebrd.jobs, link);

	/* free data */
	gebr_comm_process_free(job->process);
	gebr_geoxml_document_free(GEBR_GEOXML_DOC(job->flow));
	g_string_free(job->hostname, TRUE);
	g_string_free(job->status_string, TRUE);
	g_string_free(job->jid, TRUE);
	g_string_free(job->title, TRUE);
	g_string_free(job->start_date, TRUE);
	g_string_free(job->finish_date, TRUE);
	g_string_free(job->issues, TRUE);
	g_string_free(job->cmd_line, TRUE);
	g_string_free(job->output, TRUE);
	g_string_free(job->moab_jid, TRUE);
	g_string_free(job->queue, TRUE);
	
	if (gebrd_get_server_type() == GEBR_COMM_SERVER_TYPE_MOAB) {
		if (job->tail_process != NULL)
			gebr_comm_process_free(job->tail_process);
	}
	g_free(job);
}

void job_run_flow(struct job *job)
{
	GString *cmd_line;
	GebrGeoXmlSequence *program;
	gchar *locale_str;
	gsize __attribute__ ((unused)) bytes_written;
	gchar * quoted;
	gboolean may_run;

	/* initialization */
	may_run = TRUE;
	cmd_line = g_string_new(NULL);
	locale_str = g_filename_from_utf8(job->cmd_line->str, -1, NULL, &bytes_written, NULL);

	/* command-line */
	if (job->display->len) {
		GString *to_quote;

		to_quote = g_string_new(NULL);
		if (job->server_location == GEBR_COMM_SERVER_LOCATION_LOCAL){
			g_string_printf(to_quote, "export DISPLAY=%s; %s", job->display->str, locale_str);
			quoted = g_shell_quote(to_quote->str);
			g_string_printf(cmd_line, "bash -l -c %s", quoted);
			g_free(quoted);
		}
		else{
			g_string_printf(to_quote, "export DISPLAY=127.0.0.1%s; %s", job->display->str, locale_str);
			quoted = g_shell_quote(to_quote->str);
			g_string_printf(cmd_line, "bash -l -c %s", quoted);
			g_free(quoted);
		}

		g_string_free(to_quote, TRUE);
	} else {
		quoted = g_shell_quote(locale_str);
		g_string_printf(cmd_line, "bash -l -c %s", quoted);
		g_free(quoted);
	}

	/* we don't know yet the status */
	job_set_status(job, JOB_STATUS_UNKNOWN);

	if (gebrd_get_server_type() == GEBR_COMM_SERVER_TYPE_MOAB) {
		gchar * script;
		gchar * moab_quoted;
		gchar * standard_output, * standard_error;
		gint exit_status;
		GError * error = NULL;
		GString * moab_script;

		script = g_shell_quote(cmd_line->str);
		moab_script = g_string_new(NULL);
		g_string_printf(moab_script, "echo %s | msub -A '%s' -q '%s' -k oe -j oe", script, job->moab_account->str, job->queue->str);
		moab_quoted = g_shell_quote(moab_script->str);
		g_string_printf(cmd_line, "bash -l -c %s", moab_quoted);
		if (!g_spawn_command_line_sync(cmd_line->str, &standard_output, &standard_error, &exit_status, &error)) {
			may_run = FALSE;
			job_set_status(job, JOB_STATUS_FAILED);
			g_string_append_printf(job->issues, _("Cannot submit job to MOAB server.\n"));
			goto out;
		}
		if (standard_error != NULL && strlen(standard_error)) {
			may_run = FALSE;
			job_set_status(job, JOB_STATUS_FAILED);
			g_string_append_printf(job->issues, _("Cannot submit job to MOAB server: %s.\n"), standard_error);
			goto out2;
		}

		/* The stdout is the {job id} from MOAB. */
		guint moab_id = 0;
		sscanf(standard_output, "%u", &moab_id);
		if (!moab_id) {
			may_run = FALSE;
			job_set_status(job, JOB_STATUS_FAILED);
			g_string_append_printf(job->issues, _("Cannot get MOAB job id.\n"));
			goto out2;
		}
		g_string_printf(job->moab_jid, "%u", moab_id);

		/* run command to get script output */
		g_string_printf(cmd_line, "bash -c \"touch $HOME/STDIN.o%s; tail -f $HOME/STDIN.o%s\"", job->moab_jid->str, job->moab_jid->str);
		job->tail_process = gebr_comm_process_new();
		g_signal_connect(job->tail_process, "ready-read-stdout", G_CALLBACK(moab_process_read_stdout), job);
		gebr_comm_process_start(job->tail_process, cmd_line);

		/* pool for moab status */
		job_notify_status(job, JOB_STATUS_QUEUED, "");
		g_timeout_add(1000, (GSourceFunc)job_moab_checkjob_pooling, job); 

out2:		g_free(standard_output);
		g_free(standard_error);
out:		g_free(moab_quoted);
		g_string_free(moab_script, TRUE);
		g_free(script);
	} else {
		g_signal_connect(job->process, "ready-read-stdout", G_CALLBACK(job_process_read_stdout), job);
		g_signal_connect(job->process, "ready-read-stderr", G_CALLBACK(job_process_read_stderr), job);
		g_signal_connect(job->process, "finished", G_CALLBACK(job_process_finished), job);

		gebr_comm_process_start(job->process, cmd_line);
		job_notify_status(job, JOB_STATUS_RUNNING, "");

		/* for program that waits stdin EOF (like sfmath) */
		gebr_geoxml_flow_get_program(job->flow, &program, 0);
		if (gebr_geoxml_program_get_stdin(GEBR_GEOXML_PROGRAM(program)) == FALSE)
			gebr_comm_process_close_stdin(job->process);
	}

	if (may_run) {
		gebrd_message(GEBR_LOG_DEBUG, "Client '%s' flow about to run: %s",
			      job->hostname->str, cmd_line->str);
		gebrd.jobs = g_list_append(gebrd.jobs, job);
	}
	
	/* frees */
	g_string_free(cmd_line, TRUE);
	g_free(locale_str);
}

struct job *job_find(GString * jid)
{
	GList *link;
	struct job *job;

	job = NULL;
	link = gebrd.jobs;
	while (link != NULL) {
		struct job *i;

		i = (struct job *)link->data;
		if (!strcmp(i->jid->str, jid->str)) {
			job = i;
			break;
		}

		link = g_list_next(link);
	}

	return job;
}

void job_clear(struct job *job)
{
	if (gebr_comm_process_is_running(job->process) == FALSE)
		job_free(job);
}

void job_end(struct job *job)
{
	if (gebrd_get_server_type() == GEBR_COMM_SERVER_TYPE_REGULAR) {

		if (job->status == JOB_STATUS_QUEUED){
			gebrd_queues_remove_job_from(job->queue->str, job);
			job_notify_status(job, JOB_STATUS_CANCELED, job->finish_date->str);
		} else {
			job->user_finished = TRUE;
			gebr_comm_process_terminate(job->process);
		}
	} else if (gebrd_get_server_type() == GEBR_COMM_SERVER_TYPE_MOAB)
		job_send_signal_on_moab("SIGTERM", job);
}

void job_kill(struct job *job)
{
	if (gebrd_get_server_type() == GEBR_COMM_SERVER_TYPE_REGULAR) {
		job->user_finished = TRUE;
		gebr_comm_process_kill(job->process);
	} else if (gebrd_get_server_type() == GEBR_COMM_SERVER_TYPE_MOAB)
		job_send_signal_on_moab("SIGKILL", job);
}

void job_notify(struct job *job, struct client *client)
{
	gebr_comm_protocol_send_data(client->protocol, client->stream_socket, gebr_comm_protocol_defs.job_def,
				     11, job->jid->str, job->status_string->str, job->title->str, job->start_date->str,
				     job->finish_date->str, job->hostname->str, job->issues->str,
				     job->cmd_line->str, job->output->str, job->queue->str, job->moab_jid->str);
}

void job_list(struct client *client)
{
	GList *link;

	link = g_list_first(gebrd.jobs);
	while (link != NULL) {
		struct job *job;

		job = (struct job *)link->data;
		job_notify(job, client);

		link = g_list_next(link);
	}
}

void job_send_clients_job_notify(struct job *job)
{
	GList *link;

	link = g_list_first(gebrd.clients);
	while (link != NULL) {
		struct client *client;

		client = (struct client *)link->data;
		job_notify(job, client);

		link = g_list_next(link);
	}
}

/* Several tests to ensure flow executability */
/* All of them return TRUE upon error */
static gboolean check_for_readable_file(const gchar * file)
{
	return (g_access(file, F_OK | R_OK) == -1 ? TRUE : FALSE);
}

static gboolean check_for_write_permission(const gchar * file)
{
	gboolean ret;
	gchar *dir;

	dir = g_path_get_dirname(file);
	ret = (g_access(dir, W_OK) == -1 ? TRUE : FALSE);
	g_free(dir);

	return ret;
}

static gboolean check_for_binary(const gchar * binary)
{
	return (g_find_program_in_path(binary) == NULL ? TRUE : FALSE);
}

/**
 * \internal
 * Send a \p signal to a given \p job on MOAB cluster.
 */
static void job_send_signal_on_moab(const char * signal, struct job * job)
{
	GString *cmd_line = NULL;
	gchar *std_out = NULL;
	gchar *std_err = NULL;
	gint exit_status;
	cmd_line = g_string_new("");
	g_string_printf(cmd_line, "mjobctl -M signal=%s %s", signal, job->moab_jid->str);
	if (g_spawn_command_line_sync(cmd_line->str, &std_out, &std_err, &exit_status, NULL) == FALSE){
		g_string_append_printf(job->issues, _("Cannot cancel job at MOAB server.\n"));
		goto err;
	}
	if (std_err != NULL && strlen(std_err)) {
		g_string_append_printf(job->issues, _("Cannot cancel job at MOAB server.\n"));
		goto err1;
	}
	if (job->status != JOB_STATUS_QUEUED)
		job->user_finished = TRUE;

err1:	g_free(std_out);
	g_free(std_err);
err:	g_string_free(cmd_line, TRUE);

}
