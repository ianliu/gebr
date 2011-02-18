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

#include <glib/gi18n.h>
#include <libgebr/comm/gebr-comm-protocol.h>
#include <libgebr/comm/gebr-comm-streamsocket.h>
#include <libgebr/comm/gebr-comm-socketaddress.h>
#include <libgebr/utils.h>
#include <libgebr/date.h>

#include "job.h"
#include "gebrd.h"
#include "queues.h"
#include "mpi-implementations.h"

static gboolean job_parse_parameters(struct job *job, GebrGeoXmlParameters * parameters, GebrGeoXmlProgram * program);

static void job_process_finished(GebrCommProcess * process, struct job *job);

static gboolean check_for_readable_file(const gchar * file);

static gboolean check_for_write_permission(const gchar * file);

//static gboolean check_for_binary(const gchar * binary);

static void job_send_signal_on_moab(const char * signal, struct job * job);

static GebrdMpiInterface * job_get_mpi_impl(const gchar * mpi_name, GString * n_process);

/**
 * \internal
 */
static void job_issue(struct job *job, const gchar *message, ...)
{
	va_list argp;
	va_start(argp, message);
	gchar *issue = g_strdup_vprintf(message, argp);
	va_end(argp);

	g_string_append(job->issues, issue);
	if (job->status != JOB_STATUS_INITIAL)
		job_status_notify(job, JOB_STATUS_ISSUED, issue);

	g_free(issue);
}

/**
 * \internal
 */
