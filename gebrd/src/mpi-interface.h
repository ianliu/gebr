/*   GeBR Daemon - Process and control execution of flows
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __MPI_INTERFACE_H__
#define __MPI_INTERFACE_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct {
	GString * name;		/**< The name of the Mpi implementation */
	GString * libpath;
	GString * binpath;
	GString * host;
	GString * init_script;
	GString * end_script;
} GebrdMpiConfig;

void gebrd_mpi_config_free(GebrdMpiConfig * self);

typedef struct _GebrdMpiInterface GebrdMpiInterface;

struct _GebrdMpiInterface {
	gchar * n_processes;

	/* Virtual methods */
	gchar * (*initialize) (GebrdMpiInterface * self);
	gchar * (*build_command) (GebrdMpiInterface * self, const gchar * command);
	gchar * (*finalize) (GebrdMpiInterface * self);
	void (*free) (GebrdMpiInterface * self);
};

/**
 * Returns the initial commands that will configure the Mpi interface.
 * There are many flavors of Mpi implementations, each of which may require an initialization before being used.
 *
 * \return A newly allocated string with the initial commands to setup this Mpi interface.
 */
gchar * gebrd_mpi_interface_initialize(GebrdMpiInterface * self);

/**
 * Wraps \p cmd to execute it with Mpi.
 *
 * \return A newly allocated string with the shell command that will execute \p cmd with Mpi.
 */
gchar * gebrd_mpi_interface_build_comand(GebrdMpiInterface * self, const gchar * cmd);

/**
 * Returns commands that will finalize possible states, modified by the hole process.
 *
 * \return A newly allocated string with the shell commands to finalize (or restore) the machine state.
 */
gchar * gebrd_mpi_interface_finalize(GebrdMpiInterface * self);

/**
 */
void gebrd_mpi_interface_set_n_processes(GebrdMpiInterface * self, const gchar * n);

/**
 */
void gebrd_mpi_interface_free(GebrdMpiInterface * self);

G_END_DECLS

#endif /* __MPI_INTERFACE_H__ */
