/*   libgebr - GeBR Library
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

#include "gebr-iexpr.h"

static void gebr_iexpr_default_init(GebrIExprInterface *klass);

GType gebr_iexpr_get_type (void)
{
	static volatile gsize g_define_type_id__volatile = 0;
	if (g_once_init_enter (&g_define_type_id__volatile)) {
		GType g_define_type_id =
			g_type_register_static_simple(G_TYPE_INTERFACE,
						      g_intern_static_string ("GebrIExpr"),
						      sizeof (GebrIExprInterface),
						      (GClassInitFunc) gebr_iexpr_default_init,
						      0,
						      (GInstanceInitFunc) NULL,
						      (GTypeFlags) 0);
		g_type_interface_add_prerequisite (g_define_type_id, G_TYPE_OBJECT);
		g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
	}
	return g_define_type_id__volatile;
}

static void gebr_iexpr_default_init(GebrIExprInterface *iface)
{
}

gboolean gebr_iexpr_set_var(GebrIExpr              *self,
			    const gchar            *name,
			    GebrGeoXmlParameterType type,
			    const gchar            *value,
			    GError                **error)
{
	return GEBR_IEXPR_GET_INTERFACE(self)->set_var(self, name, type, value, error);
}

gboolean gebr_iexpr_is_valid(GebrIExpr   *self,
			     const gchar *expr,
			     GError     **error)
{
	return GEBR_IEXPR_GET_INTERFACE(self)->is_valid(self, expr, error);
}

void gebr_iexpr_reset(GebrIExpr *self)
{
	GEBR_IEXPR_GET_INTERFACE(self)->reset(self);
}

GList *gebr_iexpr_extract_vars(GebrIExpr   *self,
			       const gchar *expr)
{
	return GEBR_IEXPR_GET_INTERFACE(self)->extract_vars(self, expr);
}

GQuark gebr_iexpr_error_quark(void)
{
	return g_quark_from_static_string("gebr-iexpr-error-quark");
}
gboolean gebr_iexpr_eval(GebrIExpr   *self,
			     const gchar *expr,
			     gchar ** value,
			     GError ** error)
{
	return GEBR_IEXPR_GET_INTERFACE(self)->eval(self, expr, value, error);
}
