/*   libgebr - GÍBR Library
 *   Copyright (C) 2007 GÍBR core team (http://gebr.sourceforge.net)
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

#include <gdome.h>

#include "sequence.h"
#include "xml.h"
#include "types.h"
#include "error.h"

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
	return	(gboolean)!g_ascii_strcasecmp(
			gdome_el_nodeName(parent, &exception)->str, "parameters") ||
		(gboolean)!g_ascii_strcasecmp(
			gdome_el_nodeName(parent, &exception)->str, "group");
}

static gboolean
__geoxml_sequence_check(GeoXmlSequence * sequence)
{
	GdomeDOMString *	name;

	name = gdome_el_nodeName((GdomeElement*)sequence, &exception);
	return __geoxml_sequence_is_parameter(sequence) ||
		(gboolean)!g_ascii_strcasecmp(name->str, "program") ||
		(gboolean)!g_ascii_strcasecmp(name->str, "category") ||
		(gboolean)!g_ascii_strcasecmp(name->str, "flow") ||
		(gboolean)!g_ascii_strcasecmp(name->str, "path") ||
		(gboolean)!g_ascii_strcasecmp(name->str, "line");
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
	if (!__geoxml_sequence_check(sequence))
		return GEOXML_RETV_NOT_A_SEQUENCE;

	gdome_n_removeChild(gdome_n_parentNode((GdomeNode*)sequence, &exception),
			(GdomeNode*)sequence, &exception);
	return GEOXML_RETV_SUCCESS;
}

int
geoxml_sequence_move(GeoXmlSequence * sequence, GeoXmlSequence * before)
{
	if (sequence == NULL)
		return GEOXML_RETV_NULL_PTR;
	if (!__geoxml_sequence_check(sequence))
		return GEOXML_RETV_NOT_A_SEQUENCE;
	if (!__geoxml_sequence_is_same_sequence(sequence, before))
		return GEOXML_RETV_DIFFERENT_SEQUENCES;

	gdome_n_insertBefore(gdome_el_parentNode((GdomeElement*)sequence, &exception),
		(GdomeNode*)sequence, (GdomeNode*)before, &exception);
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
