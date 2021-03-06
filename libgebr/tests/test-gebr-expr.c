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
#include <geoxml/geoxml.h>

#include "../gebr-expr.h"

void test_gebr_expr_simple(void)
{
	gdouble result;
	GebrExpr *expr = gebr_expr_new(NULL);

	g_assert (gebr_expr_eval(expr, "2*2", &result, NULL));
	g_assert_cmpfloat (result, ==, 4);
}

void test_gebr_expr_invalid(void)
{
	gdouble result = 666;
	GebrExpr *expr = gebr_expr_new(NULL);

	g_assert (gebr_expr_eval(expr, "2*", &result, NULL) == FALSE);
	g_assert_cmpfloat (result, ==, 666);

	g_assert (gebr_expr_eval(expr, "\"2", &result, NULL) == FALSE);
	g_assert_cmpfloat (result, ==, 666);

	g_assert (gebr_expr_eval(expr, "\"2\"", &result, NULL) == FALSE);
	g_assert_cmpfloat (result, ==, 666);

	g_assert (gebr_expr_eval(expr, "2c*", &result, NULL) == FALSE);
	g_assert_cmpfloat (result, ==, 666);
}

void test_gebr_expr_mult_expression(void)
{
	gdouble result = 666;
	GebrExpr *expr = gebr_expr_new(NULL);

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
	GebrExpr *expr = gebr_expr_new(NULL);

	g_assert (gebr_expr_set_var (expr, "foo", "10", NULL));
	g_assert (gebr_expr_set_var (expr, "bar", "1.2", NULL));
	g_assert (gebr_expr_eval(expr, "bar*foo", &result, NULL));
	g_assert_cmpfloat (result, ==, 10 * 1.2);
}

void test_gebr_expr_extract_vars(void)
{
	GList *vars;

	vars = gebr_expr_extract_vars ("5*foo * bar + 5*iter");
	g_assert_cmpstr (g_list_nth_data (vars, 0), ==, "foo");
	g_assert_cmpstr (g_list_nth_data (vars, 1), ==, "bar");
	g_assert_cmpstr (g_list_nth_data (vars, 2), ==, "iter");
	g_assert_cmpint(3,==,g_list_length(vars));
	g_list_foreach (vars, (GFunc) g_free, NULL);
	g_list_free (vars);

	vars = gebr_expr_extract_vars ("var_underline * var0 + 03");
	g_assert_cmpstr (g_list_nth_data (vars, 0), ==, "var_underline");
	g_assert_cmpstr (g_list_nth_data (vars, 1), ==, "var0");
	g_assert_cmpint (g_list_length (vars), ==, 2);
	g_list_foreach (vars, (GFunc) g_free, NULL);
	g_list_free (vars);
}

void test_gebr_is_name_valid(void)
{
	g_assert(gebr_expr_is_name_valid("var"));
	g_assert(gebr_expr_is_name_valid("var1"));
	g_assert(gebr_expr_is_name_valid("var1_"));

	// This name is valid, but it cannot be defined
	// in GeBR's interface!
	g_assert(gebr_expr_is_name_valid("iter"));

	g_assert(!gebr_expr_is_name_valid("VAR"));
	g_assert(!gebr_expr_is_name_valid("_var1"));
	g_assert(!gebr_expr_is_name_valid("1var"));
	g_assert(!gebr_expr_is_name_valid("vAr1"));
	g_assert(!gebr_expr_is_name_valid("var1,2"));

	g_assert(!gebr_expr_is_name_valid("scale"));
}

void test_gebr_expr_side_effect(void){
	gdouble result = 666;
	GebrExpr *expr = gebr_expr_new(NULL);
	GError * error = NULL;

	g_assert (gebr_expr_eval(expr, "j=1;j", &result, &error) == FALSE);
	g_assert_cmpint(error->code, ==, GEBR_EXPR_ERROR_INVALID_ASSIGNMENT);
	g_assert_cmpfloat (result, ==, 666);

	g_clear_error(&error);
	g_assert (gebr_expr_eval(expr, "j=1", &result, &error) == FALSE);
	g_assert_cmpint(error->code, ==, GEBR_EXPR_ERROR_INVALID_ASSIGNMENT);
	g_assert_cmpfloat (result, ==, 666);
}

