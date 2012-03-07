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

#include "../libgebr-gettext.h"

#include "program.h"
#include "program_p.h"

#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gi18n-lib.h>
#include <gdome.h>
#include <libgebr/gebr-validator.h>

#include "error.h"
#include "object.h"
#include "parameter.h"
#include "parameter_group.h"
#include "parameter_group_p.h"
#include "parameters.h"
#include "parameters_p.h"
#include "program-parameter.h"
#include "sequence.h"
#include "types.h"
#include "value_sequence.h"
#include "xml.h"

/*
 * internal structures and funcionts
 */

struct gebr_geoxml_program {
	GdomeElement *element;
};

typedef enum{
	GEBR_GEOXML_PROGRAM_MPI_TYPE_OPENMPI,
	GEBR_GEOXML_PROGRAM_MPI_TYPE_MPICH2
} GebrGeoxmlProgramMpiType;
/*
 * library functions.
 */

static gboolean gebr_geoxml_parameters_foreach(GebrGeoXmlParameters * parameters, GebrGeoXmlCallback callback, gpointer user_data)
{
	GebrGeoXmlSequence *parameter;

	gebr_geoxml_parameters_get_parameter(parameters, &parameter, 0);
	for (; parameter != NULL; gebr_geoxml_sequence_next(&parameter)) {
		if (gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(parameter)) == GEBR_GEOXML_PARAMETER_TYPE_GROUP) {
			GebrGeoXmlSequence *instance;
			gebr_geoxml_parameter_group_get_instance(GEBR_GEOXML_PARAMETER_GROUP(parameter), &instance, 0);
			for (; instance != NULL; gebr_geoxml_sequence_next(&instance))
				if (!gebr_geoxml_parameters_foreach(GEBR_GEOXML_PARAMETERS(instance), callback, user_data)) {
					gebr_geoxml_object_unref(parameter);
					gebr_geoxml_object_unref(instance);
					return FALSE;
				}
		} else if (!callback(GEBR_GEOXML_OBJECT(parameter), user_data)) {
			gebr_geoxml_object_unref(parameter);
			return FALSE;
		}
	}

	return TRUE;
}

void gebr_geoxml_program_foreach_parameter(GebrGeoXmlProgram * program, GebrGeoXmlCallback callback, gpointer user_data)
{
	if (program == NULL)
		return;

	GebrGeoXmlParameters *params = gebr_geoxml_program_get_parameters(program);
	gebr_geoxml_parameters_foreach(params, callback, user_data);
	gebr_geoxml_object_unref(params);
}

GebrGeoXmlFlow *gebr_geoxml_program_flow(GebrGeoXmlProgram * program)
{
	return (GebrGeoXmlFlow *) gebr_geoxml_object_get_owner_document((GebrGeoXmlObject*) program);
}

GebrGeoXmlParameters *gebr_geoxml_program_get_parameters(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return (GebrGeoXmlParameters *) __gebr_geoxml_get_first_element((GdomeElement *) program, "parameters");
}

gsize gebr_geoxml_program_count_parameters(GebrGeoXmlProgram * program)
{
	gsize n = 0;
	GebrGeoXmlSequence *param;
	GebrGeoXmlParameters *parameters;
	GebrGeoXmlParameterType type;

	g_return_val_if_fail(program != NULL, 0);

	parameters = gebr_geoxml_program_get_parameters(program);
	gebr_geoxml_parameters_get_parameter(parameters, &param, 0);
	while (param) {
		type = gebr_geoxml_parameter_get_type(GEBR_GEOXML_PARAMETER(param));
		if (type == GEBR_GEOXML_PARAMETER_TYPE_GROUP) {
			GebrGeoXmlParameters *template;
			GebrGeoXmlSequence *inner_param;

			template = gebr_geoxml_parameter_group_get_template(GEBR_GEOXML_PARAMETER_GROUP(param));
			gebr_geoxml_parameters_get_parameter(template, &inner_param, 0);

			while (inner_param) {
				n++;
				gebr_geoxml_sequence_next(&inner_param);
			}
		} else
			n++;
		gebr_geoxml_sequence_next(&param);
	}
	return n;
}

