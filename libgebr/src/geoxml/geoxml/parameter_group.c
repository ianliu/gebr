/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://sites.google.com/site/gebrproject/)
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

struct geoxml_parameter_group {
	GdomeElement * element;
};

static void
__geoxml_parameter_group_turn_instance_to_reference(GeoXmlParameterGroup * parameter_group, GeoXmlParameters * instance)
{
	GeoXmlSequence *	first_instance;
	GeoXmlSequence *	fi_parameter;
	GeoXmlSequence *	parameter;

	geoxml_parameter_group_get_instance(parameter_group, &first_instance, 0);
	geoxml_parameters_get_parameter(GEOXML_PARAMETERS(first_instance), &fi_parameter, 0);
	geoxml_parameters_get_parameter(instance, &parameter, 0);
	for (; fi_parameter != NULL; __geoxml_sequence_next(&fi_parameter), __geoxml_sequence_next(&parameter)) {
		__geoxml_element_assign_new_id((GdomeElement*)parameter, NULL, FALSE);
		__geoxml_parameter_set_be_reference(GEOXML_PARAMETER(parameter),
			GEOXML_PARAMETER(fi_parameter));
	}
}

void
__geoxml_parameter_group_turn_to_reference(GeoXmlParameterGroup * parameter_group)
{
	GeoXmlSequence *	instance;

	geoxml_parameter_group_get_instance(parameter_group, &instance, 0);
	for (; instance != NULL; geoxml_sequence_next(&instance))
		__geoxml_parameter_group_turn_instance_to_reference(parameter_group, GEOXML_PARAMETERS(instance));
}

/*
 * library functions.
 */

GeoXmlParameters *
geoxml_parameter_group_instanciate(GeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return FALSE;
	if (geoxml_parameter_group_get_is_instanciable(parameter_group) == FALSE)
		return FALSE;

	GeoXmlSequence *	first_instance;
	GeoXmlParameters *	new_instance;

	geoxml_parameter_group_get_instance(parameter_group, &first_instance, 0);
	new_instance = (GeoXmlParameters*)__geoxml_sequence_append_clone(first_instance);
	__geoxml_parameter_group_turn_instance_to_reference(parameter_group, new_instance);
	geoxml_parameters_reset(new_instance, TRUE);

	return new_instance;
}

gboolean
geoxml_parameter_group_deinstanciate(GeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return FALSE;
	if (geoxml_parameter_group_get_is_instanciable(parameter_group) == FALSE)
		return FALSE;

	GeoXmlSequence *	first_instance;
	GeoXmlSequence *	last_instance;

	geoxml_parameter_group_get_instance(parameter_group, &first_instance, 0);
	geoxml_parameter_group_get_instance(parameter_group, &last_instance,
		geoxml_parameter_group_get_instances_number(parameter_group)-1);
	if (last_instance == NULL && last_instance == first_instance)
		return FALSE;
	gdome_n_removeChild(gdome_n_parentNode((GdomeNode*)last_instance, &exception),
		(GdomeNode*)last_instance, &exception);

	return TRUE;
}

int
geoxml_parameter_group_get_instance(GeoXmlParameterGroup * parameter_group,
	GeoXmlSequence ** parameters, gulong index)
{
	if (parameter_group == NULL) {
		*parameters = NULL;
		return GEOXML_RETV_NULL_PTR;
	}

	*parameters = (GeoXmlSequence*)__geoxml_get_element_at(
		__geoxml_parameter_get_type_element(GEOXML_PARAMETER(parameter_group), FALSE),
		"parameters", index, FALSE);

	return (*parameters == NULL)
		? GEOXML_RETV_INVALID_INDEX
		: GEOXML_RETV_SUCCESS;
}

glong
geoxml_parameter_group_get_instances_number(GeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return -1;
	return __geoxml_get_elements_number((GdomeElement*)
		__geoxml_parameter_get_type_element(GEOXML_PARAMETER(parameter_group), FALSE), "parameters");
}

GSList *
geoxml_parameter_group_get_parameter_in_all_instances(GeoXmlParameterGroup * parameter_group, guint index)
{
	if (parameter_group == NULL)
		return NULL;

	GeoXmlSequence *	first_instance;
	GeoXmlSequence *	parameter;
	GSList *		idref_list;

	geoxml_parameter_group_get_instance(parameter_group, &first_instance, 0);
	geoxml_parameters_get_parameter(GEOXML_PARAMETERS(first_instance), &parameter, 0);
	idref_list = __geoxml_parameter_get_referencee_list((GdomeElement*)parameter_group,
		__geoxml_get_attr_value((GdomeElement*)parameter, "id"));
	idref_list = g_slist_prepend(idref_list, parameter);

	return idref_list;
}

void
geoxml_parameter_group_set_is_instanciable(GeoXmlParameterGroup * parameter_group, gboolean enable)
{
	if (parameter_group == NULL)
		return;
	__geoxml_set_attr_value(
		__geoxml_parameter_get_type_element(GEOXML_PARAMETER(parameter_group), FALSE),
		"instanciable", (enable == TRUE ? "yes" : "no"));
}

void
geoxml_parameter_group_set_expand(GeoXmlParameterGroup * parameter_group, const gboolean enable)
{
	if (parameter_group == NULL)
		return;
	__geoxml_set_attr_value(
		__geoxml_parameter_get_type_element(GEOXML_PARAMETER(parameter_group), FALSE),
		"expand", (enable == TRUE ? "yes" : "no"));
}

gboolean
geoxml_parameter_group_get_is_instanciable(GeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return FALSE;
	return (!strcmp(__geoxml_get_attr_value(
			__geoxml_parameter_get_type_element(GEOXML_PARAMETER(parameter_group), FALSE),
			"instanciable"), "yes"))
		? TRUE : FALSE;
}

gboolean
geoxml_parameter_group_get_expand(GeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return FALSE;
	return (!strcmp(__geoxml_get_attr_value(
			__geoxml_parameter_get_type_element(GEOXML_PARAMETER(parameter_group), FALSE),
			"expand"), "yes"))
		? TRUE : FALSE;
}
