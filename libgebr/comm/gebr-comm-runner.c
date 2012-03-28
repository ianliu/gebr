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

#include <libgebr/utils.h>
#include <libgebr/geoxml/geoxml.h>
#include <libgebr/comm/gebr-comm.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "gebr-comm-daemon.h"


typedef struct {
	GebrCommDaemon *server;
	gdouble score;
} ServerScore;

struct _GebrCommRunnerPriv {
	gchar *id;
	GebrGeoXmlDocument *flow;
	GList *servers;
	GList *run_servers;
	GebrValidator *validator;
	gdouble *weights;
	gint *numprocs;
	gint *distributed_n;

	gint requests;
	gint responses;
	GList *cores_scores;

	gint total;
	gchar *ncores;
	gchar *gid;
	gchar *parent_rid;
	gchar *speed;
	gchar *nice;
	gchar *group;
	gchar *servers_list;
	gchar *paths;
	gchar *mpi_owner;
	gchar *mpi_flavor;

	void (*ran_func) (GebrCommRunner *runner,
			  gpointer data);
	gpointer user_data;

	gchar *account; // MOAB
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
                     GList *submit_servers,
                     GList *run_servers,
                     const gchar *id,
		     const gchar *gid,
		     const gchar *parent_rid,
		     const gchar *speed,
		     const gchar *nice,
		     const gchar *group,
		     const gchar *paths,
		     GebrValidator *validator)
{
	GebrCommRunner *self = g_new(GebrCommRunner, 1);
	self->priv = g_new0(GebrCommRunnerPriv, 1);
	self->priv->flow = gebr_geoxml_document_ref(flow);
	self->priv->id = g_strdup(id);
	self->priv->gid = g_strdup(gid);
	self->priv->parent_rid = g_strdup(parent_rid);
	self->priv->speed = g_strdup(speed);
	self->priv->nice = g_strdup(nice);
	self->priv->group = g_strdup(group);
	self->priv->paths = g_strdup(paths);
	self->priv->validator = validator;
	self->priv->run_servers = g_list_copy(run_servers);
	self->priv->cores_scores = NULL;
	self->priv->mpi_owner = g_strdup("");
	self->priv->mpi_flavor = g_strdup("");

	self->priv->servers = g_list_copy(submit_servers);

	return self;
}

void
gebr_comm_runner_free(GebrCommRunner *self)
{
	g_free(self->priv->id);
	g_free(self->priv->gid);
	g_free(self->priv->parent_rid);
	g_free(self->priv->speed);
	g_free(self->priv->nice);
	g_free(self->priv->group);
	g_free(self->priv->paths);
	g_free(self->priv->mpi_owner);
	g_free(self->priv->mpi_flavor);
	g_free(self->priv->weights);
	g_free(self->priv->numprocs);
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
static GList *
calculate_server_score(GebrCommDaemon *daemon, const gchar *load, gint ncores, gdouble cpu_clock, gint running_jobs)
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

	GList *score = NULL;
	gdouble base = floor(current_load/ncores);
	gdouble rest = current_load - base;

	for (gint i = 0; i < ncores; i++) {
		ServerScore *sc = g_new(ServerScore, 1);
		sc->score = base;
		if (rest > 1) {
			sc->score += 1;
			rest -= 1;
		} else if (rest > 0) {
			sc->score += rest;
			rest = 0;
		}
		sc->server = daemon;
		sc->score = cpu_clock/(sc->score + 1);

		score = g_list_prepend(score, sc);

		g_debug("SCORE = %lf FOR CORE %d OF DAEMON %s", sc->score, i, gebr_comm_daemon_get_hostname(daemon));
	}

	g_list_free(points);

	return score;
}

typedef struct {
	GebrCommDaemon *daemon;
	gdouble weight; /* The sum of all its cores scores */
	gint np; /* Number of cores to use at @daemon */
	gint nsteps; /* Number of steps this @daemon will execute */
} DaemonExecInfo;