static gboolean job_parse_parameter(struct job *job, GebrGeoXmlParameter * parameter, GebrGeoXmlProgram * program)
{
	GebrGeoXmlParameterType type;
	GebrGeoXmlProgramParameter *program_parameter;

	type = gebr_geoxml_parameter_get_type(parameter);
	if (type == GEBR_GEOXML_PARAMETER_TYPE_GROUP) {
		GebrGeoXmlSequence *instance;
		gboolean ret;

		gebr_geoxml_parameter_group_get_instance(GEBR_GEOXML_PARAMETER_GROUP(parameter), &instance, 0);
		for (ret = TRUE; instance != NULL && ret == TRUE; gebr_geoxml_sequence_next(&instance)) {
			GebrGeoXmlParameter *selected;

			/* for an exclusive instance */
			selected = gebr_geoxml_parameters_get_selection(GEBR_GEOXML_PARAMETERS(instance));
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
				gchar *quoted = g_shell_quote(value->str);
				g_string_append_printf(job->cmd_line, "%s%s ",
						       gebr_geoxml_program_parameter_get_keyword(program_parameter),
						       quoted);
				g_free(quoted);
			} else {
				/* Check if this is a required parameter */
				if (gebr_geoxml_program_parameter_get_required(program_parameter)) {
					job_issue(job,
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
		job_issue(job, _("Unknown parameter type.\n"));

		return FALSE;
	}

	return TRUE;
}

/**
 * \internal
 */
static gboolean job_parse_parameters(struct job *job, GebrGeoXmlParameters * parameters, GebrGeoXmlProgram * program)
{
	GebrGeoXmlSequence *parameter;

	parameter = gebr_geoxml_parameters_get_first_parameter(parameters);
	for (; parameter != NULL; gebr_geoxml_sequence_next(&parameter))
		if (job_parse_parameter(job, GEBR_GEOXML_PARAMETER(parameter), program) == FALSE)
			return FALSE;

	return TRUE;
}

/**
 * \internal
 */
static gboolean job_add_program_parameters(struct job *job, GebrGeoXmlProgram * program)
{
	return job_parse_parameters(job, gebr_geoxml_program_get_parameters(program), program);
}

/**
 * \internal
 */
static void job_send_clients_output(struct job *job, GString * output)
{
	/* FIXME: remove and find the real problem */
	if (!job->jid->len)
		return;

	for (GList *link = gebrd.clients; link != NULL; link = g_list_next(link)) {
		struct client *client = (struct client *)link->data;
		gebr_comm_protocol_send_data(client->protocol, client->stream_socket,
					     gebr_comm_protocol_defs.out_def, 2, job->jid->str, output->str);
	}
}

/**
 * \internal
 */
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


/**
 * \internal
 */
static void moab_process_read_stdout(GebrCommProcess *process, struct job *job)
{
	GString *stdout;
	stdout = gebr_comm_process_read_stdout_string_all(process);
	job_process_add_output(job, job->output, stdout);
	g_string_free(stdout, TRUE);
}

/**
 * \internal
 */
static void job_process_read_stdout(GebrCommProcess * process, struct job *job)
{
	GString *stdout;

	stdout = gebr_comm_process_read_stdout_string_all(process);
	job_process_add_output(job, job->output, stdout);

	g_string_free(stdout, TRUE);
}

/**
 * \internal
 */
static void job_process_read_stderr(GebrCommProcess * process, struct job *job)
{
	GString *stderr;

	stderr = gebr_comm_process_read_stderr_string_all(process);
	job_process_add_output(job, job->output, stderr);

	g_string_free(stderr, TRUE);
}

/*
 * \internal
 */
static const gchar *status_enum_to_string(enum JobStatus status)
{
	static const gchar * enum_to_string [] = {
		"unknown", "queued", "failed", "running", "finished", "canceled", "requeued", "issued", NULL };
	return enum_to_string[status];
}

/**
 * \internal
 */
static void job_status_notify_finished(struct job *job)
{
	enum JobStatus new_status = (job->user_finished == FALSE) ? JOB_STATUS_FINISHED : JOB_STATUS_CANCELED;
	if (new_status == job->status)
		return;

	if (gebrd_get_server_type() == GEBR_COMM_SERVER_TYPE_MOAB) {
		if (job->tail_process != NULL) {
			gebr_comm_process_free(job->tail_process);
			job->tail_process = NULL;
		}
	
		/* last change to get output */
		GString * output_file = g_string_new(NULL);
		g_string_printf(output_file, "%s/STDIN.o%s", g_get_home_dir(), job->moab_jid->str);
		GError * error = NULL;
		GIOChannel * file = g_io_channel_new_file(output_file->str, "r", &error);
		g_string_free(output_file, TRUE);
		error = NULL;
		if (file != NULL && g_io_channel_seek_position(file, job->output->len, G_SEEK_SET, &error) == G_IO_STATUS_NORMAL) {
			gchar * buffer;
			gsize length = 0;
			g_io_channel_read_to_end(file, &buffer, &length, &error);
			if (length > 0) {
				GString *buffer_gstring = g_string_new(buffer);
				job_process_add_output(job, job->output, buffer_gstring);
				g_string_free(buffer_gstring, TRUE);
				g_free(buffer);
			}
		}
		if (file != NULL)
			g_io_channel_shutdown(file, FALSE, &error);
	}

	g_string_assign(job->finish_date, gebr_iso_date());
	job_status_notify(job, new_status, job->finish_date->str);
}

/**
 * \internal
 * Only for regular jobs
 */
static void job_process_finished(GebrCommProcess * process, struct job *job)
{
	job_status_notify_finished(job);
}

/**
 * \internal
 */
static GString *job_generate_id(void)
{
	GString *jid;
	guint32 random;
	gboolean unique;

	unique = TRUE;
	jid = g_string_new(NULL);
	do {
		random = g_random_int();
		g_string_printf(jid, "%u", random);

		/* check if it is unique */
		for (GList *link = gebrd.jobs; link != NULL; link = g_list_next(link)) {
			struct job *job = (struct job *)link->data;
			if (!strcmp(job->jid->str, jid->str)) {
				unique = FALSE;
				break;
			}
		}
	} while (!unique);

	return jid;
}

/**
 * \internal
 */
static gboolean job_moab_checkjob_pooling(struct job * job)
{
	gboolean keep_polling = TRUE;
	gboolean error = FALSE;

	/* run checkjob and retrieve XML */
	GString *cmd_line = g_string_new("");
	g_string_printf(cmd_line, "checkjob %s --format=xml", job->moab_jid->str);
	gint exit_status;
	gchar *std_err = NULL, *std_out = NULL;
	error = !g_spawn_command_line_sync(cmd_line->str, &std_out, &std_err, &exit_status, NULL);
	g_string_free(cmd_line, TRUE);
	g_free(std_err);
	if (error == TRUE) {
		job_issue(job, _("Moab's job status could not be retrieved.\n"));
		g_free(std_out);
		goto out;
	}
	/* read XML */
	GdomeDOMImplementation *dom_impl = NULL;
	dom_impl = gdome_di_mkref();
	GdomeException exception;
	GdomeDocument *doc = gdome_di_createDocFromMemory(dom_impl, std_out, GDOME_LOAD_PARSING, &exception);
	g_free(std_out);
	if (doc == NULL) {
		gdome_di_unref(dom_impl, &exception);
		error = TRUE;
		goto out;
	}
	GdomeElement *root_element = gdome_doc_documentElement(doc, &exception);
	if (root_element == NULL) {
		error = TRUE;
		goto xmlout;
	}
	if (!strcmp(gdome_el_nodeName(root_element, &exception)->str, "Error")) {
		GdomeDOMString *attribute_name = gdome_str_mkref("Code");
		const gchar * code = gdome_el_getAttribute(root_element, attribute_name, &exception)->str;
		gdome_str_unref(attribute_name);
		GdomeNode *value = gdome_el_firstChild(root_element, &exception);
		if (value)
			job_issue(job, _("Moab's job status could not be returned with error code %s (%s).\n"),
					       code, gdome_n_nodeValue(value, &exception)->str);

		error = TRUE;
		goto xmlout;
	}
	/* check for Data root element and its job child */
	GdomeElement *job_element = (GdomeElement *)gdome_el_firstChild(root_element, &exception);
	GdomeElement *req_element = (GdomeElement *)gdome_el_firstChild(job_element, &exception);
	if (!(!strcmp(gdome_el_nodeName(root_element, &exception)->str, "Data") && job_element != NULL && req_element != NULL)) {
		job_issue(job, _("Moab's job status could not be retrieved.\n"));

		error = TRUE;
		goto xmlout;
	}

	error = FALSE;
	/* check requeue */
	GdomeDOMString *attribute_name = gdome_str_mkref("Class");
	const gchar * queue = gdome_el_getAttribute(job_element, attribute_name, &exception)->str;
	gdome_str_unref(attribute_name);
	/* check for queue change */
	if (strcmp(job->queue->str, queue)) {
		g_string_assign(job->queue, queue);
		job_status_notify(job, JOB_STATUS_REQUEUED, job->queue->str);
	}
	/* check for completion code error */
	attribute_name = gdome_str_mkref("CompletionCode");
	const gchar *CompletionCode = gdome_el_getAttribute(job_element, attribute_name, &exception)->str;
	gdome_str_unref(attribute_name);
	if (strlen(CompletionCode)) {
		error = TRUE;

		gint code = atoi(CompletionCode);
		if (code < 0) {
			attribute_name = gdome_str_mkref("AllocNodeList");
			const gchar *AllocNodeList = gdome_el_getAttribute(req_element, attribute_name, &exception)->str;
			gdome_str_unref(attribute_name);
			job_issue(job, _("Moab's job returned status code %d allocatted on nodes '%s'.\n"),
					  code, AllocNodeList);
		} else
			job_issue(job, _("Process exited with status code %d.\n"), code);

		goto xmlout;
	}
	/* change status */
	attribute_name = gdome_str_mkref("EState");
	const gchar *moab_status = gdome_el_getAttribute(job_element, attribute_name, &exception)->str;
	gdome_str_unref(attribute_name);
	if (!strcmp(moab_status, "Completed")) {
		job_status_notify_finished(job);
		keep_polling = FALSE;
	} else if (!strcmp(moab_status, "Running") ) {
		if (job->status != JOB_STATUS_RUNNING) {
			g_string_assign(job->start_date, gebr_iso_date());
			job_status_notify(job, JOB_STATUS_RUNNING, job->start_date->str);
		}
	} else
		gebrd_message(GEBR_LOG_WARNING, "Untreated state reported by checkjob (estate %s)", moab_status);

xmlout:
	gdome_doc_unref(doc, &exception);
	gdome_di_unref(dom_impl, &exception);
out:
	if (error) {
		keep_polling = FALSE;
		job_status_notify(job, JOB_STATUS_FAILED, "");
	}

	return keep_polling;
}

/**
 * \internal
 */
static void job_assembly_cmdline(struct job *job)
{
	gboolean has_error_output_file;
	gboolean previous_stdout;
	guint issue_number = 0;
	GebrGeoXmlSequence *program;
	GebrdMpiInterface * mpi;
	gulong nprog;

	if (job->flow == NULL) 
		goto err;

	has_error_output_file = strlen(gebr_geoxml_flow_io_get_error(job->flow)) ? TRUE : FALSE;
	nprog = gebr_geoxml_flow_get_programs_number(job->flow);
	if (nprog == 0) {
		job_issue(job, _("Empty flow.\n"));
		goto err;
	}

	/* Check if there is configured programs */
	gebr_geoxml_flow_get_program(job->flow, &program, 0);
	while (program != NULL &&
	       gebr_geoxml_program_get_status(GEBR_GEOXML_PROGRAM(program)) != GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED) {
		job_issue(job, _("%u) Skipping disabled/not configured program '%s'.\n"),
				       ++issue_number, gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(program)));

		gebr_geoxml_sequence_next(&program);
	}
	if (program == NULL) {
		job_issue(job, _("No configured programs.\n"));
		goto err;
	}

	/*
	 * First program
	 */
	/* Start without stdin */
	if (gebr_geoxml_program_get_stdin(GEBR_GEOXML_PROGRAM(program))) {
		if (strlen(gebr_geoxml_flow_io_get_input(job->flow)) == 0) {
			job_issue(job, _("No input file selected.\n"));
			goto err;
		}

		gchar * quoted = g_shell_quote(gebr_geoxml_flow_io_get_input(job->flow));
		g_string_append_printf(job->cmd_line, "<%s ", quoted);
		g_free(quoted);

	}

	/* Configure MPI */
	const gchar * mpiname;
	mpiname = gebr_geoxml_program_get_mpi(GEBR_GEOXML_PROGRAM(program));
	mpi = job_get_mpi_impl(mpiname, job->n_process);
	if (strlen(mpiname) && !mpi) {
		job_issue(job,
				       _("Requested MPI (%s) is not supported by this server.\n"), mpiname);
		goto err;
	}

	/* Binary followed by an space */
	const gchar * binary;
	binary = gebr_geoxml_program_get_binary(GEBR_GEOXML_PROGRAM(program));
	if (mpi == NULL)
		g_string_append_printf(job->cmd_line, "%s ", binary);
	else {
		gchar * mpicmd;
		mpicmd = gebrd_mpi_interface_build_comand(mpi, binary);
		g_string_append_printf(job->cmd_line, "%s ", mpicmd);
		g_free(mpicmd);
	}

	if (job_add_program_parameters(job, GEBR_GEOXML_PROGRAM(program)) == FALSE)
		goto err;
	/* check for error file output */
	if (has_error_output_file && gebr_geoxml_program_get_stderr(GEBR_GEOXML_PROGRAM(program))) {
		g_string_append_printf(job->cmd_line, "2>> \"%s\" ", gebr_geoxml_flow_io_get_error(job->flow));
	}

	/*
	 * Other programs
	 */
	previous_stdout = gebr_geoxml_program_get_stdout(GEBR_GEOXML_PROGRAM(program));
	gebr_geoxml_sequence_next(&program);
	while (program != NULL) {
		/* Skipping disabled/not configured programs */
		if (gebr_geoxml_program_get_status(GEBR_GEOXML_PROGRAM(program)) != GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED) {
			job_issue(job, _("%u) Skipping disabled/not configured program '%s'.\n"),
					       ++issue_number,
					       gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(program)));

			gebr_geoxml_sequence_next(&program);
			continue;
		}

		mpi = job_get_mpi_impl(gebr_geoxml_program_get_mpi(GEBR_GEOXML_PROGRAM(program)),
				       job->n_process);

		/* How to connect chained programs */
		int chain_option = gebr_geoxml_program_get_stdin(GEBR_GEOXML_PROGRAM(program)) + (previous_stdout << 1);
		switch (chain_option) {
		case 0:	{
			/* Previous does not write to stdin and current does not carry about */

			const gchar * binary;

			binary = gebr_geoxml_program_get_binary(GEBR_GEOXML_PROGRAM(program));
			if (mpi == NULL)
				g_string_append_printf(job->cmd_line, "; %s ", binary);
			else {
				gchar * mpicmd;
				mpicmd = gebrd_mpi_interface_build_comand(mpi, binary);
				g_string_append_printf(job->cmd_line, "; %s ", mpicmd);
				g_free(mpicmd);
			}
			break;
		}
		case 1:	/* Previous does not write to stdin but current expect something */
			job_issue(job, _("Broken flow before %s (no input).\n"),
					       gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(program)));
			goto err;
		case 2:	/* Previous does write to stdin but current does not carry about */
			job_issue(job, _("Broken flow before %s (unexpected output).\n"),
					       gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(program)));
			goto err;
		case 3:	{
			/* Both talk to each other */

			const gchar * binary;

			binary = gebr_geoxml_program_get_binary(GEBR_GEOXML_PROGRAM(program));

			if (mpi == NULL)
				g_string_append_printf(job->cmd_line, "| %s ", binary);
			else {
				gchar * mpicmd;
				mpicmd = gebrd_mpi_interface_build_comand(mpi, binary);
				g_string_append_printf(job->cmd_line, "| %s ", mpicmd);
				g_free(mpicmd);
			}

			break;
		}
		default:
			break;
		}

		if (job_add_program_parameters(job, GEBR_GEOXML_PROGRAM(program)) == FALSE)
			goto err;
		if (has_error_output_file && gebr_geoxml_program_get_stderr(GEBR_GEOXML_PROGRAM(program))) {
			gchar * quoted = g_shell_quote(gebr_geoxml_flow_io_get_error(job->flow));
			g_string_append_printf(job->cmd_line, "2>> %s ", quoted);
			g_free(quoted);
		}

		previous_stdout = gebr_geoxml_program_get_stdout(GEBR_GEOXML_PROGRAM(program));
		gebr_geoxml_sequence_next(&program);
	}

	if (has_error_output_file == FALSE)
		job_issue(job,
				       _("No error file selected; error output merged with standard output.\n"));
	if (previous_stdout) {
		if (strlen(gebr_geoxml_flow_io_get_output(job->flow)) == 0)
			job_issue(job, _("Proceeding without output file.\n"));
		else {
			gchar * quoted = g_shell_quote(gebr_geoxml_flow_io_get_output(job->flow));
			g_string_append_printf(job->cmd_line, ">%s", quoted);
			g_free(quoted);
		}
	}

	job->critical_error = FALSE;
	return;
