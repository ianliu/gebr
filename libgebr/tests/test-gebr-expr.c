/*   libgebr - GêBR Library
 *   Copyright (C) 2007-2011 GeBR core team (http://www.gebrproject.com/)
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
#include <glib/gstdio.h>
#include <string.h>

#include "../gebr-expr.h"

void test_gebr_expr_simple(void)
{
	gdouble result;
	GebrExpr *expr = gebr_expr_new();

	g_assert (gebr_expr_eval(expr, "2*2", &result, NULL));
	g_assert_cmpfloat (result, ==, 4);
}

void test_gebr_expr_invalid(void)
{
	gdouble result = 666;
	GebrExpr *expr = gebr_expr_new();

	g_assert (gebr_expr_eval(expr, "2*", &result, NULL) == FALSE);
	g_assert_cmpfloat (result, ==, 666);
}

void test_gebr_expr_mult_expression(void)
{
	gdouble result = 666;
	GebrExpr *expr = gebr_expr_new();

	g_assert (gebr_expr_eval(expr, "2*2 ; 3*3", &result, NULL) == FALSE);
	g_assert_cmpfloat (result, ==, 666);

	g_assert (gebr_expr_eval(expr, "2*2 ; 3*3 ; 3+", &result, NULL) == FALSE);
	g_assert_cmpfloat (result, ==, 666);

	g_assert (gebr_expr_eval(expr, "4*4", &result, NULL));
	g_assert_cmpfloat (result, ==, 4*4);
}

void test_gebr_expr_variables(void)
{
	gdouble result = 666;
	GebrExpr *expr = gebr_expr_new();

	g_assert (gebr_expr_set_var (expr, "foo", "10", NULL));
	g_assert (gebr_expr_set_var (expr, "bar", "1.2", NULL));
	g_assert (gebr_expr_eval(expr, "bar*foo", &result, NULL));
	g_assert_cmpfloat (result, ==, 10 * 1.2);
}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/libgebr/expr/invalid", test_gebr_expr_invalid);
	g_test_add_func("/libgebr/expr/simple", test_gebr_expr_simple);
	g_test_add_func("/libgebr/expr/multi_expression", test_gebr_expr_mult_expression);
	g_test_add_func("/libgebr/expr/variables", test_gebr_expr_variables);

	return g_test_run();
}

