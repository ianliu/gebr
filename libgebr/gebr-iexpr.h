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

/**
 * SECTION: gebr-iexpr
 * @title: GebrIExpr Interface
 * @short_description: Interface for defining expressions and validate them.
 *
 * The #GebrIExpr interface defines methods for creating and validating expressions.
 * There are two kinds of expressions in GeBR: #GebrArithmeticExpr and #GebrStringExpr.
 */

#ifndef __LIBGEBR_IEXPR_H__
#define __LIBGEBR_IEXPR_H__

#include <glib.h>
#include <glib-object.h>
#include <geoxml/parameter.h>

G_BEGIN_DECLS

#define GEBR_TYPE_IEXPR			(gebr_iexpr_get_type())
#define GEBR_IEXPR(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), GEBR_TYPE_IEXPR, GebrIExpr))
#define GEBR_IS_IEXPR(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), GEBR_TYPE_IEXPR))
#define GEBR_IEXPR_GET_INTERFACE(inst)	(G_TYPE_INSTANCE_GET_INTERFACE((inst), GEBR_TYPE_IEXPR, GebrIExprInterface))

/**
 * GEBR_IEXPR_ERROR:
 *
 * Error domain for handling expressions. Errors in this domain
 * will be from #GebrIExprError enumeration.
 */
#define GEBR_IEXPR_ERROR (gebr_iexpr_error_quark())

/**
 * GebrIExprError:
 * @GEBR_IEXPR_ERROR_INVAL_VAR: An invalid variable name was found.
 * @GEBR_IEXPR_ERROR_INVAL_EXPR: The expression is invalid.
 *
 * Error codes returned by expression handling functions.
 */
typedef enum {
	GEBR_IEXPR_ERROR_INVAL_VAR,
	GEBR_IEXPR_ERROR_INVAL_EXPR,
} GebrIExprError;

typedef struct _GebrIExpr GebrIExpr;
typedef struct _GebrIExprInterface GebrIExprInterface;

struct _GebrIExprInterface {
	GTypeInterface parent;

	gboolean (*set_var) (GebrIExpr              *self,
			     const gchar            *name,
			     GebrGeoXmlParameterType type,
			     const gchar            *value,
			     GError                **error);

	gboolean (*is_valid) (GebrIExpr   *self,
			      const gchar *expr,
			      GError     **error);
};

GType gebr_iexpr_get_type(void) G_GNUC_CONST;

/**
 * gebr_iexpr_set_var:
 * @expr: a #GebrIExpr
 * @name: the name of the variable
 * @type: one of #GebrGeoXmlParameterType
 * @value: the value of the variable
 * @error: return location for an #GEBR_IEXPR_ERROR, or %NULL
 *
 * Returns: %TRUE if variable was successfully set, %FALSE otherwise.
 */
gboolean gebr_iexpr_set_var(GebrIExpr              *self,
			    const gchar            *name,
			    GebrGeoXmlParameterType type,
			    const gchar            *value,
			    GError                **error);

/**
 * gebr_iexpr_is_valid:
 * @expr: a #GebrIExpr
 * @expression: the expression to check for validity
 * @error: return location for an #GEBR_IEXPR_ERROR, or %NULL
 *
 * Returns: %TRUE if @expression is valid, %FALSE otherwise.
 */
gboolean gebr_iexpr_is_valid(GebrIExpr   *self,
			     const gchar *expr,
			     GError     **error);

G_END_DECLS

#endif /* __LIBGEBR_IEXPR_H__ */