static gint *
distribute_tasks_to_cores(GebrCommRunner *self,
                          gint total_procs,
                          gint nsteps)
{
	GList *j;
	gdouble sum = 0;
	gint n_cores = g_list_length(self->priv->cores_scores);
	gint n = MIN(total_procs, n_cores);

	gdouble *weights = g_new0(gdouble, n);

	j = self->priv->cores_scores;
	for (gint i = 0; i < total_procs; i++, j = j->next) {
		if (!j)
			j = self->priv->cores_scores;
		ServerScore *sc = j->data;
		sum += sc->score;
	}

	j = self->priv->cores_scores;
	for (gint i = 0; i < total_procs; i++, j = j->next) {
		if (!j)
			j = self->priv->cores_scores;
		ServerScore *sc = j->data;
		weights[i % n] = sc->score/sum;
	}

	gint *distributed_n = gebr_geoxml_flow_calculate_proportional_n(nsteps, weights, n);

	g_free(weights);

	return distributed_n;
}

static GList * /* DaemonExecInfo */
calculate_servers_scores_and_num_procs(GebrCommRunner *self)
{
	gint n_cores = g_list_length(self->priv->cores_scores);
	gdouble speed = atof(self->priv->speed);
	GList *j;

	GebrGeoXmlProgram *loop = gebr_geoxml_flow_get_control_program(GEBR_GEOXML_FLOW(self->priv->flow));
	gint nsteps;
	if (loop && gebr_geoxml_flow_is_parallelizable(GEBR_GEOXML_FLOW(self->priv->flow), self->priv->validator))
		nsteps = gebr_geoxml_program_control_get_eval_n(loop, self->priv->validator);
	else
		nsteps = 1;

	gint total_procs = MIN(gebr_calculate_number_of_processors(n_cores, speed), nsteps);

	gint *distributed_n = distribute_tasks_to_cores(self, total_procs, nsteps);

	GHashTable *map = g_hash_table_new(g_str_hash, g_str_equal);
	for (GList *i = self->priv->servers; i; i = i->next) {
		DaemonExecInfo *data = g_new0(DaemonExecInfo, 1);
		data->daemon = i->data;
		g_hash_table_insert(map, g_strdup(gebr_comm_daemon_get_hostname(i->data)), data);
	}

	gint k;
	j = self->priv->cores_scores;
	for (k = 0; j && k < total_procs; j = j->next, k++) {
		ServerScore *sc = j->data;
		DaemonExecInfo *data = g_hash_table_lookup(map, gebr_comm_daemon_get_hostname(sc->server));
		data->np++;
		data->weight += (gdouble)distributed_n[k]/nsteps;
		data->nsteps += distributed_n[k];
	}

	if (j) {
		if (j->prev) {
			j->prev->next = NULL;
			j->prev = NULL;
		} else {
			 self->priv->cores_scores = NULL;
		}
		g_list_foreach(j, (GFunc)g_free, NULL);
		g_list_free(j);
	}


	GList *values = g_hash_table_get_values(map);

	gint n_servers = g_list_length(values);
	gint tmp = total_procs - k;
	gint base = tmp/n_servers;
	gint rest = tmp - base * n_servers;

	GList *i = values;
	while (i) {
		GList *aux = NULL;
		DaemonExecInfo *data = i->data;

		data->np += base;
		if (rest > 0) {
			data->np++;
			rest--;
		}

		if (data->np == 0) {
			aux = i->next;
			g_free(data);
			values = g_list_delete_link(values, i);
			i = aux;
		} else {
			i = i->next;
		}
	}

	g_hash_table_destroy(map);

	return values;
}

static GList *
normalize_and_sort_servers(GList *values)
{
	gdouble sum = 0;

	for (GList *i = values; i; i = i->next) {
		DaemonExecInfo *data = i->data;
		sum += data->weight;
	}

	for (GList *i = values; i; i = i->next) {
		DaemonExecInfo *data = i->data;
		data->weight /= sum;
	}

	gint comp_func(gconstpointer a, gconstpointer b) {
		const DaemonExecInfo *d1 = a, *d2 = b;
		return d2->weight - d1->weight;
	}

	return g_list_sort(values, comp_func);
}

static void
set_servers_execution_info(GebrCommRunner *self)
{
	GList *values = calculate_servers_scores_and_num_procs(self);
	values = normalize_and_sort_servers(values);

	gint n = g_list_length(values);
	double *weights = g_new0(double, n);
	gint *numprocs = g_new0(gint, n);
	gint *distributed_n = g_new0(gint, n);

	g_list_free(self->priv->servers);
	self->priv->servers = NULL;

	gint k = 0;
	for (GList *i = values; i; i = i->next, k++) {
		DaemonExecInfo *data = i->data;
		self->priv->servers = g_list_prepend(self->priv->servers, data->daemon);
		weights[k] = data->weight;
		numprocs[k] = data->np;
		distributed_n[k] = data->nsteps;
	}

	self->priv->servers = g_list_reverse(self->priv->servers);
	self->priv->weights = weights;
	self->priv->numprocs = numprocs;
	self->priv->distributed_n =distributed_n;
}