void gebr_geoxml_program_set_stdin(GebrGeoXmlProgram * program, const gboolean enable)
{
	if (program == NULL)
		return;
	__gebr_geoxml_set_attr_value((GdomeElement *) program, "stdin", (enable == TRUE ? "yes" : "no"));
}

void gebr_geoxml_program_set_stdout(GebrGeoXmlProgram * program, const gboolean enable)
{
	if (program == NULL)
		return;
	__gebr_geoxml_set_attr_value((GdomeElement *) program, "stdout", (enable == TRUE ? "yes" : "no"));
}

void gebr_geoxml_program_set_stderr(GebrGeoXmlProgram * program, const gboolean enable)
{
	if (program == NULL)
		return;
	__gebr_geoxml_set_attr_value((GdomeElement *) program, "stderr", (enable == TRUE ? "yes" : "no"));
}

void gebr_geoxml_program_set_status(GebrGeoXmlProgram * program, GebrGeoXmlProgramStatus status)
{
	const gchar * status_str;

	if (program == NULL)
		return;

	switch(status) {
	case GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED:
		status_str = "unconfigured";
		break;
	case GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED:
		status_str = "configured";
		break;
	case GEBR_GEOXML_PROGRAM_STATUS_DISABLED:
		status_str = "disabled";
		break;
	default:
		status_str = "";
		break;
	}
	__gebr_geoxml_set_attr_value((GdomeElement *) program, "status", status_str);
}

void gebr_geoxml_program_set_title(GebrGeoXmlProgram * program, const gchar * title)
{
	if (program == NULL || title == NULL)
		return;
	__gebr_geoxml_set_tag_value((GdomeElement *) program, "title", title, __gebr_geoxml_create_TextNode);
}

void gebr_geoxml_program_set_binary(GebrGeoXmlProgram * program, const gchar * binary)
{
	if (program == NULL || binary == NULL)
		return;
	__gebr_geoxml_set_tag_value((GdomeElement *) program, "binary", binary, __gebr_geoxml_create_TextNode);
}

void gebr_geoxml_program_set_description(GebrGeoXmlProgram * program, const gchar * description)
{
	if (program == NULL || description == NULL)
		return;
	__gebr_geoxml_set_tag_value((GdomeElement *) program, "description", description,
				    __gebr_geoxml_create_TextNode);
}

void gebr_geoxml_program_set_help(GebrGeoXmlProgram * program, const gchar * help)
{
	if (program == NULL || help == NULL)
		return;
	__gebr_geoxml_set_tag_value((GdomeElement *) program, "help", help, __gebr_geoxml_create_CDATASection);
}

void gebr_geoxml_program_set_version(GebrGeoXmlProgram * program, const gchar * version)
{
	if (program == NULL || version == NULL)
		return;
	__gebr_geoxml_set_attr_value((GdomeElement*)program, "version", version);
}

void gebr_geoxml_program_set_mpi(GebrGeoXmlProgram * program, const gchar * mpi_type)
{
	if (program == NULL || mpi_type == NULL)
		return;
	__gebr_geoxml_set_attr_value((GdomeElement*)program, "mpi", mpi_type);

	if (!*mpi_type)
		gebr_geoxml_program_mpi_remove_tags(program);
	else
		gebr_geoxml_program_mpi_add_tags(program, mpi_type);
}

void gebr_geoxml_program_set_url(GebrGeoXmlProgram * program, const gchar * url)
{
	if (program == NULL || url == NULL)
		return;
	__gebr_geoxml_set_tag_value((GdomeElement *) program, "url", url, __gebr_geoxml_create_TextNode);
}

gboolean gebr_geoxml_program_get_stdin(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return FALSE;
	return (!strcmp(__gebr_geoxml_get_attr_value((GdomeElement *) program, "stdin"), "yes"))
	    ? TRUE : FALSE;
}