err:	
	g_string_assign(job->cmd_line, "");
	job->critical_error = TRUE;
}


struct job *job_find(GString * jid)
{
	struct job *job;

	job = NULL;
	for (GList *link = gebrd.jobs; link != NULL; link = g_list_next(link)) {
		struct job *i = (struct job *)link->data;
		if (!strcmp(i->jid->str, jid->str)) {
			job = i;
			break;
		}
	}

	return job;
}

void job_new(struct job ** _job, struct client * client, GString * queue, GString * account, GString * xml,
	     GString * n_process, GString * run_id)
{
	/* initialization */
	struct job *job = g_new(struct job, 1);
	job->process = gebr_comm_process_new();
	job->tail_process = NULL;
	job->flow = NULL;
	job->critical_error = FALSE;
	job->user_finished = FALSE;
	job->hostname = g_string_new(client->protocol->hostname->str);
	job->display = g_string_new(client->display->str);
	job->server_location = client->server_location;
	job->status_string = g_string_new("");
	job->run_id = g_string_new(run_id->str);
	job->jid = job_generate_id();
	job->title = g_string_new("");
	job->start_date = g_string_new("");
	job->finish_date = g_string_new("");
	job->issues = g_string_new("");
	job->cmd_line = g_string_new("");
	job->output = g_string_new("");
	job->queue  = g_string_new(queue->str);
	job->moab_account = g_string_new(account->str);
	job->moab_jid = g_string_new("");
	job->n_process = g_string_new(n_process->str);
	job->status = JOB_STATUS_INITIAL;

	*_job = job;
	gebrd.jobs = g_list_append(gebrd.jobs, job);

	GebrGeoXmlDocument *document;
	int ret = gebr_geoxml_document_load_buffer(&document, xml->str);
	job->flow = GEBR_GEOXML_FLOW(document);
	if (job->flow == NULL)
		job_issue(job, gebr_geoxml_error_explained_string(ret));
	else
		g_string_assign(job->title, gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(job->flow)));

	/* just to send the client the command line, we could do this after by changing the protocol */
	job_assembly_cmdline(job);
}

