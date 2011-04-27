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

G_DEFINE_INTERFACE(GebrIExpr, gebr_iexpr, G_TYPE_OBJECT);

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
