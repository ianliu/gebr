/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2011 GeBR core team (http://www.gebrproject.com/)
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

#ifndef __GEBR_VALIDATOR_H__
#define __GEBR_VALIDATOR_H__

#include <glib.h>
#include <geoxml/gebr-geo-types.h>

G_BEGIN_DECLS

typedef struct _GebrValidator GebrValidator;

/**
 * gebr_validator_new:
 * @flow: Reference to a flow
 * @line: Reference to a line
 * @proj: Reference to a project
 *
 * Returns: A #GebrValidator to validate type parameters and/or expressions.
 */
GebrValidator *gebr_validator_new(GebrGeoXmlDocument **flow,
				  GebrGeoXmlDocument **line,
				  GebrGeoXmlDocument **proj);

/**
 * gebr_validator_def:
 * @validator: A #GebrValidator
 * @param: The variable to be defined
 * @error: Return location for error, or %NULL
 *
 * Defines a variable in the validator, if the variable already exist,
 * just updates the @param value in the validator.
 *
 * @see gebr_validator_change_value
 *
 * Returns: %TRUE if no error ocurred, %FALSE otherwise
 */
gboolean gebr_validator_insert(GebrValidator       *self,
			       GebrGeoXmlParameter *param,
			       GList              **affected,
			       GError             **error);

/**
 * gebr_validator_remove:
 * @validator: A #GebrValidator
 * @param: The variable to be deleted
 * @affected: A list containing the #GebrGeoXmlParameter's affected
 *
 * Returns: %TRUE if the variable was removed, %FALSE if variable is not defined
 */
gboolean gebr_validator_remove(GebrValidator       *self,
			   GebrGeoXmlParameter *param,
			   GList              **affected,
			   GError	      **error);

/**
 * gebr_validator_rename:
 * @validator: A #GebrValidator
 * @param: The variable to operate on
 * @new_name: The new name for @param
 * @affected: The #GebrGeoXmlParameter's affected by this operation
 * @error: Return location for error, or %NULL
 *
 * If the @param has not been inserted in validator, returns %FALSE
 *
 * Returns: %TRUE if no error ocurred, %FALSE otherwise
 */
gboolean gebr_validator_rename(GebrValidator       *self,
			       GebrGeoXmlParameter *param,
			       const gchar         *new_name,
			       GList              **affected,
			       GError             **error);

/**
 * gebr_validator_change_value:
 * @validator: A #GebrValidator
 * @param: The variable to operate on
 * @new_value: The new value for @param
 * @affected: The #GebrGeoXmlParameter's affected by this operation
 * @error: Return location for error, or %NULL
 *
 * Find variable on the correct scope, and changes the @param value to @new_value.
 *
 * Returns: %TRUE if no error ocurred, %FALSE otherwise
 */
gboolean gebr_validator_change_value(GebrValidator       *self,
				     GebrGeoXmlParameter *param,
				     const gchar         *new_value,
				     GList              **affected,
				     GError             **error);

/**
 * gebr_validator_move:
 * @validator: A #GebrValidator
 * @source: The variable to operate on
 * @pivot: The pivot for the operation, or %NULL to append
 * @pivot_scope: The scope of the pivot
 * @copy: The return location for the new parameter
 * @affected: The #GebrGeoXmlParameter's affected by this operation
 * @error: Return location for error
 *
 * Returns: %TRUE if the move was successfull, %FALSE otherwise.
 */
gboolean gebr_validator_move(GebrValidator         *self,
			     GebrGeoXmlParameter   *source,
			     GebrGeoXmlParameter   *pivot,
			     GebrGeoXmlDocumentType pivot_scope,
			     GebrGeoXmlParameter  **copy,
			     GList                **affected,
			     GError               **error);

/**
 * gebr_validator_validate_param:
 * @validator: A #GebrValidator
 * @parameter: The #GebrGeoXmlParameter to be validated
 * @validated: Return location for the validated string, free with g_free()
 * @error: Return location for the error, or %NULL
 *
 * Validates @parameter's values and returns the @validated string. If @parameter
 * is not valid, then @error is filled with the appropriate message and %NULL is
 * returned.
 *
 * Returns: %TRUE if @param is valid, %FALSE otherwise.
 */
gboolean gebr_validator_validate_param(GebrValidator       *validator,
				       GebrGeoXmlParameter *parameter,
				       gchar              **validated,
				       GError             **error);


/**
 * gebr_validator_validate_expr:
 * @validator: A #GebrValidator
 * @expression: The expression to be validated
 * @type: The type of the @expression
 * @scope: The scope to start validate
 * @error: Returns location for the error, or %NULL
 *
 * Returns: %TRUE if @expression is valid, %FALSE otherwise.
 */
gboolean gebr_validator_validate_expr_on_scope(GebrValidator          *self,
                                               const gchar            *str,
                                               GebrGeoXmlParameterType type,
                                               GebrGeoXmlDocumentType scope,
                                               GError                **err);
/**
 * gebr_validator_validate_expr:
 * @validator: A #GebrValidator
 * @expression: The expression to be validated
 * @type: The type of the @expression
 * @error: Returns location for the error, or %NULL
 *
 * Returns: %TRUE if @expression is valid, %FALSE otherwise.
 */
gboolean gebr_validator_validate_expr(GebrValidator          *validator,
				      const gchar            *expression,
				      GebrGeoXmlParameterType type,
				      GError                **error);

/**
 * gebr_validator_get_documents:
 * @validator: A #GebrValidator
 * @flow: Return location for the flow
 * @line: Return location for the line
 * @proj: Return location for the proj
 */
void gebr_validator_get_documents(GebrValidator       *validator,
				  GebrGeoXmlDocument **flow,
				  GebrGeoXmlDocument **line,
				  GebrGeoXmlDocument **proj);

/**
 * gebr_validator_update:
 * @validator: The #GebrValidator to be updated
 */
void gebr_validator_update(GebrValidator *validator);

/**
 * gebr_validator_free:
 * @validator: The #GebrValidator to be freed
 */
void gebr_validator_free(GebrValidator *validator);

/**
 * gebr_validator_evaluate_param:
 * @validator: The #GebrValidator to be used
 * @expr: The expression to be evaluated
 * @type: The type of expression (GEBR_GEOXML_PARAMETER_TYPE_STRING | GEBR_GEOXML_PARAMETER_TYPE_FLOAT)
 * @scope: Scope to evaluate expression
 * @show_interval: %TRUE if want to evaluate a interval ([1, ..., 10]), %FALSE otherwise
 * @value: Returns the value of the expression.
 * @error: Returns the error, if any.
 *
 * Calculate the value of @expr and return it at @value.
 *
 * Returns: %TRUE if @expr could be evaluated. %FALSE otherwise.
 */
gboolean gebr_validator_evaluate_interval(GebrValidator *self,
                                          const gchar *expr,
                                          GebrGeoXmlParameterType type,
                                          GebrGeoXmlDocumentType scope,
                                          gboolean show_interval,
                                          gchar **value,
                                          GError **error);

/**
 * gebr_validator_evaluate_param:
 * @validator: The #GebrValidator to be used
 * @expr: The expression to be evaluated
 * @type: The type of expression (GEBR_GEOXML_PARAMETER_TYPE_STRING | GEBR_GEOXML_PARAMETER_TYPE_FLOAT)
 * @scope: Scope to evaluate expression
 * @value: Returns the value of the expression.
 * @error: Returns the error, if any.
 *
 * Calculate the value of @expr and return it at @value.
 *
 * Returns: %TRUE if @expr could be evaluated. %FALSE otherwise.
 */
gboolean gebr_validator_evaluate(GebrValidator *self,
                                 const gchar * expr,
                                 GebrGeoXmlParameterType type,
                                 GebrGeoXmlDocumentType scope,
                                 gchar **value,
                                 GError **error);
/**
 * gebr_validator_evaluate_param:
 * @validator: The #GebrValidator to be used
 * @myparam: The parameter to evaluate
 * @value: Returns the value of the expression.
 * @error: Returns the error, if any.
 *
 * Calculate the value of @expr and return it at @value.
 *
 * Returns: %TRUE if @expr could be evaluated. %FALSE otherwise.
 */
gboolean gebr_validator_evaluate_param(GebrValidator *self,
                                       GebrGeoXmlParameter *myparam,
                                       gchar **value,
                                       GError **error);

/**
 * gebr_validator_is_var_in_scope:
 * @validator:
 * @name:
 * @scope:
 *
 * Returns: %TRUE if @name exists in @scope, %FALSE otherwise
 */
gboolean gebr_validator_is_var_in_scope(GebrValidator *self,
					const gchar *name,
					GebrGeoXmlDocumentType scope);
/**
 * gebr_validator_set_document:
 * @validator:
 * @doc:
 * @type:
 *
 * Set on validator, a new @doc
 *
 */
void gebr_validator_set_document(GebrValidator *self,
				 GebrGeoXmlDocument **doc,
				 GebrGeoXmlDocumentType type);

/**
 * gebr_validator_use_iter:
 * @self:
 * @expr:
 * @type:
 * @scope:
 *
 * Returns:
 */
gboolean gebr_validator_use_iter(GebrValidator *self,
				 const gchar *expr,
				 GebrGeoXmlParameterType type,
				 GebrGeoXmlDocumentType scope);

/**
 * gebr_validator_validate_control_parameter:
 * @self: @GebrValidator to validate parameter
 * @name:
 * @expression:
 * @error: @GError to set if has error
 *
 * Return %FALSE if parameter have error, %FALSE otherwise
 */
gboolean
gebr_validator_validate_control_parameter(GebrValidator *self,
                                          const gchar *name,
                                          const gchar *expression,
                                          GError **error);

G_END_DECLS

#endif /* __GEBR_VALIDATOR_H__ */