void job_free(struct job *job)
{
	gebrd_queues_remove_job_from(job->queue->str, job);

	GList *link = g_list_find(gebrd.jobs, job);
	/* job that failed to run are not added to this list */
	if (link != NULL)
		gebrd.jobs = g_list_delete_link(gebrd.jobs, link);

	/* free data */
	gebr_comm_process_free(job->process);
	if (gebrd_get_server_type() == GEBR_COMM_SERVER_TYPE_MOAB)
		if (job->tail_process != NULL)
			gebr_comm_process_free(job->tail_process);
	if (job->flow)
		gebr_geoxml_document_free(GEBR_GEOXML_DOC(job->flow));
	g_string_free(job->hostname, TRUE);
	g_string_free(job->status_string, TRUE);
	g_string_free(job->run_id, TRUE);
	g_string_free(job->jid, TRUE);
	g_string_free(job->title, TRUE);
	g_string_free(job->start_date, TRUE);
	g_string_free(job->finish_date, TRUE);
	g_string_free(job->issues, TRUE);
	g_string_free(job->cmd_line, TRUE);
	g_string_free(job->output, TRUE);
	g_string_free(job->moab_jid, TRUE);
	g_string_free(job->queue, TRUE);
	g_string_free(job->n_process, TRUE);
	g_free(job);
}

