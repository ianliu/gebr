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

#ifndef __GEBR_GUI_PARAMETER_H
#define __GEBR_GUI_PARAMETER_H

#include <gtk/gtk.h>

#include <geoxml/geoxml.h>
#include <gebr-validator.h>

#include "gebr-gui-file-entry.h"
#include "gebr-gui-value-sequence-edit.h"
#include <libgebr/gebr-maestro-info.h>
#include "gebr-gui-complete-variables.h"

G_BEGIN_DECLS

typedef struct _GebrGuiParam GebrGuiParam;

typedef void (*GebrGuiParameterValidatedFunc) (GebrGuiParam *widget,
					       gpointer user_data);

typedef void (*changed_callback) (GebrGuiParam *widget, gpointer user_data);

typedef struct _GebrGuiParamPriv GebrGuiParamPriv;

struct _GebrGuiParam
{
	GebrGuiParamPriv *priv;

	/*< private >*/
	GebrValidator *validator;

	GebrGeoXmlParameter *parameter;
	GebrGeoXmlProgramParameter *program_parameter;
	GebrGeoXmlParameterType parameter_type;
	gboolean use_default_value;
	gpointer data;
	gboolean readonly;
	GebrMaestroInfo *info;

	GtkWidget *widget;
	GtkWidget *value_widget;

	/* for lists */
	GtkWidget *list_value_widget;
	GebrGuiValueSequenceEdit *gebr_gui_value_sequence_edit;

	/* for group */
	GtkWidget *group_warning_widget;

	/* auto submit stuff */
	changed_callback callback;
	gpointer user_data;
};

/**
 * Create a new parameter widget.
 */
GebrGuiParam *gebr_gui_param_new(GebrGeoXmlParameter *parameter,
						      GebrValidator       *validator,
						      GebrMaestroInfo	  *info,
						      gboolean             use_default_value,
						      GebrGuiCompleteVariables *complete_var,
						      gpointer             data);

/**
 *
 */
void gebr_gui_param_set_auto_submit_callback(GebrGuiParam *parameter_widget,
							changed_callback callback, gpointer user_data);

/**
 *
 */
void gebr_gui_param_set_readonly(GebrGuiParam *parameter_widget, gboolean readonly);

/**
 * Update input widget value from the parameter's value
 */
void gebr_gui_param_update(GebrGuiParam *parameter_widget);

/**
 * gebr_gui_param_validate:
 * @parameter_widget: The parameter widget to be validated
 *
 * Set tooltip errors and icons for @parameter_widget. If the parameter is numeric
 * and have minimum/maximum values, clamp the value.
 *
 * Returns: %TRUE if the parameter was valid, %FALSE otherwise.
 */
gboolean gebr_gui_param_validate(GebrGuiParam *parameter_widget);

/**
 * Update UI of list with the new separator
 */
void gebr_gui_param_update_list_separator(GebrGuiParam *parameter_widget);

/**
 * Rebuild the UI.
 */
void gebr_gui_param_reconfigure(GebrGuiParam *parameter_widget);

gboolean gebr_gui_param_match_completion(GtkEntryCompletion *completion,
						    const gchar *key,
						    GtkTreeIter *iter,
						    gpointer user_data);

/**
 * gebr_gui_parameter_add_variables_popup:
 * @entry:
 * @flow:
 * @line:
 * @proj:
 * @type:
 *
 * Creates a popup menu with compatible variables for @entry.
 *
 * Returns: a #GtkMenu
 */
GtkWidget *gebr_gui_parameter_add_variables_popup(GtkEntry *entry,
						  GebrGeoXmlDocument *flow,
						  GebrGeoXmlDocument *line,
						  GebrGeoXmlDocument *proj,
						  GebrGeoXmlParameterType type);

/**
 * gebr_gui_parameter_get_completion_model:
 */
GtkTreeModel *gebr_gui_parameter_get_completion_model(GebrGeoXmlDocument *flow,
						      GebrGeoXmlDocument *line,
						      GebrGeoXmlDocument *proj,
						      GebrGeoXmlParameterType type) G_GNUC_DEPRECATED;

/**
 * gebr_gui_parameter_set_entry_completion:
 */
void gebr_gui_parameter_set_entry_completion(GtkEntry *entry,
					     GtkTreeModel *model,
					     GebrGeoXmlParameterType type);

/**
 * gebr_gui_group_instance_validate:
 */
gboolean gebr_gui_group_instance_validate(GebrValidator *validator, GebrGeoXmlSequence *instance, GtkWidget *icon);

/**
 * gebr_gui_param_set_validated_callback:
 *
 * Registers @callback to be called when this parameter validates.
 */
void gebr_gui_param_set_validated_callback(GebrGuiParam *widget,
		GebrGuiParameterValidatedFunc callback, gpointer user_data);

G_END_DECLS
#endif				//__GEBR_GUI_PARAMETER_H