void test_gebr_expr_parse_string_invalid(void)
{
	GList * list = NULL;

	/*Success*/
	g_assert(gebr_str_expr_extract_vars("",&list) == GEBR_EXPR_ERROR_NONE);
	g_list_foreach (list, (GFunc) g_free, NULL);

	g_assert(gebr_str_expr_extract_vars("other",&list) == GEBR_EXPR_ERROR_NONE);
	g_list_foreach (list, (GFunc) g_free, NULL);

	g_assert(gebr_str_expr_extract_vars("[int]",&list) == GEBR_EXPR_ERROR_NONE);
	g_list_foreach (list, (GFunc) g_free, NULL);

	g_assert(gebr_str_expr_extract_vars("[[]]",&list) == GEBR_EXPR_ERROR_NONE);
	g_list_foreach (list, (GFunc) g_free, NULL);

	g_assert(gebr_str_expr_extract_vars("[int][float]",&list) == GEBR_EXPR_ERROR_NONE);
	g_list_foreach (list, (GFunc) g_free, NULL);

	g_assert(gebr_str_expr_extract_vars("other[int]other",&list) == GEBR_EXPR_ERROR_NONE);
	g_list_foreach (list, (GFunc) g_free, NULL);

	g_assert(gebr_str_expr_extract_vars("[[other]]",&list) == GEBR_EXPR_ERROR_NONE);
	g_list_foreach (list, (GFunc) g_free, NULL);

	g_assert(gebr_str_expr_extract_vars("[[other]][int]other",&list) == GEBR_EXPR_ERROR_NONE);
	g_list_foreach (list, (GFunc) g_free, NULL);

	g_assert(gebr_str_expr_extract_vars("other[[other]]other",&list) == GEBR_EXPR_ERROR_NONE);
	g_list_foreach (list, (GFunc) g_free, NULL);

	g_assert(gebr_str_expr_extract_vars("[[",&list) == GEBR_EXPR_ERROR_NONE);
	g_list_foreach (list, (GFunc) g_free, NULL);

	g_assert(gebr_str_expr_extract_vars("[[other",&list) == GEBR_EXPR_ERROR_NONE);
	g_list_foreach (list, (GFunc) g_free, NULL);

	g_assert(gebr_str_expr_extract_vars("]]",&list) == GEBR_EXPR_ERROR_NONE);
	g_list_foreach (list, (GFunc) g_free, NULL);

	g_assert(gebr_str_expr_extract_vars("[[[int]]]",&list) == GEBR_EXPR_ERROR_NONE);
	g_list_foreach (list, (GFunc) g_free, NULL);


	/*Failure*/
	g_assert(gebr_str_expr_extract_vars("[[other]other]]",&list) == GEBR_EXPR_ERROR_SYNTAX);
	g_list_foreach (list, (GFunc) g_free, NULL);

	g_assert(gebr_str_expr_extract_vars("[[]",&list) == GEBR_EXPR_ERROR_SYNTAX);
	g_list_foreach (list, (GFunc) g_free, NULL);

	g_assert(gebr_str_expr_extract_vars("[[int]",&list) == GEBR_EXPR_ERROR_SYNTAX);
	g_list_foreach (list, (GFunc) g_free, NULL);

	g_assert(gebr_str_expr_extract_vars("[[[int]]",&list) == GEBR_EXPR_ERROR_SYNTAX);
	g_list_foreach (list, (GFunc) g_free, NULL);

	g_assert(gebr_str_expr_extract_vars("[]",&list) == GEBR_EXPR_ERROR_EMPTY_VAR);
	g_list_foreach (list, (GFunc) g_free, NULL);

	g_assert(gebr_str_expr_extract_vars("[",&list) == GEBR_EXPR_ERROR_SYNTAX);
	g_list_foreach (list, (GFunc) g_free, NULL);

	g_assert(gebr_str_expr_extract_vars("]",&list) == GEBR_EXPR_ERROR_SYNTAX);
	g_list_foreach (list, (GFunc) g_free, NULL);

	g_assert(gebr_str_expr_extract_vars("[int]]",&list) == GEBR_EXPR_ERROR_SYNTAX);
	g_list_foreach (list, (GFunc) g_free, NULL);

	g_assert(gebr_str_expr_extract_vars("[[]]]",&list) == GEBR_EXPR_ERROR_SYNTAX);
	g_list_foreach (list, (GFunc) g_free, NULL);

	g_list_free (list);

}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);
	gebr_geoxml_init();

	g_test_add_func("/libgebr/expr/invalid", test_gebr_expr_invalid);
	g_test_add_func("/libgebr/expr/simple", test_gebr_expr_simple);
	g_test_add_func("/libgebr/expr/multi_expression", test_gebr_expr_mult_expression);
	g_test_add_func("/libgebr/expr/variables", test_gebr_expr_variables);
	g_test_add_func("/libgebr/expr/extract_vars", test_gebr_expr_extract_vars);
	g_test_add_func("/libgebr/expr/name_valid", test_gebr_is_name_valid);
	g_test_add_func("/libgebr/expr/side_effect", test_gebr_expr_side_effect);
	g_test_add_func("/libgebr/expr/parse_string_invalid", test_gebr_expr_parse_string_invalid);

	gint ret = g_test_run();
	gebr_geoxml_finalize();
	return ret;
}

