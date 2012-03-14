/*
 * gebrd-mpi-implementations.c
 * This file is part of GêBR Project
 *
 * Copyright (C) 2012 - GêBR Core Team (www.gebrproject.com)
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

#include "gebrd-mpi-implementations.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct {
	GebrdMpiInterface parent;
	const GebrdMpiConfig * config;
	GList *servers;
	const gchar *tmp_file;
} GebrdMpich2;

typedef struct {
	GebrdMpiInterface parent;
	const GebrdMpiConfig * config;
	gchar *servers;
} GebrdOpenMpi;


/*
 * OpenMPI definition
 */

static gchar * gebrd_open_mpi_initialize(GebrdMpiInterface * mpi);
static gchar * gebrd_open_mpi_build_command(GebrdMpiInterface * mpi, const gchar * command);
static gchar * gebrd_open_mpi_finalize(GebrdMpiInterface * mpi);
static void gebrd_open_mpi_free(GebrdMpiInterface * mpi);

GebrdMpiInterface *
gebrd_open_mpi_new(const gchar *params,
		   const GebrdMpiConfig *config,
		   GList *servers)
{
	GebrdOpenMpi * self;
	GebrdMpiInterface * mpi;

	self = g_new(GebrdOpenMpi, 1);
	mpi = (GebrdMpiInterface*)self;

	self->config = config;

	mpi->initialize = gebrd_open_mpi_initialize;
	mpi->build_command = gebrd_open_mpi_build_command;
	mpi->finalize = gebrd_open_mpi_finalize;
	mpi->free = gebrd_open_mpi_free;

	GString *buf = g_string_new(NULL);
	for (GList *i = servers; i; i = g_list_next(i)) {
		g_string_append_c(buf, ',');
		g_string_append(buf, i->data);
	}

	if (buf->len)
		g_string_erase(buf, 0, 1);

	self->servers = g_string_free(buf, FALSE);

	gebrd_mpi_interface_set_params(mpi, params);

	return mpi;
}

static gchar * gebrd_open_mpi_initialize(GebrdMpiInterface * mpi)
{
	return NULL;
}

static gchar * gebrd_open_mpi_build_command(GebrdMpiInterface * mpi, const gchar * command)
{
	GebrdOpenMpi * self;
	GString * cmd;

	self = (GebrdOpenMpi*)mpi;

	cmd = g_string_new(NULL);
	g_string_printf(cmd, "LD_LIBRARY_PATH=%s:$LD_LIBRARY_PATH %s --host %s %s %s",
			self->config->libpath->str,
			self->config->mpirun->str,
			self->servers,
			mpi->params,
			command);
	return g_string_free(cmd, FALSE);
}

static gchar * gebrd_open_mpi_finalize(GebrdMpiInterface * mpi)
{
	return NULL;
}

static void gebrd_open_mpi_free(GebrdMpiInterface * mpi)
{
}

/* MPICH 2 */

static gchar *
gebrd_mpich2_initialize(GebrdMpiInterface * mpi)
{
	GebrdMpich2 *self = (GebrdMpich2*) mpi;

	gchar *tmp = g_build_filename(g_get_home_dir(), ".gebr", "gebrd",
				      g_get_host_name(), "mpd.hostsXXXXXX",
				      NULL);

	gint n = 0;
	GString *hosts = g_string_new(NULL);
	for (GList *i = self->servers; i; i = i->next) {
		g_string_append(hosts, i->data);
		g_string_append_c(hosts, '\n');
		n++;
	}

	self->tmp_file = "$_gebr_tmp";
	gchar *ret_cmd = g_strdup_printf("_gebr_tmp=`mktemp %s` && echo '%s' >> %s", tmp, hosts->str, self->tmp_file);

	g_string_free(hosts, TRUE);
	g_free(tmp);

	return ret_cmd;
}

static gchar *
gebrd_mpich2_build_command(GebrdMpiInterface *mpi,
			   const gchar *command)
{
	gchar *ld;
	GebrdMpich2 *self = (GebrdMpich2*)mpi;

	if (self->config->libpath->len)
		ld = g_strdup_printf("LD_LIBRARY_PATH=%s:$LD_LIBRARY_PATH",
				     self->config->libpath->str);
	else
		ld = g_strdup("");

	return g_strdup_printf("%s %s -f %s %s %s",
			       ld,
			       self->config->mpirun->str,
			       self->tmp_file,
			       mpi->params,
			       command);
}

static gchar *
gebrd_mpich2_finalize(GebrdMpiInterface *mpi)
{
	return g_strdup_printf("rm %s", ((GebrdMpich2*)mpi)->tmp_file);
}

static void
gebrd_mpich2_free(GebrdMpiInterface *mpi)
{
}

GebrdMpiInterface *
gebrd_mpich2_new(const gchar *params,
		 const GebrdMpiConfig *config,
		 GList *servers)
{
	GebrdMpich2 *self = g_new0(GebrdMpich2, 1);
	GebrdMpiInterface *mpi = (GebrdMpiInterface*)self;

	mpi->initialize = gebrd_mpich2_initialize;
	mpi->build_command = gebrd_mpich2_build_command;
	mpi->finalize = gebrd_mpich2_finalize;
	mpi->free = gebrd_mpich2_free;

	self->servers = g_list_copy(servers);
	self->config = config;
	gebrd_mpi_interface_set_params(mpi, params);

	return mpi;
}
