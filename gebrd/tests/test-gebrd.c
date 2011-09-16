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
#include <glib.h>

#include "../gebrd.h"

#define MAX_AGRESSION 5
#define MIN_AGRESSION 1

void
test_set_heuristic_aggression_border (void)
{
	GebrdApp *self = gebrd_app_new();
	gint nice;

	g_assert(gebrd_app_set_heuristic_aggression(self, MIN_AGRESSION, &nice) == 1);
	g_assert_cmpint(nice, ==, 0);

	g_assert(gebrd_app_set_heuristic_aggression(self, -MIN_AGRESSION, &nice) == 1);
	g_assert_cmpint(nice, ==, 19);

	g_assert(gebrd_app_set_heuristic_aggression(self, MAX_AGRESSION, &nice) == (self->nprocs));
	g_assert_cmpint(nice, ==, 0);

	g_assert(gebrd_app_set_heuristic_aggression(self, -MAX_AGRESSION, &nice) == (self->nprocs));
	g_assert_cmpint(nice, ==, 19);
}

void
test_set_heuristic_aggression (void)
{
	GebrdApp *self = gebrd_app_new();
	gint nice;

	g_assert(gebrd_app_set_heuristic_aggression(self, 2, &nice) == ((self->nprocs - 1) * 1/4 + 1));
	g_assert_cmpint(nice, ==, 0);


	g_assert(gebrd_app_set_heuristic_aggression(self, -2, &nice) == ((self->nprocs - 1) * 1/4 + 1));
	g_assert_cmpint(nice, ==, 19);
}

int main(int argc, char * argv[])
{
	g_type_init();
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/gebrd/gebrd/heuristic_aggression_border", test_set_heuristic_aggression_border);
	g_test_add_func("/gebrd/gebrd/heuristic_aggression", test_set_heuristic_aggression);

	return g_test_run();
}