void job_status_set(struct job *job, enum JobStatus status)
{
	if (job->status == status) {
		//occurs frequently with netuno
		//gebrd_message(GEBR_LOG_DEBUG, "Calling job_status_set without status change (status %d)", job->status);
		return;
	}
	/* false statuses */
	if (status == JOB_STATUS_REQUEUED || status == JOB_STATUS_ISSUED)
		return;

	g_string_assign(job->status_string, status_enum_to_string(status));
	enum JobStatus old_status = job->status;
	job->status = status;

	if (job->status == JOB_STATUS_FAILED
	    || job->status == JOB_STATUS_FINISHED
	    || (job->status == JOB_STATUS_CANCELED && old_status == JOB_STATUS_RUNNING)) {
		if (gebrd_queues_has_next(job->queue->str))
			gebrd_queues_step_queue(job->queue->str);
		else
			gebrd_queues_set_queue_busy(job->queue->str, FALSE);
	}
}

void job_status_notify(struct job *job, enum JobStatus status, const gchar *_parameter, ...)
{
	va_list argp;
	va_start(argp, _parameter);
	gchar *parameter = g_strdup_vprintf(_parameter, argp);
	va_end(argp);

	job_status_set(job, status);

	/* warn all clients of the new status */
	for (GList *link = gebrd.clients; link != NULL; link = g_list_next(link)) {
		struct client *client = (struct client *)link->data;
		gebr_comm_protocol_send_data(client->protocol, client->stream_socket, gebr_comm_protocol_defs.sta_def,
					     3, job->jid->str, status_enum_to_string(status), parameter);
	}

	g_free(parameter);
}

