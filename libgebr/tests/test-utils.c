/*   libgebr - GêBR Library
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

#include "../utils.h"

void test_gebr_str_escape (void)
{
	gchar *escaped;

	escaped = gebr_str_escape ("foo");
	g_assert_cmpstr (escaped, ==, "foo");
	g_free (escaped);

	escaped = gebr_str_escape ("foo\n");
	g_assert_cmpstr (escaped, ==, "foo\\n");
	g_free (escaped);

	escaped = gebr_str_escape ("\"foo\"");
	g_assert_cmpstr (escaped, ==, "\\\"foo\\\"");
	g_free (escaped);
}

void test_gebr_str_word_before_pos(void)
{
	gchar *word;
	gchar *text;
	gsize pos;

	text = "foo bar baz";
	pos = 6;
	word = gebr_str_word_before_pos(text, &pos);
	g_assert_cmpint(pos, ==, 4);
	g_assert_cmpstr(word, ==, "bar");

	text = "olá, tudo bem?";
	pos = 2;
	word = gebr_str_word_before_pos(text, &pos);
	g_assert(word == NULL);
	g_assert_cmpint(pos, ==, 2);
}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/libgebr/utils/str-escape", test_gebr_str_escape);
	g_test_add_func("/libgebr/utils/str_word_before_pos", test_gebr_str_word_before_pos);

	return g_test_run();
}
