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

#include "sequence.h"
#include "xml.h"
#include "types.h"
#include "error.h"
#include "parameters.h"
#include "parameters_p.h"

/*
 * internal structures and funcionts
 */

struct geoxml_sequence {
	GdomeElement * element;
};

static gboolean
__geoxml_sequence_is_parameter(GeoXmlSequence * sequence)
{
	GdomeDOMString *	name;
	GdomeElement *		parent;

	name = gdome_el_nodeName((GdomeElement*)sequence, &exception);
	parent = (GdomeElement*)gdome_el_parentNode((GdomeElement*)sequence, &exception);
	return	(gboolean)!strcmp(
			gdome_el_nodeName(parent, &exception)->str, "parameters") ||
		(gboolean)!strcmp(
			gdome_el_nodeName(parent, &exception)->str, "group");
}

static gboolean
__geoxml_sequence_check(GeoXmlSequence * sequence)
{
	GdomeDOMString *	name;

	name = gdome_el_nodeName((GdomeElement*)sequence, &exception);
	return __geoxml_sequence_is_parameter(sequence) ||
		(gboolean)!strcmp(name->str, "option") ||
		(gboolean)!strcmp(name->str, "program") ||
		(gboolean)!strcmp(name->str, "category") ||
		(gboolean)!strcmp(name->str, "flow") ||
		(gboolean)!strcmp(name->str, "path") ||
		(gboolean)!strcmp(name->str, "line");
}

static gboolean
__geoxml_sequence_is_same_sequence(GeoXmlSequence * sequence, GeoXmlSequence * other)
{
	return __geoxml_sequence_is_parameter(sequence)
		? __geoxml_sequence_is_parameter(other)
		: (gboolean)gdome_str_equal(gdome_el_nodeName((GdomeElement*)sequence, &exception),
			gdome_el_nodeName((GdomeElement*)other, &exception));
}

/*
 * library functions.
 */

int
geoxml_sequence_previous(GeoXmlSequence ** sequence)
{
	if (*sequence == NULL)
		return GEOXML_RETV_NULL_PTR;
	if (!__geoxml_sequence_check(*sequence))
		return GEOXML_RETV_NOT_A_SEQUENCE;

	if (__geoxml_sequence_is_parameter(*sequence) == TRUE)
		*sequence = (GeoXmlSequence*)__geoxml_previous_element((GdomeElement*)*sequence);
	else
		*sequence = (GeoXmlSequence*)__geoxml_previous_same_element((GdomeElement*)*sequence);

	return GEOXML_RETV_SUCCESS;
}

int
geoxml_sequence_next(GeoXmlSequence ** sequence)
{
	if (*sequence == NULL)
		return GEOXML_RETV_NULL_PTR;
	if (!__geoxml_sequence_check(*sequence))
		return GEOXML_RETV_NOT_A_SEQUENCE;

	if (__geoxml_sequence_is_parameter(*sequence) == TRUE)
		*sequence = (GeoXmlSequence*)__geoxml_next_element((GdomeElement*)*sequence);
	else
		*sequence = (GeoXmlSequence*)__geoxml_next_same_element((GdomeElement*)*sequence);

	return GEOXML_RETV_SUCCESS;
}

int
geoxml_sequence_remove(GeoXmlSequence * sequence)
{
	if (sequence == NULL)
		return GEOXML_RETV_NULL_PTR;
	if (__geoxml_sequence_is_parameter(sequence)) {
		GeoXmlParameters *	parameters;

		parameters = (GeoXmlParameters*)gdome_el_parentNode((GdomeElement*)sequence, &exception);
		if (__geoxml_parameters_adjust_group_npar(parameters, -1) == FALSE)
			return GEOXML_RETV_MORE_THAN_ONE_INSTANCES;
	} else if (!__geoxml_sequence_check(sequence))
		return GEOXML_RETV_NOT_A_SEQUENCE;

	gdome_n_removeChild(gdome_n_parentNode((GdomeNode*)sequence, &exception),
			(GdomeNode*)sequence, &exception);
	return GEOXML_RETV_SUCCESS;
}

