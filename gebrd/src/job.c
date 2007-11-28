/*   GêBR Daemon - Process and control execution of flows
 *   Copyright (C) 2007 GêBR core team (http://gebr.sourceforge.net)
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

#include <misc/protocol.h>
#include <misc/gtcpsocket.h>
#include <misc/ghostaddress.h>

#include "job.h"
#include "gebrd.h"
#include "support.h"

/*
 * Internal functions
 */

static gboolean
add_program_parameters_to_job(struct job * job, GeoXmlProgram * prog)
{
	GeoXmlProgramParameter *	param;

	if (geoxml_program_get_parameters_number(prog) == 0)
		return TRUE;

	param = geoxml_program_get_first_parameter(prog);
	while (param != NULL) {
		switch (geoxml_program_parameter_get_type(param)) {
		case GEOXML_PARAMETERTYPE_STRING:
		case GEOXML_PARAMETERTYPE_INT:
		case GEOXML_PARAMETERTYPE_FLOAT:
		case GEOXML_PARAMETERTYPE_RANGE:
		case GEOXML_PARAMETERTYPE_FILE: {
			const gchar *	value;

			value = geoxml_program_parameter_get_value(param);
			if (strlen(value) > 0) {
				g_string_append_printf(job->cmd_line, "%s\"%s\" ",
					geoxml_program_parameter_get_keyword(param),
					value);
			} else {
				/* Check if this is a required parameter */
				if (geoxml_program_parameter_get_required(param)) {
					g_string_append_printf(job->issues, "Required parameter \"%s\" not provided\n",
						geoxml_program_parameter_get_label(param));
					return FALSE;
				}
			}
			break;
		}
		case GEOXML_PARAMETERTYPE_FLAG:
			if (geoxml_program_parameter_get_flag_status(param)) {
				g_string_append_printf(job->cmd_line, "%s ", geoxml_program_parameter_get_keyword(param));
			}
			break;
		default:
			g_string_append_printf(job->issues, "Unknown parameter type\n");
			return FALSE;
		}
		geoxml_program_parameter_next(&param);
	}
	return TRUE;
}

void
job_send_clients_output(struct job * job, GString * output)
{
	GList *		link;

	/* FIXME: remove and find the real problem */
	if (!job->jid->len)
		return;

	link = g_list_first(gebrd.clients);
	while (link != NULL) {
		struct client * client;

		client = (struct client *)link->data;
		protocol_send_data(client->protocol, client->tcp_socket,
			protocol_defs.out_def, 2, job->jid->str, output->str);

		link = g_list_next(link);
	}
}

void
job_process_read_stdout(GProcess * gjob, struct job * job)
{
	GString *	stdout;

	stdout = g_process_read_stdout_string_all(gjob);

	g_string_append(job->output, stdout->str);
	job_send_clients_output(job, stdout);

	g_string_free(stdout, TRUE);
}

void
job_process_read_stderr(GProcess * gjob, struct job * job)
{
	GString *	stderr;

	stderr = g_process_read_stderr_string_all(gjob);

	g_string_append(job->output, stderr->str);
	job_send_clients_output(job, stderr);

	g_string_free(stderr, TRUE);
}

void
job_process_finished(GProcess * gjob, struct job * job)
{
	GList *	link;

	/* set the finish date */
	assign_current_date(job->finish_date);

	/* what make it quit? */
	g_string_assign(job->status, (job->user_finished == FALSE) ? "finished" : "canceled");

	/* warn all client that this job finished */
	link = g_list_first(gebrd.clients);
	while (link != NULL) {
		struct client * client;

		client = (struct client *)link->data;
		protocol_send_data(client->protocol, client->tcp_socket, protocol_defs.fin_def,
			3, job->jid->str, job->status->str, job->finish_date->str);

		link = g_list_next(link);
	}
}

