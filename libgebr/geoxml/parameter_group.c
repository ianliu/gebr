/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This parameter_group is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This parameter_group is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this parameter_group.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>

#include <gdome.h>

#include "parameter_group.h"
#include "parameter_group_p.h"
#include "xml.h"
#include "types.h"
#include "error.h"
#include "parameters.h"
#include "parameters_p.h"
#include "parameter_p.h"
#include "sequence_p.h"
#include "sequence.h"

/*
 * internal stuff
 */

struct gebr_geoxml_parameter_group {
	GdomeElement *element;
};

/*
 * __gebr_geoxml_parameter_group_turn_instance_to_reference:
 * @parameter_group:
 * @instance:
 */
static void
__gebr_geoxml_parameter_group_turn_instance_to_reference(GebrGeoXmlParameters * instance)
{
	GebrGeoXmlSequence *parameter;

	gebr_geoxml_parameters_get_parameter(instance, &parameter, 0);
	while (parameter) {
		__gebr_geoxml_parameter_set_be_reference(GEBR_GEOXML_PARAMETER(parameter));
		gebr_geoxml_sequence_next(&parameter);
	}
}

GebrGeoXmlParameters *gebr_geoxml_parameter_group_get_template(GebrGeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return NULL;

	GdomeElement *parent;
	GdomeElement *grandpa;
	GdomeElement *group;

	group = __gebr_geoxml_parameter_get_type_element(GEBR_GEOXML_PARAMETER(parameter_group));
	parent = __gebr_geoxml_get_first_element(group, "template-instance");
	grandpa = __gebr_geoxml_get_first_element(parent, "parameters");
	gdome_el_unref(group, &exception);
	gdome_el_unref(parent, &exception);

	return GEBR_GEOXML_PARAMETERS(grandpa);
}

GebrGeoXmlParameters *gebr_geoxml_parameter_group_add_instance(GebrGeoXmlParameterGroup * parameter_group)
{
	GebrGeoXmlSequence *template_instance;
	GebrGeoXmlParameters *new_instance;

	template_instance = (GebrGeoXmlSequence*)gebr_geoxml_parameter_group_get_template(parameter_group);

	new_instance = GEBR_GEOXML_PARAMETERS(__gebr_geoxml_sequence_append_clone(template_instance));
	GdomeElement *group = __gebr_geoxml_parameter_get_type_element(GEBR_GEOXML_PARAMETER(parameter_group));
	gdome_n_unref(gdome_el_insertBefore_protected(group, (GdomeNode*)new_instance, NULL, &exception), &exception);

	__gebr_geoxml_parameter_group_turn_instance_to_reference(new_instance);

	return new_instance;
}

/*
 * library functions.
 */

GebrGeoXmlParameters *gebr_geoxml_parameter_group_instanciate(GebrGeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return NULL;

	if (gebr_geoxml_parameter_group_get_is_instanciable(parameter_group) == FALSE)
		return NULL;

	return gebr_geoxml_parameter_group_add_instance(parameter_group);
}

gboolean gebr_geoxml_parameter_group_deinstanciate(GebrGeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return FALSE;

	if (gebr_geoxml_parameter_group_get_is_instanciable(parameter_group) == FALSE)
		return FALSE;

	gulong length;
	GebrGeoXmlSequence *last_instance;

	length = gebr_geoxml_parameter_group_get_instances_number(parameter_group);
	if (length <= 1)
		return FALSE;
	gebr_geoxml_parameter_group_get_instance(parameter_group, &last_instance, length-1);
	if (!last_instance)
		return FALSE;

	gdome_n_removeChild(gdome_n_parentNode((GdomeNode *) last_instance, &exception),
			    (GdomeNode *) last_instance, &exception);

	return TRUE;
}

int gebr_geoxml_parameter_group_get_instance(GebrGeoXmlParameterGroup * parameter_group,
					     GebrGeoXmlSequence ** parameters, gulong index)
{
	enum GEBR_GEOXML_RETV retval = GEBR_GEOXML_RETV_SUCCESS;

	if (parameter_group == NULL) {
		*parameters = NULL;
		return GEBR_GEOXML_RETV_NULL_PTR;
	}

	GdomeElement * type_element;

	type_element = __gebr_geoxml_parameter_get_type_element(GEBR_GEOXML_PARAMETER(parameter_group));
	*parameters = (GebrGeoXmlSequence *) __gebr_geoxml_get_element_at(type_element, "parameters", index, FALSE);
	gdome_el_unref(type_element, &exception);

	retval =  (*parameters == NULL)
	    ? GEBR_GEOXML_RETV_INVALID_INDEX : GEBR_GEOXML_RETV_SUCCESS;

	return retval;
}

