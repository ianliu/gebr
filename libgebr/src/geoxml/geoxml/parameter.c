/*   libgebr - G�BR Library
 *   Copyright (C) 2007-2008 G�BR core team (http://gebr.sourceforge.net)
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

#include <string.h>

#include <gdome.h>

#include "parameter.h"
#include "xml.h"
#include "types.h"
#include "parameters.h"
#include "parameters_p.h"
#include "parameter_group.h"
#include "program_parameter.h"

/*
 * internal stuff
 */

struct geoxml_parameter {
	GdomeElement * element;
};

const char * parameter_type_to_str[] = {
	"string", "int", "file",
	"flag", "float", "range",
	"enum", "group"
};

const int parameter_type_to_str_len = 8;

/*
 * private functions
 */



/*
 * library functions.
 */

void
geoxml_parameter_set_type(GeoXmlParameter ** parameter, enum GEOXML_PARAMETERTYPE type)
{
	if (*parameter == NULL)
		return;

	GdomeElement *		parent_element;
	GeoXmlParameter *	old_parameter;

	old_parameter = *parameter;
	parent_element = (GdomeElement*)gdome_el_parentNode((GdomeElement*)old_parameter, &exception);

	*parameter = __geoxml_parameters_new_parameter((GeoXmlParameters*)parent_element, type);
	gdome_el_insertBefore(parent_element, (GdomeNode*)*parameter, (GdomeNode*)old_parameter, &exception);

	/* restore label and keyword */
	geoxml_parameter_set_label(GEOXML_PARAMETER(*parameter),
		geoxml_parameter_get_label(GEOXML_PARAMETER(old_parameter)));
	if (geoxml_parameter_get_is_program_parameter(*parameter))
		geoxml_program_parameter_set_keyword(GEOXML_PROGRAM_PARAMETER(*parameter),
			geoxml_program_parameter_get_keyword(GEOXML_PROGRAM_PARAMETER(old_parameter)));

	gdome_el_removeChild(parent_element, (GdomeNode*)old_parameter, &exception);
}

enum GEOXML_PARAMETERTYPE
geoxml_parameter_get_type(GeoXmlParameter * parameter)
{
	if (parameter == NULL)
		return GEOXML_PARAMETERTYPE_STRING;

	GdomeDOMString*		tag_name;
	int			i;

	tag_name = gdome_el_tagName((GdomeElement*)parameter, &exception);

	for (i = 0; i < parameter_type_to_str_len; ++i)
		if (!strcmp(parameter_type_to_str[i], tag_name->str))
			return (enum GEOXML_PARAMETERTYPE)i;

	/* just to suppress warning */
	return GEOXML_PARAMETERTYPE_GROUP;
}

gboolean
geoxml_parameter_get_is_program_parameter(GeoXmlParameter * parameter)
{
	if (parameter == NULL)
		return FALSE;
	return (geoxml_parameter_get_type(parameter) != GEOXML_PARAMETERTYPE_GROUP)
		? TRUE : FALSE;
}

void
geoxml_parameter_set_label(GeoXmlParameter * parameter, const gchar * label)
{
	if (parameter == NULL || label == NULL)
		return;
	__geoxml_set_tag_value((GdomeElement*)parameter, "label", label, __geoxml_create_TextNode);
}

const gchar *
geoxml_parameter_get_label(GeoXmlParameter * parameter)
{
	if (parameter == NULL)
		return NULL;
	return __geoxml_get_tag_value((GdomeElement*)parameter, "label");
}

void
geoxml_parameter_reset(GeoXmlParameter * parameter, gboolean recursive)
{
	if (parameter == NULL)
		return;
	if (geoxml_parameter_get_type(parameter) == GEOXML_PARAMETERTYPE_GROUP) {
		GeoXmlSequence *	i;

		if (recursive == FALSE)
			return;

		/* call geoxml_parameter_reset on each child */
		i = geoxml_parameters_get_first_parameter(
			geoxml_parameter_group_get_parameters(GEOXML_PARAMETER_GROUP(parameter)));
		while (i != NULL) {
			geoxml_parameter_reset(GEOXML_PARAMETER(i), recursive);
			geoxml_sequence_next(&i);
		}
	} else {
		geoxml_program_parameter_set_value(GEOXML_PROGRAM_PARAMETER(parameter), "");
		geoxml_program_parameter_set_default(GEOXML_PROGRAM_PARAMETER(parameter), "");
	}
}
