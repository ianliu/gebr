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
#include <gebrd-job.h>

void test_gebrd_job_parse_string_expression(void)
{
	gchar *result;
	GHashTable *ht;

	ht = g_hash_table_new(g_str_hash, g_str_equal);
	g_hash_table_insert(ht, "iter", GUINT_TO_POINTER(0));
	g_hash_table_insert(ht, "foo", GUINT_TO_POINTER(1));
	g_hash_table_insert(ht, "bar", GUINT_TO_POINTER(2));

	g_assert(parse_string_expression("", ht, &result) == GEBRD_STRING_PARSER_ERROR_NONE);
	g_assert_cmpstr(result, ==, "");
	g_free(result);

	g_assert(parse_string_expression("foo[bar]baz", ht, &result) == GEBRD_STRING_PARSER_ERROR_NONE);
	g_assert_cmpstr(result, ==, "foo${V[2]}baz");
	g_free(result);

	g_assert(parse_string_expression("foo[bar]baz[[s]]", ht, &result) == GEBRD_STRING_PARSER_ERROR_NONE);
	g_assert_cmpstr(result, ==, "foo${V[2]}baz[s]");
	g_free(result);

	g_assert(parse_string_expression("[iter][foo][bar]", ht, &result) == GEBRD_STRING_PARSER_ERROR_NONE);
	g_assert_cmpstr(result, ==, "${V[0]}${V[1]}${V[2]}");
	g_free(result);

	result = NULL;

	g_assert(parse_string_expression("[iter][foo][baz]", ht, &result) == GEBRD_STRING_PARSER_ERROR_UNDEF_VAR);
	g_assert(result == NULL);

	g_assert(parse_string_expression("[iter][foo][bar]]", ht, &result) == GEBRD_STRING_PARSER_ERROR_SYNTAX);
	g_assert(result == NULL);

	g_assert(parse_string_expression("[iter", ht, &result) == GEBRD_STRING_PARSER_ERROR_SYNTAX);
	g_assert(result == NULL);

	g_hash_table_unref(ht);
}

int main(int argc, char * argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/gebrd/job/parse_string_expression", test_gebrd_job_parse_string_expression);

	return g_test_run();
}


