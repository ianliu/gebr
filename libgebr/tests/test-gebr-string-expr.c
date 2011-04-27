/*   libgebr - GêBR Library
 *   Copyright (C) 2011 GeBR core team (http://www.gebrproject.com/)
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
#include <glib-object.h>

#include "../gebr-iexpr.h"
#include "../gebr-string-expr.h"

void test_gebr_string_expr_simple(void)
{
	GebrStringExpr *s = gebr_string_expr_new();

	g_assert(gebr_iexpr_set_var(GEBR_IEXPR(s), "foo",
				    GEBR_GEOXML_PARAMETER_TYPE_STRING,
				    "Hello World!", NULL));

	g_assert(gebr_iexpr_is_valid(GEBR_IEXPR(s), "ola", NULL));
	g_object_unref(s);
}

void test_gebr_string_expr_invalid(void)
{
	GebrStringExpr *s = gebr_string_expr_new();

	/* g_assert(gebr_iexpr_set_var(GEBR_IEXPR(s), "2foo",
				    GEBR_GEOXML_PARAMETER_TYPE_STRING,
				    "Foo", NULL) == FALSE); */

	g_assert(gebr_iexpr_is_valid(GEBR_IEXPR(s), "foo]", NULL) == FALSE);

	g_object_unref(s);
}

int main(int argc, char *argv[])
{
	g_type_init();
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/libgebr/string-expr/simple", test_gebr_string_expr_simple);
	g_test_add_func("/libgebr/string-expr/invalid", test_gebr_string_expr_invalid);

	return g_test_run();
}


