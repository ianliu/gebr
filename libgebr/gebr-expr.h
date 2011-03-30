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

#ifndef __GEBR_EXPR_H__
#define __GEBR_EXPR_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct _GebrExpr GebrExpr;

/**
 * gebr_expr_new:
 *
 * Creates a new expression evaluator.
 *
 * Returns: a new expression evaluator, free with gebr_expr_free()
 */
GebrExpr *gebr_expr_new (void);

/**
 * gebr_expr_set_var:
 * @self: a #GebrExpr
 * @name: the name of the variable
 * @value: the value of the variable
 */
void gebr_expr_set_var (GebrExpr *self,
			const gchar *name,
			const gchar *value);

/**
 * gebr_expr_eval:
 * @self: a #GebrExpr
 * @expression: the arithmetic expression to evaluate
 * @error: return location for an error
 */
gdouble gebr_expr_eval (GebrExpr *self,
			const gchar *expression,
			GError **error);

/**
 * gebr_expr_free:
 * @self: a #GebrExpr
 */
void gebr_expr_free (GebrExpr *self);

G_END_DECLS

#endif /* __GEBR_EXPR_H__ */
