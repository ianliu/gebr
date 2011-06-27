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
 * @param: The variable to operate on
 * @pivot: The pivot for the operation, or %NULL to append
 * @affected: The #GebrGeoXmlParameter's affected by this operation
 */
GebrGeoXmlParameter* gebr_validator_move(GebrValidator       *self,
                                         GebrGeoXmlParameter *key,
                                         GebrGeoXmlParameter *pivot,
                                         GList              **affected);

/**
 * gebr_validator_check_using_var:
 * @validator: A #GebrValidator
 * @source: Variable which the function check if using @var
 * @var: The variable for check
 * @scope: The variable scope (GEBR_GEOXML_DOCUMENT_TYPE_FLOW | LINE | PROJECT)
 *
 * Returns: %TRUE if @source using @var (direct or indirect), %FALSE otherwise.
 */
gboolean
gebr_validator_check_using_var(GebrValidator *self,
                               const gchar   *source,
			       GebrGeoXmlDocumentType scope,
                               const gchar   *var);
/**
 * gebr_validator_expression_check_using_var:
 * @validator: A #GebrValidator
 * @expr: Variable which the function check if using @var
 * @var: The variable for check
 * @scope: The variable scope (GEBR_GEOXML_DOCUMENT_TYPE_FLOW | LINE | PROJECT)
 *
 * Important! This function works only for string variables.
 * Returns: %TRUE if @source using @var (direct or indirect), %FALSE otherwise.
 */
gboolean
gebr_validator_expression_check_using_var(GebrValidator *self,
                                          const gchar   *expr,
                                          GebrGeoXmlDocumentType scope,
                                          const gchar   *var);

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
 * gebr_validator_evaluate:
 * @validator: The #GebrValidator to be used
 * @expr: The expression to be evaluated
 * @type: The type of expression (GEBR_GEOXML_PARAMETER_TYPE_STRING | GEBR_GEOXML_PARAMETER_TYPE_FLOAT)
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
                                 gchar **value,
                                 GError **error);

G_END_DECLS

#endif /* __GEBR_VALIDATOR_H__ */