static void
divide_and_run_flows(GebrCommRunner *self)
{
	gint n = g_list_length(self->priv->servers);
	gboolean parallel = TRUE;

	if (!gebr_geoxml_flow_is_parallelizable(GEBR_GEOXML_FLOW(self->priv->flow),
	                                        self->priv->validator))
		parallel = FALSE;

	GList *flows = gebr_geoxml_flow_divide_flows(GEBR_GEOXML_FLOW(self->priv->flow),
	                                             self->priv->validator,
	                                             self->priv->distributed_n,
	                                             n);

	gint max_cores = 0;
	for (gint k = 0; k < n; k++)
		max_cores += self->priv->numprocs[k];

	if (parallel)
		self->priv->ncores = g_strdup_printf("%d", max_cores);
	else
		self->priv->ncores = g_strdup("1");


	GString *server_list = g_string_new("");

	gint k;
	GList *i = flows;
	GList *j = self->priv->servers;
	for (k = 0; i; k++, i = i->next, j = j->next) {
		GebrGeoXmlFlow *flow = i->data;
		GebrCommDaemon *daemon = j->data;
		GebrCommServer *server = gebr_comm_daemon_get_server(daemon);
		gchar *frac_str = g_strdup_printf("%d", k+1);
		gchar *flow_xml = strip_flow(self->priv->validator, flow);
		const gchar *hostname = gebr_comm_daemon_get_hostname(daemon);

		gebr_comm_daemon_add_task(daemon);

		g_string_append_printf(server_list, "%s,%lf,",
				       hostname, self->priv->weights[k]);
		gchar *numproc = g_strdup_printf("%d", self->priv->numprocs[k]);

		gebr_comm_protocol_socket_oldmsg_send(server->socket, FALSE,
						      gebr_comm_protocol_defs.run_def, 9,
						      self->priv->gid,
						      self->priv->id,
						      frac_str,
						      numproc,
						      self->priv->nice,
						      flow_xml,
						      self->priv->paths,

						      /* Moab and MPI settings */
						      self->priv->account ? self->priv->account : "",
						      "");

		g_free(frac_str);
		g_free(flow_xml);
		g_free(numproc);
	}

	self->priv->total = k;
	g_string_erase(server_list, server_list->len-1, 1);
	self->priv->servers_list = g_string_free(server_list, FALSE);

	if (self->priv->ran_func)
		self->priv->ran_func(self, self->priv->user_data);
}

static gboolean
call_ran_func(GebrCommRunner *self)
{
	if (self->priv->ran_func)
		self->priv->ran_func(self, self->priv->user_data);

	return FALSE;
}

static int
mpi_program_get_np(GebrGeoXmlProgram *prog, GebrValidator *validator) 
{
	gchar *nprocs;
	gchar *procs = gebr_geoxml_program_mpi_get_n_process(GEBR_GEOXML_PROGRAM(prog));

	if (!gebr_validator_evaluate(validator, procs,
				     GEBR_GEOXML_PARAMETER_TYPE_INT,
				     GEBR_GEOXML_DOCUMENT_TYPE_FLOW,
				     &nprocs, NULL))
		g_return_val_if_reached(0);

	return (atoi(nprocs));
}

static gboolean
daemon_has_mpi_flavor(GebrCommDaemon *daemon, const gchar *flavor)
{
	const gchar *flav = gebr_comm_daemon_get_flavors(daemon);
	if (g_strrstr(flav, flavor))
		return TRUE;
	return FALSE;
}

static void
mpi_program_contrib(GebrGeoXmlProgram *prog, GList *servers, const gchar *executor, GebrValidator *validator, gint contrib[])
{
	const gchar *mpi_flavor = gebr_geoxml_program_get_mpi(prog);
	if (!mpi_flavor || !*mpi_flavor) {
		//Refatorar em função
		gint j = 0;
		for (GList *i = servers; i; i = i->next) {
			GebrCommDaemon *daemon = i->data;
			if (g_strcmp0(gebr_comm_daemon_get_hostname(daemon), executor) == 0)
				contrib[j++]++;
		}
		return;
	}

	gint total = 0;
	for (GList *i = servers; i; i = i->next)
		if (daemon_has_mpi_flavor(i->data, mpi_flavor))
			total++;

	gint np = mpi_program_get_np(prog, validator);
	gint j = 0, acc = 0;
	for (GList *i = servers; i; i = i->next, j++) {
		GebrCommDaemon *daemon = i->data;
		if (daemon_has_mpi_flavor(daemon, mpi_flavor)) {
			contrib[j] += np / total;

			if (acc < np % total) {
				contrib[j]++;
				acc++;
			}
		}
	}
}

