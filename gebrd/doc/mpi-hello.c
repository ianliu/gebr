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

/*
 * To compile this example, make sure you have OpenMPI installed in your system
 * and execute the following shell command:
 *
 *   $ mpicc mpi-hello.c -o mpi-hello
 *
 * You can test the program by running
 *
 *   $ mpirun -np 5 mpi-hello
 */

/* Included for `printf' */
#include <stdio.h>

/* Included for `sleep' and `gethostname' */
#include <unistd.h>

/* Includes the OpenMPI header. Make sure the folder
 * structure is like written bellow. In some cases,
 * it might be #include <mpi.h>. */
//#include <mpi.h>
#include <mpi/mpi.h>

int main(int argc, char *argv[])
{
	int rank;
	int size;
	char hostname[50];

	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	/* Waits `rank' seconds until printing the greetings message. */
	sleep(rank);
	gethostname(hostname, sizeof(hostname));
	printf("Hello world from process %d of %d at %s\n",
	       rank+1, size, hostname);

	MPI_Finalize();

	return 0;
}
