/*   libgebr - GeBR Library
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

#include "gebr-expr.h"

struct _GebrExpr
{
	GHashTable *vars;
}


GebrExpr *
gebr_expr_new (void)
{
	GebrExpr *self = g_new (GebrExpr, 1);
	self->vars = g_hash_table_new_full (g_str_hash,
					    g_str_equal,
					    (GDestroyNotify) g_free,
					    (GDestroyNotify) g_free);
	return self;
}

void
gebr_expr_set_var (GebrExpr *self,
		   const gchar *name,
		   const gchar *value)
{
	g_hash_table_insert (self->vars,
			     g_strdup (name),
			     g_strdup (value));
}

gdouble
gebr_expr_eval (GebrExpr *self,
		const gchar *expr,
		GError **_error)
{
	GError *error = NULL;
	gchar *argv[] = { "bc", "-l", NULL };
	gint in_fd, out_fd, err_fd;
	const gchar *home_dir;

	home_dir = g_get_home_dir ();
	g_spawn_async_with_pipes (home_dir, argv, NULL, G_SPAWN_SEARCH_PATH,
				  NULL, NULL, NULL,
				  &in_fd, &out_fd, &err_fd, &error);

	if (error) {
		if (*_error)
			*_error = error;
		else {
			g_critical ("Error running bc: %s", error->message);
			g_error_free (error);
		}
		return 0;
	}

	gchar *name;
	gchar *value;
	gchar *decl;
	size_t len;
	GList *keys;

	keys = g_hash_table_get_keys (self->vars);

	for (GList *i = keys; i; i = i->next) {
		name = i->data;
		value = g_hash_table_lookup (self->vars, name);
		decl = g_strdup_printf ("%s = %s\n", name, value);
		len = strlen (decl);
		write (in_fd, decl, len);
		g_free (decl);
	}

	decl = g_strdup_printf ("%s\n", expr);
	len = strlen (decl);
	write (in_fd, expr, len);
}

void
gebr_expr_free (GebrExpr *self)
{
	g_hash_table_unref (self->vars);
	g_free (self);
}
