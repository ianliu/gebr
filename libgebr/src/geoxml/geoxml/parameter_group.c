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

	group = __gebr_geoxml_parameter_get_type_element(GEBR_GEOXML_PARAMETER(parameter_group), FALSE);
	tpl = __gebr_geoxml_get_first_element(group, "parameters");

	return GEBR_GEOXML_PARAMETERS(tpl);
}

/*
 * library functions.
 */

GebrGeoXmlParameters *gebr_geoxml_parameter_group_instanciate(GebrGeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return FALSE;
	if (gebr_geoxml_parameter_group_get_is_instanciable(parameter_group) == FALSE)
		return FALSE;

	GebrGeoXmlSequence *first_instance;
	GebrGeoXmlParameters *new_instance;

	first_instance = (GebrGeoXmlSequence*)gebr_geoxml_parameter_group_get_template(parameter_group);
	new_instance = GEBR_GEOXML_PARAMETERS(__gebr_geoxml_sequence_append_clone(first_instance));
	__gebr_geoxml_parameter_group_turn_instance_to_reference(parameter_group, new_instance);
	gebr_geoxml_parameters_reset(new_instance, TRUE);

	return new_instance;
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

	gebr_geoxml_parameter_group_get_instance(parameter_group, &last_instance, length - 1);

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

	/* The first 'instance' is the template for all other instances and isn't used. That's why we add 1 into
	 * index. */
	*parameters = (GebrGeoXmlSequence *)
	    __gebr_geoxml_get_element_at(__gebr_geoxml_parameter_get_type_element
					 (GEBR_GEOXML_PARAMETER(parameter_group), FALSE), "parameters", index+1, FALSE);

	return (*parameters == NULL)
	    ? GEBR_GEOXML_RETV_INVALID_INDEX : GEBR_GEOXML_RETV_SUCCESS;
}

glong gebr_geoxml_parameter_group_get_instances_number(GebrGeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return -1;

	/* The first 'instance' is the template for all other instances, so it must be ignored. That's why we subtract 1
	 * from the result. */
	return __gebr_geoxml_get_elements_number((GdomeElement *)
						 __gebr_geoxml_parameter_get_type_element(GEBR_GEOXML_PARAMETER
											  (parameter_group), FALSE),
						 "parameters") - 1;
}

GSList *gebr_geoxml_parameter_group_get_parameter_in_all_instances(GebrGeoXmlParameterGroup * parameter_group,
								   guint index)
{
	if (parameter_group == NULL)
		return NULL;

	GebrGeoXmlSequence *first_instance;
	GebrGeoXmlSequence *parameter;
	GSList *idref_list;

	gebr_geoxml_parameter_group_get_instance(parameter_group, &first_instance, 0);
	gebr_geoxml_parameters_get_parameter(GEBR_GEOXML_PARAMETERS(first_instance), &parameter, 0);
	idref_list = __gebr_geoxml_parameter_get_referencee_list((GdomeElement *) parameter_group,
								 __gebr_geoxml_get_attr_value((GdomeElement *)
											      parameter, "id"));
	idref_list = g_slist_prepend(idref_list, parameter);

	return idref_list;
}

void gebr_geoxml_parameter_group_set_is_instanciable(GebrGeoXmlParameterGroup * parameter_group, gboolean enable)
{
	if (parameter_group == NULL)
		return;
	__gebr_geoxml_set_attr_value(__gebr_geoxml_parameter_get_type_element
				     (GEBR_GEOXML_PARAMETER(parameter_group), FALSE), "instanciable",
				     (enable == TRUE ? "yes" : "no"));
}

void gebr_geoxml_parameter_group_set_expand(GebrGeoXmlParameterGroup * parameter_group, const gboolean enable)
{
	if (parameter_group == NULL)
		return;
	__gebr_geoxml_set_attr_value(__gebr_geoxml_parameter_get_type_element
				     (GEBR_GEOXML_PARAMETER(parameter_group), FALSE), "expand",
				     (enable == TRUE ? "yes" : "no"));
}

gboolean gebr_geoxml_parameter_group_get_is_instanciable(GebrGeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return FALSE;
	return (!strcmp
		(__gebr_geoxml_get_attr_value
		 (__gebr_geoxml_parameter_get_type_element(GEBR_GEOXML_PARAMETER(parameter_group), FALSE),
		  "instanciable"), "yes"))
	    ? TRUE : FALSE;
}

gboolean gebr_geoxml_parameter_group_get_expand(GebrGeoXmlParameterGroup * parameter_group)
{
	if (parameter_group == NULL)
		return FALSE;
	return (!strcmp
		(__gebr_geoxml_get_attr_value
		 (__gebr_geoxml_parameter_get_type_element(GEBR_GEOXML_PARAMETER(parameter_group), FALSE), "expand"),
		 "yes"))
	    ? TRUE : FALSE;
}