glong gebr_geoxml_parameter_group_get_instances_number(GebrGeoXmlParameterGroup * parameter_group)
{
	g_return_val_if_fail(parameter_group != NULL, -1);

	GdomeElement * type_element;

	type_element = __gebr_geoxml_parameter_get_type_element(GEBR_GEOXML_PARAMETER(parameter_group));

	gulong retval =  __gebr_geoxml_get_elements_number(type_element, "parameters");
	gdome_el_unref(type_element, &exception);

	return retval;
}

void gebr_geoxml_parameter_group_set_is_instanciable(GebrGeoXmlParameterGroup * parameter_group, gboolean enable)
{
	GdomeElement * type_element;

	g_return_if_fail(parameter_group != NULL);

	type_element = __gebr_geoxml_parameter_get_type_element(GEBR_GEOXML_PARAMETER(parameter_group));
	__gebr_geoxml_set_attr_value(type_element, "instanciable", (enable == TRUE ? "yes" : "no"));
	gdome_el_unref(type_element, &exception);
}

void gebr_geoxml_parameter_group_set_expand(GebrGeoXmlParameterGroup * parameter_group, const gboolean enable)
{
	GdomeElement * type_element;

	g_return_if_fail(parameter_group != NULL);

	type_element = __gebr_geoxml_parameter_get_type_element(GEBR_GEOXML_PARAMETER(parameter_group));
	__gebr_geoxml_set_attr_value(type_element, "expand", (enable == TRUE ? "yes" : "no"));
	gdome_el_unref(type_element, &exception);
}

gboolean gebr_geoxml_parameter_group_get_is_instanciable(GebrGeoXmlParameterGroup * parameter_group)
{
	GdomeElement * group;
	const gchar * instanciable;

	g_return_val_if_fail(parameter_group != NULL, FALSE);

	group = __gebr_geoxml_parameter_get_type_element(GEBR_GEOXML_PARAMETER(parameter_group));
	instanciable = __gebr_geoxml_get_attr_value((GdomeElement*)group, "instanciable");

	return (!strcmp(instanciable, "yes")) ? TRUE : FALSE;
}

gboolean gebr_geoxml_parameter_group_get_expand(GebrGeoXmlParameterGroup * parameter_group)
{
	const gchar * expand;
	GdomeElement * type_element;

	g_return_val_if_fail(parameter_group != NULL, FALSE);

	type_element = __gebr_geoxml_parameter_get_type_element(GEBR_GEOXML_PARAMETER(parameter_group));
	expand = __gebr_geoxml_get_attr_value(type_element, "expand");

	return (!strcmp(expand, "yes")) ? TRUE : FALSE;
}

void gebr_geoxml_parameter_group_set_exclusive(GebrGeoXmlParameterGroup * parameter_group, gboolean exclusive)
{
	GebrGeoXmlParameters * template;
	GebrGeoXmlSequence * instance;
	GebrGeoXmlSequence * parameter;

	g_return_if_fail(parameter_group != NULL);

	template = gebr_geoxml_parameter_group_get_template(parameter_group);
	if (exclusive) {
		__gebr_geoxml_set_attr_value((GdomeElement*)template, "default-selection", "1");
		__gebr_geoxml_set_attr_value((GdomeElement*)template, "selection", "1");
	} else {
		__gebr_geoxml_set_attr_value((GdomeElement*)template, "default-selection", "0");
		__gebr_geoxml_remove_attr((GdomeElement*)template, "selection");
	}

	gebr_geoxml_parameter_group_get_instance(parameter_group, &instance, 0);
	while (instance) {
		if (!exclusive)
			gebr_geoxml_parameters_set_default_selection(GEBR_GEOXML_PARAMETERS(instance), NULL);
		else {
			gebr_geoxml_parameters_get_parameter(GEBR_GEOXML_PARAMETERS(instance), &parameter, 0);
			gebr_geoxml_parameters_set_default_selection(GEBR_GEOXML_PARAMETERS(instance),
								     GEBR_GEOXML_PARAMETER(parameter));
		}
		gebr_geoxml_sequence_next(&instance);
	}
}

gboolean gebr_geoxml_parameter_group_is_exclusive(GebrGeoXmlParameterGroup * parameter_group)
{
	gchar * default_selection;
	GebrGeoXmlParameters * template;

	g_return_val_if_fail(parameter_group != NULL, FALSE);

	template = gebr_geoxml_parameter_group_get_template(parameter_group);
	default_selection = __gebr_geoxml_get_attr_value((GdomeElement*)template, "default-selection");
	gdome_el_unref((GdomeElement*)template, &exception);
	gboolean retval = default_selection[0] == '0' ? FALSE : TRUE;
	g_free(default_selection);
	return retval;
}