static void
mpi_run_flow(GebrCommRunner *self)
{
	GebrGeoXmlDocument *clone = gebr_geoxml_document_clone(self->priv->flow);
	gchar *flow_xml = strip_flow(self->priv->validator, GEBR_GEOXML_FLOW(clone));

	GString *servers = g_string_new(NULL);
	GString *servers_weigths = g_string_new(NULL);

	gfloat n_servers = g_list_length(self->priv->run_servers);

	if (!n_servers)
		g_warn_if_reached();

	gint *contrib = g_new0(gint, n_servers);
	gdouble *weights = g_new0(gdouble, n_servers);
	GebrGeoXmlSequence *seq;

	gebr_geoxml_flow_get_program(GEBR_GEOXML_FLOW(clone), &seq, 0);

	//Refatorar em função, depois
	GebrCommDaemon *executor_dae = self->priv->servers->data;
	const gchar *executor_str = gebr_comm_daemon_get_hostname(executor_dae);
	GString *mpi_flavors = g_string_new("");

	for (; seq; gebr_geoxml_sequence_next(&seq)) {
		GebrGeoXmlProgram *prog = GEBR_GEOXML_PROGRAM(seq);
		if (gebr_geoxml_program_get_control(prog) != GEBR_GEOXML_PROGRAM_CONTROL_FOR) {
			mpi_program_contrib(prog, self->priv->run_servers, executor_str, self->priv->validator, contrib);

			const gchar *mpi_flavor_sub = gebr_geoxml_program_get_mpi(prog);
			if (*mpi_flavor_sub && !g_strrstr(mpi_flavors->str, mpi_flavor_sub))
				g_string_printf(mpi_flavors, "%s,%s", mpi_flavors->str, mpi_flavor_sub);
			
			for (gint i = 0; i < n_servers; i++)
				g_debug("contrib[%d]=%d", i, contrib[i]);
			g_debug("#########################################################");
		}
	}
	if (mpi_flavors->len > 0)
		g_string_erase(mpi_flavors, 0, 1);

	gint total = 0;
	for (gint i = 0; i < n_servers; i++)
		total += contrib[i];

	for (gint i = 0; i < n_servers; i++)
		weights[i] = (float) contrib[i] / total;

	gint j = 0;
	for (GList *i = self->priv->run_servers; i; i = i->next) {
		GebrCommDaemon *daemon = i->data;
		GebrCommServer *server = gebr_comm_daemon_get_server(daemon);
		g_string_append_c(servers, ';');
		g_string_append(servers, server->address->str);
		g_string_append_c(servers, ',');
		g_string_append(servers, gebr_comm_daemon_get_flavors(daemon));

		g_string_append_c(servers_weigths, ',');
		g_string_append(servers_weigths, server->address->str);
		g_string_append_printf(servers_weigths, ",%lf", weights[j++]);
		g_debug("on %s, address:'%s', weights:'%f'",__func__, server->address->str, weights[j]);
	}
	if (servers)
		g_string_erase(servers, 0, 1);

	if (servers_weigths)
		g_string_erase(servers_weigths, 0, 1);

	GebrCommDaemon *daemon = self->priv->servers->data;
	GebrCommServer *first_server = gebr_comm_daemon_get_server(daemon);

	// Set CommRunner parameters
	self->priv->ncores = "1";
	self->priv->total = 1;
	self->priv->mpi_owner = g_strdup(first_server->address->str);
	self->priv->mpi_flavor = g_string_free(mpi_flavors, FALSE);

	gebr_comm_protocol_socket_oldmsg_send(first_server->socket, FALSE,
	                                      gebr_comm_protocol_defs.run_def, 9,
	                                      self->priv->gid,
	                                      self->priv->id,
	                                      "1",
	                                      self->priv->speed,
	                                      self->priv->nice,
	                                      flow_xml,
	                                      self->priv->paths,

	                                      /* Moab and MPI settings */
	                                      self->priv->account ? self->priv->account : "",
	                                      servers->str);


	self->priv->servers_list = g_strdup(servers_weigths->str);


	g_string_free(servers, TRUE);
	g_string_free(servers_weigths, TRUE);
	g_idle_add((GSourceFunc)call_ran_func, self);

	g_free(flow_xml);
}

