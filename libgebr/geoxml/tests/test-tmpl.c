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
#include <gebr-geoxml-tmpl.h>

#include "document.h"

void test_debr_tmpl_get (void)
{
	GString *str = g_string_new ("Foo <!-- begin val -->bar<!-- end val -->");
	gchar *val = gebr_geoxml_tmpl_get (str, "val");

	// The value of 'val' tag must be 'bar'
	g_assert_cmpstr (val, ==, "bar");

	// The 'foo' tag does not exists, so the function must return NULL
	g_assert_cmpstr (gebr_geoxml_tmpl_get (str, "foo"), ==, NULL);

	g_free (val);
	g_string_free (str, TRUE);
}

void test_debr_tmpl_set (void)
{
	GString *str = g_string_new ("Foo <!-- begin val -->bar<!-- end val -->");

	// The function must return TRUE since 'val' tag exists
	g_assert (gebr_geoxml_tmpl_set (str, "val", "foo"));

	// The substitution must be correct
	g_assert_cmpstr (str->str, ==, "Foo <!-- begin val -->foo<!-- end val -->");

	// The function must return FALSE since 'foo' tag doest not exists
	g_assert (gebr_geoxml_tmpl_set (str, "foo", "foo") == FALSE);

	g_string_free (str, TRUE);
}

void test_debr_tmpl_append (void)
{
	gchar *tag;
	GString *str = g_string_new ("Foo <!-- begin val -->foo<!-- end val -->");

	// The function must return TRUE since 'val' tag exists
	g_assert (gebr_geoxml_tmpl_append (str, "val", "bar"));

	// The substitution must be correct
	g_assert_cmpstr (str->str, ==, "Foo <!-- begin val -->foobar<!-- end val -->");

	// The tag must be equal too!
	tag = gebr_geoxml_tmpl_get (str, "val");
	g_assert_cmpstr (tag, ==, "foobar");
	g_free (tag);

	// The function must return FALSE since 'foo' tag doest not exists
	g_assert (gebr_geoxml_tmpl_append (str, "foo", "foo") == FALSE);

	g_string_free (str, TRUE);
}

int main(int argc, char *argv[])
{
	g_test_init (&argc, &argv, NULL);

	gebr_geoxml_document_set_dtd_dir(DTD_DIR);

	g_test_add_func ("/GebrGeoxml/tmpl/get", test_debr_tmpl_get);
	g_test_add_func ("/GebrGeoxml/tmpl/set", test_debr_tmpl_set);
	g_test_add_func ("/GebrGeoxml/tmpl/append", test_debr_tmpl_append);

	return g_test_run ();
}