gboolean gebr_geoxml_program_get_stdout(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return FALSE;
	return (!strcmp(__gebr_geoxml_get_attr_value((GdomeElement *) program, "stdout"), "yes"))
	    ? TRUE : FALSE;
}

gboolean gebr_geoxml_program_get_stderr(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return FALSE;
	return (!strcmp(__gebr_geoxml_get_attr_value((GdomeElement *) program, "stderr"), "yes"))
	    ? TRUE : FALSE;
}

GebrGeoXmlProgramStatus gebr_geoxml_program_get_status(GebrGeoXmlProgram * program)
{
	GebrGeoXmlProgramStatus ret = GEBR_GEOXML_PROGRAM_STATUS_UNKNOWN;
	gchar *status;

	if (program == NULL)
		return GEBR_GEOXML_PROGRAM_STATUS_UNKNOWN;

	status = __gebr_geoxml_get_attr_value((GdomeElement *) program, "status");

	if (!strcmp(status, "unconfigured"))
		ret = GEBR_GEOXML_PROGRAM_STATUS_UNCONFIGURED;

	if (!strcmp(status, "configured"))
		ret = GEBR_GEOXML_PROGRAM_STATUS_CONFIGURED;

	if (!strcmp(status, "disabled"))
		ret = GEBR_GEOXML_PROGRAM_STATUS_DISABLED;

	g_free(status);

	return ret;
}

gchar *gebr_geoxml_program_get_title(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value((GdomeElement *) program, "title");
}

const gchar *gebr_geoxml_program_get_binary(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value((GdomeElement *) program, "binary");
}

gchar *gebr_geoxml_program_get_description(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value((GdomeElement *) program, "description");
}

gchar *gebr_geoxml_program_get_help(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value((GdomeElement *) program, "help");
}

gchar *gebr_geoxml_program_get_version(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return __gebr_geoxml_get_attr_value((GdomeElement*)program, "version");
}

const gchar *gebr_geoxml_program_get_mpi(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return __gebr_geoxml_get_attr_value((GdomeElement*)program, "mpi");
}

const gchar *gebr_geoxml_program_get_url(GebrGeoXmlProgram * program)
{
	if (program == NULL)
		return NULL;
	return __gebr_geoxml_get_tag_value((GdomeElement *) program, "url");
}

GebrGeoXmlProgramControl gebr_geoxml_program_get_control(GebrGeoXmlProgram * program)
{
	gchar *control;

	// FIXME if program is NULL, should it really return GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY?
	// Shouldn't it be GEBR_GEOXML_PROGRAM_CONTROL_UNKNOWN? Or maybe display a warning?
	if (program == NULL)
		return GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY;

	control = __gebr_geoxml_get_attr_value((GdomeElement*)program, "control");

	if (g_strcmp0(control, "for") == 0) {
		g_free(control);
		return GEBR_GEOXML_PROGRAM_CONTROL_FOR;
	}

	if (strlen (control) == 0) {
		g_free(control);
		return GEBR_GEOXML_PROGRAM_CONTROL_ORDINARY;
	}

	g_free(control);

	return GEBR_GEOXML_PROGRAM_CONTROL_UNKNOWN;
}

