/*   libgebr - GeBR Library
 *   Copyright (C) 2011 GeBR core team (http://www.gebrproject.com/)
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

#include "gebr-comm-runner.h"

#include <libgebr/geoxml/geoxml.h>
#include <libgebr/comm/gebr-comm.h>

/* Private methods {{{1 */
/*
 * gebr_comm_server_run_strip_flow:
 * @validator:
 * @flow: a #GebrGeoXmlFlow.
 *
 * Make a copy of @flow and removes all help strings. All dictionaries from
 * @line and @proj are merged into the copy.
 *
 * Returns: a new flow prepared to run.
 */
static GebrGeoXmlFlow *
gebr_comm_server_run_strip_flow(GebrValidator *validator,
				GebrGeoXmlFlow *flow)
{
	GebrGeoXmlSequence *i;
	GebrGeoXmlDocument *clone;
	GebrGeoXmlDocument *line;
	GebrGeoXmlDocument *proj;

	g_return_val_if_fail (flow != NULL, NULL);

	clone = gebr_geoxml_document_clone(GEBR_GEOXML_DOCUMENT(flow));

	/* Strip flow: remove helps and revisions */
	gebr_geoxml_document_set_help(clone, "");
	gebr_geoxml_flow_get_program(GEBR_GEOXML_FLOW(clone), &i, 0);
	for (; i != NULL; gebr_geoxml_sequence_next(&i))
		gebr_geoxml_program_set_help(GEBR_GEOXML_PROGRAM(i), "");

	/* clear all revisions */
	gebr_geoxml_flow_get_revision(GEBR_GEOXML_FLOW (clone), &i, 0);
	while (i != NULL) {
		GebrGeoXmlSequence *tmp;
		gebr_geoxml_object_ref(i);
		tmp = i;
		gebr_geoxml_sequence_next(&tmp);
		gebr_geoxml_sequence_remove(i);
		i = tmp;
	}

	/* Merge and Strip invalid parameters in dictionary */
	i = gebr_geoxml_document_get_dict_parameter(clone);
	while (i != NULL) {
		if (validator && !gebr_validator_validate_param(validator, GEBR_GEOXML_PARAMETER(i), NULL, NULL)) {
			GebrGeoXmlSequence *aux = i;
			gebr_geoxml_object_ref(aux);
			gebr_geoxml_sequence_next(&i);
			gebr_geoxml_sequence_remove(aux);
			continue;
		}
		gebr_geoxml_sequence_next(&i);
	}
	gebr_validator_get_documents(validator, NULL, &line, &proj);
	gebr_geoxml_document_merge_dicts(validator, clone, line, proj, NULL);

	return GEBR_GEOXML_FLOW (clone);
}

/* Public methods {{{1 */
GebrCommRunner *
gebr_comm_runner_new(void)
{
	GebrCommRunner *self = g_new(GebrCommRunner, 1);
	self->flows = NULL;
	self->parallel = FALSE;
	self->account = self->queue = self->num_processes = NULL;
	self->is_parallelizable = FALSE;
	return self;
}

void
gebr_comm_runner_free(GebrCommRunner *self)
{
	void free_each(GebrCommRunnerFlow *run_flow)
	{
		g_free(run_flow->flow_xml);
		g_free(run_flow->frac);
		g_free(run_flow);
	}

	g_list_foreach(self->flows, (GFunc)free_each, NULL);
	g_list_free(self->flows);
	g_free(self->account);
	g_free(self->queue);
	g_free(self->execution_speed);
	g_free(self);
}

GebrCommRunnerFlow *
gebr_comm_runner_add_flow(GebrCommRunner *self,
			  GebrValidator  *validator,
			  GebrGeoXmlFlow *flow,
			  GebrCommServer *server,
			  gboolean 	  divided,
			  const gchar    *sessid,
			  gpointer        user_data)
{
	static guint run_id = 0;
	GebrCommRunnerFlow *run_flow = g_new(GebrCommRunnerFlow, 1);
	GebrGeoXmlFlow *stripped = gebr_comm_server_run_strip_flow(validator, flow);
	gchar *xml;

	gebr_geoxml_document_to_string(GEBR_GEOXML_DOC(stripped), &xml);
	gebr_geoxml_document_free(GEBR_GEOXML_DOCUMENT(stripped));

	run_flow->flow = flow;
	run_flow->server = server;
	run_flow->flow_xml = xml;
	run_flow->frac = g_strdup("1:1");
	run_flow->user_data = user_data;

	if (divided)
		run_flow->run_id = g_strdup_printf("%u:%s", run_id, sessid);
	else
		run_flow->run_id = g_strdup_printf("%u:%s", run_id++, sessid);

	self->flows = g_list_append(self->flows, run_flow);

	return run_flow;
}

void
gebr_comm_runner_flow_set_frac(GebrCommRunnerFlow *self, gint frac, gint total)
{
	if (self->frac)
		g_free(self->frac);
	self->frac = g_strdup_printf("%d:%d", frac, total);
}

void
gebr_comm_runner_run(GebrCommRunner *self)
{
	GString *server_list;
	GebrCommRunnerFlow *runflow = self->flows->data;

	server_list = g_string_new(runflow->server->address->str);
	for (GList *i = self->flows->next; i != NULL; i = g_list_next(i)) {
		runflow = i->data;
		g_string_append_printf(server_list, ",%s", runflow->server->address->str);
	}

	for (GList *i = self->flows; i != NULL; i = g_list_next(i)) {
		runflow = i->data;
		gebr_comm_protocol_socket_oldmsg_send(runflow->server->socket, FALSE,
						      gebr_comm_protocol_defs.run_def, 8,
						      runflow->flow_xml,
						      self->account ? self->account : "",
						      self->queue ? self->queue : "",
						      self->num_processes ? self->num_processes : "",
						      runflow->run_id,
						      self->execution_speed,
						      runflow->frac,
						      server_list->str);
	}

	g_string_free(server_list, TRUE);
}