GString *
job_generate_id(void)
{
	GString *	jid;
	int		random;
	GList *		link;

	jid = g_string_new(NULL);
	do {
		random = random_number();
		g_string_printf(jid, "%d", random);

		/* check if it is unique */
		link = g_list_first(gebrd.jobs);
		while (link != NULL) {
			struct job *	job;

			job = (struct job *)link->data;
			if (!g_ascii_strcasecmp(job->jid->str, jid->str))
				continue;

			link = g_list_next(link);
		}

		break;
	} while (1);

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
	GeoXmlProgram *		prog;
	gulong			nprog;
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
				"DTD specified. The <DOCTYPE ...> must not appear in XML.\n"
				"libgeoxml will find the appriate DTD installed from version.\n");
			break;
		case GEOXML_RETV_INVALID_DOCUMENT:
			g_string_append_printf(job->issues,
				"Invalid document. The has a sintax error or doesn't match the DTD.\n"
				"In this case see the errors above\n");
			break;
		case GEOXML_RETV_CANT_ACCESS_FILE:
			g_string_append_printf(job->issues,
				"Can't access file. The file doesn't exist or there is not enough "
				"permission to read it.\n");
			break;
		case GEOXML_RETV_CANT_ACCESS_DTD:
			g_string_append_printf(job->issues,
				"Can't access dtd. The file's DTD couldn't not be found.\n"
				"It may be a newer version not support by this version of libgeoxml "
				"or there is an installation problem.\n");
			break;
		case GEOXML_RETV_NO_MEMORY:
			g_string_append_printf(job->issues,
				"Not enough memory. The library stoped after an unsucessful memory allocation.\n");
			break;
		default:
			g_string_append_printf(job->issues,
				"Unspecified error %d. "
				"See library documentation at http://gebr.sf.net/libgeoxml/doc\n", ret);
			break;
		}
		goto err;
	}

	flow = GEOXML_FLOW(document);
	job->flow = flow;
	nprog = geoxml_flow_get_programs_number(flow);
	if (nprog == 0) {
		g_string_append_printf(job->issues, "Empty flow\n");
		goto err;
	}

	/* Check if there is configured programs */
	geoxml_flow_get_program(flow, &prog, 0);
	while (prog != NULL && strcmp(geoxml_program_get_status(prog), "configured") != 0) {
		g_string_append_printf(job->issues, "%u) Skiping disabled/unconfigured program \"%s\"\n",
			++issue_number, geoxml_program_get_title(prog));
		geoxml_program_next(&prog);
	}
	if (prog == NULL) {
		g_string_append_printf(job->issues, "No configured programs\n");
		goto err;
	}

	/* Start with/without stdin */
	if (geoxml_program_get_stdin(prog)) {
		if (strlen(geoxml_flow_io_get_input(flow)) == 0) {
			g_string_append_printf(job->issues, "No input file selected\n");
			goto err;
		}

		/* Input file */
		g_string_append_printf(job->cmd_line, "<\"%s\" ",
			geoxml_flow_io_get_input(flow));
	}

	/* Binary followed by an space */
	g_string_append(job->cmd_line, geoxml_program_get_binary(prog));
	g_string_append(job->cmd_line, " ");

	if (add_program_parameters_to_job(job, prog) == FALSE) {
		g_string_append_printf(job->issues, "Unable to configure properly program \"%s\"\n",
			geoxml_program_get_title(prog));
		goto err;
	}

	if (geoxml_program_get_stderr(prog)) {
		if (strlen(geoxml_flow_io_get_error(flow)) == 0)
			g_string_append_printf(job->issues, "%u) No error file selected. Ignoring it.\n",
				++issue_number);
		else
			g_string_append_printf(job->cmd_line, "2>> \"%s\" ", geoxml_flow_io_get_error(flow));
	}

	previous_stdout = geoxml_program_get_stdout(prog);
	geoxml_program_next(&prog);

	while (prog != NULL) {
		/* Skiping disabled/unconfigured programs */
		if (strcmp(geoxml_program_get_status(prog), "configured") != 0) {
			g_string_append_printf(job->issues, "%u) Skipped disabled/unconfigured program \"%s\"\n",
				++issue_number, geoxml_program_get_title(prog));
			geoxml_program_next(&prog);
			continue;
		}

		/* How to connect chainned programs */
		int chain_option = geoxml_program_get_stdin(prog) + (previous_stdout << 1);
		switch (chain_option) {
		case 0: /* Previous does not write to stdin and current does not carry about */
			g_string_append_printf(job->cmd_line, "; %s ", geoxml_program_get_binary(prog));
			break;
		case 1: /* Previous does not write to stdin but current expect something */
			g_string_append_printf(job->issues, "Broken flow before %s (no input)\n", geoxml_program_get_title(prog));
			goto err;
		case 2: /* Previous does write to stdin but current does not carry about */
			g_string_append_printf(job->issues, "Broken flow before %s (unexpected output)\n", geoxml_program_get_title(prog));
			goto err;
		case 3: /* Both talk to each other */
			g_string_append_printf(job->cmd_line, "| %s ", geoxml_program_get_binary(prog));
			break;
		default:
			break;
		}

		previous_stdout = geoxml_program_get_stdout(prog);
		if (add_program_parameters_to_job(job, prog) == FALSE) {
			g_string_append_printf(job->issues, "Unable to configure properly program \"%s\"\n",
				geoxml_program_get_title(prog));
			goto err;
		}

		if (geoxml_program_get_stderr(prog)) {
			if (strlen(geoxml_flow_io_get_error(flow)) == 0)
				g_string_append_printf(job->issues, "%u) No error file selected. Ignoring it.\n",
					++issue_number);
			else
				g_string_append_printf(job->cmd_line, "2>> \"%s\" ", geoxml_flow_io_get_error(flow));
		}

		geoxml_program_next(&prog);
	}

	if (previous_stdout) {
		if (strlen(geoxml_flow_io_get_output(flow)) == 0) {
			g_string_append_printf(job->issues, "No output file selected\n");
			goto err;
		}
		g_string_append_printf(job->cmd_line, ">%s", geoxml_flow_io_get_output(flow));
	}

	/* success exit */
	success = TRUE;
	goto out;

