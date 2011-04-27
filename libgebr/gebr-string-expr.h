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
 * SECTION: gebr-string-expr
 * @short_description:
 */

#ifndef __LIBGEBR_STRING_EXPR_H__
#define __LIBGEBR_STRING_EXPR_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GEBR_TYPE_STRING_EXPR			(gebr_string_expr_get_type())
#define GEBR_STRING_EXPR(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), GEBR_TYPE_STRING_EXPR, GebrStringExpr))
#define GEBR_STRING_EXPR_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), GEBR_TYPE_STRING_EXPR, GebrStringExprClass))
#define GEBR_IS_STRING_EXPR(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), GEBR_TYPE_STRING_EXPR))
#define GEBR_IS_STRING_EXPR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), GEBR_TYPE_STRING_EXPR))
#define GEBR_STRING_EXPR_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), GEBR_TYPE_STRING_EXPR, GebrStringExprClass))

typedef struct _GebrStringExpr GebrStringExpr;
typedef struct _GebrStringExprPriv GebrStringExprPriv;
typedef struct _GebrStringExprClass GebrStringExprClass;

struct _GebrStringExpr {
	GObject parent;

	/*< private >*/
	GebrStringExprPriv *priv;
};

struct _GebrStringExprClass {
	GObjectClass parent;
};

GType gebr_string_expr_get_type(void) G_GNUC_CONST;

/**
 * gebr_string_expr_new:
 * 
 * Returns: a newly allocated #GebrStringExpr with reference count of 1.
 */
GebrStringExpr *gebr_string_expr_new(void);

/**
 * gebr_string_expr_eval:
 * @expr: a #GebrStringExpr
 * @expression: a string expression
 * @error: return location for an error, or %NULL
 *
 * Returns: %TRUE if evaluation was successful, %FALSE otherwise.
 */
gboolean gebr_string_expr_eval(GebrStringExpr *self,
			       const gchar    *expr,
			       gchar         **result,
			       GError        **error);

G_END_DECLS

#endif /* __LIBGEBR_STRING_EXPR_H__ */

