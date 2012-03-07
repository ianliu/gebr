#include <string.h>

#include "gebrd-mpi-implementations.h"

/*
 * OpenMPI definition
 */

static gchar * gebrd_open_mpi_initialize(GebrdMpiInterface * mpi);
static gchar * gebrd_open_mpi_build_command(GebrdMpiInterface * mpi, const gchar * command);
static gchar * gebrd_open_mpi_finalize(GebrdMpiInterface * mpi);
static void gebrd_open_mpi_free(GebrdMpiInterface * mpi);

GebrdMpiInterface *
gebrd_open_mpi_new(const gchar *n_process,
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

	gebrd_mpi_interface_set_n_processes(mpi, n_process);

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
	gchar * host;

	self = (GebrdOpenMpi*)mpi;

	if (self->config->host->len == 0)
		host = "";
	else
		host = g_strconcat(" --host ", self->config->host->str, NULL);

	cmd = g_string_new(NULL);
	g_string_printf(cmd, "LD_LIBRARY_PATH=%s:$LD_LIBRARY_PATH PATH=%s:$PATH %s --host %s -np %s %s",
			self->config->libpath->str,
			self->config->binpath->str,
			self->config->mpirun->str,
			self->servers,
			mpi->n_processes,
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
