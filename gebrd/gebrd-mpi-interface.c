#include "gebrd-mpi-interface.h"

void gebrd_mpi_config_free(GebrdMpiConfig * self)
{
	g_string_free(self->binpath, TRUE);
	g_string_free(self->libpath, TRUE);
	g_string_free(self->init_cmd, TRUE);
	g_string_free(self->end_cmd, TRUE);
	g_free(self);
}

gchar * gebrd_mpi_interface_initialize(GebrdMpiInterface * self)
{
	return self->initialize(self);
}

gchar * gebrd_mpi_interface_build_comand(GebrdMpiInterface * self, const gchar * command)
{
	return self->build_command(self, command);
}

gchar * gebrd_mpi_interface_finalize(GebrdMpiInterface * self)
{
	return self->finalize(self);
}

void gebrd_mpi_interface_set_n_processes(GebrdMpiInterface * self, const gchar * n)
{
	self->n_processes = g_strdup(n);
}

void gebrd_mpi_interface_free(GebrdMpiInterface * self)
{
	self->free(self);
	g_free(self->n_processes);
	g_free(self);
}