static void
gebr_geoxml_program_control_get_info(GebrGeoXmlProgram *prog,
				     gchar *bounds[],
				     gchar *labels[])
{
	GebrGeoXmlProgramParameter *pparam;
	GebrGeoXmlParameters *params;
	GebrGeoXmlSequence *seq;
	gchar *value;
	gchar *label;
	gchar *keyword;

	params = gebr_geoxml_program_get_parameters(prog);
	gebr_geoxml_parameters_get_parameter(params, &seq, 0);
	gebr_geoxml_object_unref(params);

	for (; seq; gebr_geoxml_sequence_next(&seq)) {
		pparam = GEBR_GEOXML_PROGRAM_PARAMETER(seq);
		keyword = gebr_geoxml_program_parameter_get_keyword(pparam);
		value = gebr_geoxml_program_parameter_get_first_value(pparam, FALSE);
		if (!*value)
			value = gebr_geoxml_program_parameter_get_first_value(pparam, TRUE);
		label = gebr_geoxml_parameter_get_label(GEBR_GEOXML_PARAMETER(seq));

		if (!g_strcmp0(keyword, "niter")) {
			if (bounds) bounds[2] = value;
			if (labels) labels[2] = label;
			continue;
		}

		if (!g_strcmp0(keyword, "step")) {
			if (bounds) bounds[1] = value;
			if (labels) labels[1] = label;
			continue;
		}

		if (!g_strcmp0(keyword, "ini_value")) {
			if (bounds) bounds[0] = value;
			if (labels) labels[0] = label;
		}
	}
}

void
gebr_geoxml_program_control_get_labels(GebrGeoXmlProgram *prog,
				       gchar **ini,
				       gchar **step,
				       gchar **niter)
{
	GebrGeoXmlProgramControl c;
	gchar *labels[3];

	g_return_if_fail (prog != NULL);

	c = gebr_geoxml_program_get_control (prog);
	g_return_if_fail (c == GEBR_GEOXML_PROGRAM_CONTROL_FOR);

	gebr_geoxml_program_control_get_info(prog, NULL, labels);

	if (ini)
		*ini = labels[0];

	if (step)
		*step = labels[1];

	if (niter)
		*niter = labels[2];
}

gchar *
gebr_geoxml_program_control_get_n(GebrGeoXmlProgram *prog,
				  gchar **step,
				  gchar **ini)
{
	GebrGeoXmlProgramControl c;
	gchar *bounds[3];

	g_return_val_if_fail (prog != NULL, NULL);
	c = gebr_geoxml_program_get_control (prog);
	g_return_val_if_fail (c == GEBR_GEOXML_PROGRAM_CONTROL_FOR, NULL);

	gebr_geoxml_program_control_get_info(prog, bounds, NULL);

	if (ini)
		*ini = bounds[0];

	if (step)
		*step = bounds[1];

	return bounds[2];
}

void gebr_geoxml_program_control_set_n(GebrGeoXmlProgram *prog,
                                       const gchar *step,
                                       const gchar *ini,
                                       const gchar *n)
{
	GebrGeoXmlSequence *seq;
	GebrGeoXmlProgramControl c;
	GebrGeoXmlParameters *params;
	GebrGeoXmlValueSequence *value;
	const gchar *keyword;

	g_return_if_fail (prog != NULL);

	c = gebr_geoxml_program_get_control (prog);
	g_return_if_fail (c == GEBR_GEOXML_PROGRAM_CONTROL_FOR);

	params = gebr_geoxml_program_get_parameters (prog);
	gebr_geoxml_parameters_get_parameter(params, &seq, 0);
	gebr_geoxml_object_unref(params);

	while (seq) {
		gebr_geoxml_program_parameter_get_value (GEBR_GEOXML_PROGRAM_PARAMETER (seq),
						 	 FALSE, (GebrGeoXmlSequence**)&value, 0);
		keyword = gebr_geoxml_program_parameter_get_keyword(GEBR_GEOXML_PROGRAM_PARAMETER (seq));
		if(!strcmp(keyword,"niter"))
			gebr_geoxml_value_sequence_set(value, n);
		else if(!strcmp(keyword,"step"))
			gebr_geoxml_value_sequence_set(value, step);
		else if(!strcmp(keyword,"ini_value"))
			gebr_geoxml_value_sequence_set(value, ini);

		gebr_geoxml_object_unref(value);
		gebr_geoxml_sequence_next(&seq);
	}
}

