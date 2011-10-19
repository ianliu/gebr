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

#define OUTPUT_FLUSH_TIMEOUT 1

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
#include <libgebr/geoxml/gebr-geo-types.h>

#include "gebrd-job.h"
#include "gebrd.h"
#include "gebrd-queues.h"
#include "gebrd-mpi-implementations.h"

/* GOBJECT STUFF */
enum {
	LAST_PROPERTY
};

enum {
	LAST_SIGNAL
};

G_DEFINE_TYPE(GebrdJob, gebrd_job, GEBR_COMM_JOB_TYPE)

static void gebrd_job_init(GebrdJob * job)
{
	job->exec_speed = g_string_new(NULL);
	job->frac = g_string_new(NULL);
}

static void gebrd_job_class_init(GebrdJobClass * klass)
{
}

static void job_assembly_cmdline(GebrdJob *job);
static void job_process_finished(GebrCommProcess * process, gint status, GebrdJob *job);
static void job_send_signal_on_moab(const char * signal, GebrdJob * job);
static GebrdMpiInterface * job_get_mpi_impl(const gchar * mpi_name, GString * n_process);
static gchar *escape_quote_and_slash(const gchar *str);
static gchar *replace_quotes(gchar *str);

/**
 * \internal
 */
static void job_issue(GebrdJob *job, const gchar *message, ...)
{
	va_list argp;
	va_start(argp, message);
	gchar *issue = g_strdup_vprintf(message, argp);
	va_end(argp);

	g_string_append(job->parent.issues, issue);
	if (job->parent.status != JOB_STATUS_INITIAL)
		job_status_notify(job, JOB_STATUS_ISSUED, issue);

	g_free(issue);
}

/**
 * \internal
 */
static void job_send_clients_output(GebrdJob *job, GString * output)
{
	/* FIXME: remove and find the real problem */
	if (!job->parent.jid->len)
		return;

	for (GList *link = gebrd->clients; link != NULL; link = g_list_next(link)) {
		struct client *client = (struct client *)link->data;
		gebr_comm_protocol_socket_oldmsg_send(client->socket, FALSE,
						      gebr_comm_protocol_defs.out_def, 4, job->parent.jid->str, output->str,
						      job->parent.run_id->str, job->frac->str);
	}
}

/**
 * \internal
 */
static void job_process_add_output(GebrdJob *job, GString * destination, GString * output)
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
			gebrd_message(GEBR_LOG_ERROR, _("Job '%s' sent output not in UTF-8."), job->parent.title->str);
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

#define OUTBUF 0
#define ERRBUF 1

static void
append_output_and_send(GebrCommProcess * process, GebrdJob *job, GString *output, gint buf)
{
	GTimeVal tv;

	g_string_append(job->buf[buf], output->str);
	g_get_current_time(&tv);

	if (tv.tv_sec - job->timeout[buf] > OUTPUT_FLUSH_TIMEOUT) {
		job_process_add_output(job, job->parent.output, job->buf[buf]);
		g_string_assign(job->buf[buf], "");
		job->timeout[buf] = tv.tv_sec;
	}
}

static void
job_process_read_stdout(GebrCommProcess * process, GebrdJob *job)
{
	GString *output;
	output = gebr_comm_process_read_stdout_string_all(process);
	append_output_and_send(process, job, output, OUTBUF);
	g_string_free(output, TRUE);
}

static void
job_process_read_stderr(GebrCommProcess * process, GebrdJob *job)
{
	GString *output;
	output = gebr_comm_process_read_stderr_string_all(process);
	append_output_and_send(process, job, output, ERRBUF);
	g_string_free(output, TRUE);
}

