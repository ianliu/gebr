/*
 * gebr-comm-runner.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2011 - GêBR core team (http://www.gebrproject.com)
 *
 * GêBR Project is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GêBR Project is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GêBR Project. If not, see <http://www.gnu.org/licenses/>.
 */

#include "gebr-comm-runner.h"

#include <libgebr/geoxml/geoxml.h>
#include <libgebr/comm/gebr-comm.h>
#include <stdio.h>
#include <stdlib.h>


typedef struct {
	GebrCommServer *server;
	gdouble score;
	gint eff_ncores;
} ServerScore;

struct _GebrCommRunnerPriv {
	gchar *id;
	GebrGeoXmlDocument *flow;
	GList *servers;
	GebrValidator *validator;

	gint requests;
	gint responses;

	gint total;
	gchar *ncores;
	gchar *parent_rid;
	gchar *speed;
	gchar *nice;
	gchar *group;
	gchar *servers_str;
	gchar *nprocs;
	gchar *servers_list;

	void (*ran_func) (GebrCommRunner *runner,
			  gpointer data);
	gpointer user_data;

	gchar *account; // MOAB
	gchar *num_processes; // MPI
};

/* Private methods {{{1 */
/*
 * strip_flow:
 * @validator:
 * @flow: a #GebrGeoXmlFlow.
 *
 * Make a copy of @flow and removes all help strings. All dictionaries from
 * @line and @proj are merged into the copy.
 *
 * Returns: a new flow, in XML string, prepared to run.
 */
static gchar *
strip_flow(GebrValidator *validator,
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

	gchar *xml;
	gebr_geoxml_document_to_string(clone, &xml);
	gebr_geoxml_document_free(clone);

	return xml;
}

/* Public methods {{{1 */
GebrCommRunner *
gebr_comm_runner_new(GebrGeoXmlDocument *flow,
		     GList *servers,
		     const gchar *parent_rid,
		     const gchar *speed,
		     const gchar *nice,
		     const gchar *group,
		     GebrValidator *validator)
{
	GebrCommRunner *self = g_new(GebrCommRunner, 1);
	self->priv = g_new0(GebrCommRunnerPriv, 1);
	self->priv->flow = gebr_geoxml_document_ref(flow);
	self->priv->parent_rid = g_strdup(parent_rid);
	self->priv->speed = g_strdup(speed);
	self->priv->nice = g_strdup(nice);
	self->priv->group = g_strdup(group);
	self->priv->validator = validator;

	self->priv->servers = NULL;
	for (GList *i = servers; i; i = i->next) {
		ServerScore *sc = g_new0(ServerScore, 1);
		sc->server = i->data;
		sc->score  = 0;
		self->priv->servers = g_list_prepend(self->priv->servers, sc);
	}

	return self;
}

void
gebr_comm_runner_free(GebrCommRunner *self)
{
	// TODO gebr_comm_runner_free
}

/*
 * Compute the parameters of a polynomial (2nd order) fit
 */
void
create_2ndDegreePoly_model(GList *points, gdouble parameters[])
{
	gdouble M, MA, MB, MC, **N;
	gint j;
	GList *i;
	N = g_new(gdouble*, 3);
	for (i = points, j = 0; i; i = i->next, j++) {
		gdouble *p = i->data;
		N[j] = g_new(gdouble, 4);
		N[j][0] = 1;
		N[j][1] = p[0];
		N[j][2] = N[j][1]*N[j][1];
		N[j][3] = p[1];
	}

	M = (N[1][1] - N[0][1]) * (N[2][1] - N[0][1]) * (N[2][1] - N[1][1]);
	MA = N[2][3]*(N[1][1]-N[0][1]) + N[1][3]*(N[0][1]-N[2][1]) + N[0][3]*(N[2][1]-N[1][1]);
	MB = N[2][3]*(N[0][2]-N[1][2]) + N[1][3]*(N[2][2]-N[0][2]) + N[0][3]*(N[1][2]-N[2][2]);
	MC = N[2][3]*(N[0][1]*N[1][2] - N[1][1]*N[0][2]) + N[1][3]*(N[2][1]*N[0][2]-N[0][1]*N[2][2]) + N[0][3]*(N[1][1]*N[2][2]-N[2][1]*N[1][2]);

	// Equation Ax2 + Bx + C = 0
	parameters[0] = MA/M;
	parameters[1] = MB/M;
	parameters[2] = MC/M;

	for (gint j=0; j<3; j++)
		g_free(N[j]);
	g_free(N);
}

/*
 * Predict the future load
 */
static gdouble
predict_current_load(GList *points, gdouble delay)
{
	gdouble prediction;
	gdouble parameters[3];
	create_2ndDegreePoly_model(points, parameters);
	prediction = parameters[0]*delay*delay + parameters[1]*delay + parameters[2];

	return MAX(0, prediction);
}

/*
 * Compute the score of a server
 */
static gdouble
calculate_server_score(const gchar *load, gint ncores, gdouble cpu_clock, gint scale, gint nsteps, gint *eff_ncores)
{
	GList *points = NULL;
	gdouble delay = 1.0;
	gdouble p1[2];
	gdouble p5[2];
	gdouble p15[2];

	sscanf(load, "%lf %lf %lf", &p1[1], &p5[1], &p15[1]);

	p1[0] = -1.0;
	p5[0] = -5.0;
	p15[0] = -15.0;

	points = g_list_prepend(points, p1);
	points = g_list_prepend(points, p5);
	points = g_list_prepend(points, p15);

	gdouble current_load = predict_current_load(points, delay);
	*eff_ncores = MAX(1, MIN(nsteps, scale*ncores/5));

	gdouble score;
	if (current_load + (*eff_ncores) > ncores)
		score = cpu_clock*(*eff_ncores)/(current_load + (*eff_ncores) - ncores + 1);
	else
		score = cpu_clock*(*eff_ncores);

	g_list_free(points);

	return score;
}

