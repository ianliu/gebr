#include <string.h>

#include "mpi-implementations.h"

/*
 * OpenMPI definition
 */

static gchar * gebrd_open_mpi_initialize(GebrdMpiInterface * mpi);
static gchar * gebrd_open_mpi_build_command(GebrdMpiInterface * mpi, const gchar * command);
static gchar * gebrd_open_mpi_finalize(GebrdMpiInterface * mpi);
static void gebrd_open_mpi_free(GebrdMpiInterface * mpi);

GebrdMpiInterface * gebrd_open_mpi_new(const gchar * n_process,
				       const gchar * bin_path,
				       const gchar * lib_path,
				       const gchar * host)
{
	GebrdOpenMpi * self;
	GebrdMpiInterface * mpi;

	self = g_new(GebrdOpenMpi, 1);
	mpi = (GebrdMpiInterface*)self;

	self->bin_path = g_strdup(bin_path);
	self->lib_path = g_strdup(lib_path);
	self->host = g_strdup(host);

	mpi->initialize = gebrd_open_mpi_initialize;
	mpi->build_command = gebrd_open_mpi_build_command;
	mpi->finalize = gebrd_open_mpi_finalize;
	mpi->free = gebrd_open_mpi_free;

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

	host = strlen(self->host) == 0? "":g_strconcat(" --host ", self->host, NULL);
	cmd = g_string_new(NULL);
	g_string_printf(cmd, "LD_LIBRARY_PATH=%s:$LD_LIBRARY_PATH PATH=%s:$PATH mpirun.openmpi%s -np %s %s",
			self->lib_path,
			self->bin_path,
			host,
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
	GebrdOpenMpi * self;
	self = (GebrdOpenMpi*)mpi;
	g_free(self->bin_path);
	g_free(self->lib_path);
}
