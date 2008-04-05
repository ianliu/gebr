/*   libgebr - G�BR Library
 *   Copyright (C) 2007-2008 G�BR core team (http://gebr.sourceforge.net)
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
#include "xml.h"
#include "types.h"
#include "parameters.h"
#include "parameters_p.h"
#include "parameter_p.h"

/*
 * internal stuff
 */

struct geoxml_parameter_group {
	GdomeElement * element;
};

/*
 * library functions.
 */

GeoXmlParameters *
geoxml_parameter_group_get_parameters(GeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return NULL;
	return (GeoXmlParameters*)__geoxml_get_first_element((GdomeElement*)parameter_group, "parameters");
}

GeoXmlParameter *
geoxml_parameter_group_last_instance_parameter(GeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return FALSE;

	gint			i;
	gulong			fpoli; /* fpoli: first parameter of last instance :D */
	GeoXmlParameters *	parameters;
	glong			parameters_by_instance;
	GeoXmlSequence *	parameter;

	gulong			instances;

	/* get the number of instances and check if we have at least one */
	instances = geoxml_parameter_group_get_instances(parameter_group);
	parameters_by_instance = geoxml_parameter_group_get_parameters_by_instance(parameter_group);
	fpoli = parameters_by_instance*(instances-1);
	parameters = geoxml_parameter_group_get_parameters(parameter_group);
	parameter = geoxml_parameters_get_first_parameter(parameters);

	/* go to the last instance */
	for (i = 0; i < fpoli; ++i)
		geoxml_sequence_next(&parameter);

	return GEOXML_PARAMETER(parameter);
}

gboolean
geoxml_parameter_group_instanciate(GeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return FALSE;
	if (geoxml_parameter_group_get_can_instanciate(parameter_group) == FALSE)
		return FALSE;

	gint			i;
	GeoXmlParameters *	parameters;
	glong			parameters_by_instance;
	GeoXmlSequence *	parameter;

	gulong			instances;
	gchar *			string;

	/* instanciante by duplicating the instance parameters and appending them */
	parameters = geoxml_parameter_group_get_parameters(parameter_group);
	parameter = geoxml_parameters_get_first_parameter(parameters);
	parameters_by_instance = geoxml_parameter_group_get_parameters_by_instance(parameter_group);
	for (i = 0; i < parameters_by_instance; ++i) {
		GdomeNode *	clone;

		clone = gdome_n_cloneNode((GdomeNode*)parameter, TRUE, &exception);
		gdome_el_insertBefore((GdomeElement*)parameters, clone, NULL, &exception);
		/* reset its value */
		geoxml_parameter_reset((GeoXmlParameter*)clone, TRUE);

		geoxml_sequence_next(&parameter);
	}

	/* increase instances counter */
	instances = geoxml_parameter_group_get_instances(parameter_group) + 1;
	string = g_strdup_printf("%lu", instances);
	__geoxml_set_attr_value((GdomeElement*)parameter_group, "instances", string);

	g_free(string);

	return TRUE;
}

gboolean
geoxml_parameter_group_deinstanciate(GeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return FALSE;
	if (geoxml_parameter_group_get_can_instanciate(parameter_group) == FALSE)
		return FALSE;

	GeoXmlSequence *	parameter;
	GeoXmlParameters *	parameters;

	gulong			instances;
	gchar *			string;

	/* get the number of instances and check if we have at least one */
	instances = geoxml_parameter_group_get_instances(parameter_group);
	if (instances == 1)
		return FALSE;
	parameters = geoxml_parameter_group_get_parameters(parameter_group);
	parameter = GEOXML_SEQUENCE(geoxml_parameter_group_last_instance_parameter(parameter_group));

	/* delete last instance's parameters */
	do {
		GeoXmlSequence *	aux;

		aux = parameter;
		geoxml_sequence_next(&parameter);
		gdome_n_removeChild((GdomeNode*)parameters, (GdomeNode*)aux, &exception);
	} while (parameter != NULL);

	/* decrease instances counter */
	string = g_strdup_printf("%lu", --instances);
	__geoxml_set_attr_value((GdomeElement*)parameter_group, "instances", string);

	g_free(string);

	return TRUE;
}

gulong
geoxml_parameter_group_get_instances(GeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return 0;
	return (gulong)atol(__geoxml_get_attr_value((GdomeElement*)parameter_group, "instances"));
}

void
geoxml_parameter_group_set_can_instanciate(GeoXmlParameterGroup * parameter_group, gboolean can_instanciate)
{
	if (parameter_group == NULL)
		return;
	if (can_instanciate == FALSE)
		while (geoxml_parameter_group_deinstanciate(parameter_group) == TRUE);
	__geoxml_set_attr_value((GdomeElement*)parameter_group, "multiple",
		(can_instanciate == TRUE ? "yes" : "no"));
}

void
geoxml_parameter_group_set_parameters_by_instance(GeoXmlParameterGroup * parameter_group, gulong number)
{
	if (parameter_group == NULL)
		return;

	gchar *	string;

	string = g_strdup_printf("%lu", number);
	__geoxml_set_attr_value((GdomeElement*)parameter_group, "npar", string);
	g_free(string);
}

void
geoxml_parameter_group_set_exclusive(GeoXmlParameterGroup * parameter_group, const gboolean enable)
{
	if (parameter_group == NULL)
		return;
	__geoxml_set_attr_value((GdomeElement*)parameter_group, "exclusive", (enable == TRUE ? "yes" : "no"));
	/* clean all parameters values */
	__geoxml_parameters_reset(
		geoxml_parameter_group_get_parameters(parameter_group), FALSE);
}

void
geoxml_parameter_group_set_expand(GeoXmlParameterGroup * parameter_group, const gboolean enable)
{
	if (parameter_group == NULL)
		return;
	__geoxml_set_attr_value((GdomeElement*)parameter_group, "expand", (enable == TRUE ? "yes" : "no"));
}

gboolean
geoxml_parameter_group_get_can_instanciate(GeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return FALSE;
	return (!strcmp(__geoxml_get_attr_value((GdomeElement*)parameter_group, "multiple"), "yes"))
		? TRUE : FALSE;
}

gulong
geoxml_parameter_group_get_parameters_by_instance(GeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return 0;
	return (gulong)atol(__geoxml_get_attr_value((GdomeElement*)parameter_group, "npar"));
}

gboolean
geoxml_parameter_group_get_exclusive(GeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return FALSE;
	return (!strcmp(__geoxml_get_attr_value((GdomeElement*)parameter_group, "exclusive"), "yes"))
		? TRUE : FALSE;
}

gboolean
geoxml_parameter_group_get_expand(GeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return FALSE;
	return (!strcmp(__geoxml_get_attr_value((GdomeElement*)parameter_group, "expand"), "yes"))
		? TRUE : FALSE;
}