err:	g_string_assign(job->jid, "0");
	g_string_assign(job->status, "failed");
	success = FALSE;

out:
	/* hostname and flow title */
	g_string_assign(job->hostname, client->protocol->hostname->str);
	g_string_assign(job->title, geoxml_document_get_title(document));
	/* set the start date */
	assign_current_date(job->start_date);

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
	gchar			server_hostname[100];

	cmd_line = g_string_new(NULL);
	gethostname(server_hostname, 100);

	if (!g_ascii_strcasecmp(server_hostname, client->protocol->hostname->str)) {
		/* tell bash to run it an set display */
		g_string_printf(cmd_line, "bash -l -c \"%s\"",
				job->cmd_line->str);
	} else {
		/* tell bash to run it an set display */
		g_string_printf(cmd_line, "bash -l -c \"DISPLAY=%s%s; %s\"",
				client->protocol->hostname->str, client->display->str,
				job->cmd_line->str);
	}

	gebrd.jobs = g_list_append(gebrd.jobs, job);
	g_signal_connect(job->process, "ready-read-stdout",
			G_CALLBACK(job_process_read_stdout), job);
	g_signal_connect(job->process, "ready-read-stderr",
			G_CALLBACK(job_process_read_stderr), job);
	g_signal_connect(job->process, "finished",
			G_CALLBACK(job_process_finished), job);
	g_process_start(job->process, cmd_line);
	g_string_assign(job->status, "running");

	/* frees */
	g_string_free(cmd_line, TRUE);
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
		if (!g_ascii_strcasecmp(i->jid->str, jid->str)) {
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
	if (g_process_is_running(job->process) == FALSE) {
		job_free(job);
	}
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
		protocol_send_data(client->protocol, client->tcp_socket, protocol_defs.job_def, 9,
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
		protocol_send_data(client->protocol, client->tcp_socket, protocol_defs.job_def, 9,
			job->jid->str, job->status->str, job->title->str,
			job->start_date->str, job->finish_date->str, job->hostname->str,
			job->issues->str, job->cmd_line->str, job->output->str);

		link = g_list_next(link);
	}
}