void job_run_flow(struct job *job)
{
	GString *cmd_line;
	guint issue_number = 0;

	/* initialization */
	cmd_line = g_string_new(NULL);

	if (job->critical_error == TRUE)
		goto err;

	/* Check if there is configured programs */
	GebrGeoXmlSequence *program;
	gebr_geoxml_flow_get_program(job->flow, &program, 0);
	while (program != NULL &&
	       gebr_geoxml_program_get_status(GEBR_GEOXML_PROGRAM(program)) != GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED) {
		job_issue(job, _("%u) Skipping disabled/not configured program '%s'.\n"),
			  ++issue_number, gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(program)));

		gebr_geoxml_sequence_next(&program);
	}
	if (program == NULL)
		goto err;

	/*
	 * First program
	 */
	/* Start with/without stdin */
	if (gebr_geoxml_program_get_stdin(GEBR_GEOXML_PROGRAM(program))) {
		/* Input file */
		if (check_for_readable_file(gebr_geoxml_flow_io_get_input(job->flow))) {
			job_issue(job, _("Input file %s not present or not accessible.\n"),
				  gebr_geoxml_flow_io_get_input(job->flow));
			goto err;
		}
	}

	/* check for error file output */
	if (gebr_geoxml_program_get_stderr(GEBR_GEOXML_PROGRAM(program))) {
		if (check_for_write_permission(gebr_geoxml_flow_io_get_error(job->flow))) {
			job_issue(job, _("Write permission to %s not granted.\n"),
				  gebr_geoxml_flow_io_get_error(job->flow));
			goto err;
		}
	}

	/* check for output file */
	if (gebr_geoxml_program_get_stdout(GEBR_GEOXML_PROGRAM(program))) {
		if (check_for_write_permission(gebr_geoxml_flow_io_get_output(job->flow))) {
			job_issue(job, _("Write permission to %s not granted.\n"),
				  gebr_geoxml_flow_io_get_output(job->flow));
			goto err;
		}
	}

	/* command-line */
	gsize bytes_written;
	gchar *localized_cmd_line = g_filename_from_utf8(job->cmd_line->str, -1, NULL, &bytes_written, NULL);
	if (job->display->len) {
		GString *to_quote;

		to_quote = g_string_new(NULL);
		if (job->server_location == GEBR_COMM_SERVER_LOCATION_LOCAL) {
			g_string_printf(to_quote, "export DISPLAY=%s; %s", job->display->str, localized_cmd_line);
			gchar * quoted = g_shell_quote(to_quote->str);
			g_string_printf(cmd_line, "bash -l -c %s", quoted);
			g_free(quoted);
		}
		else{
			g_string_printf(to_quote, "export DISPLAY=127.0.0.1%s; %s", job->display->str, localized_cmd_line);
			gchar * quoted = g_shell_quote(to_quote->str);
			g_string_printf(cmd_line, "bash -l -c %s", quoted);
			g_free(quoted);
		}

		g_string_free(to_quote, TRUE);
	} else {
		gchar * quoted = g_shell_quote(localized_cmd_line);
		g_string_printf(cmd_line, "bash -l -c %s", quoted);
		g_free(quoted);
	}
	g_free(localized_cmd_line);

	if (gebrd_get_server_type() == GEBR_COMM_SERVER_TYPE_MOAB) {
		if (!g_find_program_in_path("msub")) {
			job_issue(job, _("Cannot submit job to MOAB server (msub command is not available).\n"));
			goto err;
		}

		GString * moab_script = g_string_new(NULL);
		gchar * script = g_shell_quote(cmd_line->str);
		g_string_printf(moab_script, "echo %s | msub -A '%s' -q '%s' -k oe -j oe",
			       	script, job->moab_account->str, job->queue->str);
		g_free(script);
		gchar * moab_quoted = g_shell_quote(moab_script->str);
		g_string_free(moab_script, TRUE);
		g_string_printf(cmd_line, "bash -l -c %s", moab_quoted);
		g_free(moab_quoted);
		GError * error = NULL;
		gint exit_status;
		gchar * standard_output = NULL, * standard_error = NULL;
		if (!g_spawn_command_line_sync(cmd_line->str, &standard_output, &standard_error, &exit_status, &error)) {
			job_issue(job, _("Cannot submit job to MOAB server.\n"));
			g_free(standard_output);
			g_free(standard_error);
			goto err;
		}
		if (standard_error != NULL && strlen(standard_error)) {
			job_issue(job, _("Cannot submit job to MOAB server: %s.\n"), standard_error);
			g_free(standard_output);
			g_free(standard_error);
			goto err;
		}
		g_free(standard_error);

		/* The stdout is the {job id} from MOAB. */
		guint moab_id = 0;
		sscanf(standard_output, "%u", &moab_id);
		g_free(standard_output);
		if (!moab_id) {
			job_issue(job, _("Cannot get MOAB job id.\n"));
			goto err;
		}
		g_string_printf(job->moab_jid, "%u", moab_id);

		/* run command to get script output */
		g_string_printf(cmd_line, "bash -c \"touch $HOME/STDIN.o%s; tail -f $HOME/STDIN.o%s\"", job->moab_jid->str, job->moab_jid->str);
		job->tail_process = gebr_comm_process_new();
		g_signal_connect(job->tail_process, "ready-read-stdout", G_CALLBACK(moab_process_read_stdout), job);
		gebr_comm_process_start(job->tail_process, cmd_line);

		/* pool for moab status */
		g_timeout_add(1000, (GSourceFunc)job_moab_checkjob_pooling, job); 
	} else {
		g_signal_connect(job->process, "ready-read-stdout", G_CALLBACK(job_process_read_stdout), job);
		g_signal_connect(job->process, "ready-read-stderr", G_CALLBACK(job_process_read_stderr), job);
		g_signal_connect(job->process, "finished", G_CALLBACK(job_process_finished), job);

		g_string_assign(job->start_date, gebr_iso_date());
		job_status_notify(job, JOB_STATUS_RUNNING, job->start_date->str);
		gebr_comm_process_start(job->process, cmd_line);

		/* for program that waits stdin EOF (like sfmath) */
		gebr_geoxml_flow_get_program(job->flow, &program, 0);
		if (gebr_geoxml_program_get_stdin(GEBR_GEOXML_PROGRAM(program)) == FALSE)
			gebr_comm_process_close_stdin(job->process);
	}

	/* success */
	gebrd_message(GEBR_LOG_DEBUG, "Client '%s' flow about to run %s: %s",
		      job->hostname->str, job->title->str, cmd_line->str);
	goto out;