GeoXmlSequence *
geoxml_sequence_append_clone(GeoXmlSequence * sequence)
{
	if (sequence == NULL)
		return NULL;
	if (__geoxml_sequence_is_parameter(sequence)) {
		GeoXmlParameters *	parameters;

		parameters = (GeoXmlParameters*)gdome_el_parentNode((GdomeElement*)sequence, &exception);
		if (__geoxml_parameters_adjust_group_npar(parameters, +1) == FALSE)
			return NULL;
	} else if (!__geoxml_sequence_check(sequence))
		return NULL;

	GeoXmlSequence *	clone;

	clone = (GeoXmlSequence *)gdome_n_cloneNode((GdomeNode*)sequence, TRUE, &exception);
	gdome_n_insertBefore(gdome_el_parentNode((GdomeElement*)sequence, &exception),
		(GdomeNode*)clone, NULL, &exception);

	return clone;
}

int
geoxml_sequence_move_before(GeoXmlSequence * sequence, GeoXmlSequence * position)
{
	if (sequence == NULL)
		return GEOXML_RETV_NULL_PTR;
	if (!__geoxml_sequence_check(sequence))
		return GEOXML_RETV_NOT_A_SEQUENCE;
	if (position != NULL && !__geoxml_sequence_is_same_sequence(sequence, position))
		return GEOXML_RETV_DIFFERENT_SEQUENCES;

	gdome_n_insertBefore(gdome_el_parentNode((GdomeElement*)sequence, &exception),
		(GdomeNode*)sequence, (GdomeNode*)position, &exception);
	/* TODO: handle GDOME_WRONG_DOCUMENT_ERR */
	return GEOXML_RETV_SUCCESS;
}

int
geoxml_sequence_move_after(GeoXmlSequence * sequence, GeoXmlSequence * position)
{
	if (sequence == NULL)
		return GEOXML_RETV_NULL_PTR;
	if (!__geoxml_sequence_check(sequence))
		return GEOXML_RETV_NOT_A_SEQUENCE;
	if (position != NULL && !__geoxml_sequence_is_same_sequence(sequence, position))
		return GEOXML_RETV_DIFFERENT_SEQUENCES;

	/* TODO: more eficient implementation */
	if (position == NULL) {
		GeoXmlSequence *	i;
		GeoXmlSequence *	first;

		i = sequence;
		do {
			first = i;
			geoxml_sequence_previous(&i);
		} while (i != NULL);

		gdome_n_insertBefore(gdome_el_parentNode((GdomeElement*)sequence, &exception),
			(GdomeNode*)sequence, (GdomeNode*)first, &exception);
		/* TODO: handle GDOME_WRONG_DOCUMENT_ERR */
		return GEOXML_RETV_SUCCESS;
	}

	geoxml_sequence_next(&position);
	gdome_n_insertBefore(gdome_el_parentNode((GdomeElement*)sequence, &exception),
		(GdomeNode*)sequence, (GdomeNode*)position, &exception);
	/* TODO: handle GDOME_WRONG_DOCUMENT_ERR */
	return GEOXML_RETV_SUCCESS;
}

int
geoxml_sequence_move_up(GeoXmlSequence * sequence)
{
	GeoXmlSequence *	previous;

	previous = sequence;
	geoxml_sequence_previous(&previous);
	if (previous == NULL)
		return GEOXML_RETV_INVALID_INDEX;

	gdome_n_insertBefore(gdome_el_parentNode((GdomeElement*)sequence, &exception),
		(GdomeNode*)sequence, (GdomeNode*)previous, &exception);

	return GEOXML_RETV_SUCCESS;
}

int
geoxml_sequence_move_down(GeoXmlSequence * sequence)
{
	GeoXmlSequence *	next;

	next = sequence;
	geoxml_sequence_next(&next);
	if (next == NULL)
		return GEOXML_RETV_INVALID_INDEX;
	next = (GeoXmlSequence*)__geoxml_next_element((GdomeElement*)next);

	gdome_n_insertBefore(gdome_el_parentNode((GdomeElement*)sequence, &exception),
			     (GdomeNode*)sequence, (GdomeNode*)next, &exception);

	return GEOXML_RETV_SUCCESS;
}