static void
on_response_received(GebrCommHttpMsg *request,
		     GebrCommHttpMsg *response,
		     GebrCommRunner  *self)
{
	g_return_if_fail(request->method == GEBR_COMM_HTTP_METHOD_GET);

	GebrCommJsonContent *json = gebr_comm_json_content_new(response->content->str);
	GString *value = gebr_comm_json_content_to_gstring(json);
	GebrCommDaemon *daemon = g_object_get_data(G_OBJECT(request), "current-server");
	GebrCommServer *server = gebr_comm_daemon_get_server(daemon);

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

//	gdouble factor_correction = 0.9;
	gint running_jobs = gebr_comm_daemon_get_n_running_jobs(GEBR_COMM_DAEMON(daemon));

	self->priv->cores_scores = g_list_concat(self->priv->cores_scores,
	                                         calculate_server_score(daemon,
	                                                                value->str,
	                                                                server->ncores,
	                                                                server->clock_cpu,
	                                                                running_jobs));

//	*eff_ncores = MAX(1, MIN(nsteps, gebr_calculate_number_of_processors(ncores, scale)));
//	g_debug("on '%s', eff_ncores: '%d'", __func__, *eff_ncores);

	self->priv->responses++;
	if (self->priv->responses == self->priv->requests) {
		gint comp_func(ServerScore *a, ServerScore *b) {
			gdouble res = b->score - a->score;
			if(res < 0)
				return -1;
			else if (res > 0)
				return +1;
			else
				return 0;
		}

		self->priv->cores_scores = g_list_sort(self->priv->cores_scores, (GCompareFunc)comp_func);
		set_servers_execution_info(self);

		GebrGeoXmlProgram *mpi_prog = gebr_geoxml_flow_get_first_mpi_program(GEBR_GEOXML_FLOW(self->priv->flow));
		gboolean mpi = mpi_prog != NULL;
		gebr_geoxml_object_unref(mpi_prog);

		if (mpi)
			mpi_run_flow(self);
		else
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

gboolean
gebr_comm_runner_run_async(GebrCommRunner *self)
{
	self->priv->requests = 0;
	self->priv->responses = 0;
	gboolean has_connected = FALSE;

	GList *i = self->priv->servers;
	while (i) {
		GebrCommDaemon *daemon = i->data;

		if (!gebr_comm_daemon_can_execute(daemon)) {
			GList *aux = i->next;
			self->priv->servers = g_list_remove_link(self->priv->servers, i);
			i = aux;
			continue;
		}

		GebrCommHttpMsg *request;
		GebrCommServer *server = gebr_comm_daemon_get_server(daemon);
		GebrCommUri *uri = gebr_comm_uri_new();
		gebr_comm_uri_set_prefix(uri, "/sys-load");
		gchar *url = gebr_comm_uri_to_string(uri);
		gebr_comm_uri_free(uri);
		request = gebr_comm_protocol_socket_send_request(server->socket,
								 GEBR_COMM_HTTP_METHOD_GET,
								 url, NULL);

		if (!request) {
			GList *aux = i->next;
			self->priv->servers = g_list_remove_link(self->priv->servers, i);
			i = aux;
			continue;
		}

		has_connected = TRUE;
		g_object_set_data(G_OBJECT(request), "current-server", daemon);
		g_signal_connect(request, "response-received",
				 G_CALLBACK(on_response_received), self);
		self->priv->requests++;
		g_free(url);

		i = i->next;
	}

	return has_connected;
}

GebrValidator *
gebr_comm_runner_get_validator(GebrCommRunner *self)
{
	return self->priv->validator;
}

const gchar *
gebr_comm_runner_get_ncores(GebrCommRunner *self)
{
	return self->priv->ncores;
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

const gchar *
gebr_comm_runner_get_id(GebrCommRunner *self)
{
	return self->priv->id;
}

const gchar *
gebr_comm_runner_get_mpi_owner(GebrCommRunner *self)
{
	return self->priv->mpi_owner;
}

const gchar *
gebr_comm_runner_get_mpi_flavor(GebrCommRunner *self)
{
	return self->priv->mpi_flavor;
}
