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

/*
 * internal stuff
 */

struct gebr_geoxml_parameter_group {
	GdomeElement *element;
};

static void
__gebr_geoxml_parameter_group_turn_instance_to_reference(GebrGeoXmlParameterGroup * parameter_group,
							 GebrGeoXmlParameters * instance)
{
	GebrGeoXmlParameters *first_instance;
	GebrGeoXmlSequence *fi_parameter;
	GebrGeoXmlSequence *parameter;

	first_instance = gebr_geoxml_parameter_group_get_template(parameter_group);
	gebr_geoxml_parameters_get_parameter(first_instance, &fi_parameter, 0);
	gebr_geoxml_parameters_get_parameter(instance, &parameter, 0);
	for (; fi_parameter != NULL;
	     __gebr_geoxml_sequence_next(&fi_parameter), __gebr_geoxml_sequence_next(&parameter)) {
		__gebr_geoxml_element_assign_new_id((GdomeElement *) parameter, NULL, FALSE);
		__gebr_geoxml_parameter_set_be_reference(GEBR_GEOXML_PARAMETER(parameter),
							 GEBR_GEOXML_PARAMETER(fi_parameter));
	}
}

void __gebr_geoxml_parameter_group_turn_to_reference(GebrGeoXmlParameterGroup * parameter_group)
{
	GebrGeoXmlSequence *instance;

	gebr_geoxml_parameter_group_get_instance(parameter_group, &instance, 0);
	for (; instance != NULL; gebr_geoxml_sequence_next(&instance))
		__gebr_geoxml_parameter_group_turn_instance_to_reference(parameter_group,
									 GEBR_GEOXML_PARAMETERS(instance));
}

GebrGeoXmlParameters *gebr_geoxml_parameter_group_get_template(GebrGeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return NULL;

	GdomeElement *tpl;
	GdomeElement *group;

	group = __gebr_geoxml_parameter_get_type_element(GEBR_GEOXML_PARAMETER(parameter_group));
	tpl = __gebr_geoxml_get_first_element(group, "template-instance");
	tpl = __gebr_geoxml_get_first_element(tpl, "parameters");

	return GEBR_GEOXML_PARAMETERS(tpl);
}

GebrGeoXmlParameters *gebr_geoxml_parameter_group_add_instance(GebrGeoXmlParameterGroup * parameter_group)
{
	GebrGeoXmlSequence *template_instance;
	GebrGeoXmlParameters *new_instance;

	template_instance = (GebrGeoXmlSequence*)gebr_geoxml_parameter_group_get_template(parameter_group);

	new_instance = GEBR_GEOXML_PARAMETERS(__gebr_geoxml_sequence_append_clone(template_instance));
	GdomeElement *group = __gebr_geoxml_parameter_get_type_element(GEBR_GEOXML_PARAMETER(parameter_group));
	gdome_el_insertBefore_protected(group, (GdomeNode*)new_instance, NULL, &exception);

	__gebr_geoxml_parameter_group_turn_instance_to_reference(parameter_group, new_instance);

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
	if (parameter_group == NULL) {
		*parameters = NULL;
		return GEBR_GEOXML_RETV_NULL_PTR;
	}

	GdomeElement * type_element;

	type_element = __gebr_geoxml_parameter_get_type_element(GEBR_GEOXML_PARAMETER(parameter_group));
	*parameters = (GebrGeoXmlSequence *) __gebr_geoxml_get_element_at(type_element, "parameters", index, FALSE);

	return (*parameters == NULL)
	    ? GEBR_GEOXML_RETV_INVALID_INDEX : GEBR_GEOXML_RETV_SUCCESS;
}

glong gebr_geoxml_parameter_group_get_instances_number(GebrGeoXmlParameterGroup * parameter_group)
{
	g_return_val_if_fail(parameter_group != NULL, -1);

	GdomeElement * type_element;

	type_element = __gebr_geoxml_parameter_get_type_element(GEBR_GEOXML_PARAMETER(parameter_group));

	return __gebr_geoxml_get_elements_number(type_element, "parameters");
}

void gebr_geoxml_parameter_group_set_is_instanciable(GebrGeoXmlParameterGroup * parameter_group, gboolean enable)
{
	GdomeElement * type_element;

	g_return_if_fail(parameter_group != NULL);

	type_element = __gebr_geoxml_parameter_get_type_element(GEBR_GEOXML_PARAMETER(parameter_group));
	__gebr_geoxml_set_attr_value(type_element, "instanciable", (enable == TRUE ? "yes" : "no"));
}

void gebr_geoxml_parameter_group_set_expand(GebrGeoXmlParameterGroup * parameter_group, const gboolean enable)
{
	GdomeElement * type_element;

	g_return_if_fail(parameter_group != NULL);

	type_element = __gebr_geoxml_parameter_get_type_element(GEBR_GEOXML_PARAMETER(parameter_group));
	__gebr_geoxml_set_attr_value(type_element, "expand", (enable == TRUE ? "yes" : "no"));
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
