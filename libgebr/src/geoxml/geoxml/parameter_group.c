/*   libgebr - GÍBR Library
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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

#include <gdome.h>

#include "parameter_group.h"
#include "xml.h"
#include "types.h"
#include "parameters.h"
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

void
geoxml_parameter_group_instantiate(GeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return;

	gint			i;
	glong			parameters_by_instance;
	GeoXmlSequence *	parameter;

	gulong			instances;
	gchar *			string;

	/* instanciante by duplicating the instance parameters and appending them */
	parameter = geoxml_parameters_get_first_parameter(GEOXML_PARAMETERS(parameter_group));
	parameters_by_instance = geoxml_parameter_group_get_parameters_by_instance(parameter_group);
	for (i = 0; i < parameters_by_instance; ++i) {
		GdomeNode *	clone;

		clone = gdome_n_cloneNode((GdomeNode*)parameter, TRUE, &exception);
		gdome_el_insertBefore((GdomeElement*)parameter_group, clone, NULL, &exception);

		/* reset its value */
		geoxml_parameter_reset((GeoXmlParameter*)clone);

		geoxml_sequence_next(&parameter);
	}

	/* increase instances counter */
	instances = geoxml_parameter_group_get_instances(parameter_group) + 1;
	string = g_strdup_printf("%lu", instances);
	__geoxml_set_attr_value((GdomeElement*)parameter_group, "instances", string);

	g_free(string);
}

void
geoxml_parameter_group_deinstantiate(GeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return;

	gint			i;
	gulong			fpoli; /* fpoli: first parameter of last instance :D */
	glong			parameters_by_instance;
	GeoXmlSequence *	parameter;

	gulong			instances;
	gchar *			string;

	/* get the number of instances and check if we have at least one */
	instances = geoxml_parameter_group_get_instances(parameter_group);
	if ((instances - 1) == 1)
		return;
	parameters_by_instance = geoxml_parameter_group_get_parameters_by_instance(parameter_group);
	fpoli = parameters_by_instance*(instances-1);
	parameter = geoxml_parameters_get_first_parameter(GEOXML_PARAMETERS(parameter_group));

	/* go to the last instance */
	for (i = 0; i < fpoli; ++i)
		geoxml_sequence_next(&parameter);
	/* delete its parameters */
	for (i = 0; i < parameters_by_instance; ++i) {
		GeoXmlSequence *	aux;

		aux = parameter;
		geoxml_sequence_next(&parameter);
		geoxml_sequence_remove(aux);
	}

	/* decrease instances counter */
	--instances;
	string = g_strdup_printf("%lu", instances);
	__geoxml_set_attr_value((GdomeElement*)parameter_group, "instances", string);

	g_free(string);
}

void
geoxml_parameter_group_set_exclusive(GeoXmlParameterGroup * parameter_group, const gboolean enable)
{
	if (parameter_group == NULL)
		return;
	__geoxml_set_attr_value((GdomeElement*)parameter_group, "exclusive", (enable == TRUE ? "yes" : "no"));
}

void
geoxml_parameter_group_set_expand(GeoXmlParameterGroup * parameter_group, const gboolean enable)
{
	if (parameter_group == NULL)
		return;
	__geoxml_set_attr_value((GdomeElement*)parameter_group, "expand", (enable == TRUE ? "yes" : "no"));
}

gulong
geoxml_parameter_group_get_instances(GeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return 0;
	return (gulong)atol(__geoxml_get_attr_value((GdomeElement*)parameter_group, "instances"));
}

gulong
geoxml_parameter_group_get_parameters_by_instance(GeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return 0;
	return geoxml_parameters_get_number(GEOXML_PARAMETERS(parameter_group))
		/ geoxml_parameter_group_get_instances(parameter_group);
}

gboolean
geoxml_parameter_group_get_exclusive(GeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return FALSE;
	return (!g_ascii_strcasecmp(__geoxml_get_attr_value((GdomeElement*)parameter_group, "exclusive"), "yes"))
		? TRUE : FALSE;
}

gboolean
geoxml_parameter_group_get_expand(GeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return FALSE;
	return (!g_ascii_strcasecmp(__geoxml_get_attr_value((GdomeElement*)parameter_group, "expand"), "yes"))
		? TRUE : FALSE;
}