static void
moab_process_read_stdout(GebrCommProcess *process, GebrdJob *job)
{
	job_process_read_stdout(process, job);
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
static void job_status_notify_finished(GebrdJob *job)
{
	enum JobStatus new_status = (job->user_finished == FALSE) ? JOB_STATUS_FINISHED : JOB_STATUS_CANCELED;
	if (new_status == job->parent.status)
		return;

	if (gebrd_get_server_type() == GEBR_COMM_SERVER_TYPE_MOAB) {
		if (job->tail_process != NULL) {
			gebr_comm_process_free(job->tail_process);
			job->tail_process = NULL;
		}
	
		/* last change to get output */
		GString * output_file = g_string_new(NULL);
		g_string_printf(output_file, "%s/STDIN.o%s", g_get_home_dir(), job->parent.moab_jid->str);
		GError * error = NULL;
		GIOChannel * file = g_io_channel_new_file(output_file->str, "r", &error);
		g_string_free(output_file, TRUE);
		error = NULL;
		if (file != NULL && g_io_channel_seek_position(file, job->parent.output->len, G_SEEK_SET, &error) == G_IO_STATUS_NORMAL) {
			gchar * buffer;
			gsize length = 0;
			g_io_channel_read_to_end(file, &buffer, &length, &error);
			if (length > 0) {
				GString *buffer_gstring = g_string_new(buffer);
				job_process_add_output(job, job->parent.output, buffer_gstring);
				g_string_free(buffer_gstring, TRUE);
				g_free(buffer);
			}
		}
		if (file != NULL)
			g_io_channel_shutdown(file, FALSE, &error);
	}

	g_string_assign(job->parent.finish_date, gebr_iso_date());
	job_status_notify(job, new_status, job->parent.finish_date->str);
}

/**
 * \internal
 * Only for regular jobs
 */
static void job_process_finished(GebrCommProcess * process, gint status, GebrdJob *job)
{
	if (WEXITSTATUS(status) == 0)
		job_status_notify_finished(job);
	else
		job_status_notify(job, JOB_STATUS_FAILED, _("Job exited with failure"));
}

/**
 * \internal
 */
static gboolean job_moab_checkjob_pooling(GebrdJob * job)
{
	gboolean keep_polling = TRUE;
	gboolean error = FALSE;

	/* run checkjob and retrieve XML */
	GString *cmd_line = g_string_new("");
	g_string_printf(cmd_line, "checkjob %s --format=xml", job->parent.moab_jid->str);
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
	if (strcmp(job->parent.queue_id->str, queue)) {
		g_string_assign(job->parent.queue_id, queue);
		job_status_notify(job, JOB_STATUS_REQUEUED, job->parent.queue_id->str);
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
		if (job->parent.status != JOB_STATUS_RUNNING) {
			g_string_assign(job->parent.start_date, gebr_iso_date());
			job_status_notify(job, JOB_STATUS_RUNNING, job->parent.start_date->str);
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

GebrdJob *job_find(GString * jid)
{
	GebrdJob *job;

	job = NULL;
	for (GList *link = gebrd->user->jobs; link != NULL; link = g_list_next(link)) {
		GebrdJob *i = (GebrdJob *)link->data;
		if (!strcmp(i->parent.jid->str, jid->str)) {
			job = i;
			break;
		}
	}

	return job;
}

void job_new(GebrdJob ** _job, struct client * client, GString * queue, GString * account, GString * xml,
	     GString * n_process, GString * run_id, GString *exec_speed, GString *frac)
{
	/* initialization */
	GebrdJob *job = GEBRD_JOB(g_object_new(GEBRD_JOB_TYPE, NULL, NULL));
	job->process = gebr_comm_process_new();
	job->tail_process = NULL;
	job->flow = NULL;
	job->critical_error = FALSE;
	job->user_finished = FALSE;
	job->buf[0] = g_string_new(NULL);
	job->buf[1] = g_string_new(NULL);
	job->timeout[0] = 0;
	job->timeout[1] = 0;

	g_string_assign(job->parent.client_hostname, client->socket->protocol->hostname->str);
	g_string_assign(job->parent.client_display, client->display->str);
	job->parent.server_location = client->server_location;
	g_string_assign(job->parent.run_id, run_id->str);
	g_string_assign(job->parent.queue_id, queue->str);
	g_string_assign(job->parent.moab_account, account->str);
	g_string_assign(job->parent.n_process, n_process->str);
	g_string_assign(job->exec_speed, exec_speed->str);
	g_string_assign(job->frac, frac->str);
	job->parent.status = JOB_STATUS_INITIAL;

	*_job = job;
	gebrd->user->jobs = g_list_append(gebrd->user->jobs, job);

	GebrGeoXmlDocument *document;
	int ret = gebr_geoxml_document_load_buffer(&document, xml->str);
	job->flow = GEBR_GEOXML_FLOW(document);
	gebrd->flow = document;

	if (job->flow == NULL)
		job_issue(job, gebr_geoxml_error_explained_string(ret));
	else {
		g_string_assign(job->parent.title, gebr_geoxml_document_get_title(GEBR_GEOXML_DOCUMENT(job->flow)));
		gebrd_clean_proj_line_dicts();
		gebr_geoxml_document_split_dict(gebrd->flow, gebrd->line, gebrd->proj, NULL);
		gebr_validator_update(gebrd_get_validator(gebrd));
	}

	/* just to send the client the command line, we could do this after by changing the protocol */
	job_assembly_cmdline(job);
}

void job_free(GebrdJob *job)
{
	gebrd_queues_remove_job_from(job->parent.queue_id->str, job);

	GList *link = g_list_find(gebrd->user->jobs, job);
	/* job that failed to run are not added to this list */
	if (link != NULL)
		gebrd->user->jobs = g_list_delete_link(gebrd->user->jobs, link);

	/* free data */
	gebr_comm_process_free(job->process);
	if (gebrd_get_server_type() == GEBR_COMM_SERVER_TYPE_MOAB)
		if (job->tail_process != NULL)
			gebr_comm_process_free(job->tail_process);
	if (job->flow)
		gebr_geoxml_document_free(GEBR_GEOXML_DOC(job->flow));
	g_string_free(job->exec_speed, TRUE);
	g_string_free(job->buf[0], TRUE);
	g_string_free(job->buf[1], TRUE);
	g_string_free(job->frac, TRUE);
	g_object_unref(job);
}

void job_status_set(GebrdJob *job, enum JobStatus status)
{
	if (job->parent.status == status) {
		//occurs frequently with netuno
		//gebrd_message(GEBR_LOG_DEBUG, "Calling job_status_set without status change (status %d)", job->parent.status);
		return;
	}
	/* false statuses */
	if (status == JOB_STATUS_REQUEUED || status == JOB_STATUS_ISSUED)
		return;

	enum JobStatus old_status = job->parent.status;
	job->parent.status = status;

	if (job->parent.status == JOB_STATUS_FAILED
	    || job->parent.status == JOB_STATUS_FINISHED
	    || (job->parent.status == JOB_STATUS_CANCELED && old_status == JOB_STATUS_RUNNING)) {
		if (gebrd_queues_has_next(job->parent.queue_id->str))
			gebrd_queues_step_queue(job->parent.queue_id->str);
		else
			gebrd_queues_set_queue_busy(job->parent.queue_id->str, FALSE);
	}
}

void job_status_notify(GebrdJob *job, enum JobStatus status, const gchar *_parameter, ...)
{
	va_list argp;
	va_start(argp, _parameter);
	gchar *parameter = g_strdup_vprintf(_parameter, argp);
	va_end(argp);

	job_status_set(job, status);

	/* warn all clients of the new status */
	for (GList *link = gebrd->clients; link != NULL; link = g_list_next(link)) {
		struct client *client = (struct client *)link->data;
		gebr_comm_protocol_socket_oldmsg_send(client->socket, FALSE,
						      gebr_comm_protocol_defs.sta_def, 5,
						      job->parent.jid->str, status_enum_to_string(status),
						      parameter, job->parent.run_id->str, job->frac->str);

	}

	g_free(parameter);
}

void job_run_flow(GebrdJob *job)
{
	GString *cmd_line;

	/* initialization */
	cmd_line = g_string_new(NULL);

	if (job->critical_error == TRUE)
		goto err;

	/*
	 * First program
	 */
	/* command-line */
	gsize bytes_written;
	gchar *localized_cmd_line = g_filename_from_utf8(job->parent.cmd_line->str, -1, NULL, &bytes_written, NULL);
	if (job->parent.client_display->len) {
		GString *to_quote;

		to_quote = g_string_new(NULL);
		if (job->parent.server_location == GEBR_COMM_SERVER_LOCATION_LOCAL) {
			g_string_printf(to_quote, "export DISPLAY=%s; %s", job->parent.client_display->str, localized_cmd_line);
			gchar * quoted = g_shell_quote(to_quote->str);
			g_string_printf(cmd_line, "bash -l -c %s", quoted);
			g_free(quoted);
		}
		else{
			g_string_printf(to_quote, "export DISPLAY=127.0.0.1%s; %s", job->parent.client_display->str, localized_cmd_line);
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
			       	script, job->parent.moab_account->str, job->parent.queue_id->str);
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
		g_string_printf(job->parent.moab_jid, "%u", moab_id);

		/* run command to get script output */
		g_string_printf(cmd_line, "bash -c \"touch $HOME/STDIN.o%s; tail -f $HOME/STDIN.o%s\"",
			       	job->parent.moab_jid->str, job->parent.moab_jid->str);
		job->tail_process = gebr_comm_process_new();
		g_signal_connect(job->tail_process, "ready-read-stdout", G_CALLBACK(moab_process_read_stdout), job);
		gebr_comm_process_start(job->tail_process, cmd_line);

		/* pool for moab status */
		g_timeout_add(1000, (GSourceFunc)job_moab_checkjob_pooling, job); 
	} else {
		GebrGeoXmlSequence *program;

		g_signal_connect(job->process, "ready-read-stdout", G_CALLBACK(job_process_read_stdout), job);
		g_signal_connect(job->process, "ready-read-stderr", G_CALLBACK(job_process_read_stderr), job);
		g_signal_connect(job->process, "finished", G_CALLBACK(job_process_finished), job);

		g_string_assign(job->parent.start_date, gebr_iso_date());
		job_status_notify(job, JOB_STATUS_RUNNING, job->parent.start_date->str);
		gebr_comm_process_start(job->process, cmd_line);

		/* for program that waits stdin EOF (like sfmath) */
		gebr_geoxml_flow_get_program(job->flow, &program, 0);
		if (gebr_geoxml_program_get_stdin(GEBR_GEOXML_PROGRAM(program)) == FALSE)
			gebr_comm_process_close_stdin(job->process);
	}

	/* success */
	gebrd_message(GEBR_LOG_DEBUG, "Client '%s' flow about to run %s: %s",
		      job->parent.client_hostname->str, job->parent.title->str, cmd_line->str);
	goto out;

err:	/* error */
	job_status_notify(job, JOB_STATUS_FAILED, "");

out:	
	/* frees */
	g_string_free(cmd_line, TRUE);
}

void job_clear(GebrdJob *job)
{
	/* NOTE: changes here must reflect changes in job_close at gebr */
	if (!(job->parent.status == JOB_STATUS_RUNNING || job->parent.status == JOB_STATUS_QUEUED))
		job_free(job);
}

void job_end(GebrdJob *job)
{
	if (gebrd_get_server_type() == GEBR_COMM_SERVER_TYPE_REGULAR) {
		if (job->parent.status == JOB_STATUS_QUEUED) {
			gebrd_queues_remove_job_from(job->parent.queue_id->str, job);
			g_string_assign(job->parent.finish_date, gebr_iso_date());
			job_status_notify(job, JOB_STATUS_CANCELED, job->parent.finish_date->str);
		} else {
			job->user_finished = TRUE;
			gebr_comm_process_terminate(job->process);
		}
	} else if (gebrd_get_server_type() == GEBR_COMM_SERVER_TYPE_MOAB)
		job_send_signal_on_moab("SIGTERM", job);
}

void job_kill(GebrdJob *job)
{
	if (gebrd_get_server_type() == GEBR_COMM_SERVER_TYPE_REGULAR) {
		if (job->parent.status == JOB_STATUS_QUEUED) {
			gebrd_queues_remove_job_from(job->parent.queue_id->str, job);
			g_string_assign(job->parent.finish_date, gebr_iso_date());
			job_status_notify(job, JOB_STATUS_CANCELED, job->parent.finish_date->str);
		} else {
			job->user_finished = TRUE;
			gebr_comm_process_kill(job->process);
		}
	} else if (gebrd_get_server_type() == GEBR_COMM_SERVER_TYPE_MOAB)
		job_send_signal_on_moab("SIGKILL", job);
}

void job_notify(GebrdJob *job, struct client *client)
{
	if (job->parent.status == JOB_STATUS_INITIAL)
		job_status_set(job, JOB_STATUS_QUEUED);

	gebr_comm_protocol_socket_oldmsg_send(client->socket, FALSE,
					      gebr_comm_protocol_defs.job_def, 13,
					      job->parent.jid->str,
					      status_enum_to_string(job->parent.status),
					      job->parent.title->str,
					      job->parent.start_date->str,
					      job->parent.finish_date->str,
					      job->parent.client_hostname->str,
					      job->parent.issues->str,
					      job->parent.cmd_line->str,
					      job->parent.output->str,
					      job->parent.queue_id->str,
					      job->parent.moab_jid->str,
					      job->parent.run_id->str,
					      job->frac->str);
}


void job_list(struct client *client)
{
	for (GList *link = gebrd->user->jobs; link != NULL; link = g_list_next(link)) {
		GebrdJob *job = (GebrdJob *)link->data;
		job_notify(job, client);
	}
}

void job_send_clients_job_notify(GebrdJob *job)
{
	for (GList *link = gebrd->clients; link != NULL; link = g_list_next(link)) {
		struct client *client = (struct client *)link->data;
		job_notify(job, client);
	}
}

/**
 * \internal
 * Send a \p signal to a given \p job on MOAB cluster.
 */
static void job_send_signal_on_moab(const char * signal, GebrdJob * job)
{
	GString *cmd_line = NULL;
	gchar *std_out = NULL;
	gchar *std_err = NULL;
	gint exit_status;
	cmd_line = g_string_new("");
	g_string_printf(cmd_line, "mjobctl -M signal=%s %s", signal, job->parent.moab_jid->str);
	if (g_spawn_command_line_sync(cmd_line->str, &std_out, &std_err, &exit_status, NULL) == FALSE){
		job_issue(job, _("Cannot cancel job at MOAB server.\n"));
		goto err;
	}
	if (std_err != NULL && strlen(std_err)) {
		job_issue(job, _("Cannot cancel job at MOAB server.\n"));
		goto err1;
	}
	if (job->parent.status != JOB_STATUS_QUEUED)
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

static gboolean job_parse_parameters(GebrdJob *job,
				     GebrGeoXmlParameters * parameters,
				     GebrGeoXmlProgram * program,
				     GString *expr_buf);
/**
 * \internal
 */
static gboolean job_parse_parameter(GebrdJob *job, GebrGeoXmlParameter * parameter, GebrGeoXmlProgram * program, GString *expr_buf)
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
				ret = job_parse_parameter(job, selected, program, expr_buf);
			else
				ret = job_parse_parameters(job, GEBR_GEOXML_PARAMETERS(instance), program, expr_buf);
		}

		return ret;
	}

	program_parameter = GEBR_GEOXML_PROGRAM_PARAMETER(parameter);
	switch (type) {
	case GEBR_GEOXML_PARAMETER_TYPE_FILE:
	case GEBR_GEOXML_PARAMETER_TYPE_STRING: {
		gchar *strip;
		const gchar *value;
		const gchar *separator;
		GebrGeoXmlSequence *seq;
		gboolean first = TRUE;

		GString *all_values;
		all_values = gebr_geoxml_program_parameter_get_string_value(program_parameter, FALSE);

		if (all_values->len == 0) {
			if (gebr_geoxml_program_parameter_get_required(program_parameter)) {
				job_issue(job,
					  _("Required parameter '%s' of program '%s' not provided.\n"),
					  gebr_geoxml_parameter_get_label(parameter),
					  gebr_geoxml_program_get_title(program));
				g_string_free(all_values, TRUE);
				return FALSE;
			}
			g_string_free(all_values, TRUE);
			break;
		}
		g_string_free(all_values, TRUE);

		gebr_geoxml_program_parameter_get_value(program_parameter, FALSE, &seq, 0);
		g_string_append(job->parent.cmd_line, gebr_geoxml_program_parameter_get_keyword(program_parameter));
		g_string_append_c(job->parent.cmd_line, '"');
		separator = gebr_geoxml_program_parameter_get_list_separator(program_parameter);

		while (seq) {
			value = gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(seq));
			strip = g_strstrip(g_strdup(value));

			if (*strip) {
				gchar *result;
				gchar *escaped;
				GError *error = NULL;

				gebr_validator_evaluate_interval(gebrd_get_validator(gebrd), strip, GEBR_GEOXML_PARAMETER_TYPE_STRING, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, FALSE, &result, &error);

				if (!error) {
					escaped = escape_quote_and_slash(result);
					if (!first)
						g_string_append(job->parent.cmd_line, separator);
					g_string_append_printf (job->parent.cmd_line, "%s", result);
					g_free (escaped);
					g_free (result);
					g_free (strip);
				} else {
					job_issue(job, error->message);
					g_error_free(error);
					g_free (strip);
					return FALSE;
				}
				if (first)
					first = FALSE;
			}
			gebr_geoxml_sequence_next(&seq);
		}

		g_string_append(job->parent.cmd_line, "\" ");
		break;
	}

	case GEBR_GEOXML_PARAMETER_TYPE_INT:
	case GEBR_GEOXML_PARAMETER_TYPE_RANGE:
	case GEBR_GEOXML_PARAMETER_TYPE_FLOAT: {
		const gchar *value;
		const gchar *vmin, *vmax;
		gchar *end_str;
		gchar *temp;
		gchar *strip;
		const gchar *separator;
		GebrGeoXmlSequence *seq;
		gboolean first = TRUE;
	
		GString *all_values;
		all_values = gebr_geoxml_program_parameter_get_string_value(program_parameter, FALSE);

		if (all_values->len == 0) {
			if (gebr_geoxml_program_parameter_get_required(program_parameter)) {
				job_issue(job,
					  _("Required parameter '%s' of program '%s' not provided.\n"),
					  gebr_geoxml_parameter_get_label(parameter),
					  gebr_geoxml_program_get_title(program));
				g_string_free(all_values, TRUE);
				return FALSE;
			}
			g_string_free(all_values, TRUE);
			break;
		}
		g_string_free(all_values, TRUE);

		gebr_geoxml_program_parameter_get_value(program_parameter, FALSE, &seq, 0);
		gebr_geoxml_program_parameter_get_number_min_max(program_parameter, &vmin, &vmax);
		g_string_append(job->parent.cmd_line, gebr_geoxml_program_parameter_get_keyword(program_parameter));
		g_string_append_c(job->parent.cmd_line, '"');
		separator = gebr_geoxml_program_parameter_get_list_separator(program_parameter);

		while(seq) {
			value = gebr_geoxml_value_sequence_get(GEBR_GEOXML_VALUE_SEQUENCE(seq));
			strip = g_strstrip(g_strdup(value));
	
			if (*strip) {
				g_ascii_strtod(strip, &end_str);
				if(*end_str) {
					gchar *escaped = replace_quotes(gebr_geoxml_parameter_get_label (parameter));

					if (type == GEBR_GEOXML_PARAMETER_TYPE_RANGE) {
						const gchar *digits;
						gebr_geoxml_program_parameter_get_range_properties(program_parameter, NULL, NULL, NULL, &digits);
						if (g_strcmp0(digits, "0") == 0)
							temp = g_strdup_printf("round(%s)", strip);
					} else if (type == GEBR_GEOXML_PARAMETER_TYPE_INT)
						temp = g_strdup_printf("round(%s)", strip);
					else
						temp = g_strdup(strip);

					if (*vmin && *vmax)
						g_string_append_printf (expr_buf, "\t\tmin(%s,max(%s,%s)) # V[%"G_GSIZE_FORMAT"]: %s\n", vmax, vmin, temp,
									job->expr_count + job->n_vars, escaped);
					else if(*vmin)
						g_string_append_printf (expr_buf, "\t\tmax(%s,%s) # V[%"G_GSIZE_FORMAT"]: %s\n", vmin, temp,
									job->expr_count + job->n_vars, escaped);
					else if(*vmax)
						g_string_append_printf (expr_buf, "\t\tmax(%s,%s) # V[%"G_GSIZE_FORMAT"]: %s\n", vmax, temp,
									job->expr_count + job->n_vars, escaped);
					else
						g_string_append_printf (expr_buf, "\t\t%s # V[%"G_GSIZE_FORMAT"]: %s\n", temp,
									job->expr_count + job->n_vars, escaped);

					if(!first)
						g_string_append(job->parent.cmd_line, separator);
					g_string_append_printf (job->parent.cmd_line, "${V[%"G_GSIZE_FORMAT"]}", job->expr_count + job->n_vars);
					job->expr_count++;
					g_free(temp);
					g_free(escaped);
				} else  {
					if(!first)
						g_string_append(job->parent.cmd_line, separator);
					g_string_append_printf (job->parent.cmd_line, "%s", strip);
				}
				if (first)
					first = FALSE;
			}
			g_free(strip);
			gebr_geoxml_sequence_next(&seq);
		}

		g_string_append(job->parent.cmd_line, "\" ");
		break;
	}

	case GEBR_GEOXML_PARAMETER_TYPE_ENUM: {
		GString *value;
		value = gebr_geoxml_program_parameter_get_string_value(program_parameter, FALSE);
		if (strlen(value->str) > 0) {
			gchar *quoted = g_shell_quote(value->str);
			g_string_append_printf(job->parent.cmd_line, "%s%s ",
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
			g_string_append_printf(job->parent.cmd_line, "%s ",
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
static gboolean job_parse_parameters(GebrdJob *job, GebrGeoXmlParameters * parameters, GebrGeoXmlProgram * program, GString *expr_buf)
{
	GebrGeoXmlSequence *parameter;

	parameter = gebr_geoxml_parameters_get_first_parameter(parameters);
	for (; parameter != NULL; gebr_geoxml_sequence_next(&parameter))
		if (job_parse_parameter(job, GEBR_GEOXML_PARAMETER(parameter), program, expr_buf) == FALSE)
			return FALSE;

	return TRUE;
}
/**
 * \internal
 */
static gboolean job_add_program_parameters(GebrdJob *job, GebrGeoXmlProgram * program, GString *expr_buf)
{
	return job_parse_parameters(job, gebr_geoxml_program_get_parameters(program), program, expr_buf);
}

static gchar *replace_quotes(gchar *str)
{
	gsize i = 0;
	while (str[i])
	{
		if (str[i] == '\'')
			str[i] = '"';
		i++;
	}

	return str;
}

gboolean gebrd_bc_equal_func(gconstpointer a, gconstpointer b)
{
	return g_str_equal(a + 7, b);
}

guint gebrd_bc_hash_func(gconstpointer a)
{
	gchar *i;
	const gchar *x = a;

	i = strchr(x, ':');
	if (i)
		return g_str_hash(i+1);
	return g_str_hash(a);
}

static gchar* define_bc_variables(GebrdJob *job, GString *expr_buf, GString *str_buf, gsize *n_vars, guint *issue_number)
{
	gsize j = 0;
	const gchar *value;
	const gchar *keyword;
	gchar *n;
	gchar *result = NULL;
	GebrGeoXmlSequence *seq;
	GebrGeoXmlParameters *params;
	GebrGeoXmlProgramParameter *prog_param;
	GebrGeoXmlProgram *program;
	GebrGeoXmlProgramStatus status;
	GebrGeoXmlParameter *param;
	GError *err = NULL;

	program = gebr_geoxml_flow_get_control_program(job->flow);
	status = gebr_geoxml_program_get_status(program);
	if (gebr_geoxml_program_get_control(program) == GEBR_GEOXML_PROGRAM_CONTROL_FOR
	    && status == GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED)
	{
		gchar *iter_expr;
		gchar *ini, *step;
		GebrGeoXmlProgramParameter *pparam;
		n = gebr_geoxml_program_control_get_n(program, &step, &ini);
		gebr_validator_evaluate(gebrd_get_validator(gebrd), n, GEBR_GEOXML_PARAMETER_TYPE_FLOAT, GEBR_GEOXML_DOCUMENT_TYPE_LINE, &result, &err);
		if (err) {
			*issue_number += 1;
			job_issue(job, _("%u) %s '%s'.\n"),
			          *issue_number, err->message,
			          gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(program)));
			g_clear_error(&err);
		}
		result = g_strdup_printf("%d", atoi(result));
		iter_expr = g_strdup_printf("(%s) + (%s) * '\"$counter\"'", ini, step);
		pparam = GEBR_GEOXML_PROGRAM_PARAMETER(gebr_geoxml_document_get_dict_parameter(gebrd->flow));
		gebr_geoxml_program_parameter_set_first_value(pparam, FALSE, iter_expr);
		g_free(iter_expr);
		g_free(ini);
		g_free(step);
		g_free(n);
	}

	GebrGeoXmlDocument *docs[3] = {
		gebrd->proj,
		gebrd->line,
		gebrd->flow,
	};

	for (int i = 0; i < 3; i++) {
		// Insert variables definitions
		params = gebr_geoxml_document_get_dict_parameters(GEBR_GEOXML_DOCUMENT (docs[i]));
		gebr_geoxml_parameters_get_parameter(params, &seq, 0);
		for (; seq; gebr_geoxml_sequence_next(&seq))
		{
			gchar *label;
			gchar *var_value;
			GebrGeoXmlParameterType type = gebr_geoxml_parameter_get_type (GEBR_GEOXML_PARAMETER (seq));

			switch (type)
			{
			case GEBR_GEOXML_PARAMETER_TYPE_FLOAT:
			case GEBR_GEOXML_PARAMETER_TYPE_INT: {
				prog_param = GEBR_GEOXML_PROGRAM_PARAMETER (seq);
				param = GEBR_GEOXML_PARAMETER(seq);

				label = g_strdup(gebr_geoxml_parameter_get_label (param));
				value = gebr_geoxml_program_parameter_get_first_value (prog_param, FALSE);
				keyword = gebr_geoxml_program_parameter_get_keyword (prog_param);
				g_string_append_printf(expr_buf, "\t\t%s = (%s) ; %s # V[%"G_GSIZE_FORMAT"]: %s\n",
						       keyword, value, keyword, j, replace_quotes(label));
				var_value = g_strdup_printf("${V[[%"G_GSIZE_FORMAT"]]}", j++);
				gebr_geoxml_parameter_set_type(param, GEBR_GEOXML_PARAMETER_TYPE_STRING);
				gebr_validator_change_value(gebrd_get_validator(gebrd), param, var_value, NULL, NULL);
				g_free (var_value);
				g_free (label);
				break;
			}

			case GEBR_GEOXML_PARAMETER_TYPE_STRING: {
				GebrGeoXmlDocumentType scope;
				gchar *bash_var;
				gchar *result = NULL;
				prog_param = GEBR_GEOXML_PROGRAM_PARAMETER (seq);
				param = GEBR_GEOXML_PARAMETER(seq);

				value = gebr_geoxml_program_parameter_get_first_value (prog_param, FALSE);
				keyword = gebr_geoxml_program_parameter_get_keyword (prog_param);
				scope = gebr_geoxml_parameter_get_scope(param);
				gebr_validator_evaluate_interval(gebrd_get_validator(gebrd), value, GEBR_GEOXML_PARAMETER_TYPE_STRING, scope, FALSE, &result, NULL);
				bash_var = g_strdup_printf("${%s}", keyword);
				gebr_validator_change_value(gebrd_get_validator(gebrd), param, bash_var, NULL, NULL);
				g_string_append_printf(str_buf, "\t%s=\"%s\"\n", keyword, result);
				g_free(bash_var);
				g_free(result);
				break;
			}
			default:
				continue;
			}
		}
	}

	*n_vars = j;

	return result;
}

/*
 * assemble_bc_cmd_line:
 */
static void assemble_bc_cmd_line (GString *expr_buf)
{
	// If there are no expressions, don't bother creating the command line!
	if (expr_buf->len == 0)
		return;

	 g_string_prepend(expr_buf,
			  "\tV=($(echo 'scale=5\n"
	                  "\t\tdefine min(a,b){ if(a<b) {return a;} else {return b;}}\n"
	                  "\t\tdefine max(a,b){ if(a>b) {return a;} else {return b;}}\n"
	                  "\t\tdefine round(x){ auto s; s = scale; if(x>0) x+=0.5 else x-=0.5; scale = 0; x/=1; scale = s; return (x);}\n");

	// Pipe into bc
	g_string_append (expr_buf, "\t' | bc -l ))\n");
}

gboolean gebr_output_use_var_iter(GebrdJob *job, const gchar *output_expr)
{
	return gebr_validator_use_iter(gebrd_get_validator(gebrd), output_expr,
	                               GEBR_GEOXML_PARAMETER_TYPE_STRING, GEBR_GEOXML_DOCUMENT_TYPE_FLOW);
}

static void job_assembly_cmdline(GebrdJob *job)
{
	gboolean has_error_output_file;
	gboolean previous_stdout;
	guint issue_number = 0;
	GebrGeoXmlSequence *program;
	GebrdMpiInterface *mpi;
	gulong nprog;
	gboolean has_control = FALSE;
	gchar *n;
	GString *expr_buf = g_string_new("");
	GString *str_buf = g_string_new("");

	job->expr_count = 0;

	if (job->flow == NULL) 
		goto err;

	job->is_parallelizable =
		gebr_geoxml_flow_is_parallelizable(job->flow, gebrd_get_validator(gebrd));

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

	if (gebr_geoxml_program_get_control(GEBR_GEOXML_PROGRAM(program)) == GEBR_GEOXML_PROGRAM_CONTROL_FOR) {
		has_control = TRUE;
		/* Check if there is configured programs */
		gebr_geoxml_sequence_next(&program);
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
	}

	/* Configure MPI */
	const gchar * mpiname;
	mpiname = gebr_geoxml_program_get_mpi(GEBR_GEOXML_PROGRAM(program));
	mpi = job_get_mpi_impl(mpiname, job->parent.n_process);
	if (strlen(mpiname) && !mpi) {
		job_issue(job,
			  _("Requested MPI (%s) is not supported by this server.\n"), mpiname);
		goto err;
	}

	/* Binary followed by an space */
	const gchar * binary;
	binary = gebr_geoxml_program_get_binary(GEBR_GEOXML_PROGRAM(program));
	if (mpi == NULL)
		g_string_append_printf(job->parent.cmd_line, "%s%s ",
				       job->is_parallelizable ? "$exec ":"",
				       binary);
	else {
		gchar * mpicmd;
		mpicmd = gebrd_mpi_interface_build_comand(mpi, binary);
		g_string_append_printf(job->parent.cmd_line, "%s ", mpicmd);
		g_free(mpicmd);
	}

	// define variables on bc, to use on stdin, stdout, stderr and expressions
	n = define_bc_variables(job, expr_buf, str_buf, &job->n_vars, &issue_number);

	if (job_add_program_parameters(job, GEBR_GEOXML_PROGRAM(program), expr_buf) == FALSE)
		goto err;

	/*
	 * First program
	 */
	/* Start without stdin */
	if (gebr_geoxml_program_get_stdin(GEBR_GEOXML_PROGRAM(program))) {
		if (strlen(gebr_geoxml_flow_io_get_input(job->flow)) == 0) {
			job_issue(job, _("No input file selected.\n"));
			goto err;
		}

		gchar *result;
		const gchar *input_expr;
		GError *error = NULL;

		input_expr = gebr_geoxml_flow_io_get_input(job->flow);
		gebr_validator_evaluate_interval(gebrd_get_validator(gebrd), input_expr, GEBR_GEOXML_PARAMETER_TYPE_STRING, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, FALSE, &result, &error);

		if (!error) {
			gchar *escaped = escape_quote_and_slash(result);
			g_string_append_printf(job->parent.cmd_line, "<\"%s\" ", escaped);
			g_free(result);
			g_free(escaped);
		} else {
			switch (error->code) {
			case GEBR_IEXPR_ERROR_SYNTAX:
			case GEBR_IEXPR_ERROR_INVAL_VAR:
				job_issue(job, _("Syntax error in input file expression"));
				break;
			case GEBR_IEXPR_ERROR_UNDEF_VAR:
				job_issue(job, _("Undefined variable in input file expression"));
				break;
			}
			g_error_free(error);
			goto err;
		}
	}

	/* These variables keep the parsed stdout and stderr */
	gchar *stdout_parsed = NULL;
	gchar *stderr_parsed = NULL;

	gboolean stderr_use_iter = FALSE;
	/* check for error file output */
	if (has_error_output_file && gebr_geoxml_program_get_stderr(GEBR_GEOXML_PROGRAM(program))) {
		gchar *result;
		GError *error = NULL;
		const gchar *error_expr;

		error_expr = gebr_geoxml_flow_io_get_error(job->flow);
		gebr_validator_evaluate_interval(gebrd_get_validator(gebrd), error_expr, GEBR_GEOXML_PARAMETER_TYPE_STRING, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, FALSE, &result, &error);

		if (!error) {
			stderr_use_iter = gebr_output_use_var_iter(job, error_expr);
			stderr_parsed = escape_quote_and_slash(result);
			if(gebr_geoxml_flow_io_get_error_append(job->flow) ||
			   (has_control && !stderr_use_iter))
				g_string_append_printf(job->parent.cmd_line, "2>> \"%s\" ", stderr_parsed);
			else
				g_string_append_printf(job->parent.cmd_line, "2> \"%s\" ", stderr_parsed);
			g_free(result);
		} else {
			switch (error->code) {
			case GEBR_IEXPR_ERROR_SYNTAX:
			case GEBR_IEXPR_ERROR_INVAL_VAR:
				job_issue(job, _("Syntax error in error file expression"));
				break;
			case GEBR_IEXPR_ERROR_UNDEF_VAR:
				job_issue(job, _("Undefined variable in error file expression"));
				break;
			}
			g_error_free(error);
			goto err;
		}
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

		if (gebr_geoxml_program_get_control(GEBR_GEOXML_PROGRAM(program)) == GEBR_GEOXML_PROGRAM_CONTROL_FOR) {
			has_control = TRUE;
			gebr_geoxml_sequence_next(&program);
			continue;
		}

		mpi = job_get_mpi_impl(gebr_geoxml_program_get_mpi(GEBR_GEOXML_PROGRAM(program)),
				       job->parent.n_process);

		/* How to connect chained programs */
		int chain_option = gebr_geoxml_program_get_stdin(GEBR_GEOXML_PROGRAM(program)) + (previous_stdout << 1);
		const gchar *sep = NULL;
		switch (chain_option) {
		case 0:
			/* Previous does not write to stdin and current does not carry about */
			sep = ";";
			break;
		case 1:	/* Previous does not write to stdin but current expect something */
			job_issue(job, _("Broken flow before %s (no input).\n"),
				  gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(program)));
			goto err;
		case 2:	/* Previous does write to stdin but current does not carry about */
			job_issue(job, _("Broken flow before %s (unexpected output).\n"),
				  gebr_geoxml_program_get_title(GEBR_GEOXML_PROGRAM(program)));
			goto err;
		case 3:
			/* Both talk to each other */
			sep = "|";
			break;
		default:
			break;
		}

		const gchar * binary;

		binary = gebr_geoxml_program_get_binary(GEBR_GEOXML_PROGRAM(program));
		if (mpi == NULL)
			g_string_append_printf(job->parent.cmd_line, "%s %s%s ",
					       sep,
					       job->is_parallelizable ? "$exec ":"",
					       binary);
		else {
			gchar * mpicmd;
			mpicmd = gebrd_mpi_interface_build_comand(mpi, binary);
			g_string_append_printf(job->parent.cmd_line, "%s %s ", sep, mpicmd);
			g_free(mpicmd);
		}

		if (job_add_program_parameters(job, GEBR_GEOXML_PROGRAM(program), expr_buf) == FALSE)
			goto err;

		previous_stdout = gebr_geoxml_program_get_stdout(GEBR_GEOXML_PROGRAM(program));
		gebr_geoxml_sequence_next(&program);
	}

	gboolean stdout_use_iter = FALSE;

	if (has_error_output_file == FALSE)
		job_issue(job,
			  _("No error file selected; error output merged with standard output.\n"));

	if (previous_stdout) {
		if (strlen(gebr_geoxml_flow_io_get_output(job->flow)) == 0)
			job_issue(job, _("Proceeding without output file.\n"));
		else {
			gchar *result;
			const gchar *output_expr;
			GError *error = NULL;

			output_expr = gebr_geoxml_flow_io_get_output(job->flow);
			gebr_validator_evaluate_interval(gebrd_get_validator(gebrd), output_expr, GEBR_GEOXML_PARAMETER_TYPE_STRING, GEBR_GEOXML_DOCUMENT_TYPE_FLOW, FALSE, &result, &error);
			if (!error) {
				stdout_use_iter = gebr_output_use_var_iter(job, output_expr);
				stdout_parsed = escape_quote_and_slash(result);

				if (gebr_geoxml_flow_io_get_output_append(job->flow) ||
				    (has_control && !stdout_use_iter))
					g_string_append_printf(job->parent.cmd_line, ">> \"%s\" ", stdout_parsed);
				else
					g_string_append_printf(job->parent.cmd_line, "> \"%s\" ", stdout_parsed);
				g_free(result);
			} else {
				switch (error->code) {
				case GEBR_IEXPR_ERROR_SYNTAX:
				case GEBR_IEXPR_ERROR_INVAL_VAR:
					job_issue(job, _("Syntax error in output file expression"));
					break;
				case GEBR_IEXPR_ERROR_UNDEF_VAR:
					job_issue(job, _("Undefined variable in output file expression"));
					break;
				}
				g_error_free(error);
				goto err;
			}
		}
	}
	if (has_control && n) {
		gchar *prefix;
		gchar *remove;
		gint nprocs, nice;

		assemble_bc_cmd_line (expr_buf);
		nprocs = gebrd_app_set_heuristic_aggression(gebrd, atoi(job->exec_speed->str), &nice);

		if (job->is_parallelizable) {
			job_issue(job, "This flow is executed with %d processor(s) using %s of machine.\n", nprocs, nice == 0? "all the resources" : "only the idle time");
			prefix = g_strdup_printf("PROC=%d\n"
						 "NICE=%d\n"
						 "exec=\"nice -n $NICE\"\n"
						 "for (( _outter=0; _outter < %s; _outter+=$PROC ))\n"
						 "do\n"
						 "  unset PIDS\n"
						 "  for (( counter=$_outter; counter < $_outter+$PROC && counter < %s; counter++ ))\n"
						 "  do\n"
						 "    %s\n%s",
						 nprocs, nice, n, n, expr_buf->str, str_buf->str);
			g_string_append(job->parent.cmd_line, " ) &\nPIDS=\"$! $PIDS\"");
			g_string_prepend_c(job->parent.cmd_line, '(');
		} else {
			job_issue(job, "This flow is executed with 1 processor(s) using all the resources of machine.\n");
			prefix = g_strdup_printf("for (( counter=0; counter<%s; counter++ ))\ndo\n%s\n%s",
						 n, expr_buf->str, str_buf->str);
		}
		if (!gebr_geoxml_flow_io_get_output_append(job->flow) && !stdout_use_iter &&
		    strlen(gebr_geoxml_flow_io_get_output(job->flow)) > 0 && previous_stdout) {
			remove = g_strdup_printf("\n\ttest $counter -eq 0 && > %s\n", stdout_parsed);
			g_string_prepend(job->parent.cmd_line, remove);
			g_free(remove);
		}
		if (!gebr_geoxml_flow_io_get_error_append(job->flow) && !stderr_use_iter &&
		    strlen(gebr_geoxml_flow_io_get_error(job->flow)) > 0) {
			remove = g_strdup_printf("\n\ttest $counter -eq 0 && > %s\n", stderr_parsed);
			g_string_prepend(job->parent.cmd_line, remove);
			g_free(remove);
		}
		g_string_prepend(job->parent.cmd_line, prefix);
		if (job->is_parallelizable)
			g_string_append_printf(job->parent.cmd_line, "\n"
					       "  done\n"
					       "  wait $PIDS\n");
		g_string_append(job->parent.cmd_line, "\ndone");
		g_free(prefix);
		g_free(n);
	} else {
		assemble_bc_cmd_line (expr_buf);
		g_string_prepend(job->parent.cmd_line, str_buf->str);
		g_string_prepend(job->parent.cmd_line, expr_buf->str);
	}

	job->critical_error = FALSE;
	g_string_free(expr_buf, TRUE);
	g_string_free(str_buf, TRUE);
	return;
err:	
	g_string_free(expr_buf, TRUE);
	g_string_assign(job->parent.cmd_line, "");
	job->critical_error = TRUE;
}

static gchar *escape_quote_and_slash(const gchar *str)
{
	gsize i = 0;
	gchar *s = g_new(gchar, strlen(str)*2 + 1);

	while (*str)
	{
		if (*str == '\\' || *str == '"')
			s[i++] = '\\';
		s[i++] = *str;
		str++;
	}
	s[i] = '\0';
	return s;
}