err:	/* error */
	job_status_notify(job, JOB_STATUS_FAILED, "");

out:	
	/* frees */
	g_string_free(cmd_line, TRUE);
}

void job_clear(struct job *job)
{
	/* NOTE: changes here must reflect changes in job_close at gebr */
	if (!(job->status == JOB_STATUS_RUNNING || job->status == JOB_STATUS_QUEUED))
		job_free(job);
}

void job_end(struct job *job)
{
	if (gebrd_get_server_type() == GEBR_COMM_SERVER_TYPE_REGULAR) {
		if (job->status == JOB_STATUS_QUEUED) {
			gebrd_queues_remove_job_from(job->queue->str, job);
			g_string_assign(job->finish_date, gebr_iso_date());
			job_status_notify(job, JOB_STATUS_CANCELED, job->finish_date->str);
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
		if (job->status == JOB_STATUS_QUEUED) {
			gebrd_queues_remove_job_from(job->queue->str, job);
			g_string_assign(job->finish_date, gebr_iso_date());
			job_status_notify(job, JOB_STATUS_CANCELED, job->finish_date->str);
		} else {
			job->user_finished = TRUE;
			gebr_comm_process_kill(job->process);
		}
	} else if (gebrd_get_server_type() == GEBR_COMM_SERVER_TYPE_MOAB)
		job_send_signal_on_moab("SIGKILL", job);
}

void job_notify(struct job *job, struct client *client)
{
	if (job->status == JOB_STATUS_INITIAL)
		job_status_set(job, JOB_STATUS_QUEUED);

	gebr_comm_protocol_send_data(client->protocol, client->stream_socket, gebr_comm_protocol_defs.job_def,
				     11, job->jid->str, job->status_string->str, job->title->str, job->start_date->str,
				     job->finish_date->str, job->hostname->str, job->issues->str,
				     job->cmd_line->str, job->output->str, job->queue->str, job->moab_jid->str);
}


void job_list(struct client *client)
{
	for (GList *link = gebrd.jobs; link != NULL; link = g_list_next(link)) {
		struct job *job = (struct job *)link->data;
		job_notify(job, client);
	}
}

void job_send_clients_job_notify(struct job *job)
{
	for (GList *link = gebrd.clients; link != NULL; link = g_list_next(link)) {
		struct client *client = (struct client *)link->data;
		job_notify(job, client);
	}
}

/**
 * \internal
 * Several tests to ensure flow executability 
 * All of them return TRUE upon error
 */
static gboolean check_for_readable_file(const gchar * file)
{
	return (g_access(file, F_OK | R_OK) == -1 ? TRUE : FALSE);
}

/**
 * \internal
 */
static gboolean check_for_write_permission(const gchar * file)
{
	gboolean ret;
	gchar *dir;

        /* Test if the file exists */
        if ( g_access(file, F_OK) == 0 )
                /* The file exists, but is it writeable ? */
                return (g_access(file, W_OK) == 0 ? FALSE : TRUE);

        /* Otherwise test for directory permissions */
	dir = g_path_get_dirname(file);
	ret = (g_access(dir, W_OK) == -1 ? TRUE : FALSE);
	g_free(dir);

	return ret;
}

/*
static gboolean check_for_binary(const gchar * binary)
{
	return (g_find_program_in_path(binary) == NULL ? TRUE : FALSE);
}
*/

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
		job_issue(job, _("Cannot cancel job at MOAB server.\n"));
		goto err;
	}
	if (std_err != NULL && strlen(std_err)) {
		job_issue(job, _("Cannot cancel job at MOAB server.\n"));
		goto err1;
	}
	if (job->status != JOB_STATUS_QUEUED)
		job->user_finished = TRUE;

err1:	g_free(std_out);
	g_free(std_err);
err:	g_string_free(cmd_line, TRUE);

}

static GebrdMpiInterface * job_get_mpi_impl(const gchar * mpi_name, GString * n_process)
{
	const GebrdMpiConfig * config = gebrd_get_mpi_config_by_name(mpi_name);
	if (config == NULL)
		return NULL;

	if (strcmp(mpi_name, "openmpi") == 0)
		return gebrd_open_mpi_new(n_process->str, config);

	return NULL;
}
