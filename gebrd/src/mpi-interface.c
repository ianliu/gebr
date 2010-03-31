#include "mpi-interface.h"

gchar * gebr_mpi_interface_initialize(GebrMpiInterface * self)
{
	return self->initialize(self);
}

gchar * gebr_mpi_interface_build_comand(GebrMpiInterface * self, const gchar * command)
{
	return self->build_command(self, command);
}

gchar * gebr_mpi_interface_finalize(GebrMpiInterface * self)
{
	return self->finalize(self);
}
