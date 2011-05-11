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
#include <geoxml/geoxml.h>
#include <gui/gebr-gui-validatable-widget.h>

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
 * gebr_validator_validate_widget:
 * @validator: A #GebrValidator
 * @widget: A #GebrGuiValidatableWidget
 * @parameter: The #GebrGeoXmlParameter associated with @widget
 *
 * Returns: %TRUE if @param is valid, %FALSE otherwise.
 */
gboolean gebr_validator_validate_widget(GebrValidator            *validator,
					GebrGuiValidatableWidget *widget,
					GebrGeoXmlParameter      *parameter);

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
 * gebr_validator_free:
 * @validator: The #GebrValidator to be freed
 */
void gebr_validator_free(GebrValidator *validator);

G_END_DECLS

#endif /* __GEBR_VALIDATOR_H__ */
