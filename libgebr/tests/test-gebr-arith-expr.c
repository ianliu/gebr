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
#include <string.h>

#include "../gebr-iexpr.h"
#include "../gebr-arith-expr.h"

void test_gebr_arith_expr_simple(void)
{
	gdouble result;
	GebrArithExpr *expr = gebr_arith_expr_new();

	g_assert (gebr_arith_expr_eval(expr, "2*2", &result, NULL));
	g_assert_cmpfloat (result, ==, 4);
}

void test_gebr_arith_expr_invalid(void)
{
	gdouble result = 666;
	GebrArithExpr *expr = gebr_arith_expr_new();

	g_assert (gebr_arith_expr_eval(expr, "2*", &result, NULL) == FALSE);
	g_assert_cmpfloat (result, ==, 666);

	g_assert (gebr_arith_expr_eval(expr, "\"2", &result, NULL) == FALSE);
	g_assert_cmpfloat (result, ==, 666);

	g_assert (gebr_arith_expr_eval(expr, "\"2\"", &result, NULL) == FALSE);
	g_assert_cmpfloat (result, ==, 666);

	g_assert (gebr_arith_expr_eval(expr, "2c*", &result, NULL) == FALSE);
	g_assert_cmpfloat (result, ==, 666);
}

void test_gebr_arith_expr_mult_expr(void)
{
	gdouble result = 666;
	GebrArithExpr *expr = gebr_arith_expr_new();

	g_assert (gebr_arith_expr_eval(expr, "2*2 ; 3*3", &result, NULL) == FALSE);
	g_assert_cmpfloat (result, ==, 666);

	g_assert (gebr_arith_expr_eval(expr, "2*2 ; 3*3 ; 3+", &result, NULL) == FALSE);
	g_assert_cmpfloat (result, ==, 666);

	g_assert (gebr_arith_expr_eval(expr, "4*4", &result, NULL));
	g_assert_cmpfloat (result, ==, 4*4);
}

void test_gebr_arith_expr_variables(void)
{
	gdouble result = 666;
	GebrArithExpr *expr = gebr_arith_expr_new();

	g_assert(gebr_iexpr_set_var(GEBR_IEXPR(expr), "foo",
				    GEBR_GEOXML_PARAMETER_TYPE_INT,
				    "10", NULL));

	g_assert(gebr_iexpr_set_var(GEBR_IEXPR(expr), "bar",
				    GEBR_GEOXML_PARAMETER_TYPE_FLOAT,
				    "1.2", NULL));

	g_assert(gebr_arith_expr_eval(expr, "bar*foo", &result, NULL));
	g_assert_cmpfloat (result, ==, 10 * 1.2);
}

void test_gebr_arith_expr_side_effect(void) {
	gdouble result = 666;
	GebrArithExpr *expr = gebr_arith_expr_new();
	GError * error = NULL;

	g_assert(gebr_arith_expr_eval(expr, "j=1;j", &result, &error) == FALSE);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX);
	g_assert_cmpfloat(result, ==, 666);
	g_clear_error(&error);

	g_assert(gebr_arith_expr_eval(expr, "j=1", &result, &error) == FALSE);
	g_assert_error(error, GEBR_IEXPR_ERROR, GEBR_IEXPR_ERROR_SYNTAX);
	g_assert_cmpfloat(result, ==, 666);
	g_clear_error(&error);
}

int main(int argc, char *argv[])
{
	g_type_init();
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/libgebr/arith-expr/invalid", test_gebr_arith_expr_invalid);
	g_test_add_func("/libgebr/arith-expr/simple", test_gebr_arith_expr_simple);
	g_test_add_func("/libgebr/arith-expr/mult_expression", test_gebr_arith_expr_mult_expr);
	g_test_add_func("/libgebr/arith-expr/variables", test_gebr_arith_expr_variables);
	g_test_add_func("/libgebr/arith-expr/side_effect", test_gebr_arith_expr_side_effect);

	return g_test_run();
}