static void
divide_and_run_flows(GebrCommRunner *self)
{
	gdouble sum = 0;
	gint n = 0;
	gint ncores = 0;

	for (GList *i = self->priv->servers; i; i = i->next) {
		ServerScore *sc = i->data;
		sum += sc->score;
		ncores += sc->eff_ncores;
		n++;
	}

	self->priv->ncores = g_strdup_printf("%d", ncores);

	double *weights = g_new(double, n);

	GList *j = self->priv->servers;
	for (int i = 0; i < n; i++) {
		weights[i] = ((ServerScore*)j->data)->score / sum;
		j = j->next;
	}

	GList *flows = gebr_geoxml_flow_divide_flows(GEBR_GEOXML_FLOW(self->priv->flow),
						     self->priv->validator,
						     weights, n);

	gint frac = 1;
	GList *i = flows;
	j = self->priv->servers;

	GString *server_list = g_string_new("");
	for (int k = 0; i; k++) {
		GebrGeoXmlFlow *flow = i->data;
		ServerScore *sc = j->data;
		gchar *frac_str = g_strdup_printf("%d", frac);
		gchar *flow_xml = strip_flow(self->priv->validator, flow);

		g_string_append_printf(server_list, "%s,%lf,",
				       sc->server->address->str, weights[k]);

		gebr_comm_protocol_socket_oldmsg_send(sc->server->socket, FALSE,
						      gebr_comm_protocol_defs.run_def, 7,
						      self->priv->id,
						      frac_str,
						      self->priv->speed,
						      self->priv->nice,
						      flow_xml,

						      /* Moab and MPI settings */
						      self->priv->account ? self->priv->account : "",
						      self->priv->num_processes ? self->priv->num_processes : "");

		g_free(frac_str);
		g_free(flow_xml);

		frac++;
		i = i->next;
		j = j->next;
	}

	self->priv->total = frac - 1;
	g_string_erase(server_list, server_list->len-1, 1);
	self->priv->servers_list = g_string_free(server_list, FALSE);

	if (self->priv->ran_func)
		self->priv->ran_func(self, self->priv->user_data);
}

static void
on_response_received(GebrCommHttpMsg *request,
		     GebrCommHttpMsg *response,
		     GebrCommRunner  *self)
{
	g_return_if_fail(request->method == GEBR_COMM_HTTP_METHOD_GET);

	GebrCommJsonContent *json = gebr_comm_json_content_new(response->content->str);
	GString *value = gebr_comm_json_content_to_gstring(json);
	ServerScore *sc = g_object_get_data(G_OBJECT(request), "current-server");

	gint n;
	GebrGeoXmlProgram *loop = gebr_geoxml_flow_get_control_program(GEBR_GEOXML_FLOW(self->priv->flow));

	if (loop) {
		gchar *eval_n;
		gchar *str_n = gebr_geoxml_program_control_get_n(loop, NULL, NULL);
		gebr_validator_evaluate(self->priv->validator, str_n,
					GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
					GEBR_GEOXML_DOCUMENT_TYPE_LINE, &eval_n, NULL);
		n = atoi(eval_n);
		g_free(str_n);
		g_free(eval_n);
		gebr_geoxml_object_unref(loop);
	} else
		n = 1;

	sc->score = calculate_server_score(value->str,
					   sc->server->ncores,
					   sc->server->clock_cpu,
					   atoi(self->priv->speed),
					   n, &sc->eff_ncores);

	g_debug("Score for %s: %lf", sc->server->address->str, sc->score);

	self->priv->responses++;
	if (self->priv->responses == self->priv->requests) {
		gint comp_func(ServerScore *a, ServerScore *b) {
			return b->score - a->score;
		}

		self->priv->servers = g_list_sort(self->priv->servers, (GCompareFunc)comp_func);
		divide_and_run_flows(self);
	}
}

void
gebr_comm_runner_set_ran_func(GebrCommRunner *self,
			      void (*func) (GebrCommRunner *runner,
					    gpointer data),
			      gpointer data)
{
	self->priv->ran_func = func;
	self->priv->user_data = data;
}

void
gebr_comm_runner_run_async(GebrCommRunner *self,
			   const gchar *id)
{
	self->priv->requests = 0;
	self->priv->responses = 0;
	self->priv->id = g_strdup(id);

	for (GList *i = self->priv->servers; i; i = i->next) {
		GebrCommHttpMsg *request;
		ServerScore *sc = i->data;
		request = gebr_comm_protocol_socket_send_request(sc->server->socket,
								 GEBR_COMM_HTTP_METHOD_GET,
								 "/sys-load", NULL);
		g_object_set_data(G_OBJECT(request), "current-server", sc);
		g_signal_connect(request, "response-received",
				 G_CALLBACK(on_response_received), self);
		self->priv->requests++;
	}
}

GebrValidator *
gebr_comm_runner_get_validator(GebrCommRunner *self)
{
	return self->priv->validator;
}

const gchar *
gebr_comm_runner_get_nprocs(GebrCommRunner *self)
{
	return self->priv->nprocs;
}

const gchar *
gebr_comm_runner_get_servers_list(GebrCommRunner *self)
{
	return self->priv->servers_list;
}

gint
gebr_comm_runner_get_total(GebrCommRunner *self)
{
	return self->priv->total;
}
