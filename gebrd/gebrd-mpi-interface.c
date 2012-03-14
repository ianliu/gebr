#include "gebrd-mpi-interface.h"

void gebrd_mpi_config_free(GebrdMpiConfig * self)
{
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
	gchar *init, *fina;
	GString *buf = g_string_new(NULL);

	init = self->initialize(self);
	fina = self->finalize(self);

	if (init) {
		g_string_append(buf, init);
		g_string_append(buf, "; ");
		g_free(init);
	}

	g_string_append(buf, self->build_command(self, command));

	if (fina) {
		g_string_append(buf, "; ");
		g_string_append(buf, fina);
		g_free(fina);
	}

	return g_string_free(buf, FALSE);
}

gchar * gebrd_mpi_interface_finalize(GebrdMpiInterface * self)
{
	return self->finalize(self);
}

void gebrd_mpi_interface_set_params(GebrdMpiInterface * self, const gchar * params)
{
	self->params = g_strdup(params);
}

void gebrd_mpi_interface_free(GebrdMpiInterface * self)
{
	self->free(self);
	g_free(self->params);
	g_free(self);
}
