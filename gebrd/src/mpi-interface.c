#include "mpi-interface.h"

void gebrd_mpi_config_free(GebrdMpiConfig * self)
{
	g_string_free(self->binpath, TRUE);
	g_string_free(self->libpath, TRUE);
	g_string_free(self->init_script, TRUE);
	g_string_free(self->end_script, TRUE);
	g_free(self);
}

gchar * gebr_mpi_interface_initialize(GebrdMpiInterface * self)
{
	return self->initialize(self);
}

gchar * gebr_mpi_interface_build_comand(GebrdMpiInterface * self, const gchar * command)
{
	return self->build_command(self, command);
}

gchar * gebr_mpi_interface_finalize(GebrdMpiInterface * self)
{
	return self->finalize(self);
}
