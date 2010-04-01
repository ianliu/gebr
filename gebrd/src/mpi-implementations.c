#include "mpi-implementations.h"

/*
 * OpenMPI definition
 */

static gchar * gebrd_open_mpi_initialize(GebrdMpiInterface * mpi);
static gchar * gebrd_open_mpi_build_command(GebrdMpiInterface * mpi, const gchar * command);
static gchar * gebrd_open_mpi_finalize(GebrdMpiInterface * mpi);

GebrdMpiInterface * gebrd_open_mpi_new(const gchar * bin_path, const gchar * lib_path)
{
	GebrdOpenMpi * self;
	GebrdMpiInterface * mpi;

	self = g_new(GebrdOpenMpi, 1);
	mpi = (GebrdMpiInterface*)self;

	self->bin_path = bin_path;
	self->lib_path = lib_path;
	mpi->initialize = gebrd_open_mpi_initialize;
	mpi->build_command = gebrd_open_mpi_build_command;
	mpi->finalize = gebrd_open_mpi_finalize;

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
	g_string_printf(cmd, "LD_LIBRARY_PATH=%s:$LD_LIBRARY_PATH PATH=%s:$PATH mpirun.openmpi -np %d %s",
			self->lib_path, self->bin_path, mpi->n_processes, command);
	return g_string_free(cmd, FALSE);
}

static gchar * gebrd_open_mpi_finalize(GebrdMpiInterface * mpi)
{
	return NULL;
}