gboolean gebr_geoxml_program_is_var_used (GebrGeoXmlProgram *self,
					  const gchar *var_name)
{
	GebrGeoXmlParameters *params;
	params = gebr_geoxml_program_get_parameters (self);
	return gebr_geoxml_parameters_is_var_used (params, var_name);
}

void gebr_geoxml_program_set_error_id(GebrGeoXmlProgram *self,
				      gboolean clear,
				      GebrIExprError id)
{
	gchar *str_id;

	g_return_if_fail(self != NULL);

	if (clear)
		str_id = g_strdup("");
	else
		str_id = g_strdup_printf("%d", id);

	__gebr_geoxml_set_attr_value((GdomeElement*)self, "errorid", str_id);
	g_free(str_id);
}

gboolean gebr_geoxml_program_get_error_id(GebrGeoXmlProgram *self,
					  GebrIExprError *id)
{
	const gchar *str_id;

	g_return_val_if_fail(self != NULL, FALSE);

	str_id = __gebr_geoxml_get_attr_value((GdomeElement*)self, "errorid");

	if (*str_id) {
		if (id)
			*id = g_ascii_strtoll(str_id, NULL, 10);
		return TRUE;
	}

	return FALSE;
}

typedef struct {
	GebrValidator *validator;
	GError *error;
} ValidationData;

static gboolean
validate_program_parameter(GebrGeoXmlObject *object, gpointer user_data)
{
	GebrGeoXmlParameter *parameter = GEBR_GEOXML_PARAMETER(object);
	ValidationData *data = user_data;
	GebrGeoXmlParameters *instance;
	GebrGeoXmlParameter *selected;

	/* for exclusive groups, check if this is
	 * the selected parameter of its instance */
	instance = gebr_geoxml_parameter_get_parameters(parameter);
	selected = gebr_geoxml_parameters_get_selection(instance);
	if (selected == NULL || selected == parameter)
		gebr_validator_validate_param(data->validator, parameter,
					      NULL, &data->error);
	gebr_geoxml_object_unref(instance);
	gebr_geoxml_object_unref(selected);

	if (data->error)
		return FALSE;

	return TRUE;
}

gboolean gebr_geoxml_program_is_valid(GebrGeoXmlProgram *self,
				      GebrValidator *validator,
				      GError **err)
{
	ValidationData data = {
		.validator = validator,
		.error = NULL
	};
	gebr_geoxml_program_foreach_parameter(self, validate_program_parameter, &data);

	if (data.error) {
		gebr_geoxml_program_set_error_id(self, FALSE, data.error->code);
		g_propagate_error(err, data.error);
		return FALSE;
	}

	gebr_geoxml_program_set_error_id(self, TRUE, 0);

	return TRUE;
}

void
gebr_geoxml_program_mpi_add_tags(GebrGeoXmlProgram *self,
				 const gchar *mpi)
{
	GdomeElement *tmp = __gebr_geoxml_insert_new_element((GdomeElement*)self, "mpi", NULL);
	GebrGeoXmlParameters *params = __gebr_geoxml_parameters_append_new(tmp);

	//Add Group MPI Parameters
	GebrGeoXmlParameter *group = gebr_geoxml_parameters_append_parameter(params, GEBR_GEOXML_PARAMETER_TYPE_GROUP);
	gebr_geoxml_parameter_set_label(group, _("MPI Parameters"));
	gebr_geoxml_parameter_group_set_expand(GEBR_GEOXML_PARAMETER_GROUP(group), TRUE);
	GebrGeoXmlParameters *template = gebr_geoxml_parameter_group_get_template(GEBR_GEOXML_PARAMETER_GROUP(group));

	// Add parameter NP on instance
	GebrGeoXmlParameter *param = gebr_geoxml_parameters_append_parameter(template, GEBR_GEOXML_PARAMETER_TYPE_RANGE);
	gebr_geoxml_parameter_set_label(param, _("Number of process to start"));
	gebr_geoxml_program_parameter_set_number_min_max(GEBR_GEOXML_PROGRAM_PARAMETER(param), "1", "9999999");
	gebr_geoxml_program_parameter_set_required(GEBR_GEOXML_PROGRAM_PARAMETER(param), TRUE);
	gebr_geoxml_program_parameter_set_keyword(GEBR_GEOXML_PROGRAM_PARAMETER(param), "np");
	gebr_geoxml_program_parameter_set_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(param), TRUE, "1");

	gebr_geoxml_object_unref(tmp);
	gebr_geoxml_object_unref(params);
	gebr_geoxml_object_unref(param);
	gebr_geoxml_object_unref(group);
	gebr_geoxml_object_unref(template);
}

