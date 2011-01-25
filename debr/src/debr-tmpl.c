/*   DeBR - GeBR Designer
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

#include <string.h>

#include "debr-tmpl.h"

gchar *debr_tmpl_get (GString *tmpl, const gchar *tag)
{
	gsize pos;
	gsize len;
	gchar *ptr;
	gchar *mark;
	gchar *retval;

	mark = g_strconcat ("<!-- begin ", tag, " -->", NULL);
	ptr = strstr (tmpl->str, mark);
	g_free (mark);

	if (!ptr)
		return NULL;
	else
		pos = (ptr - tmpl->str) / sizeof (gchar) + 15 + strlen (tag);

	mark = g_strconcat ("<!-- end ", tag, " -->", NULL);
	ptr = strstr (tmpl->str, mark);
	g_free (mark);

	if (!ptr)
		return NULL;
	else
		len = (ptr - tmpl->str) / sizeof (gchar) - pos;

	retval = g_new(gchar, len + 1);
	retval[len] = '\0';
	return memcpy(retval, tmpl->str + pos, len);
}

gboolean debr_tmpl_set (GString *tmpl, const gchar *tag, const gchar *value)
{
	gsize pos;
	gsize len;
	gchar *ptr;
	gchar *mark;

	mark = g_strconcat ("<!-- begin ", tag, " -->", NULL);
	ptr = strstr (tmpl->str, mark);
	g_free (mark);

	if (!ptr)
		return FALSE;
	else
		pos = (ptr - tmpl->str) / sizeof (gchar) + 15 + strlen (tag);

	mark = g_strconcat ("<!-- end ", tag, " -->", NULL);
	ptr = strstr (tmpl->str, mark);
	g_free (mark);

	if (!ptr)
		return FALSE;
	else
		len = (ptr - tmpl->str) / sizeof (gchar) - pos;

	g_string_erase (tmpl, pos, len);
	g_string_insert (tmpl, pos, value);

	return TRUE;
}

gboolean debr_tmpl_append (GString *tmpl, const gchar *tag, const gchar *value)
{
	gchar *mark;
	gchar *ptr;
	gsize pos;

	mark = g_strconcat ("<!-- end ", tag, " -->", NULL);
	ptr = strstr (tmpl->str, mark);

	if (!ptr)
		return FALSE;
	else
		pos = (ptr - tmpl->str) / sizeof (gchar);

	g_string_insert (tmpl, pos, value);

	return TRUE;
}
