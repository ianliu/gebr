/*   GÍBR Daemon - Process and control execution of flows
 *   Copyright (C) 2007-2008 GÍBR core team (http://gebr.sourceforge.net)
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

#include <stdlib.h>
#include <time.h>

#include "support.h"

/*
 * Internal stuff
 */

void
generate_seed(void)
{
	static unsigned int seed = 1;

	seed *= (time(NULL) + 12345) / 2;
	srand(seed);
}

/*
 * Public sutff
 */

int
random_number(void)
{
	generate_seed();
	return rand();
}