void
gebr_geoxml_program_mpi_remove_tags(GebrGeoXmlProgram *self)
{
	GdomeElement *tmp = __gebr_geoxml_get_first_element((GdomeElement*)self, "mpi");
	if (tmp)
		gebr_geoxml_object_unref(gdome_el_removeChild((GdomeElement*)self,
							      (GdomeNode*)tmp, &exception));
	gebr_geoxml_object_unref(tmp);
}

GebrGeoXmlParameters *
gebr_geoxml_program_mpi_get_parameters(GebrGeoXmlProgram *self)
{
	GdomeElement *tmp = __gebr_geoxml_get_first_element((GdomeElement*)self, "mpi");

	if (!tmp)
		return NULL;

	GdomeElement *params = __gebr_geoxml_get_first_element(tmp, "parameters");
	gebr_geoxml_object_unref(tmp);

	return GEBR_GEOXML_PARAMETERS(params);
}

GebrGeoXmlParameter *
get_mpi_parameter(GebrGeoXmlProgram *self,
		  const gchar *keyword)
{
	GebrGeoXmlParameters *params = gebr_geoxml_program_mpi_get_parameters(self);
	if (!params)
		return NULL;

	GebrGeoXmlSequence *seq;
	GebrGeoXmlSequence *instance;
	GebrGeoXmlSequence *group = gebr_geoxml_parameters_get_first_parameter(params);
	gebr_geoxml_parameter_group_get_instance(GEBR_GEOXML_PARAMETER_GROUP(group), &instance, 0);

	seq = gebr_geoxml_parameters_get_first_parameter(GEBR_GEOXML_PARAMETERS(instance));
	for (; seq; gebr_geoxml_sequence_next(&seq)) {
		GebrGeoXmlParameter *param = gebr_geoxml_parameter_get_referencee(GEBR_GEOXML_PARAMETER(seq));
		GebrGeoXmlProgramParameter *prog = GEBR_GEOXML_PROGRAM_PARAMETER(param);
		gchar *key = gebr_geoxml_program_parameter_get_keyword(prog);

		gebr_geoxml_object_unref(param);

		if (g_strcmp0(key, keyword) == 0)
			break;
	}

	gebr_geoxml_object_unref(params);
	gebr_geoxml_object_unref(instance);
	gebr_geoxml_object_unref(group);

	return GEBR_GEOXML_PARAMETER(seq);
}

void
gebr_geoxml_program_mpi_set_n_process(GebrGeoXmlProgram *self,
				      gint n)
{
	g_return_if_fail(self != NULL);

	GebrGeoXmlParameter *np = get_mpi_parameter(self, "np");

	if (!np)
		return;

	gchar *tmp = g_strdup_printf("%d", n);
	gebr_geoxml_program_parameter_set_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(np), FALSE, tmp);
	g_free(tmp);
}

gint
gebr_geoxml_program_mpi_get_n_process(GebrGeoXmlProgram *self)
{
	g_return_val_if_fail(self != NULL, 0);

	GebrGeoXmlParameter *np = get_mpi_parameter(self, "np");

	g_return_val_if_fail(np != NULL, 0);

	gchar *tmp = gebr_geoxml_program_parameter_get_first_value(GEBR_GEOXML_PROGRAM_PARAMETER(np), FALSE);
	gint n = atoi(tmp);
	g_free(tmp);
	return n;
}
